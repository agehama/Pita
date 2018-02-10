#pragma once
#include <cmath>
#include "Node.hpp"

namespace cgl
{
	Record MargeRecord(const Record& rec1, const Record& rec2);

	void MargeRecordInplace(Record& result, const Record& rec2);

	Evaluated Not(const Evaluated& lhs, Context& env);

	Evaluated Plus(const Evaluated& lhs, Context& env);

	Evaluated Minus(const Evaluated& lhs, Context& env);

	Evaluated And(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Or(const Evaluated& lhs, const Evaluated& rhs, Context& env);;

	bool Equal(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	bool NotEqual(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	bool LessThan(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	bool LessEqual(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	bool GreaterThan(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	bool GreaterEqual(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Max(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Min(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Abs(const Evaluated& lhs, Context& env);

	Evaluated Sin(const Evaluated& lhs);

	Evaluated Cos(const Evaluated& lhs);

	Evaluated Add(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Sub(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Mul(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Div(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Pow(const Evaluated& lhs, const Evaluated& rhs, Context& env);

	Evaluated Concat(const Evaluated& lhs, const Evaluated& rhs, Context& env);


#ifdef commentout
	Evaluated Assign(const Evaluated& lhs, const Evaluated& rhs, Context& env)
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
