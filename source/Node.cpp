#pragma warning(disable:4996)
#include <Unicode.hpp>

#include <Pita/Node.hpp>
#include <Pita/OptimizationEvaluator.hpp>

namespace cgl
{
	Identifier Identifier::MakeIdentifier(const std::u32string& name_)
	{
		return Identifier(Unicode::UTF32ToUTF8(name_));
	}

	LRValue LRValue::Float(const std::u32string& str)
	{
		return LRValue(std::stod(Unicode::UTF32ToUTF8(str)));
	}

	Expr BuildString(const std::u32string& str32)
	{
		Expr expr;
		expr = LRValue(CharString(str32));

		return expr;
	}

	void OptimizationProblemSat::addConstraint(const Expr& logicExpr)
	{
		if (candidateExpr)
		{
			candidateExpr = BinaryExpr(candidateExpr.value(), logicExpr, BinaryOp::And);
		}
		else
		{
			candidateExpr = logicExpr;
		}
	}

	void OptimizationProblemSat::constructConstraint(std::shared_ptr<Context> pEnv, std::vector<Address>& freeVariables)
	{
		if (!candidateExpr)
		{
			expr = boost::none;
			return;
		}

		/*Expr2SatExpr evaluator(0, pEnv, freeVariables);
		expr = boost::apply_visitor(evaluator, candidateExpr.value());
		refs.insert(refs.end(), evaluator.refs.begin(), evaluator.refs.end());

		{
			CGL_DebugLog("Print:");
			Printer printer;
			boost::apply_visitor(printer, expr.value());
			CGL_DebugLog("");
		}*/

		
		CGL_DebugLog("freeVariables:");
		for (const auto& val : freeVariables)
		{
			CGL_DebugLog(std::string("  Address(") + val.toString() + ")");
		}

		if (freeVariables.empty())
		{
			expr = candidateExpr;
			refs.clear();
			invRefs.clear();
			freeVariables.clear();
			return;
		}

		SatVariableBinder binder(pEnv, freeVariables);
		if (boost::apply_visitor(binder, candidateExpr.value()))
		{
			expr = candidateExpr.value();
			refs = binder.refs;
			invRefs = binder.invRefs;
			hasPlateausFunction = binder.hasPlateausFunction;

			//satに出てこないfreeVariablesの削除
			for (int i = static_cast<int>(freeVariables.size()) - 1; 0 <= i; --i)
			{
				if (binder.usedInSat[i] == 0)
				{
					freeVariables.erase(freeVariables.begin() + i);
				}
			}
		}
		else
		{
			expr = boost::none;
			refs.clear();
			invRefs.clear();
			freeVariables.clear();
			hasPlateausFunction = false;
		}

		{
			CGL_DebugLog("env:");
			pEnv->printContext(true);

			CGL_DebugLog("expr:");
			printExpr(candidateExpr.value());
		}
	}

	bool OptimizationProblemSat::initializeData(std::shared_ptr<Context> pEnv)
	{
		//std::cout << "Begin OptimizationProblemSat::initializeData" << std::endl;

		data.resize(refs.size());

		for (size_t i = 0; i < data.size(); ++i)
		{
			//const Evaluated val = pEnv->expandRef(refs[i]);
			const Evaluated val = pEnv->expand(refs[i]);
			if (auto opt = AsOpt<double>(val))
			{
				CGL_DebugLog(ToS(i) + " : " + ToS(opt.value()));
				data[i] = opt.value();
			}
			else if (auto opt = AsOpt<int>(val))
			{
				CGL_DebugLog(ToS(i) + " : " + ToS(opt.value()));
				data[i] = opt.value();
			}
			else
			{
				//存在しない参照をsatに指定した
				/*std::cerr << "Error(" << __LINE__ << "): reference does not exist.\n";
				return false;*/
				CGL_Error("存在しない参照をsatに指定した");
			}
		}

		//std::cout << "End OptimizationProblemSat::initializeData" << std::endl;
		return true;
	}

	double OptimizationProblemSat::eval(std::shared_ptr<Context> pEnv)
	{
		if (!expr)
		{
			return 0.0;
		}

		if (data.empty())
		{
			CGL_WarnLog("free式に有効な変数が指定されていません。");
			return 0.0;
		}

		/*{
			CGL_DebugLog("data:");
			for(int i=0;i<data.size();++i)
			{
				CGL_DebugLog(std::string("  ID(") + ToS(i) + ") -> " + ToS(data[i]));
			}

			CGL_DebugLog("refs:");
			for (int i = 0; i<refs.size(); ++i)
			{
				CGL_DebugLog(std::string("  ID(") + ToS(i) + ") -> Address(" + refs[i].toString() + ")");
			}

			CGL_DebugLog("invRefs:");
			for(const auto& keyval : invRefs)
			{
				CGL_DebugLog(std::string("  Address(") + keyval.first.toString() + ") -> ID(" + ToS(keyval.second) + ")");
			}

			CGL_DebugLog("env:");
			pEnv->printContext();

			CGL_DebugLog("expr:");
			printExpr(expr.value());
		}*/
		
		EvalSatExpr evaluator(pEnv, data, refs, invRefs);
		const Evaluated evaluated = boost::apply_visitor(evaluator, expr.value());
		
		if (IsType<double>(evaluated))
		{
			return As<double>(evaluated);
		}
		else if (IsType<int>(evaluated))
		{
			return As<int>(evaluated);
		}

		CGL_Error("sat式の評価結果が不正");
	}
}
