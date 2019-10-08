#pragma once
#include "Node.hpp"

namespace cgl
{
	class Context;

	//関数式を構成する識別子が関数内部で閉じているものか、外側のスコープに依存しているものかを調べ
	//外側のスコープを参照する識別子をアドレスに置き換えた式を返す
	Expr AsClosure(Context& context, const Expr& expr, const std::set<std::string>& functionArguments = {});
}
