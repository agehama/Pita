#pragma once
#pragma warning(disable:4996)
#include <iomanip>
#include <cmath>
#include "Node.hpp"
#include "Environment.hpp"
#include "BinaryEvaluator.hpp"
#include "Printer.hpp"

namespace cgl
{
	//Evaluated�̃A�h���X�l���ċA�I�ɓW�J�����N���[�����쐬����
	class ValueCloner : public boost::static_visitor<Evaluated>
	{
	public:
		ValueCloner(std::shared_ptr<Environment> pEnv) :
			pEnv(pEnv)
		{}

		std::shared_ptr<Environment> pEnv;

		Evaluated operator()(bool node)const { return node; }

		Evaluated operator()(int node)const { return node; }

		Evaluated operator()(double node)const { return node; }

		Evaluated operator()(const List& node)const
		{
			List result;
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				const Evaluated& substance = pEnv->expand(data[i]);
				const Evaluated clone = boost::apply_visitor(*this, substance);
				result.append(pEnv->makeTemporaryValue(clone));
			}

			return result;
		}

		Evaluated operator()(const KeyValue& node)const { return node; }

		Evaluated operator()(const Record& node)const
		{
			Record result;

			for (const auto& value : node.values)
			{
				const Evaluated& substance = pEnv->expand(value.second);
				const Evaluated clone = boost::apply_visitor(*this, substance);
				result.append(value.first, pEnv->makeTemporaryValue(clone));
			}

			return result;
		}

		Evaluated operator()(const FuncVal& node)const { return node; }

		Evaluated operator()(const Jump& node)const { return node; }

		//Evaluated operator()(const DeclSat& node)const { return node; }

