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
			まだ参照をスコープ間で共有できるようにしていないため、引数に与えられたオブジェクトは全て展開して渡す。
			そして、引数の評価時点ではまだ関数の中に入っていないので、スコープを変える前に展開を行う。
			*/
			std::vector<Evaluated> expandedArguments(funcVal.arguments.size());
			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				expandedArguments[i] = pEnv->expandObject(callFunc.actualArguments[i]);
			}

			/*
			関数の評価
			ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える。
			*/
			pEnv = funcVal.environment;

			//関数の引数用にスコープを一つ追加する
			pEnv->pushNormal();
			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				//現在は値も変数も全て値渡し（コピー）をしているので、単純に bindNewValue を行えばよい
				//本当は変数の場合は bindReference で参照情報だけ渡すべきなのかもしれない
				//要考察
				//pEnv->bindNewValue(funcVal.arguments[i].name, callFunc.actualArguments[i]);

				pEnv->bindNewValue(funcVal.arguments[i].name, expandedArguments[i]);
			}

			std::cout << "Function Environment:\n";
			pEnv->printEnvironment();

			std::cout << "Function Definition:\n";
			boost::apply_visitor(Printer(), funcVal.expr);

			/*
			関数の戻り値を元のスコープに戻す時も、引数と同じ理由で全て展開して渡す。
			*/
			Evaluated result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));

			//関数を抜ける時に、仮引数は全て解放される
			pEnv->pop();

			/*
			最後にローカル変数の環境を関数の実行前のものに戻す。
			*/
			pEnv = buckUp;

			//評価結果がreturn式だった場合はreturnを外して中身を返す
			//return以外のジャンプ命令は関数では効果を持たないのでそのまま上に返す
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
						//return式の中身が入って無ければとりあえずエラー
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

				//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
				if (IsLValue(result))
				{
					const Evaluated& resultValue = pEnv->dereference(result);
					const bool isMacro = IsType<Jump>(resultValue);
					if (isMacro)
					{
						result = As<Jump>(resultValue);
					}
				}

				//途中でジャンプ命令を読んだら即座に評価を終了する
				if (IsType<Jump>(result))
				{
					break;
				}

				++i;
			}

			//この後すぐ解放されるので dereference しておく
			result = pEnv->dereference(result);

			pEnv->pop();

			return result;
		}

		Evaluated operator()(const If& if_statement)
		{
			const Evaluated cond = boost::apply_visitor(*this, if_statement.cond_expr);
			if (!IsType<bool>(cond))
			{
				//条件は必ずブール値である必要がある
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

			//else式が無いケースで cond = False であったら一応警告を出す
			std::cerr << "Warning(" << __LINE__ << ")\n";
			return 0;
		}

		Evaluated operator()(const For& forExpression)
		{
			const Evaluated startVal = pEnv->dereference(boost::apply_visitor(*this, forExpression.rangeStart));
			const Evaluated endVal = pEnv->dereference(boost::apply_visitor(*this, forExpression.rangeEnd));

			//startVal <= endVal なら 1
			//startVal > endVal なら -1
			//を適切な型に変換して返す
			const auto calcStepValue = [&](const Evaluated& a, const Evaluated& b)->boost::optional<std::pair<Evaluated, bool>>
			{
				const bool a_IsInt = IsType<int>(a);
				const bool a_IsDouble = IsType<double>(a);

				const bool b_IsInt = IsType<int>(b);
				const bool b_IsDouble = IsType<double>(b);

				if (!((a_IsInt || a_IsDouble) && (b_IsInt || b_IsDouble)))
				{
					//エラー：ループのレンジが不正な型（整数か実数に評価できる必要がある）
					std::cerr << "Error(" << __LINE__ << ")\n";
					return boost::none;
				}

				const bool result_IsDouble = a_IsDouble || b_IsDouble;
				const auto lessEq = LessEqual(a, b, *pEnv);
				if (!IsType<bool>(lessEq))
				{
					//エラー：aとbの比較に失敗した
					//一応確かめているだけでここを通ることはないはず
					//LessEqualの実装ミス？
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
					//ここを通ることはないはず
					std::cerr << "Error(" << __LINE__ << ")\n";
					return boost::none;
				}

				return As<bool>(result) == isInOrder;
			};

			const auto stepOrder = calcStepValue(startVal, endVal);
			if (!stepOrder)
			{
				//エラー：ループのレンジが不正
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
					//エラー：ここを通ることはないはず
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}

				//ループの継続条件を満たさなかったので抜ける
				if (!isLoopContinuesOpt.value())
				{
					break;
				}

				pEnv->bindNewValue(forExpression.loopCounter.name, loopCountValue);

				loopResult = boost::apply_visitor(*this, forExpression.doExpr);

				//ループカウンタの更新
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
			レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
			レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアスとして定義（スコープを抜けたら元に戻る≒遮蔽）
			*/

			Record record;

			int i = 0;
			for (const auto& expr : recordConsractor.exprs)
			{
				//std::cout << "Evaluate expression(" << i << ")" << std::endl;
				Evaluated value = boost::apply_visitor(*this, expr);

				//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
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

				//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
				if (IsLValue(value))
				{
					const Evaluated& resultValue = pEnv->dereference(value);
					const bool isMacro = IsType<Jump>(resultValue);
					if (isMacro)
					{
						value = As<Jump>(resultValue);
					}
				}

				//途中でジャンプ命令を読んだら即座に評価を終了する
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
				std::cerr << "未対応" << std::endl;
				return 0;
				}

				As<Identifier>(record.freeVariables[i]).name;

				const auto val = pEnv->dereference(As<Identifier>(record.freeVariables[i]));

				if (!IsType<int>(val))
				{
				std::cerr << "未対応" << std::endl;
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
							std::cerr << "未対応" << std::endl;
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
				//エラー：識別子かリテラル以外（評価結果としてオブジェクトを返すような式）へのアクセスには未対応
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
						//エラー：list[index] の index が int 型でない
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
