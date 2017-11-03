#pragma once
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <map>

#include <boost/variant/variant.hpp>
#include <boost/mpl/vector/vector30.hpp>
#include <boost/optional.hpp>

template<class T1, class T2>
inline bool SameType(const T1& t1, const T2& t2)
{
	return std::string(t1.name()) == std::string(t2.name());
}

template<class T1, class T2>
inline bool IsType(const T2& t2)
{
	return std::string(typeid(T1).name()) == std::string(t2.name());
}

template<class T1, class T2>
inline T1& As(T2& t2)
{
	return boost::get<T1>(t2);
}

struct And;
struct Or;
struct Not;

struct Equal;
struct NotEqual;
struct LessThan;
struct LessEqual;
struct GreaterThan;
struct GreaterEqual;

struct Add;
struct Sub;
struct Mul;
struct Div;

struct Pow;
struct Assign;

struct DefFunc;

struct Identifier
{
	std::string name;

	Identifier() = default;

	Identifier(const std::string& name_) :
		name(name_)
	{}

	Identifier(char name_) :
		name({ name_ })
	{}
};

struct FuncVal;

struct CallFunc;

template <class Op>
struct UnaryExpr;

template <class Op>
struct BinaryExpr;

struct Statement;
struct Lines;

struct If;
struct Return;

/*
using Expr = boost::variant<
int,
double,
Identifier,
boost::recursive_wrapper<Statement>,
boost::recursive_wrapper<Lines>,
boost::recursive_wrapper<DefFunc>,
boost::recursive_wrapper<CallFunc>,

boost::recursive_wrapper<UnaryExpr<Not>>,
boost::recursive_wrapper<BinaryExpr<And>>,
boost::recursive_wrapper<BinaryExpr<Or>>,

boost::recursive_wrapper<BinaryExpr<LessThan>>,
boost::recursive_wrapper<BinaryExpr<LessEqual>>,
boost::recursive_wrapper<BinaryExpr<GreaterThan>>,
boost::recursive_wrapper<BinaryExpr<GreaterEqual>>,

boost::recursive_wrapper<UnaryExpr<Add>>,
boost::recursive_wrapper<UnaryExpr<Sub>>,

boost::recursive_wrapper<BinaryExpr<Add>>,
boost::recursive_wrapper<BinaryExpr<Sub>>,
boost::recursive_wrapper<BinaryExpr<Mul>>,
boost::recursive_wrapper<BinaryExpr<Div>>,

boost::recursive_wrapper<BinaryExpr<Pow>>,
boost::recursive_wrapper<BinaryExpr<Assign>>,

boost::recursive_wrapper<If>,
boost::recursive_wrapper<Return>
>;
*/

using types =
boost::mpl::vector26<
	int,
	double,
	Identifier,
	boost::recursive_wrapper<Statement>,
	boost::recursive_wrapper<Lines>,
	boost::recursive_wrapper<DefFunc>,
	boost::recursive_wrapper<CallFunc>,

	boost::recursive_wrapper<UnaryExpr<Not>>,
	boost::recursive_wrapper<BinaryExpr<And>>,
	boost::recursive_wrapper<BinaryExpr<Or>>,

	boost::recursive_wrapper<BinaryExpr<Equal>>,
	boost::recursive_wrapper<BinaryExpr<NotEqual>>,
	boost::recursive_wrapper<BinaryExpr<LessThan>>,
	boost::recursive_wrapper<BinaryExpr<LessEqual>>,
	boost::recursive_wrapper<BinaryExpr<GreaterThan>>,
	boost::recursive_wrapper<BinaryExpr<GreaterEqual>>,

	boost::recursive_wrapper<UnaryExpr<Add>>,
	boost::recursive_wrapper<UnaryExpr<Sub>>,

	boost::recursive_wrapper<BinaryExpr<Add>>,
	boost::recursive_wrapper<BinaryExpr<Sub>>,
	boost::recursive_wrapper<BinaryExpr<Mul>>,
	boost::recursive_wrapper<BinaryExpr<Div>>,

	boost::recursive_wrapper<BinaryExpr<Pow>>,
	boost::recursive_wrapper<BinaryExpr<Assign>>,

	boost::recursive_wrapper<If>,
	boost::recursive_wrapper<Return> >;

