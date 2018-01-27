#pragma once
#pragma warning(disable:4996)
#include <iomanip>
#include <cmath>
#include <functional>
#include <thread>
#include <mutex>

#include "Node.hpp"
#include "Environment.hpp"
#include "BinaryEvaluator.hpp"
#include "Printer.hpp"
#include "Geometry.hpp"

namespace cgl
{
	class ProgressStore
	{
	public:
		static void TryWrite(std::shared_ptr<Environment> env, const Evaluated& value)
		{
			ProgressStore& i = Instance();
			if (i.mtx.try_lock())
			{
				i.pEnv = env->clone();
				i.evaluated = value;

				i.mtx.unlock();
			}
		}

		static bool TryLock()
		{
			ProgressStore& i = Instance();
			return i.mtx.try_lock();
		}

		static void Unlock()
		{
			ProgressStore& i = Instance();
			i.mtx.unlock();
		}

		static std::shared_ptr<Environment> GetEnvironment()
		{
			ProgressStore& i = Instance();
			return i.pEnv;
		}

		static boost::optional<Evaluated>& GetEvaluated()
		{
			ProgressStore& i = Instance();
			return i.evaluated;
		}

		static void Reset()
		{
			ProgressStore& i = Instance();
			i.pEnv = nullptr;
			i.evaluated = boost::none;
		}

		static bool HasValue()
		{
			ProgressStore& i = Instance();
			return static_cast<bool>(i.evaluated);
		}

	private:
		static ProgressStore& Instance()
		{
			static ProgressStore i;
			return i;
		}

		std::shared_ptr<Environment> pEnv;
		boost::optional<Evaluated> evaluated;

		std::mutex mtx;
	};


	class AddressReplacer : public boost::static_visitor<Expr>
	{
	public:
		const std::unordered_map<Address, Address>& replaceMap;

		AddressReplacer(const std::unordered_map<Address, Address>& replaceMap) :
			replaceMap(replaceMap)
		{}

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}

		Expr operator()(const LRValue& node)const
		{
			if (node.isLValue())
			{
				if (auto opt = getOpt(node.address()))
				{
					return LRValue(opt.value());
				}
			}
			return node;
		}

		Expr operator()(const Identifier& node)const { return node; }

		Expr operator()(const SatReference& node)const { return node; }

