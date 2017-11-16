#pragma once
#include "Node.hpp"
#include "Environment.hpp"
#include "BinaryEvaluator.hpp"
#include "Printer.hpp"

namespace cgl
{
	class EvalSat : public boost::static_visitor<Evaluated>
	{
	public:

		EvalSat(std::shared_ptr<Environment> pEnv) :pEnv(pEnv) {}

		Evaluated operator()(bool node)
		{
			return node;
		}

		Evaluated operator()(int node)
		{
			return node;
		}

		Evaluated operator()(double node)
		{
			return node;
		}

		Evaluated operator()(const Identifier& node)
		{
			return pEnv->dereference(node);
		}

		Evaluated operator()(const UnaryExpr& node)
		{
			const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case UnaryOp::Not:   return Not(lhs, *pEnv);
			case UnaryOp::Minus: return Minus(lhs, *pEnv);
			case UnaryOp::Plus:  return Plus(lhs, *pEnv);
			}

			std::cerr << "Error(" << __LINE__ << ")\n";

			return 0;
		}

		Evaluated operator()(const BinaryExpr& node)
		{
			const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
			const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

			/*
			a == b => Minimize(C * Abs(a - b))
			a != b => Minimize(C * (a == b ? 1 : 0))
			a < b  => Minimize(C * Max(a - b, 0))
			a > b  => Minimize(C * Max(b - a, 0))

			e : error
			a == b => Minimize(C * Max(Abs(a - b) - e, 0))
			a != b => Minimize(C * (a == b ? 1 : 0))
			a < b  => Minimize(C * Max(a - b - e, 0))
			a > b  => Minimize(C * Max(b - a - e, 0))
			*/
			switch (node.op)
			{
			case BinaryOp::And: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Or:  return Max(lhs, rhs, *pEnv);

			case BinaryOp::Equal:        return Sub(lhs, rhs, *pEnv);
			case BinaryOp::NotEqual:     return As<bool>(Equal(lhs, rhs, *pEnv)) ? 1.0 : 0.0;
			case BinaryOp::LessThan:     return Max(Sub(lhs, rhs, *pEnv), 0, *pEnv);
			case BinaryOp::LessEqual:    return Max(Sub(lhs, rhs, *pEnv), 0, *pEnv);
			case BinaryOp::GreaterThan:  return Max(Sub(rhs, lhs, *pEnv), 0, *pEnv);
			case BinaryOp::GreaterEqual: return Max(Sub(rhs, lhs, *pEnv), 0, *pEnv);

			case BinaryOp::Add: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Sub: return Sub(lhs, rhs, *pEnv);
			case BinaryOp::Mul: return Mul(lhs, rhs, *pEnv);
			case BinaryOp::Div: return Div(lhs, rhs, *pEnv);

			case BinaryOp::Pow:    return Pow(lhs, rhs, *pEnv);
				//case BinaryOp::Assign: return Assign(lhs, rhs, *pEnv);
			}

			std::cerr << "Error(" << __LINE__ << ")\n";

			return 0;
		}

		Evaluated operator()(const Range& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const Lines& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const DefFunc& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const If& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const For& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const Return& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const ListConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const KeyExpr& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const RecordConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const Accessor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const FunctionCaller& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const DeclSat& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Evaluated operator()(const DeclFree& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }

	private:

		std::shared_ptr<Environment> pEnv;
	};


	class Eval : public boost::static_visitor<Evaluated>
	{
	public:

		Eval(std::shared_ptr<Environment> pEnv) :pEnv(pEnv) {}

		Evaluated operator()(bool node)
		{
			return node;
		}

		Evaluated operator()(int node)
		{
			return node;
		}

		Evaluated operator()(double node)
		{
			return node;
		}

		Evaluated operator()(const Identifier& node)
		{
			return node;
		}

		Evaluated operator()(const UnaryExpr& node)
		{
			const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case UnaryOp::Not:   return Not(lhs, *pEnv);
			case UnaryOp::Minus: return Minus(lhs, *pEnv);
			case UnaryOp::Plus:  return Plus(lhs, *pEnv);
			}