		Evaluated operator()(const DeclFree& node)const { return node; }
	};

	inline Evaluated Clone(std::shared_ptr<Environment> pEnv, const Evaluated& value)
	{
		ValueCloner cloner(pEnv);
		return boost::apply_visitor(cloner, value);
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
			const Expr lhs = boost::apply_visitor(*this, node.lhs);
			const Expr rhs = boost::apply_visitor(*this, node.rhs);

			if (node.op != BinaryOp::Assign)
			{
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
					return BinaryExpr(lhs, rhs, node.op);
				}
				else
				{
					const Address address = pEnv->findAddress(identifier);

					//���[�J���ϐ��ɖ����A�X�R�[�v�ɂ���΁A�A�h���X�ɒu��������
					if (address.isValid())
					{
						return BinaryExpr(LRValue(address), rhs, node.op);
					}
					//�X�R�[�v�ɂ������ꍇ�͐V���ȃ��[�J���ϐ��̐錾�Ȃ̂ŁA���[�J���ϐ��ɒǉ����Ă���
					else
					{
						addLocalVariable(identifier);
						return BinaryExpr(lhs, rhs, node.op);
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

		Expr operator()(const FunctionCaller& node)
		{
			FunctionCaller result;
			//TODO: Expr����FunctionCaller���̂�����
			CGL_Error("TODO: Expr����FunctionCaller���̂�����");
			return LRValue(0);
		}

		Expr operator()(const DeclSat& node)
		{
			FunctionCaller result;
			//TODO: �l�@����i�֐��Ăяo���̒���sat�錾�����邱�Ƃ͂��蓾�邩�H�j
			CGL_Error("TODO: �l�@����");
			return LRValue(0);
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

		LRValue operator()(const FunctionCaller& callFunc)
		{
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
			std::vector<Address> expandedArguments(callFunc.actualArguments.size());
			for (size_t i = 0; i < expandedArguments.size(); ++i)
			{
				expandedArguments[i] = pEnv->makeTemporaryValue(callFunc.actualArguments[i]);

				//EliminateScopeDependency elim(pEnv);
				//expandedArguments[i] = pEnv->makeTemporaryValue(boost::apply_visitor(elim, callFunc.actualArguments[i]));
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
					//const Evaluated& funcRef = funcOpt.value();
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

			if (funcVal.builtinFuncAddress)
			{
				return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments));
			}

			CGL_DebugLog("");

			//assert(funcVal.arguments.size() == callFunc.actualArguments.size());
			if (funcVal.arguments.size() != callFunc.actualArguments.size())
			{
				CGL_Error("�������̐��Ǝ������̐��������Ă��Ȃ�");
			}

			//�֐��̕]��
			//(1)�����ł̃��[�J���ϐ��͊֐����Ăяo�������ł͂Ȃ��A�֐�����`���ꂽ���̂��̂��g���̂Ń��[�J���ϐ���u��������

			//pEnv->switchFrontScope(funcVal.currentScopeDepth);
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
				//���݂͒l���ϐ����S�Ēl�n���i�R�s�[�j�����Ă���̂ŁA�P���� bindNewValue ���s���΂悢
				//�{���͕ϐ��̏ꍇ�� bindReference �ŎQ�Ə�񂾂��n���ׂ��Ȃ̂�������Ȃ�
				//�v�l�@
				//pEnv->bindNewValue(funcVal.arguments[i].name, expandedArguments[i]);

				/*
				12/14
				�����̓X�R�[�v���܂������ɎQ�Ɛ悪�ς��Ȃ��悤�ɑS��ID�œn�����Ƃɂ���B
				*/
				pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
			}

			CGL_DebugLog("Function Definition:");
			boost::apply_visitor(Printer(), funcVal.expr);

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
			/*
			//�֐��̕]��
			//(1)�����ł̃��[�J���ϐ��͊֐����Ăяo�������ł͂Ȃ��A�֐�����`���ꂽ���̂��̂��g���̂Ń��[�J���ϐ���u��������
			pEnv = funcVal.environment;

			//(2)�֐��̈����p�ɃX�R�[�v����ǉ�����
			pEnv->pushNormal();
			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
			//���݂͒l���ϐ����S�Ēl�n���i�R�s�[�j�����Ă���̂ŁA�P���� bindNewValue ���s���΂悢
			//�{���͕ϐ��̏ꍇ�� bindReference �ŎQ�Ə�񂾂��n���ׂ��Ȃ̂�������Ȃ�
			//�v�l�@
			//pEnv->bindNewValue(funcVal.arguments[i].name, callFunc.actualArguments[i]);

			pEnv->bindNewValue(funcVal.arguments[i].name, expandedArguments[i]);
			}

			std::cout << "Function Environment:\n";
			pEnv->printEnvironment();

			std::cout << "Function Definition:\n";
			boost::apply_visitor(Printer(), funcVal.expr);

			//(3)�֐��̖߂�l�����̃X�R�[�v�ɖ߂������A�����Ɠ������R�őS�ēW�J���ēn���B
			Evaluated result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));

			//(4)�֐��𔲂��鎞�ɁA�������͑S�ĉ�������
			pEnv->pop();

			//(5)�Ō�Ƀ��[�J���ϐ��̊����֐��̎��s�O�̂��̂ɖ߂��B
			pEnv = buckUp;
			*/

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
					//������ʂ邱�Ƃ͂Ȃ��͂�
					//std::cerr << "Error(" << __LINE__ << ")\n";
					//return boost::none;
					CGL_Error("������ʂ邱�Ƃ͂Ȃ��͂�");
				}

				return As<bool>(result) == isInOrder;
			};

			const auto stepOrder = calcStepValue(startVal, endVal);
			if (!stepOrder)
			{
				//�G���[�F���[�v�̃����W���s��
				//std::cerr << "Error(" << __LINE__ << ")\n";
				//return 0;
				CGL_Error("���[�v�̃����W���s��");
			}

			const Evaluated step = stepOrder.value().first;
			const bool isInOrder = stepOrder.value().second;

			Evaluated loopCountValue = startVal;

			Evaluated loopResult;

			//pEnv->pushNormal();
			pEnv->enterScope();

			while (true)
			{
				const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
				if (!isLoopContinuesOpt)
				{
					//�G���[�F������ʂ邱�Ƃ͂Ȃ��͂�
					//std::cerr << "Error(" << __LINE__ << ")\n";
					//return 0;
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

			//pEnv->pop();
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
				const Evaluated value = pEnv->expand(boost::apply_visitor(*this, expr));
				CGL_DebugLog("");
				printEvaluated(value, nullptr);

				list.append(pEnv->makeTemporaryValue(value));
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
			//pEnv->pushRecord();

			//std::cout << "RECORD_A" << std::endl;
			CGL_DebugLog("");
			pEnv->enterScope();
			//std::cout << "RECORD_B" << std::endl;
			CGL_DebugLog("");

			std::vector<Identifier> keyList;
			/*
			���R�[�h����:���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͐V���ɒ�`
			���R�[�h����=���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͂��̃X�R�[�v���ł̂ݗL���Ȓl�̃G�C���A�X�Ƃ��Ē�`�i�X�R�[�v�𔲂����猳�ɖ߂���Օ��j
			*/

			Record record;

			int i = 0;

			for (const auto& expr : recordConsractor.exprs)
			{
				CGL_DebugLog("Evaluate: ");
				//std::cout << "Evaluate expression(" << i << ")" << std::endl;
				printExpr(expr);
				Evaluated value = pEnv->expand(boost::apply_visitor(*this, expr));

				CGL_DebugLog("");

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
				else if (auto declFreeOpt = AsOpt<DeclFree>(value))
				{
					/*const auto& refs = declFreeOpt.value().refs;
					record.freeVariables.insert(record.freeVariables.end(), refs.begin(), refs.end());*/

					for (const auto& accessor : declFreeOpt.value().accessors)
					{
						/*Expr accessExpr = accessor;
						Evaluated access = boost::apply_visitor(*this, accessExpr);
						if (!IsType<ObjectReference>(access))
						{
						std::cerr << "Error(" << __LINE__ << ")\n";
						return 0;
						}

						record.freeVariables.push_back(As<ObjectReference>(access));*/

						record.freeVariables.push_back(accessor);
					}
				}

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

			for (const auto& satExpr : innerSatClosures)
			{
				record.problem.addConstraint(satExpr);
			}
			innerSatClosures.clear();

			//std::cout << "RECORD_C" << std::endl;
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

					//std::cout << "Record FreeVariablesSize: " << record.freeVariableRefs.size() << std::endl;
					//std::cout << "Record SatExpr: " << std::endl;

					CGL_DebugLog(std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size()));
					CGL_DebugLog(std::string("Record SatExpr: "));
					problem.debugPrint();
				}

				//std::cout << "Begin Record MakeMap" << std::endl;
				//DeclFree�ɏo������Q�Ƃɂ��āA���̃C���f�b�N�X -> Problem�̃f�[�^�̃C���f�b�N�X���擾����}�b�v
				std::unordered_map<int, int> variable2Data;
				for (int freeIndex = 0; freeIndex < record.freeVariableRefs.size(); ++freeIndex)
				{
					CGL_DebugLog(ToS(freeIndex));
					CGL_DebugLog(std::string("Address(") + record.freeVariableRefs[freeIndex].toString() + ")");
					const auto& ref1 = record.freeVariableRefs[freeIndex];

					bool found = false;
					for (int dataIndex = 0; dataIndex < problemRefs.size(); ++dataIndex)
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
				//std::cout << "End Record MakeMap" << std::endl;
				CGL_DebugLog("End Record MakeMap");
				
				libcmaes::FitFunc func = [&](const double *x, const int N)->double
				{
					for (int i = 0; i < N; ++i)
					{
						//std::cout << "    x[" << i << "] -> " << x[i] << std::endl;
						problem.update(variable2Data[i], x[i]);
					}

					//pEnv->switchFrontScope();
					CGL_DebugLog("");
					double result = problem.eval(pEnv);
					CGL_DebugLog(std::string("cost: ") + ToS(result, 17));
					//pEnv->switchBackScope();
					
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
			}

			//std::cout << "RECORD_D" << std::endl;
			CGL_DebugLog("");
			for (int i = 0; i < resultxs.size(); ++i)
			{
				//ObjectReference ref = record.freeVariableRefs[i];
				//pEnv->assignToObject(ref, resultxs[i]);

				Address address = record.freeVariableRefs[i];
				pEnv->assignToObject(address, resultxs[i]);
			}

			for (const auto& key : keyList)
			{
				//record.append(key.name, pEnv->dereference(key));

				//record.append(key.name, pEnv->makeTemporaryValue(pEnv->dereference(ObjectReference(key))));

				Address address = pEnv->findAddress(key);
				boost::optional<const Evaluated&> opt = pEnv->expandOpt(address);
				record.append(key, pEnv->makeTemporaryValue(opt.value()));
			}

			pEnv->printEnvironment();

			//std::cout << "RECORD_E" << std::endl;
			CGL_DebugLog("");

			//pEnv->pop();
			pEnv->exitScope();

			CGL_DebugLog("--------------------------- Print Environment ---------------------------");
			pEnv->printEnvironment();
			CGL_DebugLog("-------------------------------------------------------------------------");
			//std::cout << "RECORD_F" << std::endl;
			//CGL_DebugLog("");

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
			Record clone = As<Record>(Clone(pEnv, recordOpt.value()));

			//(2)
			//pEnv->pushRecord();
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
					//auto valOpt = pEnv->find(keyval.first);
					boost::optional<const Evaluated&> valOpt = pEnv->expandOpt(pEnv->findAddress(keyval.first));
					if (!valOpt)
					{
						//�]�����ĕϐ��������邱�Ƃ͂Ȃ��͂�
						//std::cerr << "Error(" << __LINE__ << ")\n";
						//return 0;
						CGL_Error("�]�����ĕϐ��������邱�Ƃ͂Ȃ��͂�");
					}

					//clone.values[keyval.first] = valOpt.value();
					clone.values[keyval.first] = pEnv->makeTemporaryValue(valOpt.value());
				}

				CGL_DebugLog("");

				//(5)
				MargeRecordInplace(clone, opt.value());

				CGL_DebugLog("");

				//TODO:�����Ő��񏈗����s��

				//pEnv->pop();
				pEnv->exitScope();

				return RValue(clone);

				//MargeRecord(recordOpt.value(), opt.value());
			}

			CGL_DebugLog("");

			//pEnv->pop();
			pEnv->exitScope();

			//�����͒ʂ�Ȃ��͂��B{}�ň͂܂ꂽ����]���������ʂ����R�[�h�łȂ������B
			//std::cerr << "Error(" << __LINE__ << ")\n";
			CGL_Error("�����͒ʂ�Ȃ��͂�");
			return RValue(0);
		}

		LRValue operator()(const DeclSat& node)
		{
			//�����ŃN���[�W�������K�v������
			ClosureMaker closureMaker(pEnv, {});
			const Expr closedSatExpr = boost::apply_visitor(closureMaker, node.expr);
			innerSatClosures.push_back(closedSatExpr);

			//DeclSat���̂͌��ݐ��񂪖�������Ă��邩�ǂ�����]�����ʂƂ��ĕԂ�
			const Evaluated result = pEnv->expand(boost::apply_visitor(*this, closedSatExpr));
			return RValue(result);
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
					/*Evaluated valueRef = boost::apply_visitor(*this, listAccessOpt.value().index);
					Evaluated value = pEnv->expandRef(valueRef);*/
					Evaluated value = pEnv->expand(boost::apply_visitor(*this, listAccessOpt.value().index));

					if (!IsType<List>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
					}

					const List& list = As<const List&>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						//result.appendListRef(indexOpt.value());
						address = list.get(indexOpt.value());
					}
					/*
					else
					{
						Evaluated indexValue = pEnv->dereference(value);
						if (auto indexOpt = AsOpt<int>(indexValue))
						{
							result.appendListRef(indexOpt.value());
						}
						else
						{
							//�G���[�Flist[index] �� index �� int �^�łȂ�
							std::cerr << "Error(" << __LINE__ << "): List index(" << std::endl;
							printEvaluated(indexValue);
							std::cerr << ") is not an Int value.\n" << std::endl;

							return 0;
						}
					}
					*/
					/*else if(auto indexOpt = AsOpt<Address>(value))
					{
						const Evaluated indexValue = pEnv->expandRef(indexOpt.value());

						if (auto indexOpt = AsOpt<int>(indexValue))
						{
							address = list.get(indexOpt.value());
						}
						else
						{
							ErrorLog("list[index] �� index �� int �^�łȂ�");
							return 0;
						}
					}*/
					else
					{
						CGL_Error("list[index] �� index �� int �^�łȂ�");
					}
				}
				/*else if (auto recordOpt = AsOpt<RecordAccess>(access))
				{
					result.appendRecordRef(recordOpt.value().name.name);
				}*/
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
				/*else
				{
					auto funcAccess = As<FunctionAccess>(access);

					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(boost::apply_visitor(*this, expr));
					}
					result.appendFunctionRef(std::move(args));
				}*/
				else
				{
					auto funcAccess = As<FunctionAccess>(access);

					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g���֐��łȂ�");
					}

					const FuncVal& function = As<const FuncVal&>(objRef);

					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						//args.push_back(boost::apply_visitor(*this, expr));
						args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
					}

					Expr caller = FunctionCaller(function, args);
					//const Evaluated returnedValue = boost::apply_visitor(*this, caller);
					const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
					address = pEnv->makeTemporaryValue(returnedValue);
				}
			}

			return LRValue(address);
		}

	private:
		std::shared_ptr<Environment> pEnv;
		std::vector<Expr> innerSatClosures;
	};

	//Evaluated����Identifier���폜���ăX�R�[�v�ւ̈ˑ��𖳂���
	/*
	//Evaluated��Identifier�����ĂȂ��������ߋ��炭�K�v�Ȃ��Ȃ���
	class EliminateScopeDependency : public boost::static_visitor<Evaluated>
	{
	public:

		EliminateScopeDependency(std::shared_ptr<Environment> pEnv) :
			pEnv(pEnv)
		{}

		Evaluated operator()(bool node)const { return node; }

		Evaluated operator()(int node)const { return node; }

		Evaluated operator()(double node)const { return node; }

		//address�̏ꍇ�͒��ɂ���ɎQ�Ƃ�����ꍇ������̂Œ��g��S�Č���
		Evaluated operator()(const Address& node)const { return node; }

		Evaluated operator()(const ObjectReference& node)const { return node; }

		Evaluated operator()(const List& node)const
		{
			List resultList;

			const auto& data = node.data;
			for (size_t i = 0; i < data.size(); ++i)
			{
				const auto val = boost::apply_visitor(*this, data[i]);
				const Address address = IsType<Address>(val) ? As<Address>(val) : Address(pEnv->makeTemporaryValue(val));
				resultList.data.push_back(address);
			}

			return resultList;
		}

		Evaluated operator()(const KeyValue& node)const { return node; }

		Evaluated operator()(const Record& node)const
		{
			Record resultRecord;

			for (const auto& value : node.values)
			{
				const auto val = boost::apply_visitor(*this, value.second);
				const Address address = IsType<Address>(val) ? As<Address>(val) : Address(pEnv->makeTemporaryValue(val));
				resultRecord.append(value.first, address);
			}

			return resultRecord;
		}

		Evaluated operator()(const FuncVal& node)const { return node; }

		Evaluated operator()(const Jump& node)const { return node; }

		Evaluated operator()(const DeclSat& node)const { return node; }

		Evaluated operator()(const DeclFree& node)const { return node; }

		std::shared_ptr<Environment> pEnv;
	};
	*/