using Expr = boost::make_variant_over<types>::type;


union ExprHolder
{
	Expr* expr;
};

void printExpr(const Expr& expr);

using Evaluated = boost::variant<
	int,
	double,
	Identifier,
	boost::recursive_wrapper<FuncVal>
>;

extern std::map<std::string, Evaluated> globalVariables;
extern std::map<std::string, Evaluated> localVariables;

inline boost::optional<std::map<std::string, Evaluated>::iterator> findVariable(const std::string& variableName)
{
	std::map<std::string, Evaluated>::iterator itLocal = localVariables.find(variableName);
	if (itLocal != localVariables.end())
	{
		return itLocal;
	}

	std::map<std::string, Evaluated>::iterator itGlobal = globalVariables.find(variableName);
	if (itGlobal != globalVariables.end())
	{
		return itGlobal;
	}

	return boost::none;
}

template <class Op>
struct UnaryExpr
{
	Expr lhs;

	UnaryExpr(const Expr& lhs_) :
		lhs(lhs_)
	{}
};

template <class Op>
struct BinaryExpr
{
	Expr lhs;
	Expr rhs;

	BinaryExpr(const Expr& lhs_, const Expr& rhs_) :
		lhs(lhs_), rhs(rhs_)
	{}
};

struct Statement
{
	std::vector<Expr> exprs;

	Statement() = default;

	Statement(const Expr& expr) :
		exprs({ expr })
	{}

	Statement(const std::vector<Expr>& exprs_) :
		exprs(exprs_)
	{}

	void add(const Expr& expr)
	{
		exprs.push_back(expr);
	}
};

struct Lines
{
	std::vector<Expr> exprs;

	Lines() = default;

	Lines(const Expr& expr) :
		exprs({ expr })
	{}

	Lines(const std::vector<Expr>& exprs_) :
		exprs(exprs_)
	{}

	void add(const Expr& expr)
	{
		exprs.push_back(expr);
	}

	void concat(const Lines& lines)
	{
		exprs.insert(exprs.end(), lines.exprs.begin(), lines.exprs.end());
	}

	Lines& operator+=(const Lines& lines)
	{
		exprs.insert(exprs.end(), lines.exprs.begin(), lines.exprs.end());
		return *this;
	}

	size_t size()const
	{
		return exprs.size();
	}

	const Expr& operator[](size_t index)const
	{
		return exprs[index];
	}

	Expr& operator[](size_t index)
	{
		return exprs[index];
	}
};

struct Arguments
{
	std::vector<Identifier> arguments;

	Arguments() = default;

	Arguments(const Identifier& identifier) :
		arguments({ identifier })
	{}

	void concat(const Arguments& other)
	{
		arguments.insert(arguments.end(), other.arguments.begin(), other.arguments.end());
	}

	Arguments& operator+=(const Arguments& other)
	{
		arguments.insert(arguments.end(), other.arguments.begin(), other.arguments.end());
		return *this;
	}
};

struct FuncVal
{
	std::map<std::string, Evaluated> environment;
	std::vector<Identifier> argments;
	Expr expr;

	FuncVal() = default;

	FuncVal(
		const std::map<std::string, Evaluated>& environment_,
		const std::vector<Identifier>& argments_,
		const Expr& expr_) :
		environment(environment_),
		argments(argments_),
		expr(expr_)
	{}
};

struct DefFunc
{
	std::vector<Identifier> arguments;
	Expr expr;

	DefFunc() = default;

	DefFunc(const Expr& expr_) :
		expr(expr_)
	{}

	DefFunc(const Arguments& arguments_) :
		arguments(arguments_.arguments)
	{}

	DefFunc(
		const std::vector<Identifier>& arguments_,
		const Expr& expr_) :
		arguments(arguments_),
		expr(expr_)
	{}

	DefFunc(
		const Arguments& arguments_,
		const Expr& expr_) :
		arguments(arguments_.arguments),
		expr(expr_)
	{}

	DefFunc(
		const Lines& arguments_,
		const Expr& expr_) :
		expr(expr_)
	{
		for (size_t i = 0; i < arguments_.size(); ++i)
		{
			arguments.push_back(boost::get<Identifier>(arguments_[i]));
		}
	}

