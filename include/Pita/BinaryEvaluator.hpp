#pragma once
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	Record MargeRecord(const Record& rec1, const Record& rec2);

	void MargeRecordInplace(Record& result, const Record& rec2);

	Val Not(const Val& lhs, Context& env);

	Val Plus(const Val& lhs, Context& env);

	Val Minus(const Val& lhs, Context& env);

	Val And(const Val& lhs, const Val& rhs, Context& env);

	Val Or(const Val& lhs, const Val& rhs, Context& env);;

	bool Equal(const Val& lhs, const Val& rhs, Context& env);

	bool NotEqual(const Val& lhs, const Val& rhs, Context& env);

	bool LessThan(const Val& lhs, const Val& rhs, Context& env);

	bool LessEqual(const Val& lhs, const Val& rhs, Context& env);

	bool GreaterThan(const Val& lhs, const Val& rhs, Context& env);

	bool GreaterEqual(const Val& lhs, const Val& rhs, Context& env);

	Val Max(const Val& lhs, const Val& rhs, Context& env);

	Val Min(const Val& lhs, const Val& rhs, Context& env);

	Val Abs(const Val& lhs, Context& env);

	Val Sin(const Val& lhs);

	Val Cos(const Val& lhs);

	Val Add(const Val& lhs, const Val& rhs, Context& env);

	Val Sub(const Val& lhs, const Val& rhs, Context& env);

	Val Mul(const Val& lhs, const Val& rhs, Context& env);

	Val Div(const Val& lhs, const Val& rhs, Context& env);

	Val Pow(const Val& lhs, const Val& rhs, Context& env);

	Val Concat(const Val& lhs, const Val& rhs, Context& env);


#ifdef commentout
	Val Assign(const Val& lhs, const Val& rhs, Context& env)
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
			const auto& objRefLhs = objLhsOpt.get();

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
			//const auto& objRefLhs = objLhsOpt.get();
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
