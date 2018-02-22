#pragma once
#include <cmath>
#include <Pita/Node.hpp>
#include <Pita/BinaryEvaluator.hpp>

namespace cgl
{
#ifdef commentout
	Record MargeRecord(const Record& rec1, const Record& rec2)
	{
		Record result;
		
		{
			auto& values = result.values;

			values = rec1.values;

			for (const auto& keyval : rec2.values)
			{
				values[keyval.first] = keyval.second;
			}
		}
		
		//TODO:重複した制約などを考慮するべき

		{
			
			/*auto& constraint = result.constraint;
			
			if (rec1.constraint && rec2.constraint)
			{
				constraint = BinaryExpr(rec1.constraint.value(), rec2.constraint.value(), BinaryOp::And);
			}
			else if (rec1.constraint)
			{
				constraint = rec1.constraint;
			}
			else if (rec2.constraint)
			{
				constraint = rec2.constraint;
			}*/
		}

		{
			auto& freeVals = result.freeVariables;
			freeVals = rec1.freeVariables;
			freeVals.insert(freeVals.end(), rec2.freeVariables.begin(), rec2.freeVariables.end());
		}

		return result;
	}

	void MargeRecordInplace(Record& result, const Record& rec2)
	{
		{
			auto& values = result.values;
			for (const auto& keyval : rec2.values)
			{
				values[keyval.first] = keyval.second;
			}
		}

		//TODO:重複した制約などを考慮するべき
		/*{
			auto& constraint = result.constraint;

			if (constraint && rec2.constraint)
			{
				constraint = BinaryExpr(constraint.value(), rec2.constraint.value(), BinaryOp::And);
			}
			else if (rec2.constraint)
			{
				constraint = rec2.constraint;
			}
		}*/

		{
			auto& freeVals = result.freeVariables;
			freeVals.insert(freeVals.end(), rec2.freeVariables.begin(), rec2.freeVariables.end());
		}
	}
#endif

	Val Not(const Val& lhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);

		if (IsType<bool>(lhs))
		{
			return !As<bool>(lhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Plus(const Val& lhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);

		if (IsType<int>(lhs) || IsType<double>(lhs))
		{
			return lhs;
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Minus(const Val& lhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);

		if (IsType<int>(lhs))
		{
			return -As<int>(lhs);
		}
		else if (IsType<double>(lhs))
		{
			return -As<double>(lhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val And(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) && As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Or(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) || As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	bool Equal(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);
		const double eps = 0.001;

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) == As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				//return As<int>(lhs) == As<double>(rhs);
				return std::abs(static_cast<double>(As<int>(lhs)) - As<double>(rhs)) < eps;
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				//return As<double>(lhs) == As<int>(rhs);
				return std::abs(As<double>(lhs) - static_cast<double>(As<int>(rhs))) < eps;
			}
			else if (IsType<double>(rhs))
			{
				//return As<double>(lhs) == As<double>(rhs);
				return std::abs(As<double>(lhs) - As<double>(rhs)) < eps;
			}
		}

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) == As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool NotEqual(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		return !Equal(lhs, rhs, env);
		/*
		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) != As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) != As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) != As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) != As<double>(rhs);
			}
		}

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) != As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return false;
		*/
	}

	bool LessThan(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) < As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) < As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) < As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) < As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool LessEqual(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) <= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) <= As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) <= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) <= As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool GreaterThan(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) > As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) > As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) > As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) > As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool GreaterEqual(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) >= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) >= As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) >= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) >= As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	Val Max(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::max(As<int>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::max<double>(As<int>(lhs), As<double>(rhs));
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::max<double>(As<double>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::max(As<double>(lhs), As<double>(rhs));
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Min(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::min(As<int>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::min<double>(As<int>(lhs), As<double>(rhs));
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::min<double>(As<double>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::min(As<double>(lhs), As<double>(rhs));
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Abs(const Val& lhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);

		if (IsType<int>(lhs))
		{
			return std::abs(As<int>(lhs));
		}
		else if (IsType<double>(lhs))
		{
			return std::abs(As<double>(lhs));
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Sin(const Val& lhs)
	{
		if (IsType<int>(lhs))
		{
			return std::sin(deg2rad*As<int>(lhs));
		}
		else if (IsType<double>(lhs))
		{
			return std::sin(deg2rad*As<double>(lhs));
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Cos(const Val& lhs)
	{
		if (IsType<int>(lhs))
		{
			return std::cos(deg2rad*As<int>(lhs));
		}
		else if (IsType<double>(lhs))
		{
			return std::cos(deg2rad*As<double>(lhs));
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Add(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) + As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) + As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) + As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) + As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Sub(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) - As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) - As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) - As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) - As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Mul(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) * As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) * As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) * As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) * As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Div(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) / As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) / As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) / As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) / As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Pow(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return pow(As<int>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return pow(As<int>(lhs), As<double>(rhs));
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return pow(As<double>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return pow(As<double>(lhs), As<double>(rhs));
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val Concat(const Val& lhs, const Val& rhs, Context& env)
	{
		if (!IsType<List>(lhs) || !IsType<List>(rhs))
		{
			CGL_Error("リスト結合演算子がリスト以外の式に使われています");
			return 0;
		}

		/*auto unpackedLHSOpt = As<List>(lhs).asUnpackedOpt();
		if (!unpackedLHSOpt)
		{
			CGL_Error("List is packed");
		}
		const UnpackedList& unpackedLHS = unpackedLHSOpt.value();

		auto unpackedRHSOpt = As<List>(rhs).asUnpackedOpt();
		if (!unpackedRHSOpt)
		{
			CGL_Error("List is packed");
		}
		const UnpackedList& unpackedRHS = unpackedRHSOpt.value();

		return List(UnpackedList::Concat(unpackedLHS, unpackedRHS));*/

		return List::Concat(As<List>(lhs), As<List>(rhs));
	}
}