			//std::cout << "End UnaryExpr expression(" << ")" << std::endl;
			std::cerr << "Error(" << __LINE__ << ")\n";

			//return lhs;
			return 0;
		}

		Evaluated operator()(const BinaryExpr& node)
		{
			const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
			const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

			switch (node.op)
			{
			case BinaryOp::And: return And(lhs, rhs, *pEnv);
			case BinaryOp::Or:  return Or(lhs, rhs, *pEnv);

			case BinaryOp::Equal:        return Equal(lhs, rhs, *pEnv);
			case BinaryOp::NotEqual:     return NotEqual(lhs, rhs, *pEnv);
			case BinaryOp::LessThan:     return LessThan(lhs, rhs, *pEnv);
			case BinaryOp::LessEqual:    return LessEqual(lhs, rhs, *pEnv);
			case BinaryOp::GreaterThan:  return GreaterThan(lhs, rhs, *pEnv);
			case BinaryOp::GreaterEqual: return GreaterEqual(lhs, rhs, *pEnv);

			case BinaryOp::Add: return Add(lhs, rhs, *pEnv);
			case BinaryOp::Sub: return Sub(lhs, rhs, *pEnv);
			case BinaryOp::Mul: return Mul(lhs, rhs, *pEnv);
			case BinaryOp::Div: return Div(lhs, rhs, *pEnv);

			case BinaryOp::Pow:    return Pow(lhs, rhs, *pEnv);
			case BinaryOp::Assign: return Assign(lhs, rhs, *pEnv);
			}

			std::cerr << "Error(" << __LINE__ << ")\n";

			return 0;
		}

		Evaluated operator()(const DefFunc& defFunc)
		{
			//auto val = FuncVal(globalVariables, defFunc.arguments, defFunc.expr);
			//auto val = FuncVal(pEnv, defFunc.arguments, defFunc.expr);

			//FuncVal val(std::make_shared<Environment>(*pEnv), defFunc.arguments, defFunc.expr);
			FuncVal val(Environment::Make(*pEnv), defFunc.arguments, defFunc.expr);

			return val;
		}

		Evaluated operator()(const FunctionCaller& callFunc)
		{
			std::shared_ptr<Environment> buckUp = pEnv;

			FuncVal funcVal;

			//if (IsType<FuncVal>(callFunc.funcRef))
			if (auto opt = AsOpt<FuncVal>(callFunc.funcRef))
			{
				funcVal = opt.value();
			}
			else if (auto funcOpt = pEnv->find(As<Identifier>(callFunc.funcRef).name))
			{
				const Evaluated& funcRef = funcOpt.value();
				if (IsType<FuncVal>(funcRef))
				{
					funcVal = As<FuncVal>(funcRef);
				}
				else
				{
					std::cerr << "Error(" << __LINE__ << "): variable \"" << As<Identifier>(callFunc.funcRef).name << "\" is not a function.\n";
					return 0;
				}
			}

			assert(funcVal.arguments.size() == callFunc.actualArguments.size());

			/*
			�܂��Q�Ƃ��X�R�[�v�Ԃŋ��L�ł���悤�ɂ��Ă��Ȃ����߁A�����ɗ^����ꂽ�I�u�W�F�N�g�͑S�ēW�J���ēn���B
			�����āA�����̕]�����_�ł͂܂��֐��̒��ɓ����Ă��Ȃ��̂ŁA�X�R�[�v��ς���O�ɓW�J���s���B
			*/
			std::vector<Evaluated> expandedArguments(funcVal.arguments.size());
			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				expandedArguments[i] = pEnv->expandObject(callFunc.actualArguments[i]);
			}

			/*
			�֐��̕]��
			�����ł̃��[�J���ϐ��͊֐����Ăяo�������ł͂Ȃ��A�֐�����`���ꂽ���̂��̂��g���̂Ń��[�J���ϐ���u��������B
			*/
			pEnv = funcVal.environment;

			//�֐��̈����p�ɃX�R�[�v����ǉ�����
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