#ifdef commentout
	class Expr2SatExpr : public boost::static_visitor<SatExpr>
	{
	public:

		int refID_Offset;

		//Accessor����ObjectReference�ɕϊ�����̂ɕK�v
		std::shared_ptr<Environment> pEnv;

		//free�Ɏw�肳�ꂽ�ϐ��S��
		//std::vector<ObjectReference> freeVariables;
		std::vector<Address> freeVariables;

		//free�Ɏw�肳�ꂽ�ϐ������ۂ�sat�Ɍ��ꂽ���ǂ���
		std::vector<char> usedInSat;

		//FreeVariables Index -> SatReference
		std::map<int, SatReference> satRefs;

		//TODO:vector->map�ɏ���������
		//std::vector<ObjectReference> refs;
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

		/*SatExpr operator()(bool node)
		{
			std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0.0;
		}

		SatExpr operator()(int node)
		{
			return static_cast<double>(node);
		}

		SatExpr operator()(double node)
		{
			return node;
		}*/

		SatExpr operator()(const LRValue& node);

		SatExpr operator()(const Identifier& node);

		SatExpr operator()(const UnaryExpr& node)
		{
			const SatExpr lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case UnaryOp::Not:   return SatUnaryExpr(lhs, UnaryOp::Not);
			case UnaryOp::Minus: return SatUnaryExpr(lhs, UnaryOp::Minus);
			case UnaryOp::Plus:  return lhs;
			}

			CGL_Error("invalid expression");
			return 0.0;
		}

		SatExpr operator()(const BinaryExpr& node)
		{
			const SatExpr lhs = boost::apply_visitor(*this, node.lhs);
			const SatExpr rhs = boost::apply_visitor(*this, node.rhs);

			switch (node.op)
			{
			case BinaryOp::And: return SatBinaryExpr(lhs, rhs, BinaryOp::And);
			case BinaryOp::Or:  return SatBinaryExpr(lhs, rhs, BinaryOp::Or);

			case BinaryOp::Equal:        return SatBinaryExpr(lhs, rhs, BinaryOp::Equal);
			case BinaryOp::NotEqual:     return SatBinaryExpr(lhs, rhs, BinaryOp::NotEqual);
			case BinaryOp::LessThan:     return SatBinaryExpr(lhs, rhs, BinaryOp::LessThan);
			case BinaryOp::LessEqual:    return SatBinaryExpr(lhs, rhs, BinaryOp::LessEqual);
			case BinaryOp::GreaterThan:  return SatBinaryExpr(lhs, rhs, BinaryOp::GreaterThan);
			case BinaryOp::GreaterEqual: return SatBinaryExpr(lhs, rhs, BinaryOp::GreaterEqual);

			case BinaryOp::Add: return SatBinaryExpr(lhs, rhs, BinaryOp::Add);
			case BinaryOp::Sub: return SatBinaryExpr(lhs, rhs, BinaryOp::Sub);
			case BinaryOp::Mul: return SatBinaryExpr(lhs, rhs, BinaryOp::Mul);
			case BinaryOp::Div: return SatBinaryExpr(lhs, rhs, BinaryOp::Div);

			case BinaryOp::Pow: return SatBinaryExpr(lhs, rhs, BinaryOp::Pow);
			}

			CGL_Error("invalid expression");
			return 0;
		}

		SatExpr operator()(const DefFunc& node) { CGL_Error("invalid expression"); return 0; }

		SatExpr operator()(const FunctionCaller& node) { CGL_Error("invalid expression"); return 0; }

		SatExpr operator()(const Range& node) { CGL_Error("invalid expression"); return 0; }

		SatExpr operator()(const Lines& node)
		{
			if (node.exprs.size() != 1)
			{
				CGL_Error("invalid expression");
			}

			const SatExpr lhs = boost::apply_visitor(*this, node.exprs.front());
			return lhs;
		}

		SatExpr operator()(const If& node) { CGL_Error("invalid expression"); return 0; }
		SatExpr operator()(const For& node) { CGL_Error("invalid expression"); return 0; }
		SatExpr operator()(const Return& node) { CGL_Error("invalid expression"); return 0; }
		SatExpr operator()(const ListConstractor& node) { CGL_Error("invalid expression"); return 0; }
		SatExpr operator()(const KeyExpr& node) { CGL_Error("invalid expression"); return 0; }
		SatExpr operator()(const RecordConstractor& node) { CGL_Error("invalid expression"); return 0; }
		SatExpr operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return 0; }
		SatExpr operator()(const DeclSat& node) { CGL_Error("invalid expression"); return 0; }

		SatExpr operator()(const Accessor& node);
	};