		Expr operator()(const UnaryExpr& node)const
		{
			return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op);
		}

		Expr operator()(const BinaryExpr& node)const
		{
			return BinaryExpr(
				boost::apply_visitor(*this, node.lhs), 
				boost::apply_visitor(*this, node.rhs), 
				node.op);
		}

		Expr operator()(const Range& node)const
		{
			return Range(
				boost::apply_visitor(*this, node.lhs),
				boost::apply_visitor(*this, node.rhs));
		}

		Expr operator()(const Lines& node)const
		{
			Lines result;
			for (size_t i = 0; i < node.size(); ++i)
			{
				result.add(boost::apply_visitor(*this, node.exprs[i]));
			}
			return result;
		}

		Expr operator()(const DefFunc& node)const
		{
			return DefFunc(node.arguments, boost::apply_visitor(*this, node.expr));
		}

		Expr operator()(const If& node)const
		{
			If result(boost::apply_visitor(*this, node.cond_expr));
			result.then_expr = boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
			}

			return result;
		}

		Expr operator()(const For& node)const
		{
			For result;
			result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
			result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
			result.loopCounter = node.loopCounter;
			result.doExpr = boost::apply_visitor(*this, node.doExpr);
			return result;
		}

		Expr operator()(const Return& node)const
		{
			return Return(boost::apply_visitor(*this, node.expr));
		}

		Expr operator()(const ListConstractor& node)const
		{
			ListConstractor result;
			for (const auto& expr : node.data)
			{
				result.data.push_back(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const KeyExpr& node)const
		{
			KeyExpr result(node.name);
			result.expr = boost::apply_visitor(*this, node.expr);
			return result;
		}

		Expr operator()(const RecordConstractor& node)const
		{
			RecordConstractor result;
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(*this, expr));
			}
			return result;
		}
		
		Expr operator()(const RecordInheritor& node)const
		{
			RecordInheritor result;
			result.original = boost::apply_visitor(*this, node.original);

			Expr originalAdder = node.adder;
			Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
			if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
			{
				result.adder = opt.value();
				return result;
			}

			CGL_Error("node.adder�̒u���������ʂ�RecordConstractor�łȂ�");
			return LRValue(0);
		}

		Expr operator()(const Accessor& node)const
		{
			Accessor result;
			result.head = boost::apply_visitor(*this, node.head);
			
			for (const auto& access : node.accesses)
			{
				if (auto listAccess = AsOpt<ListAccess>(access))
				{
					result.add(ListAccess(boost::apply_visitor(*this, listAccess.value().index)));
				}
				else if (auto recordAccess = AsOpt<RecordAccess>(access))
				{
					result.add(recordAccess.value());
				}
				else if (auto functionAccess = AsOpt<FunctionAccess>(access))
				{
					FunctionAccess access;
					for (const auto& argument : functionAccess.value().actualArguments)
					{
						access.add(boost::apply_visitor(*this, argument));
					}
					result.add(access);
				}
				else
				{
					CGL_Error("aaa");
				}
			}

			return result;
		}

		Expr operator()(const DeclSat& node)const
		{
			return DeclSat(boost::apply_visitor(*this, node.expr));
		}

		Expr operator()(const DeclFree& node)const
		{
			DeclFree result;
			for (const auto& accessor : node.accessors)
			{
				Expr expr = accessor;
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}
	};

	//Evaluated�̃A�h���X�l���ċA�I�ɓW�J�����N���[�����쐬����
	class ValueCloner : public boost::static_visitor<Evaluated>
	{
	public:
		ValueCloner(std::shared_ptr<Environment> pEnv) :
			pEnv(pEnv)
		{}

		std::shared_ptr<Environment> pEnv;
		std::unordered_map<Address, Address> replaceMap;

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}

		Evaluated operator()(bool node) { return node; }

		Evaluated operator()(int node) { return node; }

		Evaluated operator()(double node) { return node; }

		Evaluated operator()(const List& node)
		{
			List result;
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				if (auto opt = getOpt(data[i]))
				{
					result.append(opt.value());
				}
				else
				{
					const Evaluated& substance = pEnv->expand(data[i]);
					const Evaluated clone = boost::apply_visitor(*this, substance);
					const Address newAddress = pEnv->makeTemporaryValue(clone);
					result.append(newAddress);
					replaceMap[data[i]] = newAddress;
				}
			}

			return result;
		}

		Evaluated operator()(const KeyValue& node) { return node; }

		Evaluated operator()(const Record& node)
		{
			Record result;

			for (const auto& value : node.values)
			{
				if (auto opt = getOpt(value.second))
				{
					result.append(value.first, opt.value());
				}
				else
				{
					const Evaluated& substance = pEnv->expand(value.second);
					const Evaluated clone = boost::apply_visitor(*this, substance);
					const Address newAddress = pEnv->makeTemporaryValue(clone);
					result.append(value.first, newAddress);
					replaceMap[value.second] = newAddress;
				}
			}

			result.problem = node.problem;
			result.freeVariables = node.freeVariables;
			result.freeVariableRefs = node.freeVariableRefs;

			return result;
		}

		Evaluated operator()(const FuncVal& node) { return node; }

		Evaluated operator()(const Jump& node) { return node; }

		//Evaluated operator()(const DeclFree& node) { return node; }
	};

	class ValueCloner2 : public boost::static_visitor<Evaluated>
	{
	public:
		ValueCloner2(std::shared_ptr<Environment> pEnv, const std::unordered_map<Address, Address>& replaceMap) :
			pEnv(pEnv),
			replaceMap(replaceMap)
		{}

		std::shared_ptr<Environment> pEnv;
		const std::unordered_map<Address, Address>& replaceMap;

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}
		Evaluated operator()(bool node) { return node; }

		Evaluated operator()(int node) { return node; }

		Evaluated operator()(double node) { return node; }

		Evaluated operator()(const List& node)
		{
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				//ValueCloner1�ŃN���[���͊��ɍ�����̂ŁA���̃N���[���𒼐ڏ���������
				const Evaluated& substance = pEnv->expand(data[i]);
				pEnv->assignToObject(data[i], boost::apply_visitor(*this, substance));
			}

			return node;
		}

		Evaluated operator()(const KeyValue& node) { return node; }

		Evaluated operator()(const Record& node)
		{
			for (const auto& value : node.values)
			{
				//ValueCloner1�ŃN���[���͊��ɍ�����̂ŁA���̃N���[���𒼐ڏ���������
				const Evaluated& substance = pEnv->expand(value.second);
				pEnv->assignToObject(value.second, boost::apply_visitor(*this, substance));
			}

			node.problem;
			node.freeVariables;
			node.freeVariableRefs;

			return node;
		}

		Evaluated operator()(const FuncVal& node)const
		{
			if (node.builtinFuncAddress)
			{
				return node;
			}

			FuncVal result;
			result.arguments = node.arguments;
			result.builtinFuncAddress = node.builtinFuncAddress;

			AddressReplacer replacer(replaceMap);
			result.expr = boost::apply_visitor(replacer, node.expr);

			return result;
		}

		Evaluated operator()(const Jump& node) { return node; }

		//Evaluated operator()(const DeclFree& node) { return node; }
	};

	class ValueCloner3 : public boost::static_visitor<void>
	{
	public:
		ValueCloner3(std::shared_ptr<Environment> pEnv, const std::unordered_map<Address, Address>& replaceMap) :
			pEnv(pEnv),
			replaceMap(replaceMap)
		{}

		std::shared_ptr<Environment> pEnv;
		const std::unordered_map<Address, Address>& replaceMap;

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}

		Address replaced(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return address;
		}

		void operator()(bool& node) {}

		void operator()(int& node) {}

		void operator()(double& node) {}

		void operator()(List& node) {}

		void operator()(KeyValue& node) {}

		void operator()(Record& node)
		{
			AddressReplacer replacer(replaceMap);

			auto& problem = node.problem;
			if (problem.candidateExpr)
			{
				problem.candidateExpr = boost::apply_visitor(replacer, problem.candidateExpr.value());
			}

			for (size_t i = 0; i < problem.refs.size(); ++i)
			{
				const Address oldAddress = problem.refs[i];
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.value();
					problem.refs[i] = newAddress;
					problem.invRefs[newAddress] = problem.invRefs[oldAddress];
					problem.invRefs.erase(oldAddress);
				}
			}

			for (auto& freeVariable : node.freeVariables)
			{
				Expr expr = freeVariable;
				freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
			}
		}

		void operator()(FuncVal& node) {}

		void operator()(Jump& node) {}

		//Evaluated operator()(const DeclFree& node) { return node; }
	};

	inline Evaluated Clone(std::shared_ptr<Environment> pEnv, const Evaluated& value)
	{
		/*
		�֐��l���A�h���X������Ɏ����Ă��鎞�A�N���[���쐬�̑O��ł��̈ˑ��֌W��ۑ�����K�v������̂ŁA�N���[���쐬��2�X�e�b�v�ɕ����čs���B
		1. ���X�g�E���R�[�h�̍ċA�I�ȃR�s�[
		2. �֐��̎��A�h���X��V�������ɕt���ւ���		
		*/
		ValueCloner cloner(pEnv);
		const Evaluated& evaluated = boost::apply_visitor(cloner, value);
		ValueCloner2 cloner2(pEnv, cloner.replaceMap);
		ValueCloner3 cloner3(pEnv, cloner.replaceMap);
		Evaluated evaluated2 = boost::apply_visitor(cloner2, evaluated);
		boost::apply_visitor(cloner3, evaluated2);
		return evaluated2;
	}

	//�֐������\�����鎯�ʎq���֐������ŕ��Ă�����̂��A�O���̃X�R�[�v�Ɉˑ����Ă�����̂��𒲂�
	//�O���̃X�R�[�v���Q�Ƃ��鎯�ʎq���A�h���X�ɒu������������Ԃ�
	class ClosureMaker : public boost::static_visitor<Expr>
	{
	public:

		//�֐������ŕ��Ă��郍�[�J���ϐ�
		std::set<std::string> localVariables;

		std::shared_ptr<Environment> pEnv;

		//���R�[�h�p���̍\�����������߂ɕK�v�K�v
		bool isInnerRecord;

		ClosureMaker(std::shared_ptr<Environment> pEnv, const std::set<std::string>& functionArguments, bool isInnerRecord = false) :
			pEnv(pEnv),
			localVariables(functionArguments),
			isInnerRecord(isInnerRecord)
		{}

		ClosureMaker& addLocalVariable(const std::string& name)
		{
			localVariables.insert(name);
			return *this;
		}

		bool isLocalVariable(const std::string& name)const
		{
			return localVariables.find(name) != localVariables.end();
		}

		Expr operator()(const LRValue& node) { return node; }

		Expr operator()(const Identifier& node)
		{
			//���̊֐��̃��[�J���ϐ��ł���Ί֐��̎��s���ɕ]������΂悢�̂ŁA���O���c��
			if (isLocalVariable(node))
			{
				return node;
			}
			//���[�J���ϐ��ɖ�����΃A�h���X�ɒu��������
			const Address address = pEnv->findAddress(node);
			if (address.isValid())
			{
				//Identifier RecordConstructor �̌`���������R�[�h�p���� head ����
				//�Ƃ肠�����Q�Ɛ�̃��R�[�h�̃����o�̓��[�J���ϐ��Ƃ���
				if (isInnerRecord)
				{
					const Evaluated& evaluated = pEnv->expand(address);
					if (auto opt = AsOpt<Record>(evaluated))
					{
						const Record& record = opt.value();
						for (const auto& keyval : record.values)
						{
							addLocalVariable(keyval.first);
						}
					}
				}

				return LRValue(address);
			}

			CGL_Error("���ʎq����`����Ă��܂���");
			return LRValue(0);
		}

		Expr operator()(const SatReference& node) { return node; }

		Expr operator()(const UnaryExpr& node)
		{
			return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op);
		}

		Expr operator()(const BinaryExpr& node)
		{
			const Expr rhs = boost::apply_visitor(*this, node.rhs);

			if (node.op != BinaryOp::Assign)
			{
				const Expr lhs = boost::apply_visitor(*this, node.lhs);
				return BinaryExpr(lhs, rhs, node.op);
			}

			//Assign�̏ꍇ�Alhs �� Address or Identifier or Accessor �Ɍ�����
			//�܂茻���_�ł́A(if cond then x else y) = true �̂悤�Ȏ��������Ă��Ȃ�
			//�����ō��ӂɒ��ڃA�h���X�������Ă��邱�Ƃ͗L�蓾��H
			//a = b = 10�@�̂悤�Ȏ��ł��A�E�����ł��荶���͏�Ɏ��ʎq���c���Ă���͂��Ȃ̂ŁA���蓾�Ȃ��Ǝv��
			/*if (auto valOpt = AsOpt<Evaluated>(node.lhs))
			{
				const Evaluated& val = valOpt.value();
				if (IsType<Address>(val))
				{
					if (Address address = As<Address>(val))
					{

					}
					else
					{
						ErrorLog("");
						return 0;
					}
				}
				else
				{
					ErrorLog("");
					return 0;
				}
			}
			else */if (auto valOpt = AsOpt<Identifier>(node.lhs))
			{
				const Identifier identifier = valOpt.value();

				//���[�J���ϐ��ɂ���΁A���̏�ŉ����ł��鎯�ʎq�Ȃ̂ŉ������Ȃ�
				if (isLocalVariable(identifier))
				{
					return BinaryExpr(node.lhs, rhs, node.op);
				}
				else
				{
					const Address address = pEnv->findAddress(identifier);

					//���[�J���ϐ��ɖ����A�X�R�[�v�ɂ���΁A�A�h���X�ɒu��������
					if (address.isValid())
					{
						//TODO: ���񎮂̏ꍇ�́A�����ł͂����K�v������
						return BinaryExpr(LRValue(address), rhs, node.op);
					}
					//�X�R�[�v�ɂ������ꍇ�͐V���ȃ��[�J���ϐ��̐錾�Ȃ̂ŁA���[�J���ϐ��ɒǉ����Ă���
					else
					{
						addLocalVariable(identifier);
						return BinaryExpr(node.lhs, rhs, node.op);
					}
				}
			}
			else if (auto valOpt = AsOpt<Accessor>(node.lhs))
			{
				//�A�N�Z�b�T�̏ꍇ�͏��Ȃ��Ƃ��ϐ��錾�ł͂Ȃ�
				//���[�J���ϐ� or �X�R�[�v
				/*�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`
				Accessor��head�����]�����ăA�h���X�l�ɕϊ�������
					head����������΂��Ƃ͂�������H���̂�
					���̎����ł�head�͎��ɂȂ��Ă��邪�A���ꂾ�Ɨǂ��Ȃ�
					���͍��ӂɂ͂���Ȃɕ��G�Ȏ��͋����Ă��Ȃ��̂ŁA��������ʎq���炢�̒P���Ȍ`�ɐ������Ă悢�̂ł͂Ȃ���
				�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`�`*/

				//�]�����邱�Ƃɂ���
				const Expr lhs = boost::apply_visitor(*this, node.lhs);

				return BinaryExpr(lhs, rhs, node.op);
			}

			CGL_Error("�񍀉��Z�q\"=\"�̍��ӂ͒P��̍��Ӓl�łȂ���΂Ȃ�܂���");
			return LRValue(0);
		}

		Expr operator()(const Range& node)
		{
			return Range(
				boost::apply_visitor(*this, node.lhs),
				boost::apply_visitor(*this, node.rhs));
		}

		Expr operator()(const Lines& node)
		{
			Lines result;
			for (size_t i = 0; i < node.size(); ++i)
			{
				result.add(boost::apply_visitor(*this, node.exprs[i]));
			}
			return result;
		}

		Expr operator()(const DefFunc& node)
		{
			//ClosureMaker child(*this);
			ClosureMaker child(pEnv, localVariables, false);
			for (const auto& argument : node.arguments)
			{
				child.addLocalVariable(argument);
			}

			return DefFunc(node.arguments, boost::apply_visitor(child, node.expr));
		}

		Expr operator()(const If& node)
		{
			If result(boost::apply_visitor(*this, node.cond_expr));
			result.then_expr = boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
			}

			return result;
		}

		Expr operator()(const For& node)
		{
			For result;
			result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
			result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

			//ClosureMaker child(*this);
			ClosureMaker child(pEnv, localVariables, false);
			child.addLocalVariable(node.loopCounter);
			result.doExpr = boost::apply_visitor(child, node.doExpr);
			result.loopCounter = node.loopCounter;

			return result;
		}

		Expr operator()(const Return& node)
		{
			return Return(boost::apply_visitor(*this, node.expr));
			//���ꂾ�ƃ_����������Ȃ��H
			//return a = 6, a + 2
		}

		Expr operator()(const ListConstractor& node)
		{
			ListConstractor result;
			//ClosureMaker child(*this);
			ClosureMaker child(pEnv, localVariables, false);
			for (const auto& expr : node.data)
			{
				result.data.push_back(boost::apply_visitor(child, expr));
			}
			return result;
		}

		Expr operator()(const KeyExpr& node)
		{
			//�ϐ��錾��
			//�đ���̉\�������邪�ǂ����ɂ��낱��ȍ~���̎��ʎq�̓��[�J���ϐ��ƈ����Ă悢
			addLocalVariable(node.name);

			KeyExpr result(node.name);
			result.expr = boost::apply_visitor(*this, node.expr);
			return result;
		}

		/*
		Expr operator()(const RecordConstractor& node)
		{
			RecordConstractor result;
			ClosureMaker child(*this);
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(child, expr));
			}
			return result;
		}
		*/

		Expr operator()(const RecordConstractor& node)
		{
			RecordConstractor result;
			
			if (isInnerRecord)
			{
				isInnerRecord = false;
				for (const auto& expr : node.exprs)
				{
					result.exprs.push_back(boost::apply_visitor(*this, expr));
				}
				isInnerRecord = true;
			}
			else
			{
				ClosureMaker child(pEnv, localVariables, false);
				for (const auto& expr : node.exprs)
				{
					result.exprs.push_back(boost::apply_visitor(child, expr));
				}
			}			
			
			return result;
		}

		//���R�[�h�p���\���ɂ��Ă͓���ŁAadder��]�����鎞�̃X�R�[�v��head�Ɠ����ł���K�v������B
		//�܂�Ahead��]�����鎞�ɂ͂��̒��g���A��i�K�����i�g���ʈ�������j�W�J����悤�ɂ��ĕ]�����Ȃ���΂Ȃ�Ȃ��B
		Expr operator()(const RecordInheritor& node)
		{
			RecordInheritor result;

			//�V���ɒǉ�
			ClosureMaker child(pEnv, localVariables, true);

			//result.original = boost::apply_visitor(*this, node.original);
			result.original = boost::apply_visitor(child, node.original);
			
			Expr originalAdder = node.adder;
			//Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
			Expr replacedAdder = boost::apply_visitor(child, originalAdder);
			if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
			{
				result.adder = opt.value();
				return result;
			}

			CGL_Error("node.adder�̒u���������ʂ�RecordConstractor�łȂ�");
			return LRValue(0);
		}

		Expr operator()(const Accessor& node)
		{
			Accessor result;
			
			result.head = boost::apply_visitor(*this, node.head);
			//DeclSat�̕]����ł�sat�����̃A�N�Z�b�T�i�̓�sat���̃��[�J���ϐ��łȂ����́j��head�̓A�h���X�l�ɕ]������Ă���K�v������B
			//�������A�����ł�*this���g���Ă���̂ŁA�C�ӂ̎����A�h���X�l�ɕ]�������킯�ł͂Ȃ��B
			//�Ⴆ�΁A���̎� ([f1,f2] @ [f3])[0](x) ����head���̓��X�g�̌������ł���AEval��ʂ��Ȃ��ƃA�h���X�l�ɂł��Ȃ��B
			//�������A������Eval�͎g�������Ȃ��iClosureMaker������p���N�����̂͗ǂ��Ȃ��j���߁A�����_�ł̓A�N�Z�b�T��head���͒P��̎��ʎq�݂̂ō\���������̂Ɖ��肵�Ă���B
			//�������邱�Ƃɂ��A���ʎq�����[�J���ϐ��Ȃ炻�̂܂܎c��A�O���̕ϐ��Ȃ�A�h���X�l�ɕϊ�����邱�Ƃ��ۏ؂ł���B

			for (const auto& access : node.accesses)
			{
				if (auto listAccess = AsOpt<ListAccess>(access))
				{
					result.add(ListAccess(boost::apply_visitor(*this, listAccess.value().index)));
				}
				else if (auto recordAccess = AsOpt<RecordAccess>(access))
				{
					result.add(recordAccess.value());
				}
				else if (auto functionAccess = AsOpt<FunctionAccess>(access))
				{
					FunctionAccess access;
					for (const auto& argument : functionAccess.value().actualArguments)
					{
						access.add(boost::apply_visitor(*this, argument));
					}
					result.add(access);
				}
				else
				{
					CGL_Error("aaa");
				}
			}

			return result;
		}
		
		Expr operator()(const DeclSat& node)
		{
			DeclSat result;
			result.expr = boost::apply_visitor(*this, node.expr);
			return result;
		}

		Expr operator()(const DeclFree& node)
		{
			DeclFree result;
			for (const auto& accessor : node.accessors)
			{
				const Expr expr = accessor;
				const Expr closedAccessor = boost::apply_visitor(*this, expr);
				if (!IsType<Accessor>(closedAccessor))
				{
					CGL_Error("aaa");
				}
				result.accessors.push_back(As<Accessor>(closedAccessor));
			}
			return result;
		}
	};

	class ConstraintProblem : public cppoptlib::Problem<double>
	{
	public:
		using typename cppoptlib::Problem<double>::TVector;

		std::function<double(const TVector&)> evaluator;
		Record originalRecord;
		std::vector<Identifier> keyList;
		std::shared_ptr<Environment> pEnv;

		bool callback(const cppoptlib::Criteria<cppoptlib::Problem<double>::Scalar> &state, const TVector &x)override
		{
			Record tempRecord = originalRecord;
			
			for (size_t i = 0; i < x.size(); ++i)
			{
				Address address = originalRecord.freeVariableRefs[i];
				pEnv->assignToObject(address, x[i]);
			}

			for (const auto& key : keyList)
			{
				Address address = pEnv->findAddress(key);
				tempRecord.append(key, address);
			}

			ProgressStore::TryWrite(pEnv, tempRecord);

			return true;
		}

		double value(const TVector &x)
		{
			return evaluator(x);
		}
	};

	class Eval : public boost::static_visitor<LRValue>
	{
	public:

		Eval(std::shared_ptr<Environment> pEnv) :pEnv(pEnv) {}

		LRValue operator()(const LRValue& node) { return node; }

		LRValue operator()(const Identifier& node) { return pEnv->findAddress(node); }

		LRValue operator()(const SatReference& node)
		{
			CGL_Error("�s���Ȏ�");
			return LRValue(0);
		}

		LRValue operator()(const UnaryExpr& node)
		{
			const Evaluated lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs));

			switch (node.op)
			{
			case UnaryOp::Not:   return RValue(Not(lhs, *pEnv));
			case UnaryOp::Minus: return RValue(Minus(lhs, *pEnv));
			case UnaryOp::Plus:  return RValue(Plus(lhs, *pEnv));
			}

			CGL_Error("Invalid UnaryExpr");
			return RValue(0);
		}

		LRValue operator()(const BinaryExpr& node)
		{
			//const Evaluated lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs));
			const LRValue lhs_ = boost::apply_visitor(*this, node.lhs);
			//const Evaluated lhs = pEnv->expand(lhs_);
			Evaluated lhs;
			if (node.op != BinaryOp::Assign)
			{
				lhs = pEnv->expand(lhs_);
			}
			
			const Evaluated rhs = pEnv->expand(boost::apply_visitor(*this, node.rhs));

			switch (node.op)
			{
			case BinaryOp::And: return RValue(And(lhs, rhs, *pEnv));
			case BinaryOp::Or:  return RValue(Or(lhs, rhs, *pEnv));

			case BinaryOp::Equal:        return RValue(Equal(lhs, rhs, *pEnv));
			case BinaryOp::NotEqual:     return RValue(NotEqual(lhs, rhs, *pEnv));
			case BinaryOp::LessThan:     return RValue(LessThan(lhs, rhs, *pEnv));
			case BinaryOp::LessEqual:    return RValue(LessEqual(lhs, rhs, *pEnv));
			case BinaryOp::GreaterThan:  return RValue(GreaterThan(lhs, rhs, *pEnv));
			case BinaryOp::GreaterEqual: return RValue(GreaterEqual(lhs, rhs, *pEnv));

			case BinaryOp::Add: return RValue(Add(lhs, rhs, *pEnv));
			case BinaryOp::Sub: return RValue(Sub(lhs, rhs, *pEnv));
			case BinaryOp::Mul: return RValue(Mul(lhs, rhs, *pEnv));
			case BinaryOp::Div: return RValue(Div(lhs, rhs, *pEnv));

			case BinaryOp::Pow:    return RValue(Pow(lhs, rhs, *pEnv));
			case BinaryOp::Concat: return RValue(Concat(lhs, rhs, *pEnv));
			case BinaryOp::Assign:
			{
				//return Assign(lhs, rhs, *pEnv);

				//Assign�̏ꍇ�Alhs �� Address or Identifier or Accessor �Ɍ�����
				//�܂茻���_�ł́A(if cond then x else y) = true �̂悤�Ȏ��������Ă��Ȃ�
				//�����ō��ӂɒ��ڃA�h���X�������Ă��邱�Ƃ͗L�蓾��H
				//a = b = 10�@�̂悤�Ȏ��ł��A�E�����ł��荶���͏�Ɏ��ʎq���c���Ă���͂��Ȃ̂ŁA���蓾�Ȃ��Ǝv��
				if (auto valOpt = AsOpt<LRValue>(node.lhs))
				{
					const LRValue& val = valOpt.value();
					if (val.isLValue())
					{
						if (val.address().isValid())
						{
							pEnv->assignToObject(val.address(), rhs);
						}
						else
						{
							CGL_Error("reference error");
						}
					}
					else
					{
						CGL_Error("");
					}

					/*const RValue& val = valOpt.value();
					if (IsType<Address>(val.value))
					{
						if (Address address = As<Address>(val.value))
						{
							pEnv->assignToObject(address, rhs);
						}
						else
						{
							CGL_Error("Invalid address");
							return 0;
						}
					}
					else
					{
						CGL_Error("");
						return 0;
					}*/
				}
				else if (auto valOpt = AsOpt<Identifier>(node.lhs))
				{
					const Identifier& identifier = valOpt.value();

					const Address address = pEnv->findAddress(identifier);
					//�ϐ������݂���F�����
					if (address.isValid())
					{
						CGL_DebugLog("�����");
						pEnv->assignToObject(address, rhs);
					}
					//�ϐ������݂��Ȃ��F�ϐ��錾��
					else
					{
						CGL_DebugLog("�ϐ��錾��");
						pEnv->bindNewValue(identifier, rhs);
						CGL_DebugLog("");
					}

					return RValue(rhs);
				}
				else if (auto valOpt = AsOpt<Accessor>(node.lhs))
				{
					if (lhs_.isLValue())
					{
						Address address = lhs_.address();
						if (address.isValid())
						{
							pEnv->assignToObject(address, rhs);
							return RValue(rhs);
						}
						else
						{
							CGL_Error("�Q�ƃG���[");
						}
					}
					else
					{
						CGL_Error("�A�N�Z�b�T�̕]�����ʂ��A�h���X�łȂ�");
					}
				}
			}
			}

			CGL_Error("Invalid BinaryExpr");
			return RValue(0);
		}

		LRValue operator()(const DefFunc& defFunc)
		{
			return pEnv->makeFuncVal(pEnv, defFunc.arguments, defFunc.expr);
		}

		//LRValue operator()(const FunctionCaller& callFunc)
		LRValue callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments)
		{
			std::cout << "callFunction:" << std::endl;

			CGL_DebugLog("Function Environment:");
			pEnv->printEnvironment();

			/*
			�܂��Q�Ƃ��X�R�[�v�Ԃŋ��L�ł���悤�ɂ��Ă��Ȃ����߁A�����ɗ^����ꂽ�I�u�W�F�N�g�͑S�ēW�J���ēn���B
			�����āA�����̕]�����_�ł͂܂��֐��̒��ɓ����Ă��Ȃ��̂ŁA�X�R�[�v��ς���O�ɓW�J���s���B
			*/

			/*
			12/14
			�S�Ă̒l��ID�ŊǗ�����悤�ɂ���B
			�����ăX�R�[�v���ς��ƁA�ϐ��̃}�b�s���O�͕ς�邪�A�l�͋��ʂȂ̂łǂ��炩����Q�Ƃł���B
			*/
			/*
			std::vector<Address> expandedArguments(callFunc.actualArguments.size());
			for (size_t i = 0; i < expandedArguments.size(); ++i)
			{
				expandedArguments[i] = pEnv->makeTemporaryValue(callFunc.actualArguments[i]);
			}

			CGL_DebugLog("");

			FuncVal funcVal;

			if (auto opt = AsOpt<FuncVal>(callFunc.funcRef))
			{
				funcVal = opt.value();
			}
			else
			{
				const Address funcAddress = pEnv->findAddress(As<Identifier>(callFunc.funcRef));
				if (funcAddress.isValid())
				{
					if (auto funcOpt = pEnv->expandOpt(funcAddress))
					{
						if (IsType<FuncVal>(funcOpt.value()))
						{
							funcVal = As<FuncVal>(funcOpt.value());
						}
						else
						{
							CGL_Error("�w�肳�ꂽ�ϐ����ɕR����ꂽ�l���֐��łȂ�");
						}
					}
					else
					{
						CGL_Error("�����͒ʂ�Ȃ��͂�");
					}
				}
				else
				{
					CGL_Error("�w�肳�ꂽ�ϐ����ɒl���R�����Ă��Ȃ�");
				}
			}
			*/

			/*if (funcVal.builtinFuncAddress)
			{
				return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments));
			}*/
			if (funcVal.builtinFuncAddress)
			{
				return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments));
			}

			CGL_DebugLog("");

			//if (funcVal.arguments.size() != callFunc.actualArguments.size())
			if (funcVal.arguments.size() != expandedArguments.size())
			{
				CGL_Error("�������̐��Ǝ������̐��������Ă��Ȃ�");
			}

			//�֐��̕]��
			//(1)�����ł̃��[�J���ϐ��͊֐����Ăяo�������ł͂Ȃ��A�֐�����`���ꂽ���̂��̂��g���̂Ń��[�J���ϐ���u��������

			pEnv->switchFrontScope();
			//���̈Ӗ��_�ł̓X�R�[�v�ւ̎Q�Ƃ͑S�ăA�h���X�l�ɕϊ����Ă���
			//�����ŁA�֐�������O���[�o���ϐ��̏����������s�����Ƃ���ƁA�A�h���X�ɕR����ꂽ�l�𒼐ڕύX���邱�ƂɂȂ�
			//TODO: ����́A�Ӗ��_�I�ɐ������̂���x�l����K�v������
			//�Ƃ肠�����֐����X�R�[�v�Ɉˑ����邱�Ƃ͂Ȃ��Ȃ����̂ŁA�P���ɕʂ̃X�R�[�v�ɐ؂�ւ��邾���ŗǂ�

			CGL_DebugLog("");

			//(2)�֐��̈����p�ɃX�R�[�v����ǉ�����
			pEnv->enterScope();

			CGL_DebugLog("");

			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				/*
				12/14
				�����̓X�R�[�v���܂������ɎQ�Ɛ悪�ς��Ȃ��悤�ɑS��ID�œn�����Ƃɂ���B
				*/
				pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
			}

			CGL_DebugLog("Function Definition:");
			printExpr(funcVal.expr);

			//(3)�֐��̖߂�l�����̃X�R�[�v�ɖ߂������A�����Ɠ������R�őS�ēW�J���ēn���B
			//Evaluated result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));
			Evaluated result;
			{
				//const Evaluated resultValue = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
				//result = pEnv->expandRef(pEnv->makeTemporaryValue(resultValue));
				/*
				EliminateScopeDependency elim(pEnv);
				result = pEnv->makeTemporaryValue(boost::apply_visitor(elim, resultValue));
				*/
				result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
				CGL_DebugLog("Function Evaluated:");
				printEvaluated(result, nullptr);
			}
			//Evaluated result = pEnv->expandObject();

			CGL_DebugLog("");

			//(4)�֐��𔲂��鎞�ɁA�������͑S�ĉ�������
			pEnv->exitScope();

			CGL_DebugLog("");

			//(5)�Ō�Ƀ��[�J���ϐ��̊����֐��̎��s�O�̂��̂ɖ߂��B
			pEnv->switchBackScope();

			CGL_DebugLog("");
			
			//�]�����ʂ�return���������ꍇ��return���O���Ē��g��Ԃ�
			//return�ȊO�̃W�����v���߂͊֐��ł͌��ʂ������Ȃ��̂ł��̂܂܏�ɕԂ�
			if (IsType<Jump>(result))
			{
				auto& jump = As<Jump>(result);
				if (jump.isReturn())
				{
					if (jump.lhs)
					{
						return RValue(jump.lhs.value());
					}
					else
					{
						CGL_Error("return���̒��g�������Ė���");
					}
				}
			}

			return RValue(result);
		}

		LRValue operator()(const Range& range)
		{
			return RValue(0);
		}

		LRValue operator()(const Lines& statement)
		{
			pEnv->enterScope();

			Evaluated result;
			int i = 0;
			for (const auto& expr : statement.exprs)
			{
				//std::cout << "Evaluate expression(" << i << ")" << std::endl;
				CGL_DebugLog("Evaluate expression(" + std::to_string(i) + ")");
				pEnv->printEnvironment();

				//std::cout << "LINES_A Expr:" << std::endl;
				//printExpr(expr);
				CGL_DebugLog("");
				result = pEnv->expand(boost::apply_visitor(*this, expr));
				printEvaluated(result, pEnv);

				//std::cout << "LINES_B" << std::endl;
				CGL_DebugLog("");

				//TODO: ��ōl����
				//���̕]�����ʂ����Ӓl�̏ꍇ�͒��g�����āA���ꂪ�}�N���ł���Β��g��W�J�������ʂ����̕]�����ʂƂ���
				/*
				if (IsLValue(result))
				{
					//const Evaluated& resultValue = pEnv->dereference(result);
					const Evaluated resultValue = pEnv->expandRef(result);
					const bool isMacro = IsType<Jump>(resultValue);
					if (isMacro)
					{
						result = As<Jump>(resultValue);
					}
				}
				*/

				//std::cout << "LINES_C" << std::endl;
				CGL_DebugLog("");
				//�r���ŃW�����v���߂�ǂ񂾂瑦���ɕ]�����I������
				if (IsType<Jump>(result))
				{
					//std::cout << "LINES_D" << std::endl;
					CGL_DebugLog("");
					break;
				}

				//std::cout << "LINES_D" << std::endl;
				CGL_DebugLog("");

				++i;
			}

			//std::cout << "LINES_E" << std::endl;
			CGL_DebugLog("");

			//���̌シ����������̂� dereference ���Ă���
			bool deref = true;
			/*
			if (auto refOpt = AsOpt<ObjectReference>(result))
			{
				if (IsType<unsigned>(refOpt.value().headValue))
				{
					deref = false;
				}
			}

			std::cout << "LINES_F" << std::endl;

			if (deref)
			{
				result = pEnv->dereference(result);
			}
			*/

			/*
			if (auto refOpt = AsOpt<Address>(result))
			{
				//result = pEnv->dereference(refOpt.value());
				//result = pEnv->expandRef(refOpt.value());
				result = pEnv->expand(refOpt.value());
			}
			*/

			//std::cout << "LINES_G" << std::endl;
			CGL_DebugLog("");

			pEnv->exitScope();

			//std::cout << "LINES_H" << std::endl;
			CGL_DebugLog("");
			return LRValue(result);
		}

		LRValue operator()(const If& if_statement)
		{
			const Evaluated cond = pEnv->expand(boost::apply_visitor(*this, if_statement.cond_expr));
			if (!IsType<bool>(cond))
			{
				//�����͕K���u�[���l�ł���K�v������
				//std::cerr << "Error(" << __LINE__ << ")\n";
				CGL_Error("�����͕K���u�[���l�ł���K�v������");
			}

			if (As<bool>(cond))
			{
				const Evaluated result = pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr));
				return RValue(result);

			}
			else if (if_statement.else_expr)
			{
				const Evaluated result = pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.value()));
				return RValue(result);
			}

			//else���������P�[�X�� cond = False �ł�������ꉞ�x�����o��
			//std::cerr << "Warning(" << __LINE__ << ")\n";
			CGL_WarnLog("else���������P�[�X�� cond = False �ł�����");
			return RValue(0);
		}

		LRValue operator()(const For& forExpression)
		{
			std::cout << "For:" << std::endl;

			const Evaluated startVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeStart));
			const Evaluated endVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeEnd));

			//startVal <= endVal �Ȃ� 1
			//startVal > endVal �Ȃ� -1
			//��K�؂Ȍ^�ɕϊ����ĕԂ�
			const auto calcStepValue = [&](const Evaluated& a, const Evaluated& b)->boost::optional<std::pair<Evaluated, bool>>
			{
				const bool a_IsInt = IsType<int>(a);
				const bool a_IsDouble = IsType<double>(a);

				const bool b_IsInt = IsType<int>(b);
				const bool b_IsDouble = IsType<double>(b);

				if (!((a_IsInt || a_IsDouble) && (b_IsInt || b_IsDouble)))
				{
					//�G���[�F���[�v�̃����W���s���Ȍ^�i�����������ɕ]���ł���K�v������j
					//std::cerr << "Error(" << __LINE__ << ")\n";
					//return boost::none;
					CGL_Error("���[�v�̃����W���s���Ȍ^�i�����������ɕ]���ł���K�v������j");
				}

				const bool result_IsDouble = a_IsDouble || b_IsDouble;
				//const auto lessEq = LessEqual(a, b, *pEnv);
				//if (!IsType<bool>(lessEq))
				//{
				//	//�G���[�Fa��b�̔�r�Ɏ��s����
				//	//�ꉞ�m���߂Ă��邾���ł�����ʂ邱�Ƃ͂Ȃ��͂�
				//	//LessEqual�̎����~�X�H
				//	//std::cerr << "Error(" << __LINE__ << ")\n";
				//	//return boost::none;
				//	CGL_Error("LessEqual�̎����~�X�H");
				//}

				//const bool isInOrder = As<bool>(lessEq);
				const bool isInOrder = LessEqual(a, b, *pEnv);

				const int sign = isInOrder ? 1 : -1;

				if (result_IsDouble)
				{
					/*const Evaluated xx = Mul(1.0, sign, *pEnv);
					return std::pair<Evaluated, bool>(xx, isInOrder);*/

					//std::pair<Evaluated, bool> xa(Mul(1.0, sign, *pEnv), isInOrder);

					/*const Evaluated xx = Mul(1.0, sign, *pEnv);
					std::pair<Evaluated, bool> xa(xx, isInOrder);
					return boost::optional<std::pair<Evaluated, bool>>(xa);*/

					return std::pair<Evaluated, bool>(Mul(1.0, sign, *pEnv), isInOrder);
				}

				return std::pair<Evaluated, bool>(Mul(1, sign, *pEnv), isInOrder);
			};

			const auto loopContinues = [&](const Evaluated& loopCount, bool isInOrder)->boost::optional<bool>
			{
				//isInOrder == true
				//loopCountValue <= endVal

				//isInOrder == false
				//loopCountValue > endVal

				const Evaluated result = LessEqual(loopCount, endVal, *pEnv);
				if (!IsType<bool>(result))
				{
					CGL_Error("������ʂ邱�Ƃ͂Ȃ��͂�");
				}

				return As<bool>(result) == isInOrder;
			};

			const auto stepOrder = calcStepValue(startVal, endVal);
			if (!stepOrder)
			{
				CGL_Error("���[�v�̃����W���s��");
			}

			const Evaluated step = stepOrder.value().first;
			const bool isInOrder = stepOrder.value().second;

			Evaluated loopCountValue = startVal;

			Evaluated loopResult;

			pEnv->enterScope();

			while (true)
			{
				const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
				if (!isLoopContinuesOpt)
				{
					CGL_Error("������ʂ邱�Ƃ͂Ȃ��͂�");
				}

				//���[�v�̌p�������𖞂����Ȃ������̂Ŕ�����
				if (!isLoopContinuesOpt.value())
				{
					break;
				}

				pEnv->bindNewValue(forExpression.loopCounter, loopCountValue);

				loopResult = pEnv->expand(boost::apply_visitor(*this, forExpression.doExpr));

				//���[�v�J�E���^�̍X�V
				loopCountValue = Add(loopCountValue, step, *pEnv);
			}

			pEnv->exitScope();

			return RValue(loopResult);
		}

		LRValue operator()(const Return& return_statement)
		{
			const Evaluated lhs = pEnv->expand(boost::apply_visitor(*this, return_statement.expr));

			return RValue(Jump::MakeReturn(lhs));
		}

		LRValue operator()(const ListConstractor& listConstractor)
		{
			List list;
			for (const auto& expr : listConstractor.data)
			{
				/*const Evaluated value = pEnv->expand(boost::apply_visitor(*this, expr));
				CGL_DebugLog("");
				printEvaluated(value, nullptr);

				list.append(pEnv->makeTemporaryValue(value));*/
				LRValue lrvalue = boost::apply_visitor(*this, expr);
				if (lrvalue.isLValue())
				{
					list.append(lrvalue.address());
				}
				else
				{
					list.append(pEnv->makeTemporaryValue(lrvalue.evaluated()));
				}
			}

			return RValue(list);
		}

		LRValue operator()(const KeyExpr& keyExpr)
		{
			const Evaluated value = pEnv->expand(boost::apply_visitor(*this, keyExpr.expr));

			return RValue(KeyValue(keyExpr.name, value));
		}

		LRValue operator()(const RecordConstractor& recordConsractor)
		{
			std::cout << "RecordConstractor:" << std::endl;

			CGL_DebugLog("");
			pEnv->enterScope();
			CGL_DebugLog("");

			std::vector<Identifier> keyList;
			/*
			���R�[�h����:���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͐V���ɒ�`
			���R�[�h����=���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͂��̃X�R�[�v���ł̂ݗL���Ȓl�̃G�C���A�X�Ƃ��Ē�`�i�X�R�[�v�𔲂����猳�ɖ߂���Օ��j
			*/

			for (size_t i = 0; i < recordConsractor.exprs.size(); ++i)
			{
				CGL_DebugLog(std::string("RecordExpr(") + ToS(i) + "): ");
				printExpr(recordConsractor.exprs[i]);
			}

			Record newRecord;

			if (temporaryRecord)
			{
				currentRecords.push(temporaryRecord.value());
				temporaryRecord = boost::none;
			}
			else
			{
				currentRecords.push(std::ref(newRecord));
			}

			Record& record = currentRecords.top();
			
			int i = 0;

			for (const auto& expr : recordConsractor.exprs)
			{
				CGL_DebugLog("");
				Evaluated value = pEnv->expand(boost::apply_visitor(*this, expr));
				CGL_DebugLog("Evaluate: ");
				printExpr(expr);
				CGL_DebugLog("Result: ");
				printEvaluated(value, pEnv);

				//�L�[�ɕR�Â�����l�͂��̌�̎葱���ōX�V����邩������Ȃ��̂ŁA���͖��O�����T���Ă����Č�Œl���Q�Ƃ���
				if (auto keyValOpt = AsOpt<KeyValue>(value))
				{
					const auto keyVal = keyValOpt.value();
					keyList.push_back(keyVal.name);

					CGL_DebugLog(std::string("assign to ") + static_cast<std::string>(keyVal.name));

					//Assign(keyVal.name, keyVal.value, *pEnv);

					//���ʎq��Evaluated����͂������̂ŁA���ʎq�ɑ΂��Ē��ڑ�����s�����Ƃ͂ł��Ȃ��Ȃ���
					//Assign(ObjectReference(keyVal.name), keyVal.value, *pEnv);

					//���������āA��x�����������Ă��炻���]������
					//Expr exprVal = RValue(keyVal.value);
					Expr exprVal = LRValue(keyVal.value);
					Expr expr = BinaryExpr(keyVal.name, exprVal, BinaryOp::Assign);
					boost::apply_visitor(*this, expr);

					CGL_DebugLog("");
					//Assign(ObjectReference(keyVal.name), keyVal.value, *pEnv);
				}
				/*
				else if (auto declSatOpt = AsOpt<DeclSat>(value))
				{
					//record.problem.addConstraint(declSatOpt.value().expr);
					//�����ŃN���[�W�������K�v������
					ClosureMaker closureMaker(pEnv, {});
					const Expr closedSatExpr = boost::apply_visitor(closureMaker, declSatOpt.value().expr);
					record.problem.addConstraint(closedSatExpr);
				}
				*/
				/*
				else if (auto declFreeOpt = AsOpt<DeclFree>(value))
				{
					for (const auto& accessor : declFreeOpt.value().accessors)
					{
						ClosureMaker closureMaker(pEnv, {});
						const Expr freeExpr = accessor;
						const Expr closedFreeExpr = boost::apply_visitor(closureMaker, freeExpr);
						if (!IsType<Accessor>(closedFreeExpr))
						{
							CGL_Error("�����͒ʂ�Ȃ��͂�");
						}

						record.freeVariables.push_back(As<Accessor>(closedFreeExpr));
					}
				}
				*/

				//value�͍��͉E�Ӓl�݂̂ɂȂ��Ă���
				//TODO: ������x�l�@����
				/*
				//���̕]�����ʂ����Ӓl�̏ꍇ�͒��g�����āA���ꂪ�}�N���ł���Β��g��W�J�������ʂ����̕]�����ʂƂ���
				if (IsLValue(value))
				{
					const Evaluated resultValue = pEnv->expandRef(value);
					const bool isMacro = IsType<Jump>(resultValue);
					if (isMacro)
					{
						value = As<Jump>(resultValue);
					}
				}

				//�r���ŃW�����v���߂�ǂ񂾂瑦���ɕ]�����I������
				if (IsType<Jump>(value))
				{
					break;
				}
				*/

				++i;
			}

			pEnv->printEnvironment();
			CGL_DebugLog("");

			/*for (const auto& satExpr : innerSatClosures)
			{
				record.problem.addConstraint(satExpr);
			}
			innerSatClosures.clear();*/

			CGL_DebugLog("");

			std::vector<double> resultxs;
			if (record.problem.candidateExpr)
			{
				auto& problem = record.problem;
				auto& freeVariableRefs = record.freeVariableRefs;

				const auto& problemRefs = problem.refs;
				const auto& freeVariables = record.freeVariables;

				{
					//record.freeVariables�����Ƃ�record.freeVariableRefs���\�z
					//�S�ẴA�N�Z�b�T��W�J���A�e�ϐ��̎Q�ƃ��X�g���쐬����
					freeVariableRefs.clear();
					for (const auto& accessor : record.freeVariables)
					{
						const Address refAddress = pEnv->evalReference(accessor);
						//const Address refAddress = accessor;
						//�P��̒l or List or Record
						if (refAddress.isValid())
						{
							const auto addresses = pEnv->expandReferences(refAddress);
							freeVariableRefs.insert(freeVariableRefs.end(), addresses.begin(), addresses.end());
						}
						else
						{
							CGL_Error("accessor refers null address");
						}
					}

					CGL_DebugLog(std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size()));

					//��xsat���̒��g��W�J���A
					//Accessor��W�J����visitor�iExpr -> Expr�j�����A���s����
					//���̌�Sat2Expr�Ɋ|����
					//Expr2SatExpr�ł͒P�����Z�q�E�񍀉��Z�q�̕]���̍ہA���g���݂Ē萔�ł��������ݍ��ޏ������s��

					//sat�̊֐��Ăяo����S�Ď��ɓW�J����
					//{
					//	//problem.candidateExpr = pEnv->expandFunction(problem.candidateExpr.value());
					//	Expr ee = pEnv->expandFunction(problem.candidateExpr.value());
					//	problem.candidateExpr = ee;
					//}

					//�W�J���ꂽ����SatExpr�ɕϊ�����
					//�{sat�̎��Ɋ܂܂��ϐ��̓��AfreeVariableRefs�ɓ����Ă��Ȃ����̂͑����ɕ]�����ď�ݍ���
					//freeVariableRefs�ɓ����Ă�����͍̂œK���Ώۂ̕ϐ��Ƃ���data�ɒǉ����Ă���
					//�{freeVariableRefs�̕ϐ��ɂ��Ă�sat���ɏo���������ǂ������L�^���폜����
					problem.constructConstraint(pEnv, record.freeVariableRefs);

					//�ݒ肳�ꂽdata���Q�Ƃ��Ă���l�����ď����l��ݒ肷��
					if (!problem.initializeData(pEnv))
					{
						return LRValue(0);
					}

					CGL_DebugLog(std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size()));
					CGL_DebugLog(std::string("Record SatExpr: "));
					std::cout << (std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size())) << std::endl;
				}

				//DeclFree�ɏo������Q�Ƃɂ��āA���̃C���f�b�N�X -> Problem�̃f�[�^�̃C���f�b�N�X���擾����}�b�v
				std::unordered_map<int, int> variable2Data;
				for (size_t freeIndex = 0; freeIndex < record.freeVariableRefs.size(); ++freeIndex)
				{
					CGL_DebugLog(ToS(freeIndex));
					CGL_DebugLog(std::string("Address(") + record.freeVariableRefs[freeIndex].toString() + ")");
					const auto& ref1 = record.freeVariableRefs[freeIndex];

					bool found = false;
					for (size_t dataIndex = 0; dataIndex < problemRefs.size(); ++dataIndex)
					{
						CGL_DebugLog(ToS(dataIndex));
						CGL_DebugLog(std::string("Address(") + problemRefs[dataIndex].toString() + ")");

						const auto& ref2 = problemRefs[dataIndex];

						if (ref1 == ref2)
						{
							//std::cout << "    " << freeIndex << " -> " << dataIndex << std::endl;

							found = true;
							variable2Data[freeIndex] = dataIndex;
							break;
						}
					}

					//DeclFree�ɂ�����DeclSat�ɖ����ϐ��͈Ӗ����Ȃ��B
					//�P�ɖ������Ă��ǂ����A���炭���͂̃~�X�Ǝv����̂Ōx�����o��
					if (!found)
					{
						//std::cerr << "Error(" << __LINE__ << "):free�Ɏw�肳�ꂽ�ϐ��������ł��B\n";
						//return 0;
						CGL_WarnLog("free�Ɏw�肳�ꂽ�ϐ��������ł�");
					}
				}
				CGL_DebugLog("End Record MakeMap");

				/*
				ConstraintProblem constraintProblem;
				constraintProblem.evaluator = [&](const ConstraintProblem::TVector& v)->double
				{
					//-1000 -> 1000
					for (int i = 0; i < v.size(); ++i)
					{
						problem.update(variable2Data[i], v[i]);
						//problem.update(variable2Data[i], (v[i] - 0.5)*2000.0);
					}

					{
						for (const auto& keyval : problem.invRefs)
						{
							pEnv->assignToObject(keyval.first, problem.data[keyval.second]);
						}
					}

					pEnv->switchFrontScope();
					double result = problem.eval(pEnv);
					pEnv->switchBackScope();

					CGL_DebugLog(std::string("cost: ") + ToS(result, 17));
					std::cout << std::string("cost: ") << ToS(result, 17) << "\n";
					return result;
				};
				constraintProblem.originalRecord = record;
				constraintProblem.keyList = keyList;
				constraintProblem.pEnv = pEnv;

				Eigen::VectorXd x0s(record.freeVariableRefs.size());
				for (int i = 0; i < x0s.size(); ++i)
				{
					x0s[i] = problem.data[variable2Data[i]];
					//x0s[i] = (problem.data[variable2Data[i]] / 2000.0) + 0.5;
					CGL_DebugLog(ToS(i) + " : " + ToS(x0s[i]));
				}

				cppoptlib::BfgsSolver<ConstraintProblem> solver;
				solver.minimize(constraintProblem, x0s);

				resultxs.resize(x0s.size());
				for (int i = 0; i < x0s.size(); ++i)
				{
					resultxs[i] = x0s[i];
				}
				//*/

				//*
				libcmaes::FitFunc func = [&](const double *x, const int N)->double
				{
					for (int i = 0; i < N; ++i)
					{
						problem.update(variable2Data[i], x[i]);
					}

					{
						for (const auto& keyval : problem.invRefs)
						{
							pEnv->assignToObject(keyval.first, problem.data[keyval.second]);
						}
					}

					pEnv->switchFrontScope();
					double result = problem.eval(pEnv);
					pEnv->switchBackScope();

					CGL_DebugLog(std::string("cost: ") + ToS(result, 17));
					
					return result;
				};

				CGL_DebugLog("");

				std::vector<double> x0(record.freeVariableRefs.size());
				for (int i = 0; i < x0.size(); ++i)
				{
					x0[i] = problem.data[variable2Data[i]];
					CGL_DebugLog(ToS(i) + " : " + ToS(x0[i]));
				}

				CGL_DebugLog("");

				const double sigma = 0.1;

				const int lambda = 100;

				libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda);
				CGL_DebugLog("");
				libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
				CGL_DebugLog("");
				resultxs = cmasols.best_candidate().get_x();
				CGL_DebugLog("");
				//*/
			}

			CGL_DebugLog("");
			for (size_t i = 0; i < resultxs.size(); ++i)
			{
				Address address = record.freeVariableRefs[i];
				pEnv->assignToObject(address, resultxs[i]);
				//pEnv->assignToObject(address, (resultxs[i] - 0.5)*2000.0);
			}

			for (const auto& key : keyList)
			{
				//record.append(key.name, pEnv->dereference(key));

				//record.append(key.name, pEnv->makeTemporaryValue(pEnv->dereference(ObjectReference(key))));

				/*Address address = pEnv->findAddress(key);
				boost::optional<const Evaluated&> opt = pEnv->expandOpt(address);
				record.append(key, pEnv->makeTemporaryValue(opt.value()));*/

				Address address = pEnv->findAddress(key);
				record.append(key, address);
			}

			pEnv->printEnvironment();

			CGL_DebugLog("");

			currentRecords.pop();

			//pEnv->pop();
			pEnv->exitScope();

			CGL_DebugLog("--------------------------- Print Environment ---------------------------");
			pEnv->printEnvironment();
			CGL_DebugLog("-------------------------------------------------------------------------");

			return RValue(record);
		}

		LRValue operator()(const RecordInheritor& record)
		{
			boost::optional<Record> recordOpt;

			/*
			if (auto opt = AsOpt<Identifier>(record.original))
			{
				//auto originalOpt = pEnv->find(opt.value().name);
				boost::optional<const Evaluated&> originalOpt = pEnv->dereference(pEnv->findAddress(opt.value()));
				if (!originalOpt)
				{
					//�G���[�F����`�̃��R�[�h���Q�Ƃ��悤�Ƃ���
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}

				recordOpt = AsOpt<Record>(originalOpt.value());
				if (!recordOpt)
				{
					//�G���[�F���ʎq�̎w���I�u�W�F�N�g�����R�[�h�^�ł͂Ȃ�
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}
			}
			else if (auto opt = AsOpt<Record>(record.original))
			{
				recordOpt = opt.value();
			}
			*/

			//Evaluated originalRecordRef = boost::apply_visitor(*this, record.original);
			//Evaluated originalRecordVal = pEnv->expandRef(originalRecordRef);
			Evaluated originalRecordVal = pEnv->expand(boost::apply_visitor(*this, record.original));
			if (auto opt = AsOpt<Record>(originalRecordVal))
			{
				recordOpt = opt.value();
			}
			else
			{
				CGL_Error("not record");
			}

			/*
			a{}��]������菇
			(1) �I���W�i���̃��R�[�ha�̃N���[��(a')�����
			(2) a'�̊e�L�[�ƒl�ɑ΂���Q�Ƃ����[�J���X�R�[�v�ɒǉ�����
			(3) �ǉ����郌�R�[�h�̒��g��]������
			(4) ���[�J���X�R�[�v�̎Q�ƒl��ǂ݃��R�[�h�ɏ㏑������ //���X�g�A�N�Z�X�Ȃǂ̕ύX����
			(5) ���R�[�h���}�[�W���� //���[�J���ϐ��Ȃǂ̕ύX����
			*/

			//(1)
			//Record clone = recordOpt.value();
			
			pEnv->printEnvironment(true);
			CGL_DebugLog("Original:");
			printEvaluated(originalRecordVal, pEnv);

			Record clone = As<Record>(Clone(pEnv, recordOpt.value()));

			CGL_DebugLog("Clone:");
			printEvaluated(clone, pEnv);

			if (temporaryRecord)
			{
				CGL_Error("���R�[�h�g���Ɏ��s");
			}
			temporaryRecord = clone;

			pEnv->enterScope();
			for (auto& keyval : clone.values)
			{
				pEnv->makeVariable(keyval.first, keyval.second);

				CGL_DebugLog(std::string("Bind ") + keyval.first + " -> " + "Address(" + keyval.second.toString() + ")");
			}

			Expr expr = record.adder;
			Evaluated recordValue = pEnv->expand(boost::apply_visitor(*this, expr));

			pEnv->exitScope();

			return pEnv->makeTemporaryValue(recordValue);

			/*
			//(2)
			pEnv->enterScope();
			for (auto& keyval : clone.values)
			{
				//pEnv->bindObjectRef(keyval.first, keyval.second);
				pEnv->makeVariable(keyval.first, keyval.second);

				CGL_DebugLog(std::string("Bind ") + keyval.first + " -> " + "Address(" + keyval.second.toString() + ")");
			}

			CGL_DebugLog("");

			//(3)
			Expr expr = record.adder;
			//Evaluated recordValue = boost::apply_visitor(*this, expr);
			Evaluated recordValue = pEnv->expand(boost::apply_visitor(*this, expr));
			if (auto opt = AsOpt<Record>(recordValue))
			{
				CGL_DebugLog("");

				//(4)
				for (auto& keyval : recordOpt.value().values)
				{
					clone.values[keyval.first] = pEnv->findAddress(keyval.first);
				}

				CGL_DebugLog("");

				//(5)
				MargeRecordInplace(clone, opt.value());

				CGL_DebugLog("");

				//TODO:�����Ő��񏈗����s��

				//pEnv->pop();
				pEnv->exitScope();

				//return RValue(clone);
				return pEnv->makeTemporaryValue(clone);

				//MargeRecord(recordOpt.value(), opt.value());
			}

			CGL_DebugLog("");

			//pEnv->pop();
			pEnv->exitScope();
			*/

			//�����͒ʂ�Ȃ��͂��B{}�ň͂܂ꂽ����]���������ʂ����R�[�h�łȂ������B
			//std::cerr << "Error(" << __LINE__ << ")\n";
			CGL_Error("�����͒ʂ�Ȃ��͂�");
			return RValue(0);
		}

		LRValue operator()(const DeclSat& node)
		{
			//std::cout << "DeclSat:" << std::endl;

			//�����ŃN���[�W�������K�v������
			ClosureMaker closureMaker(pEnv, {});
			const Expr closedSatExpr = boost::apply_visitor(closureMaker, node.expr);
			//innerSatClosures.push_back(closedSatExpr);

			pEnv->enterScope();
			//DeclSat���̂͌��ݐ��񂪖�������Ă��邩�ǂ�����]�����ʂƂ��ĕԂ�
			const Evaluated result = pEnv->expand(boost::apply_visitor(*this, closedSatExpr));
			pEnv->exitScope();

			if (currentRecords.empty())
			{
				CGL_Error("sat�錾�̓��R�[�h�̒��ɂ����������Ƃ��ł��܂���");
			}

			currentRecords.top().get().problem.addConstraint(closedSatExpr);

			return RValue(result);
			//return RValue(false);
		}

		LRValue operator()(const DeclFree& node)
		{
			//std::cout << "DeclFree:" << std::endl;
			for (const auto& accessor : node.accessors)
			{
				//std::cout << "  accessor:" << std::endl;
				if (currentRecords.empty())
				{
					CGL_Error("var�錾�̓��R�[�h�̒��ɂ����������Ƃ��ł��܂���");
				}

				ClosureMaker closureMaker(pEnv, {});
				const Expr varExpr = accessor;
				const Expr closedVarExpr = boost::apply_visitor(closureMaker, varExpr);

				if (IsType<Accessor>(closedVarExpr))
				{
					//std::cout << "    Free Expr:" << std::endl;
					//printExpr(closedVarExpr, std::cout);
					currentRecords.top().get().freeVariables.push_back(As<Accessor>(closedVarExpr));
				}
				else if (IsType<Identifier>(closedVarExpr))
				{
					Accessor result(closedVarExpr);
					currentRecords.top().get().freeVariables.push_back(result);
				}
				else
				{
					CGL_Error("var�錾�Ɏw�肳�ꂽ�ϐ��������ł�");
				}
				/*const LRValue result = boost::apply_visitor(*this, expr);
				if (!result.isLValue())
				{
					CGL_Error("var�錾�Ɏw�肳�ꂽ�ϐ��͖����ł�");
				}*/

				//currentRecords.top().get().freeVariables.push_back(result.address());
			}

			return RValue(0);
		}

		LRValue operator()(const Accessor& accessor)
		{
			/*
			ObjectReference result;

			Evaluated headValue = boost::apply_visitor(*this, accessor.head);
			if (auto opt = AsOpt<Identifier>(headValue))
			{
				result.headValue = opt.value();
			}
			else if (auto opt = AsOpt<Record>(headValue))
			{
				result.headValue = opt.value();
			}
			else if (auto opt = AsOpt<List>(headValue))
			{
				result.headValue = opt.value();
			}
			else if (auto opt = AsOpt<FuncVal>(headValue))
			{
				result.headValue = opt.value();
			}
			else
			{
				//�G���[�F���ʎq�����e�����ȊO�i�]�����ʂƂ��ăI�u�W�F�N�g��Ԃ��悤�Ȏ��j�ւ̃A�N�Z�X�ɂ͖��Ή�
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
			*/

			Address address;

			/*
			Evaluated headValue = boost::apply_visitor(*this, accessor.head);
			if (auto opt = AsOpt<Address>(headValue))
			{
				address = opt.value();
			}
			else if (auto opt = AsOpt<Record>(headValue))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<List>(headValue))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<FuncVal>(headValue))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else
			{
				//�G���[�F���ʎq�����e�����ȊO�i�]�����ʂƂ��ăI�u�W�F�N�g��Ԃ��悤�Ȏ��j�ւ̃A�N�Z�X�ɂ͖��Ή�
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
			*/

			LRValue headValue = boost::apply_visitor(*this, accessor.head);
			if (headValue.isLValue())
			{
				address = headValue.address();
			}
			else
			{
				Evaluated evaluated = headValue.evaluated();
				if (auto opt = AsOpt<Record>(evaluated))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else if (auto opt = AsOpt<List>(evaluated))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else if (auto opt = AsOpt<FuncVal>(evaluated))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else
				{
					CGL_Error("�A�N�Z�b�T�̃w�b�h�̕]�����ʂ��s��");
				}
			}

			for (const auto& access : accessor.accesses)
			{
				boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(address);
				if (!objOpt)
				{
					CGL_Error("�Q�ƃG���[");
				}

				const Evaluated& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					Evaluated value = pEnv->expand(boost::apply_visitor(*this, listAccessOpt.value().index));

					if (!IsType<List>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
					}

					const List& list = As<const List&>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						address = list.get(indexOpt.value());
					}
					else
					{
						CGL_Error("list[index] �� index �� int �^�łȂ�");
					}
				}
				else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
				{
					if (!IsType<Record>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����R�[�h�łȂ�");
					}

					const Record& record = As<const Record&>(objRef);
					auto it = record.values.find(recordAccessOpt.value().name);
					if (it == record.values.end())
					{
						CGL_Error("�w�肳�ꂽ���ʎq�����R�[�h���ɑ��݂��Ȃ�");
					}

					address = it->second;
				}
				else
				{
					auto funcAccess = As<FunctionAccess>(access);

					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g���֐��łȂ�");
					}

					const FuncVal& function = As<const FuncVal&>(objRef);

					/*
					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
					}
					
					Expr caller = FunctionCaller(function, args);
					//const Evaluated returnedValue = boost::apply_visitor(*this, caller);
					const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
					address = pEnv->makeTemporaryValue(returnedValue);
					*/

					std::vector<Address> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue currentArgument = pEnv->expand(boost::apply_visitor(*this, expr));
						if (currentArgument.isLValue())
						{
							args.push_back(currentArgument.address());
						}
						else
						{
							args.push_back(pEnv->makeTemporaryValue(currentArgument.evaluated()));
						}
					}

					const Evaluated returnedValue = pEnv->expand(callFunction(function, args));
					address = pEnv->makeTemporaryValue(returnedValue);
				}
			}

			return LRValue(address);
		}

	private:
		std::shared_ptr<Environment> pEnv;
		
		//sat/var�錾�͌��݂̏ꏊ���猩�čł������̃��R�[�h�ɑ΂��ēK�p�����ׂ��Ȃ̂ŁA���̊K�w�����X�^�b�N�Ŏ����Ă���
		std::stack<std::reference_wrapper<Record>> currentRecords;

		//���R�[�h�p�����s�����ɁA���R�[�h������Ă��獇������͓̂���̂ŁA�Â����R�[�h���g������`�ō�邱�Ƃɂ���
		boost::optional<Record&> temporaryRecord;
	};

	class HasFreeVariables : public boost::static_visitor<bool>
	{
	public:

		HasFreeVariables(std::shared_ptr<Environment> pEnv, const std::vector<Address>& freeVariables) :
			pEnv(pEnv),
			freeVariables(freeVariables)
		{}

		//Accessor����ObjectReference�ɕϊ�����̂ɕK�v
		std::shared_ptr<Environment> pEnv;

		//free�Ɏw�肳�ꂽ�ϐ��S��
		std::vector<Address> freeVariables;

		bool operator()(const LRValue& node)
		{
			if (node.isRValue())
			{
				const Evaluated& val = node.evaluated();
				if (IsType<double>(val) || IsType<int>(val) || IsType<bool>(val))
				{
					return false;
				}

				CGL_Error("�s���Ȓl");
			}

			Address address = node.address();
			for (const auto& freeVal : freeVariables)
			{
				if (address == freeVal)
				{
					return true;
				}
			}

			return false;
		}

		bool operator()(const Identifier& node)
		{
			Address address = pEnv->findAddress(node);
			for (const auto& freeVal : freeVariables)
			{
				if (address == freeVal)
				{
					return true;
				}
			}

			return false;
		}

		bool operator()(const SatReference& node) { return true; }

		bool operator()(const UnaryExpr& node)
		{
			return boost::apply_visitor(*this, node.lhs);
		}

		bool operator()(const BinaryExpr& node)
		{
			const bool lhs = boost::apply_visitor(*this, node.lhs);
			if (lhs)
			{
				return true;
			}

			const bool rhs = boost::apply_visitor(*this, node.rhs);
			return rhs;
		}

		bool operator()(const DefFunc& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Range& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Lines& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const If& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const For& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Return& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const ListConstractor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const KeyExpr& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const RecordConstractor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclSat& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclFree& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const Accessor& node)
		{
			Expr expr = node;
			Eval evaluator(pEnv);
			const LRValue value = boost::apply_visitor(evaluator, expr);

			if (value.isLValue())
			{
				const Address address = value.address();

				for (const auto& freeVal : freeVariables)
				{
					if (address == freeVal)
					{
						return true;
					}
				}

				return false;
			}

			CGL_Error("invalid expression");
			return false;
		}
	};

	class Expr2SatExpr : public boost::static_visitor<Expr>
	{
	public:

		int refID_Offset;

		//Accessor����ObjectReference�ɕϊ�����̂ɕK�v
		std::shared_ptr<Environment> pEnv;

		//free�Ɏw�肳�ꂽ�ϐ��S��
		std::vector<Address> freeVariables;

		//free�Ɏw�肳�ꂽ�ϐ������ۂ�sat�Ɍ��ꂽ���ǂ���
		std::vector<char> usedInSat;

		//FreeVariables Index -> SatReference
		std::map<int, SatReference> satRefs;

		//TODO:vector->map�ɏ���������
		std::vector<Address> refs;

		Expr2SatExpr(int refID_Offset, std::shared_ptr<Environment> pEnv, const std::vector<Address>& freeVariables) :
			refID_Offset(refID_Offset),
			pEnv(pEnv),
			freeVariables(freeVariables),
			usedInSat(freeVariables.size(), 0)
		{}

		boost::optional<size_t> freeVariableIndex(Address reference)
		{
			for (size_t i = 0; i < freeVariables.size(); ++i)
			{
				if (freeVariables[i] == reference)
				{
					return i;
				}
			}

			return boost::none;
		}

		boost::optional<SatReference> getSatRef(Address reference)
		{
			for (size_t i = 0; i < refs.size(); ++i)
			{
				if (refs[i] == reference)
				{
					return SatReference(refID_Offset + i);
					//return refs[i];
				}
			}

			return boost::none;
		}

		boost::optional<SatReference> addSatRef(Address reference)
		{
			if (auto indexOpt = freeVariableIndex(reference))
			{
				//�ȑO�ɏo�����ēo�^�ς݂�free�ϐ��͂��̂܂ܕԂ�
				if (auto satRefOpt = getSatRef(reference))
				{
					return satRefOpt;
				}

				//���߂ďo������free�ϐ��͓o�^���Ă���Ԃ�
				usedInSat[indexOpt.value()] = 1;
				SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
				refs.push_back(reference);
				return satRef;
			}

			return boost::none;
		}
				
		Expr operator()(const LRValue& node)
		{
			if (node.isRValue())
			{
				return node;
			}
			else
			{
				const Address address = node.address();

				if (!address.isValid())
				{
					CGL_Error("���ʎq����`����Ă��܂���");
				}

				//free�ϐ��ɂ������ꍇ�͐���p�̎Q�ƒl��Ԃ�
				if (auto satRefOpt = addSatRef(address))
				{
					return satRefOpt.value();
				}
				//free�ϐ��ɂȂ������ꍇ�͕]���������ʂ�Ԃ�
				else
				{
					const Evaluated evaluated = pEnv->expand(address);
					return LRValue(evaluated);
				}
			}

			CGL_Error("�����͒ʂ�Ȃ��͂�");
			return 0;
		}

		//������Identifier���c���Ă��鎞�_��ClosureMaker�Ƀ��[�J���ϐ����Ɣ��肳�ꂽ�ϐ��̂͂�
		Expr operator()(const Identifier& node)
		{
			Address address = pEnv->findAddress(node);
			if (!address.isValid())
			{
				CGL_Error("���ʎq����`����Ă��܂���");
			}

			//free�ϐ��ɂ������ꍇ�͐���p�̎Q�ƒl��Ԃ�
			if (auto satRefOpt = addSatRef(address))
			{
				return satRefOpt.value();
			}
			//free�ϐ��ɂȂ������ꍇ�͕]���������ʂ�Ԃ�
			else
			{
				const Evaluated evaluated = pEnv->expand(address);
				return LRValue(evaluated);
			}

			CGL_Error("�����͒ʂ�Ȃ��͂�");
			return LRValue(0);
		}

		Expr operator()(const SatReference& node) { return node; }

		Expr operator()(const UnaryExpr& node)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case UnaryOp::Not:   return UnaryExpr(lhs, UnaryOp::Not);
			case UnaryOp::Minus: return UnaryExpr(lhs, UnaryOp::Minus);
			case UnaryOp::Plus:  return lhs;
			}

			CGL_Error("invalid expression");
			return LRValue(0);
		}

		Expr operator()(const BinaryExpr& node)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);
			const Expr rhs = boost::apply_visitor(*this, node.rhs);

			if (node.op != BinaryOp::Assign)
			{
				return BinaryExpr(lhs, rhs, node.op);
			}

			CGL_Error("invalid expression");
			return LRValue(0);
		}

		Expr operator()(const DefFunc& node) { CGL_Error("invalid expression"); return LRValue(0); }

		Expr operator()(const Range& node) { CGL_Error("invalid expression"); return LRValue(0); }

		Expr operator()(const Lines& node)
		{
			Lines result;
			for (const auto& expr : node.exprs)
			{
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const If& node)
		{
			If result;
			result.cond_expr = boost::apply_visitor(*this, node.cond_expr);
			result.then_expr = boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
			}

			return result;
		}

		Expr operator()(const For& node)
		{
			For result;
			result.loopCounter = node.loopCounter;
			result.rangeEnd = node.rangeEnd;
			result.rangeStart = node.rangeStart;
			result.doExpr = boost::apply_visitor(*this, node.doExpr);
			return result;
		}

		Expr operator()(const Return& node) { CGL_Error("invalid expression"); return 0; }
		
		Expr operator()(const ListConstractor& node)
		{
			ListConstractor result;
			for (const auto& expr : node.data)
			{
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const KeyExpr& node) { return node; }

		Expr operator()(const RecordConstractor& node)
		{
			RecordConstractor result;
			for (const auto& expr : node.exprs)
			{
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return 0; }
		Expr operator()(const DeclSat& node) { CGL_Error("invalid expression"); return 0; }
		Expr operator()(const DeclFree& node) { CGL_Error("invalid expression"); return 0; }

		Expr operator()(const Accessor& node)
		{
			Address headAddress;
			const Expr& head = node.head;

			//head��sat�����̃��[�J���ϐ�
			if (IsType<Identifier>(head))
			{
				Address address = pEnv->findAddress(As<Identifier>(head));
				if (!address.isValid())
				{
					CGL_Error("���ʎq����`����Ă��܂���");
				}

				//head�͕K�� Record/List/FuncVal �̂ǂꂩ�ł���Adouble�^�ł��邱�Ƃ͂��蓾�Ȃ��B
				//���������āAfree�ϐ��ɂ��邩�ǂ����͍l�������ifree�ϐ��͏璷�Ɏw��ł���̂ł������Ƃ��Ă��ʂɃG���[�ł͂Ȃ��j�A
				//����Evaluated�Ƃ��ēW�J����
				//result.head = LRValue(address);
				headAddress = address;
			}
			//head���A�h���X�l
			else if (IsType<LRValue>(head))
			{
				const LRValue& headAddressValue = As<LRValue>(head);
				if (!headAddressValue.isLValue())
				{
					CGL_Error("sat�����̃A�N�Z�b�T�̐擪�����s���Ȓl�ł�");
				}

				const Address address = headAddressValue.address();

				//����Identifier�Ɠ��l�ɒ��ړW�J����
				//result.head = LRValue(address);
				headAddress = address;
			}
			else
			{
				CGL_Error("sat���̃A�N�Z�b�T�̐擪���ɒP��̎��ʎq�ȊO�̎���p���邱�Ƃ͂ł��܂���");
			}

			Eval evaluator(pEnv);

			Accessor result;

			//TODO: �A�N�Z�b�T��free�ϐ��������Ȃ��ԁA���ꎩ�g��free�ϐ��w�肳���܂ł̃A�h���X����ݍ���
			bool selfDependsOnFreeVariables = false;
			bool childDependsOnFreeVariables = false;
			{
				HasFreeVariables searcher(pEnv, freeVariables);
				const Expr headExpr = LRValue(headAddress);
				selfDependsOnFreeVariables = boost::apply_visitor(searcher, headExpr);
			}

			for (const auto& access : node.accesses)
			{
				const Address lastHeadAddress = headAddress;

				boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(headAddress);
				if (!objOpt)
				{
					CGL_Error("�Q�ƃG���[");
				}

				const Evaluated& objRef = objOpt.value();

				if (IsType<ListAccess>(access))
				{
					const ListAccess& listAccess = As<ListAccess>(access);

					{
						HasFreeVariables searcher(pEnv, freeVariables);
						childDependsOnFreeVariables = childDependsOnFreeVariables || boost::apply_visitor(searcher, listAccess.index);
					}

					if (childDependsOnFreeVariables)
					{
						Expr accessIndex = boost::apply_visitor(*this, listAccess.index);
						result.add(ListAccess(accessIndex));
					}
					else
					{
						Evaluated value = pEnv->expand(boost::apply_visitor(evaluator, listAccess.index));

						if (!IsType<List>(objRef))
						{
							CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
						}

						const List& list = As<const List&>(objRef);

						if (auto indexOpt = AsOpt<int>(value))
						{
							headAddress = list.get(indexOpt.value());
						}
						else
						{
							CGL_Error("list[index] �� index �� int �^�łȂ�");
						}
					}
				}
				else if (IsType<RecordAccess>(access))
				{
					const RecordAccess& recordAccess = As<RecordAccess>(access);

					if (childDependsOnFreeVariables)
					{
						result.add(access);
					}
					else
					{
						if (!IsType<Record>(objRef))
						{
							CGL_Error("�I�u�W�F�N�g�����R�[�h�łȂ�");
						}

						const Record& record = As<const Record&>(objRef);
						auto it = record.values.find(recordAccess.name);
						if (it == record.values.end())
						{
							CGL_Error("�w�肳�ꂽ���ʎq�����R�[�h���ɑ��݂��Ȃ�");
						}

						headAddress = it->second;
					}
				}
				else
				{
					const FunctionAccess& funcAccess = As<FunctionAccess>(access);

					{
						//TODO: HasFreeVariables�̎����͕s���S�ŁA�A�N�Z�b�T���֐��Ăяo���ł���ɂ��̈�����free�ϐ��̏ꍇ�ɑΉ����Ă��Ȃ�
						//      ��x����]�����āA���̉ߒ���free�ϐ��Ŏw�肵���A�h���X�ւ̃A�N�Z�X���������邩�ǂ����Ō���ׂ�
						//      AddressAppearanceChecker�̂悤�Ȃ��̂����(Eval�̊ȈՔ�)
						//inline bool HasFreeVariables::operator()(const Accessor& node)

						HasFreeVariables searcher(pEnv, freeVariables);
						for (const auto& arg : funcAccess.actualArguments)
						{
							childDependsOnFreeVariables = childDependsOnFreeVariables || boost::apply_visitor(searcher, arg);
						}
					}

					if (childDependsOnFreeVariables)
					{
						FunctionAccess resultAccess;
						for (const auto& arg : funcAccess.actualArguments)
						{
							resultAccess.add(boost::apply_visitor(*this, arg));
						}
						result.add(resultAccess);
					}
					else
					{
						if (!IsType<FuncVal>(objRef))
						{
							CGL_Error("�I�u�W�F�N�g���֐��łȂ�");
						}

						const FuncVal& function = As<const FuncVal&>(objRef);

						/*
						std::vector<Evaluated> args;
						for (const auto& expr : funcAccess.actualArguments)
						{
						args.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
						}

						Expr caller = FunctionCaller(function, args);
						const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
						headAddress = pEnv->makeTemporaryValue(returnedValue);
						*/
						std::vector<Address> args;
						for (const auto& expr : funcAccess.actualArguments)
						{
							const LRValue lrvalue = boost::apply_visitor(evaluator, expr);
							if (lrvalue.isLValue())
							{
								args.push_back(lrvalue.address());
							}
							else
							{
								args.push_back(pEnv->makeTemporaryValue(lrvalue.evaluated()));
							}
						}

						const Evaluated returnedValue = pEnv->expand(evaluator.callFunction(function, args));
						headAddress = pEnv->makeTemporaryValue(returnedValue);
					}
				}

				if (lastHeadAddress != headAddress && !selfDependsOnFreeVariables)
				{
					HasFreeVariables searcher(pEnv, freeVariables);
					const Expr headExpr = LRValue(headAddress);
					selfDependsOnFreeVariables = boost::apply_visitor(searcher, headExpr);
				}
			}

			/*
			selfDependsOnFreeVariables��childDependsOnFreeVariables������true : �G���[
			selfDependsOnFreeVariables��true  : �A�N�Z�b�T�{�̂�free�i�A�N�Z�b�T��]������ƕK���P���double�^�ɂȂ�j
			childDependsOnFreeVariables��true : �A�N�Z�b�T�̈�����free�i���X�g�C���f�b�N�X���֐������j
			*/
			if (selfDependsOnFreeVariables && childDependsOnFreeVariables)
			{
				CGL_Error("sat�����̃A�N�Z�b�T�ɂ��āA�{�̂ƈ����̗�����free�ϐ��Ɏw�肷�邱�Ƃ͂ł��܂���");
			}
			else if (selfDependsOnFreeVariables)
			{
				if (!result.accesses.empty())
				{
					CGL_Error("�����͒ʂ�Ȃ��͂�");
				}

				if (auto satRefOpt = addSatRef(headAddress))
				{
					return satRefOpt.value();
				}
				else
				{
					CGL_Error("�����͒ʂ�Ȃ��͂�");
				}
			}
			/*else if (childDependsOnFreeVariables)
			{
			CGL_Error("TODO: �A�N�Z�b�T�̈�����free�ϐ��w��͖��Ή�");
			}*/

			result.head = LRValue(headAddress);

			return result;
		}
	};

	inline Evaluated evalExpr(const Expr& expr)
	{
		auto env = Environment::Make();

		Eval evaluator(env);

		const Evaluated result = env->expand(boost::apply_visitor(evaluator, expr));

		std::cout << "Environment:\n";
		env->printEnvironment();

		std::cout << "Result Evaluation:\n";
		printEvaluated(result, env);

		return result;
	}

	inline bool IsEqualEvaluated(const Evaluated& value1, const Evaluated& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
			if ((IsType<int>(value1) || IsType<double>(value1)) && (IsType<int>(value2) || IsType<double>(value2)))
			{
				const Evaluated d1 = IsType<int>(value1) ? static_cast<double>(As<int>(value1)) : As<double>(value1);
				const Evaluated d2 = IsType<int>(value2) ? static_cast<double>(As<int>(value2)) : As<double>(value2);
				return IsEqualEvaluated(d1, d2);
			}

			std::cerr << "Values are not same type." << std::endl;
			return false;
		}

		if (IsType<bool>(value1))
		{
			return As<bool>(value1) == As<bool>(value2);
		}
		else if (IsType<int>(value1))
		{
			return As<int>(value1) == As<int>(value2);
		}
		else if (IsType<double>(value1))
		{
			const double d1 = As<double>(value1);
			const double d2 = As<double>(value2);
			if (d1 == d2)
			{
				return true;
			}
			else
			{
				std::cerr << "Warning: Comparison of floating point numbers might be incorrect." << std::endl;
				const double eps = 0.001;
				const bool result = abs(d1 - d2) < eps;
				std::cerr << std::setprecision(17);
				std::cerr << "    " << d1 << " == " << d2 << " => " << (result ? "Maybe true" : "Maybe false") << std::endl;
				std::cerr << std::setprecision(6);
				return result;
			}
			//return As<double>(value1) == As<double>(value2);
		}
		/*else if (IsType<Address>(value1))
		{
			return As<Address>(value1) == As<Address>(value2);
		}*/
		else if (IsType<List>(value1))
		{
			return As<List>(value1) == As<List>(value2);
		}
		else if (IsType<KeyValue>(value1))
		{
			return As<KeyValue>(value1) == As<KeyValue>(value2);
		}
		else if (IsType<Record>(value1))
		{
			return As<Record>(value1) == As<Record>(value2);
		}
		else if (IsType<FuncVal>(value1))
		{
			return As<FuncVal>(value1) == As<FuncVal>(value2);
		}
		else if (IsType<Jump>(value1))
		{
			return As<Jump>(value1) == As<Jump>(value2);
		};
		/*else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		};*/

		std::cerr << "IsEqualEvaluated: Type Error" << std::endl;
		return false;
	}

	inline bool IsEqual(const Expr& value1, const Expr& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
			std::cerr << "Values are not same type." << std::endl;
			return false;
		}
		/*
		if (IsType<bool>(value1))
		{
			return As<bool>(value1) == As<bool>(value2);
		}
		else if (IsType<int>(value1))
		{
			return As<int>(value1) == As<int>(value2);
		}
		else if (IsType<double>(value1))
		{
			const double d1 = As<double>(value1);
			const double d2 = As<double>(value2);
			if (d1 == d2)
			{
				return true;
			}
			else
			{
				std::cerr << "Warning: Comparison of floating point numbers might be incorrect." << std::endl;
				const double eps = 0.001;
				const bool result = abs(d1 - d2) < eps;
				std::cerr << std::setprecision(17);
				std::cerr << "    " << d1 << " == " << d2 << " => " << (result ? "Maybe true" : "Maybe false") << std::endl;
				std::cerr << std::setprecision(6);
				return result;
			}
			//return As<double>(value1) == As<double>(value2);
		}
		*/
		/*if (IsType<RValue>(value1))
		{
			return IsEqualEvaluated(As<RValue>(value1).value, As<RValue>(value2).value);
		}*/
		if (IsType<LRValue>(value1))
		{
			const LRValue& lrvalue1 = As<LRValue>(value1);
			const LRValue& lrvalue2 = As<LRValue>(value2);
			if (lrvalue1.isLValue() && lrvalue2.isLValue())
			{
				return lrvalue1.address() == lrvalue2.address();
			}
			else if (lrvalue1.isRValue() && lrvalue2.isRValue())
			{
				return IsEqualEvaluated(lrvalue1.evaluated(), lrvalue2.evaluated());
			}
			std::cerr << "Values are not same type." << std::endl;
			return false;
		}
		else if (IsType<Identifier>(value1))
		{
			return As<Identifier>(value1) == As<Identifier>(value2);
		}
		else if (IsType<SatReference>(value1))
		{
			return As<SatReference>(value1) == As<SatReference>(value2);
		}
		else if (IsType<UnaryExpr>(value1))
		{
			return As<UnaryExpr>(value1) == As<UnaryExpr>(value2);
		}
		else if (IsType<BinaryExpr>(value1))
		{
			return As<BinaryExpr>(value1) == As<BinaryExpr>(value2);
		}
		else if (IsType<DefFunc>(value1))
		{
			return As<DefFunc>(value1) == As<DefFunc>(value2);
		}
		else if (IsType<Range>(value1))
		{
			return As<Range>(value1) == As<Range>(value2);
		}
		else if (IsType<Lines>(value1))
		{
			return As<Lines>(value1) == As<Lines>(value2);
		}
		else if (IsType<If>(value1))
		{
			return As<If>(value1) == As<If>(value2);
		}
		else if (IsType<For>(value1))
		{
			return As<For>(value1) == As<For>(value2);
		}
		else if (IsType<Return>(value1))
		{
			return As<Return>(value1) == As<Return>(value2);
		}
		else if (IsType<ListConstractor>(value1))
		{
			return As<ListConstractor>(value1) == As<ListConstractor>(value2);
		}
		else if (IsType<KeyExpr>(value1))
		{
			return As<KeyExpr>(value1) == As<KeyExpr>(value2);
		}
		else if (IsType<RecordConstractor>(value1))
		{
			return As<RecordConstractor>(value1) == As<RecordConstractor>(value2);
		}
		else if (IsType<RecordInheritor>(value1))
		{
			return As<RecordInheritor>(value1) == As<RecordInheritor>(value2);
		}
		else if (IsType<DeclSat>(value1))
		{
			return As<DeclSat>(value1) == As<DeclSat>(value2);
		}
		else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		}
		else if (IsType<Accessor>(value1))
		{
			return As<Accessor>(value1) == As<Accessor>(value2);
		};

		std::cerr << "IsEqual: Type Error" << std::endl;
		return false;
	}

	Address Environment::makeFuncVal(std::shared_ptr<Environment> pEnv, const std::vector<Identifier>& arguments, const Expr& expr)
	{
		std::set<std::string> functionArguments;
		for (const auto& arg : arguments)
		{
			functionArguments.insert(arg);
		}

		ClosureMaker maker(pEnv, functionArguments);
		const Expr closedFuncExpr = boost::apply_visitor(maker, expr);

		FuncVal funcVal(arguments, closedFuncExpr);
		return makeTemporaryValue(funcVal);
		//Address address = makeTemporaryValue(funcVal);
		//return address;
	}

	Address Environment::evalReference(const Accessor & access)
	{
		if (auto sharedThis = m_weakThis.lock())
		{
			Eval evaluator(sharedThis);

			const Expr accessor = access;
			/*
			const Evaluated refVal = boost::apply_visitor(evaluator, accessor);

			if (!IsType<Address>(refVal))
			{
			CGL_Error("a");
			}

			return As<Address>(refVal);
			*/

			const LRValue refVal = boost::apply_visitor(evaluator, accessor);
			if (refVal.isLValue())
			{
				return refVal.address();
			}

			CGL_Error("�A�N�Z�b�T�̕]�����ʂ��A�h���X�l�łȂ�");
		}

		CGL_Error("shared this does not exist.");
		return Address::Null();
	}

	void Environment::initialize()
	{
		registerBuiltInFunction(
			"sin",
			[](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("�����̐�������������܂���");
			}

			return Sin(pEnv->expand(arguments[0]));
		}
		);

		registerBuiltInFunction(
			"cos",
			[](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("�����̐�������������܂���");
			}

			return Cos(pEnv->expand(arguments[0]));
		}
		);

		registerBuiltInFunction(
			"diff",
			[&](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 2)
			{
				CGL_Error("�����̐�������������܂���");
			}

			return ShapeDifference(pEnv->expand(arguments[0]), pEnv->expand(arguments[1]), m_weakThis.lock());
		}
		);

		registerBuiltInFunction(
			"area",
			[&](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("�����̐�������������܂���");
			}

			return ShapeArea(pEnv->expand(arguments[0]), m_weakThis.lock());
		}
		);
	}
}