	static bool IsValidArgs(const Lines& arguments_, const Expr& expr_)
	{
		for (size_t i = 0; i < arguments_.size(); ++i)
		{
			if (!SameType(typeid(Identifier), arguments_[i].type()))
			{
				return false;
			}
		}

		return true;
	}
};

inline FuncVal GetFuncVal(const Identifier& funcName)
{
	const auto funcItOpt = findVariable(funcName.name);

	if (!funcItOpt)
	{
		std::cerr << "Error(" << __LINE__ << "): function \"" << funcName.name << "\" was not found." << "\n";
	}

	const auto funcIt = funcItOpt.get();

	if (!SameType(funcIt->second.type(), typeid(FuncVal)))
	{
		std::cerr << "Error(" << __LINE__ << "): function \"" << funcName.name << "\" is not a function." << "\n";
	}

	return boost::get<FuncVal>(funcIt->second);
}

struct CallFunc
{
	boost::variant<FuncVal, Identifier> funcRef;
	std::vector<Expr> actualArguments;

	CallFunc() = default;

	CallFunc(const Identifier& funcName) :
		funcRef(funcName)
	{}

	CallFunc(
		const FuncVal& funcVal_,
		const std::vector<Expr>& actualArguments_) :
		funcRef(funcVal_),
		actualArguments(actualArguments_)
	{}

	CallFunc(
		const Identifier& funcName,
		const std::vector<Expr>& actualArguments_) :
		funcRef(funcName),
		actualArguments(actualArguments_)
	{}
};

struct If
{
	Expr cond_expr;
	Expr then_expr;
	boost::optional<Expr> else_expr;

	If() = default;

	If(const Expr& cond) :
		cond_expr(cond)
	{}

	static If Make(const Expr& cond)
	{
		return If(cond);
	}

	static void SetThen(If& if_statement, const Expr& then_expr_)
	{
		if_statement.then_expr = then_expr_;
	}

	static void SetElse(If& if_statement, const Expr& else_expr_)
	{
		if_statement.else_expr = else_expr_;
	}
};

struct Return
{
	Expr expr;

	Return() = default;

	Return(const Expr& expr) :
		expr(expr)
	{}

	static Return Make(const Expr& expr_)
	{
		return Return(expr_);
	}
};

struct EvalOpt
{
	int m_0;
	double m_1;

	int m_which;

	static EvalOpt Int(int v)
	{
		return{ v,0,0 };
	}
	static EvalOpt Double(double v)
	{
		return{ 0,v,1 };
	}
};

inline EvalOpt Ref(const Evaluated& lhs)
{
	if (SameType(lhs.type(), typeid(int)))
	{
		return EvalOpt::Int(boost::get<int>(lhs));
	}
	else if (SameType(lhs.type(), typeid(double)))
	{
		return EvalOpt::Double(boost::get<double>(lhs));
	}
	else if (SameType(lhs.type(), typeid(Identifier)))
	{
		const auto name = boost::get<Identifier>(lhs).name;
		const auto itOpt = findVariable(name);
		if (!itOpt)
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return EvalOpt::Double(0);
		}
		return Ref(itOpt.get()->second);
		//return EvalOpt::Double();
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return EvalOpt::Double(0);
}

class Eval : public boost::static_visitor<Evaluated>
{
public:

	Evaluated operator()(int node)const
	{
		std::cout << "Begin-End int expression(" << ")" << std::endl;

		return node;
	}

	Evaluated operator()(double node)const
	{
		std::cout << "Begin-End double expression(" << ")" << std::endl;

		return node;
	}

	Evaluated operator()(const Identifier& node)const
	{
		std::cout << "Begin-End Identifier expression(" << ")" << std::endl;

		return node;
	}

	Evaluated operator()(const UnaryExpr<Not>& node)const
	{
		std::cout << "Begin UnaryExpr<Not> expression(" << ")" << std::endl;

		//const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

		std::cout << "End UnaryExpr<Add> expression(" << ")" << std::endl;

		//return lhs;
		return 0;
	}

	Evaluated operator()(const UnaryExpr<Add>& node)const
	{
		std::cout << "Begin UnaryExpr<Add> expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

		std::cout << "End UnaryExpr<Add> expression(" << ")" << std::endl;

		return lhs;
	}

