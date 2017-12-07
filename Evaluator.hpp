#pragma once
#pragma warning(disable:4996)
#include <iomanip>
#include "Node.hpp"
#include "Environment.hpp"
#include "BinaryEvaluator.hpp"
#include "Printer.hpp"

namespace cgl
{
	class Expr2SatExpr : public boost::static_visitor<SatExpr>
	{
	public:

		int refID_Offset;

		//AccessorからObjectReferenceに変換するのに必要
		std::shared_ptr<Environment> pEnv;

		//freeに指定された変数全て
		std::vector<ObjectReference> freeVariables;
		
		//freeに指定された変数が実際にsatに現れたかどうか
		std::vector<char> usedInSat;

		//FreeVariables Index -> SatReference
		std::map<int, SatReference> satRefs;

		//std::vector<Accessor> refs;
		//TODO:vector->mapに書き換える
		std::vector<ObjectReference> refs;

		Expr2SatExpr(int refID_Offset, std::shared_ptr<Environment> pEnv, const std::vector<ObjectReference>& freeVariables) :
			refID_Offset(refID_Offset),
			pEnv(pEnv),
			freeVariables(freeVariables),
			usedInSat(freeVariables.size(), 0)
		{}

		boost::optional<size_t> freeVariableIndex(const ObjectReference& reference)
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

		boost::optional<SatReference> getSatRef(const ObjectReference& reference)
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

		boost::optional<SatReference> addSatRef(const ObjectReference& reference)
		{
			if (auto indexOpt = freeVariableIndex(reference))
			{
				//以前に出現して登録済みのfree変数はそのまま返す
				if (auto satRefOpt = getSatRef(reference))
				{
					return satRefOpt;
				}

				//初めて出現したfree変数は登録してから返す
				usedInSat[indexOpt.value()] = 1;
				SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
				refs.push_back(reference);
				return satRef;
			}

			return boost::none;
		}

		SatExpr operator()(bool node)
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
		}

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

			std::cerr << "Error(" << __LINE__ << ")\n";

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

			std::cerr << "Error(" << __LINE__ << ")\n";

