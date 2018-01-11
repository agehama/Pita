#pragma once
#include <cmath>
#include "Node.hpp"

namespace cgl
{
	inline Record MargeRecord(const Record& rec1, const Record& rec2)
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

	inline void MargeRecordInplace(Record& result, const Record& rec2)
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

	inline Evaluated Not(const Evaluated& lhs, Environment& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);

		if (IsType<bool>(lhs))
		{
			return !As<bool>(lhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	inline Evaluated Plus(const Evaluated& lhs, Environment& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);

		if (IsType<int>(lhs) || IsType<double>(lhs))
		{
			return lhs;
		}

		CGL_Error("不正な式です");
		return 0;
	}

	inline Evaluated Minus(const Evaluated& lhs, Environment& env)
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

	inline Evaluated And(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Or(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline bool Equal(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) == As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) == As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) == As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) == As<double>(rhs);
			}
		}

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) == As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return false;
	}

	inline bool NotEqual(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
	{
		//const Evaluated lhs = env.expandRef(lhs_);
		//const Evaluated rhs = env.expandRef(rhs_);

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
	}

	inline bool LessThan(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline bool LessEqual(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline bool GreaterThan(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline bool GreaterEqual(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Max(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Min(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Abs(const Evaluated& lhs, Environment& env)
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

	inline Evaluated Sin(const Evaluated& lhs)
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

	inline Evaluated Cos(const Evaluated& lhs)
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

	inline Evaluated Add(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Sub(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Mul(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Div(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

	inline Evaluated Pow(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
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

#ifdef commentout
	inline Evaluated Assign(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
	{
		if (!IsLValue(lhs))
		{
			//代入式の左辺が左辺値でない
			CGL_Error("不正な式です");
			return 0;
		}

		/*
		代入式は、左辺と右辺の形の組み合わせにより4パターンの挙動が起こり得る。

		式の左辺：現在の環境にすでに存在する識別子か、新たに束縛を行う識別子か
		式の右辺：左辺値であるか、右辺値であるか

		左辺値の場合は、その参照先を取得し新たに
		*/

		/*
		if (auto nameLhsOpt = AsOpt<Identifier>(lhs))
		{
			const auto& nameLhs = As<Identifier>(lhs).name;

			const bool rhsIsIdentifier = IsType<Identifier>(rhs);
			const bool rhsIsObjRef = IsType<ObjectReference>(rhs);
			if (rhsIsIdentifier)
			{
				const auto& nameRhs = As<Identifier>(rhs).name;

				env.bindReference(nameLhs, nameRhs);
			}
			else if (rhsIsObjRef)
			{
				env.bindObjectRef(nameLhs, As<ObjectReference>(rhs));
			}
			else if (IsType<List>(rhs))
			{
				env.bindNewValue(nameLhs, env.expandList(rhs));
			}
			else
			{
				env.bindNewValue(nameLhs, rhs);
			}
		}
		else if (auto objLhsOpt = AsOpt<ObjectReference>(lhs))
		{
			const auto& objRefLhs = objLhsOpt.value();

			const bool rhsIsIdentifier = IsType<Identifier>(rhs);
			const bool rhsIsObjRef = IsType<ObjectReference>(rhs);
			if (rhsIsIdentifier)
			{
				const auto& nameRhs = As<Identifier>(rhs).name;
				const auto& valueRhs = env.expandRef(nameRhs);

				env.assignToObject(objRefLhs, valueRhs);
			}
			else if (rhsIsObjRef)
			{
				const auto& valueRhs = env.expandRef(rhs);
				env.assignToObject(objRefLhs, valueRhs);
			}
			else if (IsType<List>(rhs))
			{
				env.assignToObject(objRefLhs, env.expandList(rhs));
			}
			else
			{
				env.assignToObject(objRefLhs, rhs);
			}
		}
		*/
		
		//if (auto objLhsOpt = AsOpt<ObjectReference>(lhs))
		if (IsType<Address>(lhs))
		{
			//const auto& objRefLhs = objLhsOpt.value();
			const Address lhsAddress = As<Address>(lhs);

			if (IsType<Address>(rhs))
			{
				env.assignAddress(lhsAddress, As<Address>(rhs));
			}
			else
			{
				env.assignToObject(lhsAddress, rhs);
			}
			/*const bool rhsIsIdentifier = IsType<Identifier>(rhs);
			const bool rhsIsObjRef = IsType<ObjectReference>(rhs);

			if (rhsIsObjRef)
			{
				const auto& valueRhs = env.expandRef(rhs);
				env.assignToObject(objRefLhs, valueRhs);
			}
			else
			{
				env.assignToObject(objRefLhs, rhs);
			}*/
		}
		else
		{
			CGL_Error("不正な式です");
		}

		return lhs;
	}
#endif
}