	Evaluated operator()(const UnaryExpr<Sub>& node)const
	{
		std::cout << "Begin UnaryExpr<Sub> expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

		const auto ref = Ref(lhs);
		if (ref.m_which == 0)
		{
			std::cout << "End UnaryExpr<Sub> expression(" << ")" << std::endl;

			return -ref.m_0;
		}

		std::cout << "End UnaryExpr<Sub> expression(" << ")" << std::endl;

		return -ref.m_1;
	}

	Evaluated operator()(const BinaryExpr<And>& node)const
	{
		std::cout << "Begin BinaryExpr<And> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<Or>& node)const
	{
		std::cout << "Begin BinaryExpr<Or> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<Equal>& node)const
	{
		std::cout << "Begin BinaryExpr<Equal> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<NotEqual>& node)const
	{
		std::cout << "Begin BinaryExpr<NotEqual> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<LessThan>& node)const
	{
		std::cout << "Begin BinaryExpr<LessThan> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<LessEqual>& node)const
	{
		std::cout << "Begin BinaryExpr<LessEqual> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<GreaterThan>& node)const
	{
		std::cout << "Begin BinaryExpr<GreaterThan> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<GreaterEqual>& node)const
	{
		std::cout << "Begin BinaryExpr<GreaterEqual> expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const BinaryExpr<Add>& node)const
	{
		std::cout << "Begin BinaryExpr<Add> expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);
		//return lhs + rhs;

		const auto vl = Ref(lhs);
		const auto vr = Ref(rhs);
		if (vl.m_which == 0 && vr.m_which == 0)
		{
			std::cout << "End BinaryExpr<Add> expression(" << ")" << std::endl;
			return vl.m_0 + vr.m_0;
		}

		const double dl = vl.m_which == 0 ? vl.m_0 : vl.m_1;
		const double dr = vr.m_which == 0 ? vr.m_0 : vr.m_1;

		std::cout << "End BinaryExpr<Add> expression(" << ")" << std::endl;

		return dl + dr;
	}

	Evaluated operator()(const BinaryExpr<Sub>& node)const
	{
		std::cout << "Begin BinaryExpr<Sub> expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

		const auto vl = Ref(lhs);
		const auto vr = Ref(rhs);
		if (vl.m_which == 0 && vr.m_which == 0)
		{
			std::cout << "End BinaryExpr<Sub> expression(" << ")" << std::endl;
			return vl.m_0 - vr.m_0;
		}

		const double dl = vl.m_which == 0 ? vl.m_0 : vl.m_1;
		const double dr = vr.m_which == 0 ? vr.m_0 : vr.m_1;

		std::cout << "End BinaryExpr<Sub> expression(" << ")" << std::endl;

		return dl - dr;
	}

	Evaluated operator()(const BinaryExpr<Mul>& node)const
	{
		std::cout << "Begin BinaryExpr<Mul> expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

		const auto vl = Ref(lhs);
		const auto vr = Ref(rhs);
		if (vl.m_which == 0 && vr.m_which == 0)
		{
			std::cout << "End BinaryExpr<Mul> expression(" << ")" << std::endl;
			return vl.m_0 * vr.m_0;
		}

		const double dl = vl.m_which == 0 ? vl.m_0 : vl.m_1;
		const double dr = vr.m_which == 0 ? vr.m_0 : vr.m_1;

		std::cout << "End BinaryExpr<Mul> expression(" << ")" << std::endl;

		return dl * dr;
	}

	Evaluated operator()(const BinaryExpr<Div>& node)const
	{
		std::cout << "Begin BinaryExpr<Div> expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

		const auto vl = Ref(lhs);
		const auto vr = Ref(rhs);
		if (vl.m_which == 0 && vr.m_which == 0)
		{
			std::cout << "End BinaryExpr<Div> expression(" << ")" << std::endl;
			return vl.m_0 / vr.m_0;
		}

		const double dl = vl.m_which == 0 ? vl.m_0 : vl.m_1;
		const double dr = vr.m_which == 0 ? vr.m_0 : vr.m_1;

		std::cout << "End BinaryExpr<Div> expression(" << ")" << std::endl;

		return dl / dr;
	}

	Evaluated operator()(const BinaryExpr<Pow>& node)const
	{
		std::cout << "Begin BinaryExpr<Pow> expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

		const auto vl = Ref(lhs);
		const auto vr = Ref(rhs);
		if (vl.m_which == 0 && vr.m_which == 0)
		{
			std::cout << "End BinaryExpr<Pow> expression(" << ")" << std::endl;
			return vl.m_0 / vr.m_0;
		}

		const double dl = vl.m_which == 0 ? vl.m_0 : vl.m_1;
		const double dr = vr.m_which == 0 ? vr.m_0 : vr.m_1;

		std::cout << "End BinaryExpr<Pow> expression(" << ")" << std::endl;

		return pow(dl, dr);
	}

	Evaluated operator()(const BinaryExpr<Assign>& node)const
	{
		std::cout << "Begin Assign expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

		//const auto vr = Ref(rhs);
		//const double dr = vr.m_which == 0 ? vr.m_0 : vr.m_1;

		if (!SameType(lhs.type(), typeid(Identifier)))
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			std::cout << "End Assign expression(" << ")" << std::endl;

			return 0.0;
		}

		const auto name = boost::get<Identifier>(lhs).name;
		const auto it = globalVariables.find(name);
		if (it == globalVariables.end())
		{
			std::cout << "New Variable(" << name << ")\n";
		}
		//std::cout << "Variable(" << name << ") -> " << dr << "\n";
		//variables[name] = dr;
		globalVariables[name] = rhs;

		//return dr;

		std::cout << "End Assign expression(" << ")" << std::endl;

		return rhs;
	}

	Evaluated operator()(const DefFunc& defFunc)const
	{
		std::cout << "Begin DefFunc expression(" << ")" << std::endl;

		//auto val = FuncVal(globalVariables, defFunc.argments, defFunc.expr);

		std::cout << "End DefFunc expression(" << ")" << std::endl;

		//return val;
		return 0;
	}

	Evaluated operator()(const CallFunc& callFunc)const
	{
		std::cout << "Begin CallFunc expression(" << ")" << std::endl;

		const auto buckUp = localVariables;

		FuncVal funcVal;

		if (SameType(callFunc.funcRef.type(), typeid(FuncVal)))
		{
			funcVal = boost::get<FuncVal>(callFunc.funcRef);
		}
		else if (auto itOpt = findVariable(boost::get<Identifier>(callFunc.funcRef).name))
		{
			const Evaluated& funcRef = (*itOpt.get()).second;
			if (SameType(funcRef.type(), typeid(FuncVal)))
			{
				funcVal = boost::get<FuncVal>(funcRef);
			}
			else
			{
				std::cerr << "Error(" << __LINE__ << "): variable \"" << (*itOpt.get()).first << "\" is not a function.\n";
				return 0;
			}
		}

		//const auto& funcVal = callFunc.funcVal;
		assert(funcVal.argments.size() == callFunc.actualArguments.size());

		/*
		引数に与えられた式の評価
		この時点ではまだ関数の外なので、ローカル変数は変わらない。
		*/
		std::vector<Evaluated> argmentValues(funcVal.argments.size());

		for (size_t i = 0; i < funcVal.argments.size(); ++i)
		{
			argmentValues[i] = boost::apply_visitor(*this, callFunc.actualArguments[i]);
		}

		/*
		関数の評価
		ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える。
		*/
		localVariables = funcVal.environment;

		for (size_t i = 0; i < funcVal.argments.size(); ++i)
		{
			localVariables[funcVal.argments[i].name] = argmentValues[i];
		}

		Evaluated result = boost::apply_visitor(*this, funcVal.expr);

		/*
		最後にローカル変数の環境を関数の実行前のものに戻す。
		*/
		localVariables = buckUp;

		std::cout << "End CallFunc expression(" << ")" << std::endl;

		return result;
	}

	Evaluated operator()(const Statement& statement)const
	{
		std::cout << "Begin Statement expression(" << ")" << std::endl;

		Evaluated result;
		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			std::cout << "Evaluate expression(" << i << ")" << std::endl;
			result = boost::apply_visitor(*this, expr);
			++i;
		}

		std::cout << "End Statement expression(" << ")" << std::endl;

		return result;
	}

	Evaluated operator()(const Lines& statement)const
	{
		std::cout << "Begin Statement expression(" << ")" << std::endl;

		Evaluated result;
		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			std::cout << "Evaluate expression(" << i << ")" << std::endl;
			result = boost::apply_visitor(*this, expr);
			++i;
		}

		std::cout << "End Statement expression(" << ")" << std::endl;

		return result;
	}

	Evaluated operator()(const If& if_statement)const
	{
		std::cout << "Begin If expression(" << ")" << std::endl;

		return 0;
	}

	Evaluated operator()(const Return& return_statement)const
	{
		std::cout << "Begin Return expression(" << ")" << std::endl;

		return 0;
	}
};

class Printer : public boost::static_visitor<void>
{
public:

	Printer(int indent = 0)
		:m_indent(indent)
	{}

	int m_indent;

	std::string indent()const
	{
		const int tabSize = 4;
		std::string str;
		for (int i = 0; i < m_indent*tabSize; ++i)
		{
			str += ' ';
		}
		return str;
	}

	auto operator()(int node)const -> void
	{
		std::cout << indent() << "Int(" << node << ")" << std::endl;
	}

	auto operator()(double node)const -> void
	{
		std::cout << indent() << "Double(" << node << ")" << std::endl;
	}

	auto operator()(const Identifier& node)const -> void
	{
		std::cout << indent() << "Identifier(" << node.name << ")" << std::endl;
	}

	/*auto operator()(const IntHolder& node)const -> void
	{
	std::cout << "IntHolder(" << node.n << ")";
	}*/

	void operator()(const UnaryExpr<Not>& node)const
	{
		std::cout << indent() << "Not(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const UnaryExpr<Add>& node)const -> void
	{
		std::cout << indent() << "Plus(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const UnaryExpr<Sub>& node)const -> void
	{
		std::cout << indent() << "Minus(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<And>& node)const
	{
		std::cout << indent() << "And(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<Or>& node)const
	{
		std::cout << indent() << "Or(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<Equal>& node)const
	{
		std::cout << indent() << "Equal(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<NotEqual>& node)const
	{
		std::cout << indent() << "NotEqual(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<LessThan>& node)const
	{
		std::cout << indent() << "LessThan(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<LessEqual>& node)const
	{
		std::cout << indent() << "LessEqual(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<GreaterThan>& node)const
	{
		std::cout << indent() << "GreaterThan(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr<GreaterEqual>& node)const
	{
		std::cout << indent() << "GreaterEqual(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const BinaryExpr<Add>& node)const -> void
	{
		std::cout << indent() << "Add(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		//std::cout << indent() << ", " << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.rhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const BinaryExpr<Sub>& node)const -> void
	{
		std::cout << indent() << "Sub(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		//std::cout << indent() << ", " << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.rhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const BinaryExpr<Mul>& node)const -> void
	{
		std::cout << indent() << "Mul(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		//std::cout << indent() << ", " << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.rhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const BinaryExpr<Div>& node)const -> void
	{
		std::cout << indent() << "Div(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		//std::cout << indent() << ", " << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.rhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const BinaryExpr<Pow>& node)const -> void
	{
		std::cout << indent() << "Pow(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		//std::cout << indent() << ", " << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.rhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const BinaryExpr<Assign>& node)const -> void
	{
		std::cout << indent() << "Assign(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		//std::cout << indent() << ", " << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.rhs);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const DefFunc& defFunc)const -> void
	{
		std::cout << indent() << "DefFunc(" << std::endl;

		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "Arguments(" << std::endl;
			{
				const auto grandChild = Printer(child.m_indent + 1);

				const auto& args = defFunc.arguments;
				for (size_t i = 0; i < args.size(); ++i)
				{
					std::cout << grandChild.indent() << args[i].name << (i + 1 == args.size() ? "\n" : ",\n");
				}
			}
			std::cout << child.indent() << ")" << std::endl;
		}

		boost::apply_visitor(Printer(m_indent + 1), defFunc.expr);

		std::cout << indent() << ")" << std::endl;
	}

	auto operator()(const CallFunc& callFunc)const -> void
	{
		std::cout << indent() << "CallFunc(" << std::endl;

		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "FuncRef(" << std::endl;

			if (IsType<Identifier>(callFunc.funcRef.type()))
			{
				const auto& funcName = boost::get<Identifier>(callFunc.funcRef);
				boost::apply_visitor(Printer(m_indent + 2), Expr(funcName));
			}
			else
			{
				std::cout << indent() << "(FuncVal)" << std::endl;
			}

			std::cout << child.indent() << ")" << std::endl;
		}

		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "Arguments(" << std::endl;

			const auto& args = callFunc.actualArguments;
			for (size_t i = 0; i < args.size(); ++i)
			{
				boost::apply_visitor(Printer(m_indent + 2), args[i]);
			}

			//boost::apply_visitor(Printer(m_indent + 2), callFunc.actualArguments);
			std::cout << child.indent() << ")" << std::endl;
		}

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const Statement& statement)const
	{
		std::cout << indent() << "Statement begin" << std::endl;

		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			std::cout << indent() << "(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(m_indent + 1), expr);
			++i;
		}

		std::cout << indent() << "Statement end" << std::endl;
	}

	void operator()(const Lines& statement)const
	{
		std::cout << indent() << "Statement begin" << std::endl;

		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			std::cout << indent() << "(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(m_indent + 1), expr);
			++i;
		}

		std::cout << indent() << "Statement end" << std::endl;
	}

	void operator()(const If& if_statement)const
	{
		std::cout << indent() << "If(" << std::endl;

		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "Cond(" << std::endl;

			boost::apply_visitor(Printer(m_indent + 2), if_statement.cond_expr);

			std::cout << child.indent() << ")" << std::endl;
		}

		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "Then(" << std::endl;

			boost::apply_visitor(Printer(m_indent + 2), if_statement.then_expr);

			std::cout << child.indent() << ")" << std::endl;
		}

		if (if_statement.else_expr)
		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "Else(" << std::endl;

			boost::apply_visitor(Printer(m_indent + 2), if_statement.else_expr.value());

			std::cout << child.indent() << ")" << std::endl;
		}

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const Return& return_statement)const
	{
		std::cout << indent() << "Return(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), return_statement.expr);

		std::cout << indent() << ")" << std::endl;
	}
};

inline void printStatement(const Statement& statement)
{
	std::cout << "Statement begin" << std::endl;

	int i = 0;
	for (const auto& expr : statement.exprs)
	{
		std::cout << "Expr(" << i << "): " << std::endl;
		boost::apply_visitor(Printer(), expr);
		++i;
	}

	std::cout << "Statement end" << std::endl;
}

inline void printLines(const Lines& statement)
{
	std::cout << "Lines begin" << std::endl;

	int i = 0;
	for (const auto& expr : statement.exprs)
	{
		std::cout << "Expr(" << i << "): " << std::endl;
		boost::apply_visitor(Printer(), expr);
		++i;
	}

	std::cout << "Lines end" << std::endl;
}

inline void printExpr(const Expr& expr)
{
	std::cout << "PrintExpr(\n";
	boost::apply_visitor(Printer(), expr);
	std::cout << ") " << std::endl;
}

inline void Concat(Expr& lines1, const Expr& lines2)
{
	if (!IsType<Lines>(lines1.type()) || !IsType<Lines>(lines2.type()))
	{
		std::cerr << "Error";
		return;
	}

	As<Lines>(lines1).concat(As<const Lines>(lines2));
}

inline void Append(Expr& lines, const Expr& expr)
{
	if (!IsType<Lines>(lines.type()))
	{
		std::cerr << "Error";
		return;
	}

	As<Lines>(lines).add(expr);
}

inline DefFunc MakeDefFunc(Expr& lines, const Expr& expr)
{
	if (!IsType<Lines>(lines.type()))
	{
		std::cerr << "Error";
		return DefFunc();
	}

	auto& ls = As<Lines>(lines);
	if (!DefFunc::IsValidArgs(ls, expr))
	{
		std::cerr << "Error";
		return DefFunc();
	}

	return DefFunc(ls, expr);
}

inline DefFunc* NewDefFunc(Expr& lines, const Expr& expr)
{
	if (!IsType<Lines>(lines.type()))
	{
		std::cerr << "Error";
		return new DefFunc();
	}

	auto& ls = As<Lines>(lines);
	if (!DefFunc::IsValidArgs(ls, expr))
	{
		std::cerr << "Error";
		return new DefFunc();
	}

	return new DefFunc(ls, expr);
}
