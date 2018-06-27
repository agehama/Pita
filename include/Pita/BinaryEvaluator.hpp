#pragma once
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	Val NotFunc(const Val& lhs, Context& env);

	Val PlusFunc(const Val& lhs, Context& env);

	Val MinusFunc(const Val& lhs, Context& env);

	Val AndFunc(const Val& lhs, const Val& rhs, Context& env);

	Val OrFunc(const Val& lhs, const Val& rhs, Context& env);;

	bool EqualFunc(const Val& lhs, const Val& rhs, Context& env);

	bool NotEqualFunc(const Val& lhs, const Val& rhs, Context& env);

	bool LessThanFunc(const Val& lhs, const Val& rhs, Context& env);

	bool LessEqualFunc(const Val& lhs, const Val& rhs, Context& env);

	bool GreaterThanFunc(const Val& lhs, const Val& rhs, Context& env);

	bool GreaterEqualFunc(const Val& lhs, const Val& rhs, Context& env);

	Val MaxFunc(const Val& lhs, const Val& rhs, Context& env);

	Val MinFunc(const Val& lhs, const Val& rhs, Context& env);

	Val AbsFunc(const Val& lhs, Context& env);

	Val SinFunc(const Val& lhs);

	Val CosFunc(const Val& lhs);

	Val AddFunc(const Val& lhs, const Val& rhs, Context& env);

	Val SubFunc(const Val& lhs, const Val& rhs, Context& env);

	Val MulFunc(const Val& lhs, const Val& rhs, Context& env);

	Val DivFunc(const Val& lhs, const Val& rhs, Context& env);

	Val PowFunc(const Val& lhs, const Val& rhs, Context& env);

	Val ConcatFunc(const Val& lhs, const Val& rhs, Context& env);

	Val SetDiffFunc(const Val& lhs, const Val& rhs, Context& env);
}