#endif

	class Expr2SatExpr : public boost::static_visitor<Expr>
	{
	public:

		int refID_Offset;

		//Accessor����ObjectReference�ɕϊ�����̂ɕK�v
		std::shared_ptr<Environment> pEnv;

		//free�Ɏw�肳�ꂽ�ϐ��S��
		//std::vector<ObjectReference> freeVariables;
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
		
		Expr operator()(const LRValue& node);

		Expr operator()(const Identifier& node);

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

		Expr operator()(const FunctionCaller& node) { CGL_Error("invalid expression"); return LRValue(0); }

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

		Expr operator()(const Accessor& node);
	};

	//sat���̒��Ɉ�ł�free�ϐ��������true��Ԃ�
	class SatVariableBinder : public boost::static_visitor<bool>
	{
	public:

		//Accessor����ObjectReference�ɕϊ�����̂ɕK�v
		std::shared_ptr<Environment> pEnv;

		//free�ϐ��W��->free�Ɏw�肳�ꂽ�ϐ��S��
		std::vector<Address> freeVariables;

		//free�ϐ��W��->free�Ɏw�肳�ꂽ�ϐ������ۂ�sat�Ɍ��ꂽ���ǂ���
		std::vector<char> usedInSat;

		//�Q��ID->SatReference
		//std::map<int, SatReference> satRefs;

		//�Q��ID->Address
		std::vector<Address> refs;

		//Address->�Q��ID
		std::unordered_map<Address, int> invRefs;

		SatVariableBinder(std::shared_ptr<Environment> pEnv, const std::vector<Address>& freeVariables) :
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

		//Address -> �Q��ID
		boost::optional<int> addSatRef(Address reference)
		{
			//�ȑO�ɏo�����ēo�^�ς݂�free�ϐ��͂��̂܂ܕԂ�
			auto refID_It = invRefs.find(reference);
			if (refID_It != invRefs.end())
			{
				return refID_It->second;
			}

			//���߂ďo������free�ϐ��͓o�^���Ă���Ԃ�
			if (auto indexOpt = freeVariableIndex(reference))
			{
				const int referenceID = refs.size();
				usedInSat[indexOpt.value()] = 1;
				invRefs[reference] = referenceID;
				refs.push_back(reference);
				
				return referenceID;
			}

			return boost::none;
		}

		bool operator()(const LRValue& node);

		bool operator()(const Identifier& node);

		bool operator()(const SatReference& node) { return true; }

		bool operator()(const UnaryExpr& node)
		{
			return boost::apply_visitor(*this, node.lhs);
		}

		bool operator()(const BinaryExpr& node)
		{
			return boost::apply_visitor(*this, node.lhs)
				|| boost::apply_visitor(*this, node.rhs);
		}

		bool operator()(const DefFunc& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const FunctionCaller& node);

		bool operator()(const Range& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const Lines& node)
		{
			bool result = false;
			for (const auto& expr : node.exprs)
			{
				result = result || boost::apply_visitor(*this, expr);
			}
			return result;
		}

		bool operator()(const If& node)
		{
			bool result = false;
			result = result || boost::apply_visitor(*this, node.cond_expr);
			result = result || boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				result = result || boost::apply_visitor(*this, node.else_expr.value());
			}
			return result;
		}

		bool operator()(const For& node)
		{
			bool result = false;
			
			//for���͈̔͂𐧖�Ő���ł���悤�ɂ���Ӗ��͂��邩�H
			//result = result || boost::apply_visitor(*this, node.rangeEnd);
			//result = result || boost::apply_visitor(*this, node.rangeStart);

			result = result || boost::apply_visitor(*this, node.doExpr);
			return result;
		}

		bool operator()(const Return& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const ListConstractor& node)
		{
			bool result = false;
			for (const auto& expr : node.data)
			{
				result = result || boost::apply_visitor(*this, expr);
			}
			return result;
		}

		bool operator()(const KeyExpr& node) { return false; }

		bool operator()(const RecordConstractor& node)
		{
			bool result = false;
			for (const auto& expr : node.exprs)
			{
				result = result || boost::apply_visitor(*this, expr);
			}
			return result;
		}

		bool operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclSat& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const Accessor& node);
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
		
		bool operator()(const LRValue& node);

		bool operator()(const Identifier& node);

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
		bool operator()(const FunctionCaller& node) { CGL_Error("invalid expression"); return false; }
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

		bool operator()(const Accessor& node);
	};