			return 0;
		}

		SatExpr operator()(const Range& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		
		SatExpr operator()(const Lines& node)
		{
			if (node.exprs.size() != 1)
			{
				std::cerr << "Error(" << __LINE__ << "): invalid expression\n";
				return 0;
			}

			const SatExpr lhs = boost::apply_visitor(*this, node.exprs.front());
			return lhs;
		}

		SatExpr operator()(const DefFunc& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const If& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const For& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const Return& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const ListConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const KeyExpr& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const RecordConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const RecordInheritor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }

		SatExpr operator()(const Accessor& node);

		SatExpr operator()(const FunctionCaller& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const DeclSat& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		SatExpr operator()(const DeclFree& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	};

	class ExprFuncExpander : public boost::static_visitor<Expr>
	{
	public:

		std::shared_ptr<Environment> pEnv;

		ExprFuncExpander(std::shared_ptr<Environment> pEnv) :
			pEnv(pEnv)
		{}

		Expr operator()(bool node) { return node; }

		Expr operator()(int node) { return node; }

		Expr operator()(double node) { return node; }

		Expr operator()(const Identifier& node) { return node; }

		Expr operator()(const UnaryExpr& node)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case UnaryOp::Not:   return UnaryExpr(lhs, UnaryOp::Not);
			case UnaryOp::Minus: return UnaryExpr(lhs, UnaryOp::Minus);
			case UnaryOp::Plus:  return lhs;
			}

			std::cerr << "Error(" << __LINE__ << ")\n";

			return 0.0;
		}

		Expr operator()(const BinaryExpr& node)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);
			const Expr rhs = boost::apply_visitor(*this, node.rhs);

			switch (node.op)
			{
			case BinaryOp::And: return BinaryExpr(lhs, rhs, BinaryOp::And);
			case BinaryOp::Or:  return BinaryExpr(lhs, rhs, BinaryOp::Or);

			case BinaryOp::Equal:        return BinaryExpr(lhs, rhs, BinaryOp::Equal);
			case BinaryOp::NotEqual:     return BinaryExpr(lhs, rhs, BinaryOp::NotEqual);
			case BinaryOp::LessThan:     return BinaryExpr(lhs, rhs, BinaryOp::LessThan);
			case BinaryOp::LessEqual:    return BinaryExpr(lhs, rhs, BinaryOp::LessEqual);
			case BinaryOp::GreaterThan:  return BinaryExpr(lhs, rhs, BinaryOp::GreaterThan);
			case BinaryOp::GreaterEqual: return BinaryExpr(lhs, rhs, BinaryOp::GreaterEqual);

			case BinaryOp::Add: return BinaryExpr(lhs, rhs, BinaryOp::Add);
			case BinaryOp::Sub: return BinaryExpr(lhs, rhs, BinaryOp::Sub);
			case BinaryOp::Mul: return BinaryExpr(lhs, rhs, BinaryOp::Mul);
			case BinaryOp::Div: return BinaryExpr(lhs, rhs, BinaryOp::Div);

			case BinaryOp::Pow: return BinaryExpr(lhs, rhs, BinaryOp::Pow);
			}

			std::cerr << "Error(" << __LINE__ << ")\n";

			return 0;
		}

		Expr operator()(const Range& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const Lines& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const DefFunc& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const If& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const For& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const Return& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const ListConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const KeyExpr& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const RecordConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const RecordInheritor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }

		Expr operator()(const Accessor& node);

		Expr operator()(const FunctionCaller& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const DeclSat& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
		Expr operator()(const DeclFree& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	};

	class HasFreeVariables : public boost::static_visitor<bool>
	{
	public:

		HasFreeVariables(std::shared_ptr<Environment> pEnv, const std::vector<ObjectReference>& freeVariables) :
			pEnv(pEnv),
			freeVariables(freeVariables)
		{}

		//AccessorからObjectReferenceに変換するのに必要
		std::shared_ptr<Environment> pEnv;

		//freeに指定された変数全て
		std::vector<ObjectReference> freeVariables;

		bool operator()(bool node) { return false; }

		bool operator()(int node) { return false; }

		bool operator()(double node) { return false; }

		bool operator()(const Identifier& node);
		bool operator()(const Accessor& node);

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

		bool operator()(const Range& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const Lines& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const DefFunc& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const If& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const For& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const Return& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const ListConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const KeyExpr& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const RecordConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const RecordInheritor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const FunctionCaller& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const DeclSat& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
		bool operator()(const DeclFree& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return false; }
	};

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

			const auto currentFunction = unaryFunctions[node.funcName];
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
			//FuncVal val(Environment::Make(*pEnv), defFunc.arguments, defFunc.expr);
			//FuncVal val(defFunc.arguments, defFunc.expr, pEnv->scopeDepth());
			//FuncVal val(defFunc.arguments, defFunc.expr, pEnv->currentReferenceableVariables(), pEnv->scopeDepth());
			return pEnv->makeFuncVal(defFunc.arguments, defFunc.expr);
		}

		Evaluated operator()(const FunctionCaller& callFunc)
		{
			//std::shared_ptr<Environment> buckUp = pEnv;

			std::cout << "CALLFUNC_A" << std::endl;

			std::cout << "Function Environment:\n";
			pEnv->printEnvironment();

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

			std::cout << "CALLFUNC_B" << std::endl;

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

			std::cout << "CALLFUNC_C" << std::endl;

			//関数の評価
			//(1)ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える

			
			pEnv->switchFrontScope(funcVal.currentScopeDepth);

			std::cout << "CALLFUNC_D" << std::endl;

			//(2)関数の引数用にスコープを一つ追加する
			pEnv->enterScope();

			std::cout << "CALLFUNC_E" << std::endl;

			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				//現在は値も変数も全て値渡し（コピー）をしているので、単純に bindNewValue を行えばよい
				//本当は変数の場合は bindReference で参照情報だけ渡すべきなのかもしれない
				//要考察
				//pEnv->bindNewValue(funcVal.arguments[i].name, callFunc.actualArguments[i]);

				pEnv->bindNewValue(funcVal.arguments[i].name, expandedArguments[i]);
			}

			std::cout << "CALLFUNC_F" << std::endl;

			std::cout << "Function Definition:\n";
			boost::apply_visitor(Printer(), funcVal.expr);

			//(3)関数の戻り値を元のスコープに戻す時も、引数と同じ理由で全て展開して渡す。
			Evaluated result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));

			std::cout << "CALLFUNC_G" << std::endl;
			
			//(4)関数を抜ける時に、仮引数は全て解放される
			pEnv->exitScope();

			std::cout << "CALLFUNC_H" << std::endl;

			//(5)最後にローカル変数の環境を関数の実行前のものに戻す。
			pEnv->switchBackScope();

			std::cout << "CALLFUNC_I" << std::endl;
			/*
			//関数の評価
			//(1)ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える
			pEnv = funcVal.environment;

			//(2)関数の引数用にスコープを一つ追加する
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

			//(3)関数の戻り値を元のスコープに戻す時も、引数と同じ理由で全て展開して渡す。
			Evaluated result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));

			//(4)関数を抜ける時に、仮引数は全て解放される
			pEnv->pop();

			//(5)最後にローカル変数の環境を関数の実行前のものに戻す。
			pEnv = buckUp;
			*/

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
			//pEnv->pushNormal();
			pEnv->enterScope();

			Evaluated result;
			int i = 0;
			for (const auto& expr : statement.exprs)
			{
				std::cout << "Evaluate expression(" << i << ")" << std::endl;
				pEnv->printEnvironment();

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
			bool deref = true;
			if (auto refOpt = AsOpt<ObjectReference>(result))
			{
				if (IsType<unsigned>(refOpt.value().headValue))
				{
					deref = false;
				}
			}
			
			if (deref)
			{
				result = pEnv->dereference(result);
			}
			
			//pEnv->pop();
			pEnv->exitScope();

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

			//pEnv->pushNormal();
			pEnv->enterScope();

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

			//pEnv->pop();
			pEnv->exitScope();

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
			//pEnv->pushRecord();

			std::cout << "RECORD_A" << std::endl;
			pEnv->enterScope();
			std::cout << "RECORD_B" << std::endl;

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
					/*if (record.constraint)
					{
						record.constraint = BinaryExpr(record.constraint.value(), declSatOpt.value().expr, BinaryOp::And);
					}
					else
					{
						record.constraint = declSatOpt.value().expr;
					}*/

					record.problem.addConstraint(declSatOpt.value().expr);
					//record.problem.candidateExprs.push_back(declSatOpt.value().expr);
				}
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


			std::cout << "RECORD_C" << std::endl;
			//制約の解決を行ってからまとめて代入する
			/*for (const auto& key : keyList)
			{
				record.append(key.name, pEnv->dereference(key));
			}*/

			std::vector<double> resultxs;
			if (record.problem.candidateExpr)
			{
				auto& problem = record.problem;

				const auto& problemRefs = problem.refs;
				const auto& freeVariables = record.freeVariables;

				{
					//record.freeVariablesをもとにrecord.freeVariableRefsを構築
					//全てのアクセッサを展開し、各変数の参照リストを作成する
					record.freeVariableRefs.clear();
					for (const auto& accessor : record.freeVariables)
					{
						//単一の値 or List or Record
						if (auto refVal = pEnv->evalReference(accessor))
						{
							pEnv->expandReferences(refVal.value(), record.freeVariableRefs);
						}
						else
						{
							std::cerr << "Error(" << __LINE__ << "): accessor was not reference.\n";
						}
					}

					//一度sat式の中身を展開し、
					//Accessorを展開するvisitor（Expr -> Expr）を作り、実行する
					//その後Sat2Exprに掛ける
					//Expr2SatExprでは単項演算子・二項演算子の評価の際、中身をみて定数であったら畳み込む処理を行う

					//satの関数呼び出しを全て式に展開する
					problem.candidateExpr = pEnv->expandFunction(problem.candidateExpr.value());

					//展開された式をSatExprに変換する
					//＋satの式に含まれる変数の内、freeVariableRefsに入っていないものは即時に評価して畳み込む
					//freeVariableRefsに入っているものは最適化対象の変数としてdataに追加していく
					//＋freeVariableRefsの変数についてはsat式に出現したかどうかを記録し削除する
					problem.constructConstraint(pEnv, record.freeVariableRefs);

					//設定されたdataが参照している値を見て初期値を設定する
					if (!problem.initializeData(pEnv))
					{
						return 0;
					}

					std::cout << "Record FreeVariablesSize: " << record.freeVariableRefs.size() << std::endl;
					std::cout << "Record SatExpr: " << std::endl;
					//problem.debugPrint();
				}

				//std::cout << "Begin Record MakeMap" << std::endl;
				//DeclFreeに出現する参照について、そのインデックス -> Problemのデータのインデックスを取得するマップ
				std::unordered_map<int, int> variable2Data;
				for (int freeIndex = 0; freeIndex < record.freeVariableRefs.size(); ++freeIndex)
				{
					const auto& ref1 = record.freeVariableRefs[freeIndex];

					bool found = false;
					for (int dataIndex = 0; dataIndex < problemRefs.size(); ++dataIndex)
					{
						const auto& ref2 = problemRefs[dataIndex];

						if (ref1 == ref2)
						{
							//std::cout << "    " << freeIndex << " -> " << dataIndex << std::endl;

							found = true;
							variable2Data[freeIndex] = dataIndex;
							break;
						}
					}

					//TODO:satに出てくる値をまとめたレコードをfreeに書けるようにしたい
					//その場合は、freeに出現する変数が冗長であることも考えられるので、再帰的に中身を見ていって関係ある変数（satに出現する変数だけ取り出す処理も行うべき）

					//DeclFreeにあってDeclSatに無い変数は意味がない。
					//単に無視しても良いが、恐らく入力のミスなので一応エラーを返す。
					if (!found)
					{
						std::cerr << "Error(" << __LINE__ << "):freeに指定された変数が無効です。\n";
						return 0;
					}
				}
				//std::cout << "End Record MakeMap" << std::endl;

				libcmaes::FitFunc func = [&](const double *x, const int N)->double
				{
					for (int i = 0; i < N; ++i)
					{
						//std::cout << "    x[" << i << "] -> " << x[i] << std::endl;
						problem.update(variable2Data[i], x[i]);
					}

					return problem.eval();
				};

				std::vector<double> x0(record.freeVariableRefs.size());
				for (int i = 0; i < x0.size(); ++i)
				{
					x0[i] = problem.data[variable2Data[i]];
				}

				const double sigma = 0.1;

				const int lambda = 100;

				libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda);
				libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);

				resultxs = cmasols.best_candidate().get_x();
			}

			std::cout << "RECORD_D" << std::endl;
			for (int i = 0; i < resultxs.size(); ++i)
			{
				ObjectReference ref = record.freeVariableRefs[i];
				pEnv->assignToObject(ref, resultxs[i]);
			}

			for (const auto& key : keyList)
			{
				record.append(key.name, pEnv->dereference(key));
			}

			std::cout << "RECORD_E" << std::endl;

			//pEnv->pop();
			pEnv->exitScope();

			std::cout << "RECORD_F" << std::endl;

			return record;
		}

		Evaluated operator()(const RecordInheritor& record)
		{
			auto originalOpt = pEnv->find(record.original.name);
			if (!originalOpt)
			{
				//エラー：未定義のレコードを参照しようとした
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
			
			auto recordOpt = AsOpt<Record>(originalOpt.value());
			if (!recordOpt)
			{
				//エラー：識別子の指すオブジェクトがレコード型ではない
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}

			/*
			a{}を評価する手順
			(1) オリジナルのレコードaのクローン(a')を作る
			(2) a'の各キーと値に対する参照をローカルスコープに追加する
			(3) 追加するレコードの中身を評価する
			(4) ローカルスコープの参照値を読みレコードに上書きする //リストアクセスなどの変更処理
			(5) レコードをマージする //ローカル変数などの変更処理
			*/

			//(1)
			Record clone = recordOpt.value();

			//(2)
			//pEnv->pushRecord();
			pEnv->enterScope();
			for (auto& keyval : clone.values)
			{
				pEnv->bindNewValue(keyval.first, keyval.second);
			}

			//(3)
			Expr expr = record.adder;
			Evaluated recordValue = boost::apply_visitor(*this, expr);
			if (auto opt = AsOpt<Record>(recordValue))
			{
				//(4)
				for (auto& keyval : recordOpt.value().values)
				{
					auto valOpt = pEnv->find(keyval.first);
					if (!valOpt)
					{
						//評価して変数が消えることはないはず
						std::cerr << "Error(" << __LINE__ << ")\n";
						return 0;
					}

					clone.values[keyval.first] = valOpt.value();
				}

				//(5)
				MargeRecordInplace(clone, opt.value());

				//TODO:ここで制約処理を行う

				//pEnv->pop();
				pEnv->exitScope();

				return clone;

				//MargeRecord(recordOpt.value(), opt.value());
			}

			//pEnv->pop();
			pEnv->exitScope();

			//ここは通らないはず。{}で囲まれた式を評価した結果がレコードでなかった。
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
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
						Evaluated indexValue = pEnv->dereference(value);
						if (auto indexOpt = AsOpt<int>(indexValue))
						{
							result.appendListRef(indexOpt.value());
						}
						else
						{
							//エラー：list[index] の index が int 型でない
							std::cerr << "Error(" << __LINE__ << "): List index(" << std::endl;
							printEvaluated(indexValue);
							std::cerr << ") is not an Int value.\n" << std::endl;

							return 0;
						}
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
			if ((IsType<int>(value1) || IsType<double>(value1)) && (IsType<int>(value2) || IsType<double>(value2)))
			{
				const Evaluated d1 = IsType<int>(value1) ? static_cast<double>(As<int>(value1)) : As<double>(value1);
				const Evaluated d2 = IsType<int>(value2) ? static_cast<double>(As<int>(value2)) : As<double>(value2);
				return IsEqual(d1, d2);
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

	inline bool IsEqual(const Expr& value1, const Expr& value2)
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
		else if (IsType<Identifier>(value1))
		{
			return As<Identifier>(value1) == As<Identifier>(value2);
		}
		else if (IsType<Range>(value1))
		{
			return As<Range>(value1) == As<Range>(value2);
		}
		else if (IsType<Lines>(value1))
		{
			return As<Lines>(value1) == As<Lines>(value2);
		}
		else if (IsType<DefFunc>(value1))
		{
			return As<DefFunc>(value1) == As<DefFunc>(value2);
		}
		else if (IsType<UnaryExpr>(value1))
		{
			return As<UnaryExpr>(value1) == As<UnaryExpr>(value2);
		}
		else if (IsType<BinaryExpr>(value1))
		{
			return As<BinaryExpr>(value1) == As<BinaryExpr>(value2);
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
		else if (IsType<Accessor>(value1))
		{
			return As<Accessor>(value1) == As<Accessor>(value2);
		}
		else if (IsType<FunctionCaller>(value1))
		{
			return As<FunctionCaller>(value1) == As<FunctionCaller>(value2);
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