			/*
			�֐��̖߂�l�����̃X�R�[�v�ɖ߂������A�����Ɠ������R�őS�ēW�J���ēn���B
			*/
			Evaluated result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));

			//�֐��𔲂��鎞�ɁA�������͑S�ĉ�������
			pEnv->pop();

			/*
			�Ō�Ƀ��[�J���ϐ��̊����֐��̎��s�O�̂��̂ɖ߂��B
			*/
			pEnv = buckUp;

			//�]�����ʂ�return���������ꍇ��return���O���Ē��g��Ԃ�
			//return�ȊO�̃W�����v���߂͊֐��ł͌��ʂ������Ȃ��̂ł��̂܂܏�ɕԂ�
			if (IsType<Jump>(result))
			{
				auto& jump = As<Jump>(result);
				if (jump.isReturn())
				{
					if (jump.lhs)
					{
						return jump.lhs.value();
					}
					else
					{
						//return���̒��g�������Ė�����΂Ƃ肠�����G���[
						std::cerr << "Error(" << __LINE__ << ")\n";
						return 0;
					}
				}
			}

			return result;
		}

		Evaluated operator()(const Range& range)
		{
			return 0;
		}

		Evaluated operator()(const Lines& statement)
		{
			pEnv->pushNormal();

			Evaluated result;
			int i = 0;
			for (const auto& expr : statement.exprs)
			{
				std::cout << "Evaluate expression(" << i << ")" << std::endl;
				result = boost::apply_visitor(*this, expr);

				//���̕]�����ʂ����Ӓl�̏ꍇ�͒��g�����āA���ꂪ�}�N���ł���Β��g��W�J�������ʂ����̕]�����ʂƂ���
				if (IsLValue(result))
				{
					const Evaluated& resultValue = pEnv->dereference(result);
					const bool isMacro = IsType<Jump>(resultValue);
					if (isMacro)
					{
						result = As<Jump>(resultValue);
					}
				}

				//�r���ŃW�����v���߂�ǂ񂾂瑦���ɕ]�����I������
				if (IsType<Jump>(result))
				{
					break;
				}

				++i;
			}

			//���̌シ����������̂� dereference ���Ă���
			result = pEnv->dereference(result);

			pEnv->pop();

			return result;
		}

		Evaluated operator()(const If& if_statement)
		{
			const Evaluated cond = boost::apply_visitor(*this, if_statement.cond_expr);
			if (!IsType<bool>(cond))
			{
				//�����͕K���u�[���l�ł���K�v������
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}

			if (As<bool>(cond))
			{
				const Evaluated result = boost::apply_visitor(*this, if_statement.then_expr);
				return result;

			}
			else if (if_statement.else_expr)
			{
				const Evaluated result = boost::apply_visitor(*this, if_statement.else_expr.value());
				return result;
			}

			//else���������P�[�X�� cond = False �ł�������ꉞ�x�����o��
			std::cerr << "Warning(" << __LINE__ << ")\n";
			return 0;
		}

		Evaluated operator()(const For& forExpression)
		{
			const Evaluated startVal = pEnv->dereference(boost::apply_visitor(*this, forExpression.rangeStart));
			const Evaluated endVal = pEnv->dereference(boost::apply_visitor(*this, forExpression.rangeEnd));

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
					std::cerr << "Error(" << __LINE__ << ")\n";
					return boost::none;
				}

				const bool result_IsDouble = a_IsDouble || b_IsDouble;
				const auto lessEq = LessEqual(a, b, *pEnv);
				if (!IsType<bool>(lessEq))
				{
					//�G���[�Fa��b�̔�r�Ɏ��s����
					//�ꉞ�m���߂Ă��邾���ł�����ʂ邱�Ƃ͂Ȃ��͂�
					//LessEqual�̎����~�X�H
					std::cerr << "Error(" << __LINE__ << ")\n";
					return boost::none;
				}

				const bool isInOrder = As<bool>(lessEq);

				const int sign = isInOrder ? 1 : -1;

				if (result_IsDouble)
				{
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
					std::cerr << "Error(" << __LINE__ << ")\n";
					return boost::none;
				}

				return As<bool>(result) == isInOrder;
			};

			const auto stepOrder = calcStepValue(startVal, endVal);
			if (!stepOrder)
			{
				//�G���[�F���[�v�̃����W���s��
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}

			const Evaluated step = stepOrder.value().first;
			const bool isInOrder = stepOrder.value().second;

			Evaluated loopCountValue = startVal;

			Evaluated loopResult;

			pEnv->pushNormal();

			while (true)
			{
				const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
				if (!isLoopContinuesOpt)
				{
					//�G���[�F������ʂ邱�Ƃ͂Ȃ��͂�
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}

				//���[�v�̌p�������𖞂����Ȃ������̂Ŕ�����
				if (!isLoopContinuesOpt.value())
				{
					break;
				}

				pEnv->bindNewValue(forExpression.loopCounter.name, loopCountValue);

				loopResult = boost::apply_visitor(*this, forExpression.doExpr);

				//���[�v�J�E���^�̍X�V
				loopCountValue = Add(loopCountValue, step, *pEnv);
			}

			pEnv->pop();

			return loopResult;
		}

		Evaluated operator()(const Return& return_statement)
		{
			const Evaluated lhs = boost::apply_visitor(*this, return_statement.expr);

			return Jump::MakeReturn(lhs);
		}

		Evaluated operator()(const ListConstractor& listConstractor)
		{
			List list;
			for (const auto& expr : listConstractor.data)
			{
				const Evaluated value = boost::apply_visitor(*this, expr);
				list.append(value);
			}

			return list;
		}

		Evaluated operator()(const KeyExpr& keyExpr)
		{
			const Evaluated value = boost::apply_visitor(*this, keyExpr.expr);

			return KeyValue(keyExpr.name, value);
		}

		Evaluated operator()(const RecordConstractor& recordConsractor)
		{
			pEnv->pushRecord();

			std::vector<Identifier> keyList;
			/*
			���R�[�h����:���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͐V���ɒ�`
			���R�[�h����=���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͂��̃X�R�[�v���ł̂ݗL���Ȓl�̃G�C���A�X�Ƃ��Ē�`�i�X�R�[�v�𔲂����猳�ɖ߂���Օ��j
			*/

			Record record;

			int i = 0;
			for (const auto& expr : recordConsractor.exprs)
			{
				//std::cout << "Evaluate expression(" << i << ")" << std::endl;
				Evaluated value = boost::apply_visitor(*this, expr);

				//�L�[�ɕR�Â�����l�͂��̌�̎葱���ōX�V����邩������Ȃ��̂ŁA���͖��O�����T���Ă����Č�Œl���Q�Ƃ���
				if (auto keyValOpt = AsOpt<KeyValue>(value))
				{
					const auto keyVal = keyValOpt.value();
					keyList.push_back(keyVal.name);

					Assign(keyVal.name, keyVal.value, *pEnv);
				}
				else if (auto declSatOpt = AsOpt<DeclSat>(value))
				{

					//Expr constraints;
					//std::vector<DeclFree::Ref> freeVariables;

					//record.constraint = declSatOpt.value().expr;
					if (record.constraint)
					{
						record.constraint = BinaryExpr(record.constraint.value(), declSatOpt.value().expr, BinaryOp::And);
					}
					else
					{
						record.constraint = declSatOpt.value().expr;
					}
				}
				else if (auto declFreeOpt = AsOpt<DeclFree>(value))
				{
					const auto& refs = declFreeOpt.value().refs;
					//record.freeVariables.push_back(declFreeOpt.value().refs);

					record.freeVariables.insert(record.freeVariables.end(), refs.begin(), refs.end());
				}

				//���̕]�����ʂ����Ӓl�̏ꍇ�͒��g�����āA���ꂪ�}�N���ł���Β��g��W�J�������ʂ����̕]�����ʂƂ���
				if (IsLValue(value))
				{
					const Evaluated& resultValue = pEnv->dereference(value);
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

				++i;
			}

			for (const auto& key : keyList)
			{
				record.append(key.name, pEnv->dereference(key));
			}

			//auto Environment::Make(*pEnv);
			if (record.constraint)
			{
				const Expr& constraint = record.constraint.value();

				/*std::vector<double> xs(record.freeVariables.size());
				for (size_t i = 0; i < xs.size(); ++i)
				{
				if (!IsType<Identifier>(record.freeVariables[i]))
				{
				std::cerr << "���Ή�" << std::endl;
				return 0;
				}

				As<Identifier>(record.freeVariables[i]).name;

				const auto val = pEnv->dereference(As<Identifier>(record.freeVariables[i]));

				if (!IsType<int>(val))
				{
				std::cerr << "���Ή�" << std::endl;
				return 0;
				}

				xs[i] = As<int>(val);
				}*/

				/*
				a == b => Minimize(C * Abs(a - b))
				a != b => Minimize(C * (a == b ? 1 : 0))
				a < b  => Minimize(C * Max(a - b, 0))
				a > b  => Minimize(C * Max(b - a, 0))

				e : error
				a == b => Minimize(C * Max(Abs(a - b) - e, 0))
				a != b => Minimize(C * (a == b ? 1 : 0))
				a < b  => Minimize(C * Max(a - b - e, 0))
				a > b  => Minimize(C * Max(b - a - e, 0))
				*/

				const auto func = [&](const std::vector<double>& xs)->double
				{
					pEnv->pushNormal();

					for (size_t i = 0; i < xs.size(); ++i)
					{
						if (!IsType<Identifier>(record.freeVariables[i]))
						{
							std::cerr << "���Ή�" << std::endl;
							return 0;
						}

						As<Identifier>(record.freeVariables[i]).name;

						const auto val = pEnv->dereference(As<Identifier>(record.freeVariables[i]));

						pEnv->bindNewValue(As<Identifier>(record.freeVariables[i]).name, val);
					}

					EvalSat evaluator(pEnv);
					Evaluated errorVal = boost::apply_visitor(evaluator, constraint);

					pEnv->pop();

					const double d = As<double>(errorVal);
					return d;
				};
			}

			pEnv->pop();
			return record;
		}

		Evaluated operator()(const Accessor& accessor)
		{
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

			for (const auto& access : accessor.accesses)
			{
				if (auto listOpt = AsOpt<ListAccess>(access))
				{
					Evaluated value = boost::apply_visitor(*this, listOpt.value().index);

					if (auto indexOpt = AsOpt<int>(value))
					{
						result.appendListRef(indexOpt.value());
					}
					else
					{
						//�G���[�Flist[index] �� index �� int �^�łȂ�
						std::cerr << "Error(" << __LINE__ << ")\n";
						return 0;
					}
				}
				else if (auto recordOpt = AsOpt<RecordAccess>(access))
				{
					result.appendRecordRef(recordOpt.value().name.name);
				}
				else
				{
					auto funcAccess = As<FunctionAccess>(access);

					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(boost::apply_visitor(*this, expr));
					}
					result.appendFunctionRef(std::move(args));
				}
			}

			return result;
		}

		Evaluated operator()(const DeclSat& decl)
		{
			return decl;
		}

		Evaluated operator()(const DeclFree& decl)
		{
			return decl;
		}

	private:

		std::shared_ptr<Environment> pEnv;
	};

	inline Evaluated evalExpr(const Expr& expr)
	{
		auto env = Environment::Make();

		Eval evaluator(env);

		const Evaluated result = boost::apply_visitor(evaluator, expr);

		return result;
	}

	inline bool IsEqual(const Evaluated& value1, const Evaluated& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
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
			return As<double>(value1) == As<double>(value2);
		}
		else if (IsType<Identifier>(value1))
		{
			return As<Identifier>(value1) == As<Identifier>(value2);
		}
		else if (IsType<ObjectReference>(value1))
		{
			return As<ObjectReference>(value1) == As<ObjectReference>(value2);
		}
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
		else if (IsType<DeclSat>(value1))
		{
			return As<DeclSat>(value1) == As<DeclSat>(value2);
		}
		else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		};

		std::cerr << "IsEqual: Type Error" << std::endl;
		return false;
	}
}