#ifdef commentout
	//�����̕ϐ����̏o�����`�F�b�N
	class CheckNameAppearance : public boost::static_visitor<void>
	{
	public:

		//�����ł���Ƀ��[�J���ϐ�����`�����ꍇ
		std::set<std::string> ignoreNames;

		std::vector<std::string> variableNames;
		std::vector<char> appearances;

		CheckNameAppearance(const std::vector<std::string>& variableNames) :
			variableNames(variableNames),
			appearances(variableNames.size(), 0)
		{}

		CheckNameAppearance& ignore(const std::string& name)
		{
			ignoreNames.insert(name);
			return *this;
		}

		void check(const std::string& name)
		{
			for (size_t i = 0; i < variableNames.size(); ++i)
			{
				if (variableNames[i] == name && ignoreNames.find(name) == ignoreNames.end())
				{
					appearances[i] = 1;
				}
			}
		}

		void merge(const CheckNameAppearance& other)
		{
			assert(appearances.size() == other.appearances.size());

			for (size_t i = 0; i < appearances.size(); ++i)
			{
				appearances[i] |= other.appearances[i];
			}
		}

		void operator()(bool node) {}

		void operator()(int node) {}

		void operator()(double node) {}

		void operator()(const Identifier& node)
		{
			check(node.name);
		}

		void operator()(const List& node) {}
		void operator()(const Record& node) {}
		void operator()(const FuncVal& node) {}

		void operator()(const UnaryExpr& node)
		{
			boost::apply_visitor(*this, node.lhs);
		}

		void operator()(const BinaryExpr& node)
		{
			boost::apply_visitor(*this, node.lhs);
			boost::apply_visitor(*this, node.rhs);
		}

		void operator()(const Range& node)
		{
			boost::apply_visitor(*this, node.lhs);
			boost::apply_visitor(*this, node.rhs);
		}

		void operator()(const Lines& node)
		{
			for (size_t i = 0; i < node.size(); ++i)
			{
				boost::apply_visitor(*this, node.exprs[i]);
			}
		}

		void operator()(const DefFunc& node)
		{
			CheckNameAppearance child(*this);
			for (const auto& argument : node.arguments)
			{
				child.ignore(argument.name);
			}

			boost::apply_visitor(child, node.expr);
			merge(child);
		}

		void operator()(const If& node)
		{
			boost::apply_visitor(*this, node.cond_expr);
			boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				boost::apply_visitor(*this, node.else_expr.value());
			}
		}

		void operator()(const For& node)
		{
			boost::apply_visitor(*this, node.rangeStart);
			boost::apply_visitor(*this, node.rangeEnd);

			CheckNameAppearance child(*this);
			child.ignore(node.loopCounter.name);
			boost::apply_visitor(child, node.doExpr);
			merge(child);
		}

		void operator()(const Return& node)
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const ListConstractor& node)
		{
			for (const auto& expr : node.data)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const KeyExpr& node)
		{
			//���O�t���ϐ��̐錾��������ȍ~��������
			ignore(node.name.name);

			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const RecordConstractor& node)
		{
			for (const auto& expr : node.exprs)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const RecordInheritor& node)
		{
			if (auto opt = AsOpt<Identifier>(node.original))
			{
				check(opt.value().name);
			}

			const Expr expr = node.adder;
			boost::apply_visitor(*this, expr);
		}

		void operator()(const Accessor& node)
		{
			boost::apply_visitor(*this, node.head);

			for (const auto& access : node.accesses)
			{
				if (auto listAccess = AsOpt<ListAccess>(access))
				{
					boost::apply_visitor(*this, listAccess.value().index);
				}
				else if (auto recordAccess = AsOpt<RecordAccess>(access))
				{
					//.a�Ƃ��ɂ��Ă͓��ɉ������Ȃ�
				}
				else if (auto functionAccess = AsOpt<FunctionAccess>(access))
				{
					for (const auto& argument : functionAccess.value().actualArguments)
					{
						boost::apply_visitor(*this, argument);
					}
				}
			}
		}

		void operator()(const FunctionCaller& node)
		{
			if (auto funcNameOpt = AsOpt<Identifier>(node.funcRef))
			{
				const Expr expr = funcNameOpt.value();
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const DeclSat& node)
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const DeclFree& node)
		{
			for (const auto& access : node.accessors)
			{
				const Expr expr = access;
				boost::apply_visitor(*this, expr);
			}
		}
	};
#endif

#ifdef commentout
	//�����̎w�肳�ꂽ�ϐ���l�Œu������������Ԃ�
	class ReplaceExprValue : public boost::static_visitor<Expr>
	{
	public:

		//�����ł���Ƀ��[�J���ϐ�����`�����ꍇ
		std::set<std::string> ignoreNames;
		std::map<std::string, Evaluated> variableValues;

		/*ReplaceExprValue(const std::map<std::string, Evaluated>& variableNames) :
		variableValues(variableValues)
		{}*/
		ReplaceExprValue(const std::map<std::string, Evaluated>& variableNames) :
			variableValues(variableNames)
		{}

		ReplaceExprValue& ignore(const std::string& name)
		{
			ignoreNames.insert(name);
			return *this;
		}

		bool isIgnored(const std::string& name)const
		{
			return ignoreNames.find(name) != ignoreNames.end();
		}

		Expr operator()(bool node) { return node; }

		Expr operator()(int node) { return node; }

		Expr operator()(double node) { return node; }

		Expr operator()(const Identifier& node)
		{
			auto it = variableValues.find(node.name);

			std::cout << "Identifier(" << node.name << ") was " << (it == variableValues.end() ? "not found.\n" : "found.\n");
			std::cout << "  and was " << (isIgnored(node.name) ? "ignored." : "not ignored.");
			std::cout << std::endl;

			if (it == variableValues.end() || isIgnored(node.name))
			{
				std::cout << "return node;" << std::endl;
				return node;
			}

			if (auto opt = AsOpt<bool>(it->second))
			{
				std::cout << "return bool;" << std::endl;
				return opt.value();
			}
			else if (auto opt = AsOpt<int>(it->second))
			{
				std::cout << "return int;" << std::endl;
				return opt.value();
			}
			else if (auto opt = AsOpt<double>(it->second))
			{
				std::cout << "return double;" << std::endl;
				return opt.value();
			}
			else if (auto opt = AsOpt<Identifier>(it->second))
			{
				std::cout << "return Identifier;" << std::endl;
				return opt.value();
			}
			else if (auto opt = AsOpt<List>(it->second))
			{
				std::cout << "return List;" << std::endl;
				return opt.value();
			}
			else if (auto opt = AsOpt<Record>(it->second))
			{
				std::cout << "return Record;" << std::endl;
				return opt.value();
			}
			else if (auto opt = AsOpt<FuncVal>(it->second))
			{
				std::cout << "return FuncVal;" << std::endl;
				return opt.value();
			}

			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
		}

		Expr operator()(const List& node) { return node; }
		Expr operator()(const Record& node) { return node; }
		Expr operator()(const FuncVal& node) { return node; }

		Expr operator()(const UnaryExpr& node)
		{
			const auto resultExpr = boost::apply_visitor(*this, node.lhs);
			return UnaryExpr(resultExpr, node.op);
		}

		Expr operator()(const BinaryExpr& node)
		{
			const auto resultExprLhs = boost::apply_visitor(*this, node.lhs);
			const auto resultExprRhs = boost::apply_visitor(*this, node.rhs);
			return BinaryExpr(resultExprLhs, resultExprRhs, node.op);
		}

		Expr operator()(const Range& node)
		{
			const auto resultExprLhs = boost::apply_visitor(*this, node.lhs);
			const auto resultExprRhs = boost::apply_visitor(*this, node.rhs);
			return Range(resultExprLhs, resultExprRhs);
		}

		Expr operator()(const Lines& node)
		{
			Lines resultLines;
			for (size_t i = 0; i < node.size(); ++i)
			{
				resultLines.add(boost::apply_visitor(*this, node.exprs[i]));
			}
			return resultLines;
		}

		Expr operator()(const DefFunc& node)
		{
			DefFunc resultDefFunc;
			resultDefFunc.arguments = node.arguments;

			{
				ReplaceExprValue child(*this);
				for (const auto& argument : node.arguments)
				{
					child.ignore(argument.name);
				}

				resultDefFunc.expr = boost::apply_visitor(child, node.expr);
			}

			//�K�w�ɂ���ĎՕ������/����Ȃ��̋������ς��Ǝv���̂�
			//����ς�variableValues�͒P�Ȃ�map���Ⴞ�߂ȋC������

			return resultDefFunc;
		}

		Expr operator()(const If& node)
		{
			If resultIf;
			resultIf.cond_expr = boost::apply_visitor(*this, node.cond_expr);
			resultIf.then_expr = boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				resultIf.else_expr = boost::apply_visitor(*this, node.else_expr.value());
			}

			return resultIf;
		}

		Expr operator()(const For& node)
		{
			For resultFor;
			resultFor.rangeStart = boost::apply_visitor(*this, node.rangeStart);
			resultFor.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
			resultFor.loopCounter = node.loopCounter;

			{
				ReplaceExprValue child(*this);
				child.ignore(node.loopCounter.name);
				resultFor.doExpr = boost::apply_visitor(child, node.doExpr);
			}

			return resultFor;
		}

		Expr operator()(const Return& node)
		{
			return boost::apply_visitor(*this, node.expr);
		}

		Expr operator()(const ListConstractor& node)
		{
			ListConstractor resultListConstractor;
			for (const auto& expr : node.data)
			{
				resultListConstractor.data.push_back(boost::apply_visitor(*this, expr));
			}
			return resultListConstractor;
		}

		Expr operator()(const KeyExpr& node)
		{
			KeyExpr resultKeyExpr;

			resultKeyExpr.name = node.name;

			//���O�t���ϐ��̐錾��������ȍ~��������
			ignore(node.name.name);

			resultKeyExpr.expr = boost::apply_visitor(*this, node.expr);

			return resultKeyExpr;
		}

		Expr operator()(const RecordConstractor& node)
		{
			RecordConstractor resultRecordConstractor;
			for (const auto& expr : node.exprs)
			{
				resultRecordConstractor.exprs.push_back(boost::apply_visitor(*this, expr));
			}
			return resultRecordConstractor;
		}

		Expr operator()(const RecordInheritor& node)
		{
			RecordInheritor resultRecordInheritor;

			auto resultOriginal = boost::apply_visitor(*this, node.original);
			if (IsType<Identifier>(resultOriginal))
			{
				resultRecordInheritor.original = As<Identifier>(resultOriginal);
			}
			else if (IsType<Record>(resultOriginal))
			{
				resultRecordInheritor.original = As<Record>(resultOriginal);
			}
			else
			{
				std::cerr << "Error(" << __LINE__ << "): invalid expression\n";
				return 0;
			}

			const Expr expr = node.adder;
			resultRecordInheritor.adder = As<RecordConstractor>(boost::apply_visitor(*this, expr));

			return resultRecordInheritor;
		}

		Expr operator()(const Accessor& node)
		{
			return node;
			//TODO: node.head�����ʎq�ł��Q�Ƃ���Ă����ꍇ�A�S�̂�ObjectReference�ɒu�������ĕԂ�
		}

		Expr operator()(const FunctionCaller& node) { return node; }

		Expr operator()(const DeclSat& node) { return node; }

		Expr operator()(const DeclFree& node) { return node; }
	};
