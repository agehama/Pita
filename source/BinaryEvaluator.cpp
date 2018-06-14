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
				constraint = BinaryExpr(rec1.constraint.get(), rec2.constraint.get(), BinaryOp::And);
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
				constraint = BinaryExpr(constraint.get(), rec2.constraint.get(), BinaryOp::And);
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

	Val NotFunc(const Val& lhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);

		if (IsType<bool>(lhs))
		{
			return !As<bool>(lhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val PlusFunc(const Val& lhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);

		if (IsType<int>(lhs) || IsType<double>(lhs))
		{
			return lhs;
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val MinusFunc(const Val& lhs, Context& env)
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

	Val AndFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) && As<bool>(rhs);
		}
		else if (IsNum(lhs) && IsNum(rhs))
		{
			const double eps = 1.e-4;
			return AsDouble(lhs) < eps && AsDouble(rhs) < eps;
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val OrFunc(const Val& lhs, const Val& rhs, Context& env)
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

	bool EqualFunc(const Val& lhs, const Val& rhs, Context& env)
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

	bool NotEqualFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		//const Val lhs = env.expandRef(lhs_);
		//const Val rhs = env.expandRef(rhs_);

		return !EqualFunc(lhs, rhs, env);
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

	bool LessThanFunc(const Val& lhs, const Val& rhs, Context& env)
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

	bool LessEqualFunc(const Val& lhs, const Val& rhs, Context& env)
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

	bool GreaterThanFunc(const Val& lhs, const Val& rhs, Context& env)
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

	bool GreaterEqualFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val MaxFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val MinFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val AbsFunc(const Val& lhs, Context& env)
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

	Val SinFunc(const Val& lhs)
	{
		if (IsNum(lhs))
		{
			return std::sin(AsDouble(lhs));
		}

		/*if (IsType<int>(lhs))
		{
			return std::sin(deg2rad*As<int>(lhs));
		}
		else if (IsType<double>(lhs))
		{
			return std::sin(deg2rad*As<double>(lhs));
		}*/

		CGL_Error("不正な式です");
		return 0;
	}

	Val CosFunc(const Val& lhs)
	{
		if (IsNum(lhs))
		{
			return std::cos(AsDouble(lhs));
		}

		/*if (IsType<int>(lhs))
		{
			return std::cos(deg2rad*As<int>(lhs));
		}
		else if (IsType<double>(lhs))
		{
			return std::cos(deg2rad*As<double>(lhs));
		}*/

		CGL_Error("不正な式です");
		return 0;
	}

	Val AddFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val SubFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val MulFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val DivFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val PowFunc(const Val& lhs, const Val& rhs, Context& env)
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

	Val ConcatFunc(const Val& lhs, const Val& rhs, Context& env)
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
		const UnpackedList& unpackedLHS = unpackedLHSOpt.get();

		auto unpackedRHSOpt = As<List>(rhs).asUnpackedOpt();
		if (!unpackedRHSOpt)
		{
			CGL_Error("List is packed");
		}
		const UnpackedList& unpackedRHS = unpackedRHSOpt.get();

		return List(UnpackedList::Concat(unpackedLHS, unpackedRHS));*/

		return List::Concat(As<List>(lhs), As<List>(rhs));
	}
}
