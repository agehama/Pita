#pragma once
#include <cmath>
#include <Pita/Node.hpp>
#include <Pita/BinaryEvaluator.hpp>

namespace cgl
{
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

	Evaluated Not(const Evaluated& lhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);

		if (IsType<bool>(lhs))
		{
			return !As<bool>(lhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Evaluated Plus(const Evaluated& lhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);

		if (IsType<int>(lhs) || IsType<double>(lhs))
		{
			return lhs;
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Evaluated Minus(const Evaluated& lhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);

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

	Evaluated And(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) && As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Evaluated Or(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) || As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	bool Equal(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);
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

	bool NotEqual(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	bool LessThan(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	bool LessEqual(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	bool GreaterThan(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	bool GreaterEqual(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Max(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Min(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Abs(const Evaluated& lhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);

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

	Evaluated Sin(const Evaluated& lhs)
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

	Evaluated Cos(const Evaluated& lhs)
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

	Evaluated Add(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Sub(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Mul(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Div(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Pow(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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

	Evaluated Concat(const Evaluated& lhs, const Evaluated& rhs, Context& env)
	{
		if (!IsType<List>(lhs) || !IsType<List>(rhs))
		{
			CGL_Error("リスト結合演算子がリスト以外の式に使われています");
			return 0;
		}

		return List::Concat(As<List>(lhs), As<List>(rhs));
	}
}