#endif

#ifdef commentout
	class EvalSatExpr : public boost::static_visitor<double>
	{
	public:

		const std::vector<double>& data;

		EvalSatExpr(const std::vector<double>& data) :
			data(data)
		{}

		double operator()(double node)const
		{
			return node;
		}

		double operator()(const SatReference& node)const
		{
			return data[node.refID];
		}

		double operator()(const SatUnaryExpr& node)const
		{
			const double lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
				//case UnaryOp::Not:   return lhs;
			case UnaryOp::Minus: return -lhs;
			case UnaryOp::Plus:  return lhs;
			}

			std::cerr << "Error(" << __LINE__ << ")\n";

return 0.0;
		}

		double operator()(const SatBinaryExpr& node)const
		{
			const double lhs = boost::apply_visitor(*this, node.lhs);
			const double rhs = boost::apply_visitor(*this, node.rhs);

			switch (node.op)
			{
			case BinaryOp::And: return lhs + rhs;
			case BinaryOp::Or:  return std::min(lhs, rhs);

			case BinaryOp::Equal:        return std::abs(lhs - rhs);
			case BinaryOp::NotEqual:     return lhs == rhs ? 1.0 : 0.0;
			case BinaryOp::LessThan:     return std::max(lhs - rhs, 0.0);
			case BinaryOp::LessEqual:    return std::max(lhs - rhs, 0.0);
			case BinaryOp::GreaterThan:  return std::max(rhs - lhs, 0.0);
			case BinaryOp::GreaterEqual: return std::max(rhs - lhs, 0.0);

			case BinaryOp::Add: return lhs + rhs;
			case BinaryOp::Sub: return lhs - rhs;
			case BinaryOp::Mul: return lhs * rhs;
			case BinaryOp::Div: return lhs / rhs;

			case BinaryOp::Pow: return std::pow(lhs, rhs);
			}

			std::cerr << "Error(" << __LINE__ << ")\n";

			return 0.0;
		}

		/*double operator()(const SatLines& node)const
		{
		node.exprs;
		}*/

		double operator()(const SatFunctionReference& node)const
		{
			std::map<std::string, std::function<double(double)>> unaryFunctions;
			//unaryFunctions["sin"] = std::sin;
			//unaryFunctions["cos"] = std::cos;
			unaryFunctions["sin"] = [](double x) {return sin(x); };
			unaryFunctions["cos"] = [](double x) {return cos(x); };

			//const auto currentFunction = unaryFunctions[node.funcName];
			const auto currentFunction = unaryFunctions[0];
			const auto funcAccess = As<SatFunctionReference::FunctionRef>(node.references.front());

			const double firstArgument = boost::apply_visitor(*this, funcAccess.args.front());
			return currentFunction(firstArgument);

			/*
			const auto frontVal = funcAccess.args.front();
			if (IsType<SatReference>(frontVal))
			{
			SatExpr expr = As<SatReference>(frontVal);
			const double lhs = boost::apply_visitor(*this, expr);
			return currentFunction(lhs);
			}
			else
			{
			const auto evaluated = As<Evaluated>(frontVal);
			if (IsType<double>(evaluated))
			{
			return currentFunction(As<double>(evaluated));
			}
			else if (IsType<int>(evaluated))
			{
			return currentFunction(As<int>(evaluated));
			}
			else
			{
			std::cerr << "Error(" << __LINE__ << "):Function argument is invalid\n";
			return 0;
			}
			}
			*/
		}
	};
#endif

#ifdef commentout
	class EvalSatExpr : public boost::static_visitor<Evaluated>
	{
	public:
		std::shared_ptr<Environment> pEnv;
		const std::vector<double>& data;

		//����pEnv�͊O���̊����������������Ȃ��̂ŁA�Ɨ��������̂�ݒ肷��
		EvalSatExpr(std::shared_ptr<Environment> pEnv, const std::vector<double>& data) :
			pEnv(pEnv),
			data(data)
		{}

		Evaluated operator()(const LRValue& node)
		{
			if (node.isLValue())
			{
				return pEnv->expand(node.address());
			}
			return node.evaluated();
		}

		Evaluated operator()(const SatReference& node)
		{
			return data[node.refID];
		}
		
		Evaluated operator()(const Identifier& node)
		{
			const Address address = pEnv->findAddress(node);
			return pEnv->expand(address);
		}

		Evaluated operator()(const UnaryExpr& node)
		{
			if (node.op == UnaryOp::Not)
			{
				CGL_Error("TODO: sat�錾���̒P�����Z�q�͖��Ή��ł�");
			}
			
			return 0;
		}

		Evaluated operator()(const BinaryExpr& node)
		{
			const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
			const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

			switch (node.op)
			{
			case BinaryOp::And: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Or:  return Min(lhs, rhs, *pEnv);

			case BinaryOp::Equal:        return Abs(Sub(lhs, rhs, *pEnv), *pEnv);
			case BinaryOp::NotEqual:     return Equal(lhs, rhs, *pEnv) ? 1.0 : 0.0;
			case BinaryOp::LessThan:     return Max(Sub(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::LessEqual:    return Max(Sub(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterThan:  return Max(Sub(rhs, lhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterEqual: return Max(Sub(rhs, lhs, *pEnv), 0.0, *pEnv);

			case BinaryOp::Add: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Sub: return Sub(lhs, rhs, *pEnv);
			case BinaryOp::Mul: return Mul(lhs, rhs, *pEnv);
			case BinaryOp::Div: return Div(lhs, rhs, *pEnv);

			case BinaryOp::Pow: return Pow(lhs, rhs, *pEnv);
			}

			CGL_Error(std::string("sat�錾�̒��ł͓񍀉��Z�q") + "\"" + BinaryOpToStr(node.op) + "\"" + "�͎g�p�ł��܂���");

			return 0;
		}

		Evaluated operator()(const DefFunc& node) { CGL_Error("�s���Ȏ��ł�"); return 0; }

		Evaluated operator()(const FunctionCaller& callFunc)
		{
			//CGL_DebugLog("Function Environment:");
			//pEnv->printEnvironment();

			std::vector<Address> expandedArguments(callFunc.actualArguments.size());
			for (size_t i = 0; i < expandedArguments.size(); ++i)
			{
				expandedArguments[i] = pEnv->makeTemporaryValue(callFunc.actualArguments[i]);
			}

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

			if (funcVal.builtinFuncAddress)
			{
				return pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments);
			}

			//CGL_DebugLog("");

			if (funcVal.arguments.size() != callFunc.actualArguments.size())
			{
				CGL_Error("�������̐��Ǝ������̐��������Ă��Ȃ�");
			}

			//CGL_DebugLog("");

			pEnv->switchFrontScope();
			
			//CGL_DebugLog("");

			pEnv->enterScope();

			//CGL_DebugLog("");

			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
			}

			//CGL_DebugLog("");

			//CGL_DebugLog("Function Definition:");
			//boost::apply_visitor(Printer(), funcVal.expr);

			Evaluated result;
			{
				//�֐����ʏ�̊֐��ł͂Ȃ��A�����\���֐��ł���͂��Ȃ̂ŁA�]����Eval�ł͂Ȃ�*this�ōs��
				result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
				//CGL_DebugLog("Function Evaluated:");
				//printEvaluated(result, nullptr);
			}
			//Evaluated result = pEnv->expandObject();

			//CGL_DebugLog("");

			//(4)�֐��𔲂��鎞�ɁA�������͑S�ĉ�������
			pEnv->exitScope();

			//CGL_DebugLog("");

			//(5)�Ō�Ƀ��[�J���ϐ��̊����֐��̎��s�O�̂��̂ɖ߂��B
			pEnv->switchBackScope();

			//CGL_DebugLog("");

			return result;
		}

		Evaluated operator()(const Range& node) { CGL_Error("�s���Ȏ��ł�"); return 0; }

		Evaluated operator()(const Lines& node)
		{
			if (node.exprs.size() != 1)
			{
				CGL_Error("�s���Ȏ��ł�"); return 0;
			}
			
			return boost::apply_visitor(*this, node.exprs.front());
		}

		Evaluated operator()(const If& if_statement)
		{
			Eval evaluator(pEnv);
			
			//if���̏������͕��ʂɐ��񂪖�������Ă��邩�ǂ�����]������ׂ�
			const Evaluated cond = pEnv->expand(boost::apply_visitor(evaluator, if_statement.cond_expr));
			if (!IsType<bool>(cond))
			{
				CGL_Error("�����͕K���u�[���l�ł���K�v������");
			}

			//then��else�͕��ʂɐ��񂪖��������܂ł̋������v�Z����
			if (As<bool>(cond))
			{
				return pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr));
			}
			else if (if_statement.else_expr)
			{
				return pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.value()));
			}

			CGL_Error("if����else������");
			return 0;
		}

		Evaluated operator()(const For& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const Return& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const ListConstractor& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const KeyExpr& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const RecordConstractor& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return 0; }
		Evaluated operator()(const DeclSat& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const Accessor& node)
		{
			//Expr2SatExpr�ɂ��node.head�̓A�h���X�l�ɂȂ��Ă���͂�
			if (!IsType<LRValue>(node.head))
			{
				CGL_Error("�����͒ʂ�Ȃ��͂�");
			}

			const LRValue& headAddressValue = As<LRValue>(node.head);
			if (!headAddressValue.isLValue())
			{
				CGL_Error("�����͒ʂ�Ȃ��͂�");
			}

			Address address = headAddressValue.address();

			for (const auto& access : node.accesses)
			{
				boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(address);
				if (!objOpt)
				{
					CGL_Error("�Q�ƃG���[");
				}

				const Evaluated& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					Evaluated value = boost::apply_visitor(*this, listAccessOpt.value().index);

					if (!IsType<List>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
					}

					const List& list = As<const List&>(objRef);

					const auto clampAddress = [&](int index)->int {return std::max(0, std::min(index, static_cast<int>(list.data.size()) - 1)); };

					if (auto indexOpt = AsOpt<int>(value))
					{
						address = list.get(clampAddress(indexOpt.value()));
					}
					else if (auto indexOpt = AsOpt<double>(value))
					{
						address = list.get(clampAddress(std::round(indexOpt.value())));
					}
					else
					{
						CGL_Error("list[index] �� index �����l�^�łȂ�");
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

					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
					}

					Expr caller = FunctionCaller(function, args);
					const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
					address = pEnv->makeTemporaryValue(returnedValue);
				}
			}

			return pEnv->expand(address);
		}
	};
#endif

	class EvalSatExpr : public boost::static_visitor<Evaluated>
	{
	public:
		std::shared_ptr<Environment> pEnv;
		const std::vector<double>& data;//�Q��ID->data
		const std::vector<Address>& refs;//�Q��ID->Address
		const std::unordered_map<Address, int>& invRefs;//Address->�Q��ID

		//TODO:����pEnv�͊O���̊����������������Ȃ��̂ŁA�Ɨ��������̂�ݒ肷��
		EvalSatExpr(
			std::shared_ptr<Environment> pEnv, 
			const std::vector<double>& data, 
			const std::vector<Address>& refs,
			const std::unordered_map<Address, int>& invRefs) :
			pEnv(pEnv),
			data(data),
			refs(refs),
			invRefs(invRefs)
		{}

		/*bool expand(const LRValue& lrvalue)
		{

		}*/
		bool isFreeVariable(Address address)const
		{
			return invRefs.find(address) != invRefs.end();
		}

		boost::optional<double> expandFreeOpt(Address address)const
		{
			auto it = invRefs.find(address);
			if (it != invRefs.end())
			{
				return data[it->second];
			}
			return boost::none;
		}

		Evaluated operator()(const LRValue& node)
		{
			if (node.isLValue())
			{
				if (auto opt = expandFreeOpt(node.address()))
				{
					return opt.value();
				}
				//CGL_DebugLog(std::string("address: ") + node.address().toString());
				return pEnv->expand(node.address());
			}
			return node.evaluated();
		}

		Evaluated operator()(const SatReference& node)
		{
			CGL_Error("�s���Ȏ�");
			return 0;
		}

		Evaluated operator()(const Identifier& node)
		{
			pEnv->printEnvironment();
			//CGL_DebugLog(std::string("find: ") + std::string(node));
			const Address address = pEnv->findAddress(node);
			//CGL_DebugLog(std::string("address: ") + address.toString());
			if (auto opt = expandFreeOpt(address))
			{
				return opt.value();
			}
			return pEnv->expand(address);
		}

		Evaluated operator()(const UnaryExpr& node)
		{
			if (node.op == UnaryOp::Not)
			{
				CGL_Error("TODO: sat�錾���̒P�����Z�q�͖��Ή��ł�");
			}

			return 0;
		}

		Evaluated operator()(const BinaryExpr& node)
		{
			const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
			const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

			switch (node.op)
			{
			case BinaryOp::And: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Or:  return Min(lhs, rhs, *pEnv);

			case BinaryOp::Equal:        return Abs(Sub(lhs, rhs, *pEnv), *pEnv);
			case BinaryOp::NotEqual:     return Equal(lhs, rhs, *pEnv) ? 1.0 : 0.0;
			case BinaryOp::LessThan:     return Max(Sub(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::LessEqual:    return Max(Sub(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterThan:  return Max(Sub(rhs, lhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterEqual: return Max(Sub(rhs, lhs, *pEnv), 0.0, *pEnv);

			case BinaryOp::Add: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Sub: return Sub(lhs, rhs, *pEnv);
			case BinaryOp::Mul: return Mul(lhs, rhs, *pEnv);
			case BinaryOp::Div: return Div(lhs, rhs, *pEnv);

			case BinaryOp::Pow: return Pow(lhs, rhs, *pEnv);
			}

			CGL_Error(std::string("sat�錾�̒��ł͓񍀉��Z�q") + "\"" + BinaryOpToStr(node.op) + "\"" + "�͎g�p�ł��܂���");

			return 0;
		}

		Evaluated operator()(const DefFunc& node) { CGL_Error("�s���Ȏ��ł�"); return 0; }

		Evaluated operator()(const FunctionCaller& callFunc)
		{
			std::vector<Address> expandedArguments(callFunc.actualArguments.size());
			for (size_t i = 0; i < expandedArguments.size(); ++i)
			{
				expandedArguments[i] = pEnv->makeTemporaryValue(callFunc.actualArguments[i]);
			}

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

			if (funcVal.builtinFuncAddress)
			{
				return pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments);
			}

			if (funcVal.arguments.size() != callFunc.actualArguments.size())
			{
				CGL_Error("�������̐��Ǝ������̐��������Ă��Ȃ�");
			}

			//CGL_DebugLog("");

			pEnv->switchFrontScope();

			//CGL_DebugLog("");

			pEnv->enterScope();

			//CGL_DebugLog("");

			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
			}

			//CGL_DebugLog("");

			//CGL_DebugLog("Function Definition:");
			//boost::apply_visitor(Printer(), funcVal.expr);

			Evaluated result;
			{
				//�֐����ʏ�̊֐��ł͂Ȃ��A�����\���֐��ł���͂��Ȃ̂ŁA�]����Eval�ł͂Ȃ�*this�ōs��
				result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
				//CGL_DebugLog("Function Evaluated:");
				//printEvaluated(result, nullptr);
			}
			//Evaluated result = pEnv->expandObject();

			//CGL_DebugLog("");

			//(4)�֐��𔲂��鎞�ɁA�������͑S�ĉ�������
			pEnv->exitScope();

			//CGL_DebugLog("");

			//(5)�Ō�Ƀ��[�J���ϐ��̊����֐��̎��s�O�̂��̂ɖ߂��B
			pEnv->switchBackScope();

			//CGL_DebugLog("");

			return result;
		}

		Evaluated operator()(const Range& node) { CGL_Error("�s���Ȏ��ł�"); return 0; }

		Evaluated operator()(const Lines& node)
		{
			if (node.exprs.size() != 1)
			{
				CGL_Error("�s���Ȏ��ł�"); return 0;
			}

			return boost::apply_visitor(*this, node.exprs.front());
		}

		Evaluated operator()(const If& if_statement)
		{
			Eval evaluator(pEnv);

			//if���̏������͐��񂪖�������Ă��邩�ǂ�����]������ׂ�
			const Evaluated cond = pEnv->expand(boost::apply_visitor(evaluator, if_statement.cond_expr));
			if (!IsType<bool>(cond))
			{
				CGL_Error("�����͕K���u�[���l�ł���K�v������");
			}

			//then��else�͐��񂪖��������܂ł̋������v�Z����
			if (As<bool>(cond))
			{
				return pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr));
			}
			else if (if_statement.else_expr)
			{
				return pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.value()));
			}

			CGL_Error("if����else������");
			return 0;
		}

		Evaluated operator()(const For& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const Return& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const ListConstractor& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const KeyExpr& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const RecordConstractor& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return 0; }
		Evaluated operator()(const DeclSat& node) { CGL_Error("invalid expression"); return 0; }

		Evaluated operator()(const Accessor& node)
		{
			//Expr2SatExpr�ɂ��node.head�̓A�h���X�l�ɂȂ��Ă���͂�
			if (!IsType<LRValue>(node.head))
			{
				CGL_Error("�����͒ʂ�Ȃ��͂�");
			}

			const LRValue& headAddressValue = As<LRValue>(node.head);
			if (!headAddressValue.isLValue())
			{
				CGL_Error("�����͒ʂ�Ȃ��͂�");
			}

			Address address = headAddressValue.address();

			for (const auto& access : node.accesses)
			{
				boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(address);
				if (!objOpt)
				{
					CGL_Error("�Q�ƃG���[");
				}

				const Evaluated& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					Evaluated value = boost::apply_visitor(*this, listAccessOpt.value().index);

					if (!IsType<List>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
					}

					const List& list = As<const List&>(objRef);

					const auto clampAddress = [&](int index)->int {return std::max(0, std::min(index, static_cast<int>(list.data.size()) - 1)); };

					if (auto indexOpt = AsOpt<int>(value))
					{
						address = list.get(clampAddress(indexOpt.value()));
					}
					else if (auto indexOpt = AsOpt<double>(value))
					{
						address = list.get(clampAddress(std::round(indexOpt.value())));
					}
					else
					{
						CGL_Error("list[index] �� index �����l�^�łȂ�");
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

					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
					}

					Expr caller = FunctionCaller(function, args);
					const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
					address = pEnv->makeTemporaryValue(returnedValue);
				}
			}

			return pEnv->expand(address);
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
		}
		else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		};

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
		else if (IsType<FunctionCaller>(value1))
		{
			return As<FunctionCaller>(value1) == As<FunctionCaller>(value2);
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
		else if (IsType<Accessor>(value1))
		{
			return As<Accessor>(value1) == As<Accessor>(value2);
		};

		std::cerr << "IsEqual: Type Error" << std::endl;
		return false;
	}
}
