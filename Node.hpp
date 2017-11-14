#pragma once
#include <cmath>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>

#include <boost/fusion/include/vector.hpp>

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
	return std::string(typeid(T1).name()) == std::string(t2.type().name());
}

template<class T1, class T2>
inline boost::optional<T1&> AsOpt(T2& t2)
{
	//return IsType<T1>(t2) ? boost::get<T1>(t2) : boost::none;
	if (IsType<T1>(t2))
	{
		return boost::get<T1>(t2);
	}
	return boost::none;
	//return boost::get<T1>(t2);
}

template<class T1, class T2>
inline boost::optional<const T1&> AsOpt(const T2& t2)
{
	//return IsType<T1>(t2) ? boost::get<T1>(t2) : boost::none;
	if (IsType<T1>(t2))
	{
		return boost::get<T1>(t2);
	}
	return boost::none;
	//return boost::get<T1>(t2);
}

template<class T1, class T2>
inline T1& As(T2& t2)
{
	return boost::get<T1>(t2);
}

template<class T1, class T2>
inline const T1& As(const T2& t2)
{
	return boost::get<T1>(t2);
}

//struct And;
//struct Or;
//struct Not;
//
//struct Equal;
//struct NotEqual;
//struct LessThan;
//struct LessEqual;
//struct GreaterThan;
//struct GreaterEqual;
//
//struct Add;
//struct Sub;
//struct Mul;
//struct Div;
//
//struct Pow;
//struct Assign;

enum class UnaryOp
{
	Not,

	Plus,
	Minus
};

enum class BinaryOp
{
	And,
	Or,

	Equal,
	NotEqual,
	LessThan,
	LessEqual,
	GreaterThan,
	GreaterEqual,

	Add,
	Sub,
	Mul,
	Div,
	
	Pow,
	Assign
};

/*
inline std::string BinaryOpToStr(BinaryOp op)
{
	switch (op)
	{
	case BinaryOp::And: return "BinaryOp::And";
	case BinaryOp::Or:  return "BinaryOp::Or";

	case BinaryOp::Equal:        return "BinaryOp::Equal";
	case BinaryOp::NotEqual:     return "BinaryOp::NotEqual";
	case BinaryOp::LessThan:     return "BinaryOp::LessThan";
	case BinaryOp::LessEqual:    return "BinaryOp::LessEqual";
	case BinaryOp::GreaterThan:  return "BinaryOp::GreaterThan";
	case BinaryOp::GreaterEqual: return "BinaryOp::GreaterEqual";

	case BinaryOp::Add: return "BinaryOp::Add";
	case BinaryOp::Sub: return "BinaryOp::Sub";
	case BinaryOp::Mul: return "BinaryOp::Mul";
	case BinaryOp::Div: return "BinaryOp::Div";

	case BinaryOp::Pow:    return "BinaryOp::Pow";
	case BinaryOp::Assign: return "BinaryOp::Assign";
	}
}
*/

struct DefFunc;

struct Identifier
{
	std::string name;

	Identifier() = default;

	Identifier(const std::string& name_) :
		name(name_)
	{}

	Identifier(const boost::fusion::vector2<char, std::vector<char, std::allocator<char>>>& name_)
	{
	}
	//boost::fusion::vector2<char,std::vector<char,std::allocator<char>>>
	Identifier(char name_) :
		name({ name_ })
	{}

	bool operator==(const Identifier& other)const
	{
		return name == other.name;
	}

	bool operator!=(const Identifier& other)const
	{
		return !(*this == other);
	}
};

struct FuncVal;

struct CallFunc;

//template <class Op>
//struct UnaryExpr;
//
//template <class Op>
//struct BinaryExpr;

struct UnaryExpr;
struct BinaryExpr;

struct Lines;

struct If;
struct For;

struct Return;

struct Range;

struct ListConstractor;
struct List;

struct ListAccess;

struct KeyExpr;
struct RecordConstractor;

struct KeyValue;
struct Record;

struct RecordAccess;
struct FunctionAccess;

struct Accessor;

struct FunctionCaller;

struct DeclSat;
struct DeclFree;

using types =
boost::mpl::vector20<
	bool,
	int,
	double,
	Identifier,

	boost::recursive_wrapper<Range>,

	boost::recursive_wrapper<Lines>,
	boost::recursive_wrapper<DefFunc>,
	boost::recursive_wrapper<CallFunc>,

	boost::recursive_wrapper<UnaryExpr>,
	boost::recursive_wrapper<BinaryExpr>,

	boost::recursive_wrapper<If>,
	boost::recursive_wrapper<For>,
	boost::recursive_wrapper<Return>,

	boost::recursive_wrapper<ListConstractor>,

	boost::recursive_wrapper<KeyExpr>,
	boost::recursive_wrapper<RecordConstractor>,
	
	boost::recursive_wrapper<Accessor>,
	boost::recursive_wrapper<FunctionCaller>,

	boost::recursive_wrapper<DeclSat>,
	boost::recursive_wrapper<DeclFree>
	>;

using Expr = boost::make_variant_over<types>::type;

void printExpr(const Expr& expr);

struct Jump;

struct ObjectReference;

using Evaluated = boost::variant<
	bool,
	int,
	double,
	Identifier,
	boost::recursive_wrapper<ObjectReference>,
	boost::recursive_wrapper<List>,
	boost::recursive_wrapper<KeyValue>,
	boost::recursive_wrapper<Record>,
	boost::recursive_wrapper<FuncVal>,
	boost::recursive_wrapper<Jump>,
	boost::recursive_wrapper<DeclSat>,
	boost::recursive_wrapper<DeclFree>
>;

bool IsEqual(const Evaluated& value1, const Evaluated& value2);

using SatExpr = boost::variant<
	bool,
	int,
	double,
	Identifier,
	boost::recursive_wrapper<UnaryExpr>,
	boost::recursive_wrapper<BinaryExpr>
>;

bool IsLValue(const Evaluated& value)
{
	return IsType<Identifier>(value) || IsType<ObjectReference>(value);
}

bool IsRValue(const Evaluated& value)
{
	return !IsLValue(value);
}

class Environment;

struct UnaryExpr
{
	Expr lhs;
	UnaryOp op;

	UnaryExpr(const Expr& lhs, UnaryOp op) :
		lhs(lhs), op(op)
	{}
};

struct BinaryExpr
{
	Expr lhs;
	Expr rhs;
	BinaryOp op;

	BinaryExpr(const Expr& lhs, const Expr& rhs, BinaryOp op) :
		lhs(lhs), rhs(rhs), op(op)
	{}
};

struct Range
{
	Expr lhs;
	Expr rhs;

	Range() = default;

	Range(const Expr& lhs) :
		lhs(lhs)
	{}

	Range(const Expr& lhs, const Expr& rhs) :
		lhs(lhs), rhs(rhs)
	{}

	static Range Make(const Expr& lhs)
	{
		return Range(lhs);
	}

	static void SetRhs(Range& range, const Expr& rhs)
	{
		range.rhs = rhs;
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

	static Lines Make(const Expr& expr)
	{
		return Lines(expr);
	}

	static void Append(Lines& lines, const Expr& expr)
	{
		lines.add(expr);
	}

	static void Concat(Lines& lines1, const Lines& lines2)
	{
		lines1.concat(lines2);
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
	std::shared_ptr<Environment> environment;
	std::vector<Identifier> arguments;
	Expr expr;

	FuncVal() = default;

	FuncVal(
		std::shared_ptr<Environment> environment,
		const std::vector<Identifier>& arguments,
		const Expr& expr) :
		environment(environment),
		arguments(arguments),
		expr(expr)
	{}

	bool operator==(const FuncVal& other)const
	{
		if (arguments.size() != other.arguments.size())
		{
			return false;
		}

		for (size_t i = 0; i < arguments.size(); ++i)
		{
			if (arguments[i] != other.arguments[i])
			{
				return false;
			}
		}

		//environment;
		//expr;
		std::cerr << "Warning: IsEqual<FuncVal>() don't care about environment and expr" << std::endl;

		return true;
	}
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

struct FunctionCaller
{
	boost::variant<FuncVal, Identifier> funcRef;
	std::vector<Evaluated> actualArguments;

	FunctionCaller() = default;

	FunctionCaller(const Identifier& funcName) :
		funcRef(funcName)
	{}

	FunctionCaller(
		const FuncVal& funcVal_,
		const std::vector<Evaluated>& actualArguments_) :
		funcRef(funcVal_),
		actualArguments(actualArguments_)
	{}

	FunctionCaller(
		const Identifier& funcName,
		const std::vector<Evaluated>& actualArguments_) :
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

//for i in a..b do statement
struct For
{
	Identifier loopCounter;
	Expr rangeStart;
	Expr rangeEnd;
	Expr doExpr;

	For() = default;

	For(const Identifier& loopCounter, const Expr& rangeStart, const Expr& rangeEnd, const Expr& doExpr) :
		loopCounter(loopCounter),
		rangeStart(rangeStart),
		rangeEnd(rangeEnd),
		doExpr(doExpr)
	{}

	static For Make(const Identifier& loopCounter)
	{
		For obj;
		obj.loopCounter = loopCounter;
		return obj;
	}

	static void SetRangeStart(For& forExpression, const Expr& rangeStart)
	{
		forExpression.rangeStart = rangeStart;
	}

	static void SetRangeEnd(For& forExpression, const Expr& rangeEnd)
	{
		forExpression.rangeEnd = rangeEnd;
	}

	static void SetDo(For& forExpression, const Expr& doExpr)
	{
		forExpression.doExpr = doExpr;
	}
};

struct Return
{
	Expr expr;

	Return() = default;

	Return(const Expr& expr) :
		expr(expr)
	{}

	static Return Make(const Expr& expr)
	{
		return Return(expr);
	}
};

struct ListConstractor
{
	std::vector<Expr> data;

	ListConstractor() = default;

	ListConstractor(const Expr& expr) :
		data({ expr })
	{}

	static ListConstractor Make(const Expr& expr)
	{
		return ListConstractor(expr);
	}

	static void Append(ListConstractor& list, const Expr& expr)
	{
		list.data.push_back(expr);
	}
};

struct List
{
	std::vector<Evaluated> data;

	List() = default;
	
	void append(const Evaluated& value)
	{
		data.push_back(value);
	}

	std::vector<Evaluated>::iterator get(int index)
	{
		return data.begin() + index;
	}

	bool operator==(const List& other)const
	{
		if (data.size() != other.data.size())
		{
			return false;
		}

		for (size_t i = 0; i < data.size(); ++i)
		{
			if (!IsEqual(data[i], other.data[i]))
			{
				return false;
			}
		}

		return true;
	}
};

//struct ListAccess
//{
//	//一番上の親はlistRefを必ず持つ
//	boost::optional<boost::variant<ListConstractor, Identifier>> listRef;
//
//	Expr index;
//
//	//一番下の子供はchildを持たない
//	//boost::optional<ListAccess> child;
//	std::vector<ListAccess> child;
//
//	ListAccess() = default;
//
//	ListAccess(const Identifier& identifier):
//		listRef(identifier)
//	{}
//
//	ListReference fold(std::shared_ptr<Environment> pEnv)const;
//	void foldImpl(ListReference& listReference, std::shared_ptr<Environment> pEnv)const;
//
//	static ListAccess Make(const Identifier& identifier)
//	{
//		return ListAccess(identifier);
//	}
//
//	static void SetIndex(ListAccess& listAccess, const Expr& index)
//	{
//		listAccess.index = index;
//	}
//
//	static void SetChild(ListAccess& listAccess, const ListAccess& listAccessChild)
//	{
//		listAccess.child.clear();
//		listAccess.child.push_back(listAccessChild);
//	}
//};

struct KeyExpr
{
	Identifier name;
	Expr expr;

	KeyExpr() = default;

	KeyExpr(const Identifier& name) :
		name(name)
	{}

	static KeyExpr Make(const Identifier& name)
	{
		return KeyExpr(name);
	}

	static void SetExpr(KeyExpr& keyval, const Expr& expr)
	{
		keyval.expr = expr;
	}
};

struct RecordConstractor
{
	std::vector<Expr> exprs;

	RecordConstractor() = default;

	static void AppendKeyExpr(RecordConstractor& rec, const KeyExpr& KeyExpr)
	{
		rec.exprs.push_back(KeyExpr);
	}

	static void AppendExpr(RecordConstractor& rec, const Expr& expr)
	{
		rec.exprs.push_back(expr);
	}
};

struct KeyValue
{
	Identifier name;
	Evaluated value;

	KeyValue() = default;

	KeyValue(const Identifier& name, const Evaluated& value):
		name(name),
		value(value)
	{}

	bool operator==(const KeyValue& other)const
	{
		if (name != other.name)
		{
			return false;
		}

		return IsEqual(value, other.value);
	}
};

struct DeclSat
{
	Expr expr;

	DeclSat() = default;

	DeclSat(const Expr& expr) :
		expr(expr)
	{}

	static DeclSat Make(const Expr& expr)
	{
		return DeclSat(expr);
	}

	bool operator==(const DeclSat& other)const
	{
		std::cerr << "Warning: IsEqual<DeclSat>() don't care about expr" << std::endl;
		return true;
	}
};

struct ObjectReference
{
	struct ListRef
	{
		int index;

		ListRef() = default;
		ListRef(int index) :index(index) {}

		bool operator==(const ListRef& other)const
		{
			return index == other.index;
		}
	};

	struct RecordRef
	{
		std::string key;

		RecordRef() = default;
		RecordRef(const std::string& key) :key(key) {}

		bool operator==(const RecordRef& other)const
		{
			return key == other.key;
		}
	};

	struct FunctionRef
	{
		std::vector<Evaluated> args;

		FunctionRef() = default;
		FunctionRef(const std::vector<Evaluated>& args) :args(args) {}

		bool operator==(const FunctionRef& other)const
		{
			if (args.size() != other.args.size())
			{
				return false;
			}

			for (size_t i = 0; i < args.size(); ++i)
			{
				if (!IsEqual(args[i], other.args[i]))
				{
					return false;
				}
			}

			return true;
		}
	};

	using Ref = boost::variant<ListRef, RecordRef, FunctionRef>;

	std::string name;

	std::vector<Ref> references;

	void appendListRef(int index)
	{
		references.push_back(ListRef(index));
	}

	void appendRecordRef(const std::string& key)
	{
		references.push_back(RecordRef(key));
	}

	void appendFunctionRef(const std::vector<Evaluated>& args)
	{
		references.push_back(FunctionRef(args));
	}

	bool operator==(const ObjectReference& other)const
	{
		if (name != other.name)
		{
			return false;
		}

		if (references.size() != other.references.size())
		{
			return false;
		}

		for (size_t i=0;i<references.size();++i)
		{
			if (!(references[i] == other.references[i]))
			{
				return false;
			}
		}

		return true;
	}
};

struct DeclFree
{
	using Ref = boost::variant<Identifier, boost::recursive_wrapper<ObjectReference>>;

	std::vector<Ref> refs;

	DeclFree() = default;

	static void AddIdentifier(DeclFree& decl, const Identifier& ref)
	{
		decl.refs.push_back(ref);
	}

	/*static void AddReference(DeclFree& decl, const ObjectReference& ref)
	{
		decl.refs.push_back(ref);
	}*/

	bool operator==(const DeclFree& other)const
	{
		if (refs.size() != other.refs.size())
		{
			return false;
		}

		for (size_t i = 0; i < refs.size(); ++i)
		{
			if (!SameType(refs[i].type(), other.refs[i].type()))
			{
				return false;
			}

			if (IsType<Identifier>(refs[i]))
			{
				if (As<Identifier>(refs[i]) != As<Identifier>(other.refs[i]))
				{
					return false;
				}
			}
			else if (IsType<ObjectReference>(refs[i]))
			{
				if (!(As<ObjectReference>(refs[i]) == As<ObjectReference>(other.refs[i])))
				{
					return false;
				}
			}
		}

		return true;
	}
};

struct Record
{
	std::unordered_map<std::string, Evaluated> values;
	boost::optional<Expr> constraint;
	std::vector<DeclFree::Ref> freeVariables;

	Record() = default;

	Record(const std::string& name, const Evaluated& value)
	{
		append(name, value);
	}

	Record& append(const std::string& name, const Evaluated& value)
	{
		values[name] = value;
		return *this;
	}

	bool operator==(const Record& other)const
	{
		if (values.size() != other.values.size())
		{
			return false;
		}

		const auto& vs = other.values;

		for (const auto& keyval : values)
		{
			const auto otherIt = vs.find(keyval.first);
			if (otherIt == vs.end())
			{
				return false;
			}

			if (!IsEqual(keyval.second, otherIt->second))
			{
				return false;
			}
		}

		std::cerr << "Warning: IsEqual<Record>() don't care about constraint" << std::endl;
		//constraint;
		//freeVariables;
		return true;
	}
};

struct ListAccess
{
	Expr index;

	ListAccess() = default;

	static void SetIndex(ListAccess& listAccess, const Expr& index)
	{
		listAccess.index = index;
	}
};

struct RecordAccess
{
	Identifier name;

	RecordAccess() = default;
	
	RecordAccess(const Identifier& name):
		name(name)
	{}

	static RecordAccess Make(const Identifier& name)
	{
		return RecordAccess(name);
	}
};

struct FunctionAccess
{
	std::vector<Expr> actualArguments;

	FunctionAccess() = default;

	void append(const Expr& argument)
	{
		actualArguments.push_back(argument);
	}

	static void Append(FunctionAccess& obj, const Expr& argument)
	{
		obj.append(argument);
	}
};

struct Accessor
{
	using Access = boost::variant<ListAccess, RecordAccess, FunctionAccess>;
	
	//先頭のオブジェクト(識別子かもしくは関数・リスト・レコードのリテラル)
	Expr head;

	std::vector<Access> accesses;

	Accessor() = default;

	Accessor(const Expr& head) :
		head(head)
	{}

	static Accessor Make(const Expr& head)
	{
		return Accessor(head);
	}

	static void AppendList(Accessor& obj, const ListAccess& access)
	{
		obj.accesses.push_back(access);
	}

	static void AppendRecord(Accessor& obj, const RecordAccess& access)
	{
		obj.accesses.push_back(access);
	}

	static void AppendFunction(Accessor& obj, const FunctionAccess& access)
	{
		obj.accesses.push_back(access);
	}
};

struct Jump
{
	enum Op { Return, Break, Continue };

	boost::optional<Evaluated> lhs;
	Op op;

	Jump() = default;

	Jump(Op op) :op(op) {}

	Jump(Op op, const Evaluated& lhs) :
		op(op), lhs(lhs)
	{}

	static Jump MakeReturn(const Evaluated& value)
	{
		return Jump(Op::Return, value);
	}

	static Jump MakeBreak()
	{
		return Jump(Op::Break);
	}

	static Jump MakeContinue()
	{
		return Jump(Op::Continue);
	}

	bool isReturn()const
	{
		return op == Op::Return;
	}

	bool isBreak()const
	{
		return op == Op::Break;
	}

	bool isContinue()const
	{
		return op == Op::Continue;
	}

	bool operator==(const Jump& other)const
	{
		if (op != other.op)
		{
			return false;
		}

		if (lhs)
		{
			if (!other.lhs)
			{
				return false;
			}

			return IsEqual(lhs.value(), other.lhs.value());
		}
		else
		{
			return !other.lhs;
		}
	}
};

class ValuePrinter : public boost::static_visitor<void>
{
public:

	ValuePrinter(int indent = 0, const std::string& header = "") :
		m_indent(indent),
		m_header(header)
	{}

	int m_indent;
	mutable std::string m_header;

	std::string indent()const
	{
		const int tabSize = 4;
		std::string str;
		for (int i = 0; i < m_indent*tabSize; ++i)
		{
			str += ' ';
		}
		str += m_header;
		m_header.clear();
		return str;
	}

	void operator()(bool node)const
	{
		std::cout << indent() << "Bool(" << (node ? "true" : "false") << ")" << std::endl;
	}

	void operator()(int node)const
	{
		std::cout << indent() << "Int(" << node << ")" << std::endl;
	}

	void operator()(double node)const
	{
		std::cout << indent() << "Double(" << node << ")" << std::endl;
	}

	void operator()(const Identifier& node)const
	{
		std::cout << indent() << "Identifier(" << node.name << ")" << std::endl;
	}
		
	void operator()(const ObjectReference& node)const
	{
		std::cout << indent() << "ObjectReference(" << node.name << ")" << std::endl;
	}

	void operator()(const List& node)const
	{
		std::cout << indent() << "[" << std::endl;
		const auto& data = node.data;

		for (size_t i = 0; i < data.size(); ++i)
		{
			const std::string header = std::string("(") + std::to_string(i) + "): ";
			const auto child = ValuePrinter(m_indent + 1, header);
			boost::apply_visitor(child, data[i]);
		}

		std::cout << indent() << "]" << std::endl;
	}

	void operator()(const KeyValue& node)const
	{
		//std::cout << indent() << "KeyVal(" << std::endl;
		
		/*std::cout << indent() << node.name.name << ":" << std::endl;
		{
			const auto child = ValuePrinter(m_indent + 1);
			boost::apply_visitor(child, node.value);
		}*/

		const std::string header = node.name.name + ": ";
		{
			const auto child = ValuePrinter(m_indent, header);
			boost::apply_visitor(child, node.value);
		}

		//std::cout << indent() << ")" << std::endl;
	}

	void operator()(const Record& node)const
	{
		std::cout << indent() << "{" << std::endl;

		for (const auto& value : node.values)
		{
			const std::string header = value.first + ": ";

			const auto child = ValuePrinter(m_indent + 1, header);
			boost::apply_visitor(child, value.second);
		}
		
		std::cout << indent() << "}" << std::endl;
	}

	void operator()(const FuncVal& node)const;

	void operator()(const Jump& node)const
	{
		std::cout << indent() << "Jump(" << node.op << ")" << std::endl;
	}

	void operator()(const DeclSat& node)const
	{
		std::cout << indent() << "DeclSat(" << ")" << std::endl;
	}

	void operator()(const DeclFree& node)const
	{
		std::cout << indent() << "DeclFree(" << ")" << std::endl;
	}
};

inline void printEvaluated(const Evaluated& evaluated)
{
	ValuePrinter printer;
	boost::apply_visitor(printer, evaluated);
}

class Values
{
public:

	using ValueList = std::unordered_map<unsigned, Evaluated>;
	
	unsigned add(const Evaluated& value)
	{
		m_values.insert({ nextID(), value });

		return m_ID;
	}

	size_t size()const
	{
		return m_values.size();
	}

	Evaluated& operator[](unsigned key)
	{
		auto it = m_values.find(key);
		if (it == m_values.end())
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
		}
		return it->second;
	}

	const Evaluated& operator[](unsigned key)const
	{
		auto it = m_values.find(key);
		if (it == m_values.end())
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
		}
		return it->second;
	}

	ValueList::const_iterator begin()const
	{
		return m_values.cbegin();
	}

	ValueList::const_iterator end()const
	{
		return m_values.cend();
	}

private:

	unsigned nextID()
	{
		return ++m_ID;
	}
	
	ValueList m_values;

	unsigned m_ID = 0;

};

class LocalEnvironment
{
public:

	enum Type { NormalScope, RecordScope };

	LocalEnvironment(Type scopeType):
		type(scopeType)
	{}

	using LocalVariables = std::unordered_map<std::string, unsigned>;

	void bind(const std::string& name, unsigned ID)
	{
		localVariables[name] = ID;
	}

	boost::optional<unsigned> find(const std::string& name)const
	{
		const auto it = localVariables.find(name);
		if (it == localVariables.end())
		{
			return boost::none;
		}

		return it->second;
	}

	LocalVariables::const_iterator begin()const
	{
		return localVariables.cbegin();
	}

	LocalVariables::const_iterator end()const
	{
		return localVariables.cend();
	}

private:

	LocalVariables localVariables;

	Type type;

	//isInnerScope = false の時GCの対象となる
	bool isInnerScope = true;
};

class Environment
{
public:

	boost::optional<const Evaluated&> find(const std::string& name)const
	{
		const auto valueIDOpt = findValueID(name);
		if (!valueIDOpt)
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return boost::none;
		}

		return m_values[valueIDOpt.value()];
	}

	const Evaluated& dereference(const Evaluated& reference);

	//{a=1,b=[2,3]}, [a, b] => [1, [2, 3]]
	Evaluated expandList(const Evaluated& reference)
	{
		if (auto listOpt = AsOpt<List>(reference))
		{
			List expanded;
			for (const auto& elem : listOpt.value().data)
			{
				expanded.append(expandList(elem));
			}
			return expanded;
		}
		
		return dereference(reference);
	}

	void bindNewValue(const std::string& name, const Evaluated& value)
	{
		const unsigned newID = m_values.add(value);
		bindValueID(name, newID);
	}

	void bindReference(const std::string& nameLhs, const std::string& nameRhs)
	{
		const auto valueIDOpt = findValueID(nameRhs);
		if (!valueIDOpt)
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return;
		}

		bindValueID(nameLhs, valueIDOpt.value());
	}

	void bindValueID(const std::string& name, unsigned valueID)
	{
		/*
		for(auto it = m_bindingNames.rbegin(); it != m_bindingNames.rend(); ++it)
		{
			auto& localEnv = *it;
			if (localEnv.find(name))
			{
				localEnv.bind(name, valueID);
				return;
			}
		}
		*/

		/*
		レコード
		レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
		レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアス（スコープを抜けたら元に戻る≒遮蔽）
		*/

		//現在の環境に変数が存在しなければ、
		//環境リストの末尾（＝最も内側のスコープ）に変数を追加する
		m_bindingNames.back().bind(name, valueID);
	}

	void pushNormal()
	{
		m_bindingNames.emplace_back(LocalEnvironment::Type::NormalScope);
	}

	void pushRecord()
	{
		m_bindingNames.emplace_back(LocalEnvironment::Type::RecordScope);
	}

	void pop()
	{
		m_bindingNames.pop_back();
	}

	void printEnvironment()const
	{
		std::cout << "Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			std::cout << keyval.first << " : ";

			printEvaluated(val);
		}

		std::cout << "References:\n";
		for (size_t d = 0; d < m_bindingNames.size(); ++d)
		{
			std::cout << "Depth : " << d << "\n";
			const auto& names = m_bindingNames[d];
			
			for (const auto& keyval : names)
			{
				std::cout << keyval.first << " : " << keyval.second << "\n";
			}
		}
	}

	void assignToObject(const ObjectReference& objectRef, const Evaluated& newValue);

	static std::shared_ptr<Environment> Make()
	{
		auto p = std::make_shared<Environment>();
		p->m_weakThis = p;
		return p;
	}

	static std::shared_ptr<Environment> Make(const Environment& other)
	{
		auto p = std::make_shared<Environment>(other);
		p->m_weakThis = p;
		return p;
	}

	Environment() = default;

private:

	//内側のスコープから順番に変数を探して返す
	boost::optional<unsigned> findValueID(const std::string& name)const
	{
		boost::optional<unsigned> valueIDOpt;

		for (auto it = m_bindingNames.rbegin(); it != m_bindingNames.rend(); ++it)
		{
			valueIDOpt = it->find(name);
			if (valueIDOpt)
			{
				break;
			}
		}

		return valueIDOpt;
	}

	void garbageCollect();

	Values m_values;

	//変数はスコープ単位で管理される
	//スコープを抜けたらそのスコープで管理している変数を環境ごと削除する
	//したがって環境はネストの浅い順にリストで管理することができる（同じ深さの環境が二つ存在することはない）
	//リストの最初の要素はグローバル変数とするとする
	std::vector<LocalEnvironment> m_bindingNames;

	std::weak_ptr<Environment> m_weakThis;
};

inline Evaluated Not(const Evaluated& lhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);

	if (IsType<bool>(lhs))
	{
		return !As<bool>(lhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Plus(const Evaluated& lhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);

	if (IsType<int>(lhs) || IsType<double>(lhs))
	{
		return lhs;
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Minus(const Evaluated& lhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);

	if (IsType<int>(lhs))
	{
		return -As<int>(lhs);
	}
	else if (IsType<double>(lhs))
	{
		return -As<double>(lhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated And(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<bool>(lhs) && IsType<bool>(rhs))
	{
		return As<bool>(lhs) && As<bool>(rhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Or(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<bool>(lhs) && IsType<bool>(rhs))
	{
		return As<bool>(lhs) || As<bool>(rhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Equal(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated NotEqual(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated LessThan(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated LessEqual(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated GreaterThan(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated GreaterEqual(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Max(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Add(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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
	else if(IsType<double>(lhs))
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
	
	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Sub(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Mul(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Div(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Pow(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

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

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Assign(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
{
	if (!IsLValue(lhs))
	{
		//代入式の左辺が左辺値でない
		std::cerr << "Error(" << __LINE__ << ")\n";
		return 0;
	}

	if (auto nameLhsOpt = AsOpt<Identifier>(lhs))
	{
		/*
		代入式は、左辺と右辺の形の組み合わせにより4パターンの挙動が起こり得る。

		式の左辺：現在の環境にすでに存在する識別子か、新たに束縛を行う識別子か
		式の右辺：左辺値であるか、右辺値であるか

		左辺値の場合は、その参照先を取得し新たに
		*/

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
			const auto& valueRhs = env.dereference(rhs);
			env.bindNewValue(nameLhs, valueRhs);
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
			const auto& valueRhs = env.dereference(nameRhs);

			env.assignToObject(objRefLhs, valueRhs);
		}
		else if (rhsIsObjRef)
		{
			const auto& valueRhs = env.dereference(rhs);
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

	return lhs;
}

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

	auto operator()(bool node)const -> void
	{
		std::cout << indent() << "Bool(" << node << ")" << std::endl;
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

	void operator()(const UnaryExpr& node)const
	{
		std::cout << indent() << "Unary(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const BinaryExpr& node)const
	{
		std::cout << indent() << "Binary(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
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

			if (IsType<Identifier>(callFunc.funcRef))
			{
				const auto& funcName = As<Identifier>(callFunc.funcRef);
				Expr expr(funcName);
				boost::apply_visitor(Printer(m_indent + 2), expr);
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

	auto operator()(const FunctionCaller& callFunc)const -> void
	{
		std::cout << indent() << "CallFunc(" << std::endl;

		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "FuncRef(" << std::endl;

			if (IsType<Identifier>(callFunc.funcRef))
			{
				const auto& funcName = As<Identifier>(callFunc.funcRef);
				Expr expr(funcName);
				boost::apply_visitor(Printer(m_indent + 2), expr);
			}
			else
			{
				std::cout << indent() << "(FuncVal)" << std::endl;
			}

			std::cout << child.indent() << ")" << std::endl;
		}

		{
			/*const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "Arguments(" << std::endl;

			const auto& args = callFunc.actualArguments;
			for (size_t i = 0; i < args.size(); ++i)
			{
				boost::apply_visitor(Printer(m_indent + 2), args[i]);
			}

			std::cout << child.indent() << ")" << std::endl;
			*/
		}

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const Range& range)const
	{
		std::cout << indent() << "Range(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), range.lhs);
		boost::apply_visitor(Printer(m_indent + 1), range.rhs);
		std::cout << indent() << ")" << std::endl;
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

	void operator()(const For& forExpression)const
	{
		std::cout << indent() << "For(" << std::endl;

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const Return& return_statement)const
	{
		std::cout << indent() << "Return(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), return_statement.expr);

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const ListConstractor& listConstractor)const
	{
		std::cout << indent() << "ListConstractor(" << std::endl;
		
		int i = 0;
		for (const auto& expr : listConstractor.data)
		{
			std::cout << indent() << "(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(m_indent + 1), expr);
			++i;
		}
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const ListAccess& listAccess)const
	{
		std::cout << indent() << "ListAccess(" << std::endl;

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const KeyExpr& keyExpr)const
	{
		std::cout << indent() << "KeyExpr(" << std::endl;

		const auto child = Printer(m_indent + 1);
		
		std::cout << child.indent() << keyExpr.name.name << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), keyExpr.expr);
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const RecordConstractor& recordConstractor)const
	{
		std::cout << indent() << "RecordConstractor(" << std::endl;

		int i = 0;
		for (const auto& expr : recordConstractor.exprs)
		{
			std::cout << indent() << "(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(m_indent + 1), expr);
			++i;
		}
		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const Accessor& accessor)const
	{
		std::cout << indent() << "Accessor(" << std::endl;

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const DeclSat& decl)const
	{
		std::cout << indent() << "DeclSat(" << std::endl;

		std::cout << indent() << ")" << std::endl;
	}

	void operator()(const DeclFree& decl)const
	{
		std::cout << indent() << "DeclFree(" << std::endl;

		std::cout << indent() << ")" << std::endl;
	}
};


class EvalSat : public boost::static_visitor<Evaluated>
{
public:

	EvalSat(std::shared_ptr<Environment> pEnv) :pEnv(pEnv) {}

	Evaluated operator()(bool node)
	{
		return node;
	}

	Evaluated operator()(int node)
	{
		return node;
	}

	Evaluated operator()(double node)
	{
		return node;
	}

	Evaluated operator()(const Identifier& node)
	{
		return pEnv->dereference(node);
	}

	Evaluated operator()(const UnaryExpr& node)
	{
		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

		switch (node.op)
		{
		case UnaryOp::Not:   return Not(lhs, *pEnv);
		case UnaryOp::Minus: return Minus(lhs, *pEnv);
		case UnaryOp::Plus:  return Plus(lhs, *pEnv);
		}

		std::cerr << "Error(" << __LINE__ << ")\n";

		return 0;
	}

	Evaluated operator()(const BinaryExpr& node)
	{
		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

		/*
		a == b => Minimize(C * Abs(a - b))
		a != b => Minimize(C * (a == b ? 1 : 0))
		a < b  => Minimize(C * Max(a - b, 0))
		a > b  => Minimize(C * Max(b - a, 0))

		e : error
		a == b => Minimize(C * Max(Abs(a - b) - e, 0))
		a != b => Minimize(C * (a == b ? 1 : 0))
		a < b  => Minimize(C * Max(a - b - e, 0))
		a > b  => Minimize(C * Max(b - a - e, 0))
		*/
		switch (node.op)
		{
		case BinaryOp::And: return Add(lhs, rhs, *pEnv);
		case BinaryOp::Or:  return Max(lhs, rhs, *pEnv);

		case BinaryOp::Equal:        return Sub(lhs, rhs, *pEnv);
		case BinaryOp::NotEqual:     return As<bool>(Equal(lhs, rhs, *pEnv)) ? 1.0 : 0.0;
		case BinaryOp::LessThan:     return Max(Sub(lhs, rhs, *pEnv), 0, *pEnv);
		case BinaryOp::LessEqual:    return Max(Sub(lhs, rhs, *pEnv), 0, *pEnv);
		case BinaryOp::GreaterThan:  return Max(Sub(rhs, lhs, *pEnv), 0, *pEnv);
		case BinaryOp::GreaterEqual: return Max(Sub(rhs, lhs, *pEnv), 0, *pEnv);

		case BinaryOp::Add: return Add(lhs, rhs, *pEnv);
		case BinaryOp::Sub: return Sub(lhs, rhs, *pEnv);
		case BinaryOp::Mul: return Mul(lhs, rhs, *pEnv);
		case BinaryOp::Div: return Div(lhs, rhs, *pEnv);

		case BinaryOp::Pow:    return Pow(lhs, rhs, *pEnv);
		//case BinaryOp::Assign: return Assign(lhs, rhs, *pEnv);
		}

		std::cerr << "Error(" << __LINE__ << ")\n";

		return 0;
	}

	Evaluated operator()(const Range& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const Lines& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const DefFunc& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const CallFunc& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const If& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const For& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const Return& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const ListConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const KeyExpr& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const RecordConstractor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const Accessor& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const FunctionCaller& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const DeclSat& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }
	Evaluated operator()(const DeclFree& node) { std::cerr << "Error(" << __LINE__ << "): invalid expression\n"; return 0; }

private:

	std::shared_ptr<Environment> pEnv;
};


class Eval : public boost::static_visitor<Evaluated>
{
public:

	Eval(std::shared_ptr<Environment> pEnv) :pEnv(pEnv) {}

	Evaluated operator()(bool node)
	{
		return node;
	}

	Evaluated operator()(int node)
	{
		return node;
	}

	Evaluated operator()(double node)
	{
		return node;
	}

	Evaluated operator()(const Identifier& node)
	{
		return node;
	}

	Evaluated operator()(const UnaryExpr& node)
	{
		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);

		switch (node.op)
		{
		case UnaryOp::Not:   return Not(lhs, *pEnv);
		case UnaryOp::Minus: return Minus(lhs, *pEnv);
		case UnaryOp::Plus:  return Plus(lhs, *pEnv);
		}

		//std::cout << "End UnaryExpr expression(" << ")" << std::endl;
		std::cerr << "Error(" << __LINE__ << ")\n";

		//return lhs;
		return 0;
	}

	Evaluated operator()(const BinaryExpr& node)
	{
		const Evaluated lhs = boost::apply_visitor(*this, node.lhs);
		const Evaluated rhs = boost::apply_visitor(*this, node.rhs);

		switch (node.op)
		{
		case BinaryOp::And: return And(lhs, rhs, *pEnv);
		case BinaryOp::Or:  return Or(lhs, rhs, *pEnv);

		case BinaryOp::Equal:        return Equal(lhs, rhs, *pEnv);
		case BinaryOp::NotEqual:     return NotEqual(lhs, rhs, *pEnv);
		case BinaryOp::LessThan:     return LessThan(lhs, rhs, *pEnv);
		case BinaryOp::LessEqual:    return LessEqual(lhs, rhs, *pEnv);
		case BinaryOp::GreaterThan:  return GreaterThan(lhs, rhs, *pEnv);
		case BinaryOp::GreaterEqual: return GreaterEqual(lhs, rhs, *pEnv);

		case BinaryOp::Add: return Add(lhs, rhs, *pEnv);
		case BinaryOp::Sub: return Sub(lhs, rhs, *pEnv);
		case BinaryOp::Mul: return Mul(lhs, rhs, *pEnv);
		case BinaryOp::Div: return Div(lhs, rhs, *pEnv);

		case BinaryOp::Pow:    return Pow(lhs, rhs, *pEnv);
		case BinaryOp::Assign: return Assign(lhs, rhs, *pEnv);
		}

		std::cerr << "Error(" << __LINE__ << ")\n";

		return 0;
	}

	Evaluated operator()(const DefFunc& defFunc)
	{
		//auto val = FuncVal(globalVariables, defFunc.arguments, defFunc.expr);
		//auto val = FuncVal(pEnv, defFunc.arguments, defFunc.expr);

		//FuncVal val(std::make_shared<Environment>(*pEnv), defFunc.arguments, defFunc.expr);
		FuncVal val(Environment::Make(*pEnv), defFunc.arguments, defFunc.expr);

		return val;
	}

	Evaluated operator()(const CallFunc& callFunc)
	{
		std::shared_ptr<Environment> buckUp = pEnv;
		//const auto buckUp = localVariables;

		FuncVal funcVal;

		//if (SameType(callFunc.funcRef.type(), typeid(FuncVal)))
		if(IsType<FuncVal>(callFunc.funcRef))
		{
			//funcVal = boost::get<FuncVal>(callFunc.funcRef);
			funcVal = As<FuncVal>(callFunc.funcRef);
		}
		//else if (auto itOpt = findVariable(boost::get<Identifier>(callFunc.funcRef).name))
		else if (auto funcOpt = pEnv->find(As<Identifier>(callFunc.funcRef).name))
		{
			/*const Evaluated& funcRef = (*itOpt.get()).second;
			if (SameType(funcRef.type(), typeid(FuncVal)))
			{
				funcVal = boost::get<FuncVal>(funcRef);
			}
			else
			{
				std::cerr << "Error(" << __LINE__ << "): variable \"" << (*itOpt.get()).first << "\" is not a function.\n";
				return 0;
			}*/
			const Evaluated& funcRef = funcOpt.value();
			if (IsType<FuncVal>(funcRef))
			{
				funcVal = As<FuncVal>(funcRef);
			}
			else
			{
				std::cerr << "Error(" << __LINE__ << "): variable \"" << As<Identifier>(callFunc.funcRef).name << "\" is not a function.\n";
				return 0;
			}
		}

		//const auto& funcVal = callFunc.funcVal;
		assert(funcVal.arguments.size() == callFunc.actualArguments.size());

		/*
		引数に与えられた式の評価
		この時点ではまだ関数の外なので、ローカル変数は変わらない。
		*/
		std::vector<Evaluated> argmentValues(funcVal.arguments.size());

		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			argmentValues[i] = boost::apply_visitor(*this, callFunc.actualArguments[i]);

			//引数に変数が指定された場合は、
			//関数の実行される環境にその変数があるかはわからないので、
			//中身を取り出して値渡しにする
			argmentValues[i] = pEnv->dereference(argmentValues[i]);
		}

		/*
		関数の評価
		ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える。
		*/
		//localVariables = funcVal.environment;
		pEnv = funcVal.environment;

		/*for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			localVariables[funcVal.arguments[i].name] = argmentValues[i];
		}*/
		
		//関数の引数用にスコープを一つ追加する
		pEnv->pushNormal();
		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			//現在は値も変数も全て値渡し（コピー）をしているので、単純に bindNewValue を行えばよい
			//本当は変数の場合は bindReference で参照情報だけ渡すべきなのかもしれない
			//要考察
			pEnv->bindNewValue(funcVal.arguments[i].name, argmentValues[i]);
			
			//localVariables[funcVal.arguments[i].name] = argmentValues[i];
		}

		std::cout << "Function Environment:\n";
		pEnv->printEnvironment();

		std::cout << "Function Definition:\n";
		boost::apply_visitor(Printer(), funcVal.expr);

		Evaluated result = boost::apply_visitor(*this, funcVal.expr);

		//関数を抜ける時に、仮引数は全て解放される
		pEnv->pop();

		/*
		最後にローカル変数の環境を関数の実行前のものに戻す。
		*/
		pEnv = buckUp;

		//評価結果がreturn式だった場合はreturnを外して中身を返す
		//return以外のジャンプ命令は関数では効果を持たないのでそのまま上に返す
		if (IsType<Jump>(result))
		{
			auto& jump = As<Jump>(result);
			if (jump.isReturn())
			{
				if (jump.lhs)
				{
					return jump.lhs.value();
				}
				else
				{
					//return式の中身が入って無ければとりあえずエラー
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}
			}
		}

		return result;
	}

	Evaluated operator()(const FunctionCaller& callFunc)
	{
		std::shared_ptr<Environment> buckUp = pEnv;
		//const auto buckUp = localVariables;

		FuncVal funcVal;

		//if (SameType(callFunc.funcRef.type(), typeid(FuncVal)))
		if (IsType<FuncVal>(callFunc.funcRef))
		{
			//funcVal = boost::get<FuncVal>(callFunc.funcRef);
			funcVal = As<FuncVal>(callFunc.funcRef);
		}
		//else if (auto itOpt = findVariable(boost::get<Identifier>(callFunc.funcRef).name))
		else if (auto funcOpt = pEnv->find(As<Identifier>(callFunc.funcRef).name))
		{
			/*const Evaluated& funcRef = (*itOpt.get()).second;
			if (SameType(funcRef.type(), typeid(FuncVal)))
			{
			funcVal = boost::get<FuncVal>(funcRef);
			}
			else
			{
			std::cerr << "Error(" << __LINE__ << "): variable \"" << (*itOpt.get()).first << "\" is not a function.\n";
			return 0;
			}*/
			const Evaluated& funcRef = funcOpt.value();
			if (IsType<FuncVal>(funcRef))
			{
				funcVal = As<FuncVal>(funcRef);
			}
			else
			{
				std::cerr << "Error(" << __LINE__ << "): variable \"" << As<Identifier>(callFunc.funcRef).name << "\" is not a function.\n";
				return 0;
			}
		}

		//const auto& funcVal = callFunc.funcVal;
		assert(funcVal.arguments.size() == callFunc.actualArguments.size());
		
		/*
		関数の評価
		ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える。
		*/
		//localVariables = funcVal.environment;
		pEnv = funcVal.environment;

		/*for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
		localVariables[funcVal.arguments[i].name] = argmentValues[i];
		}*/

		//関数の引数用にスコープを一つ追加する
		pEnv->pushNormal();
		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			//現在は値も変数も全て値渡し（コピー）をしているので、単純に bindNewValue を行えばよい
			//本当は変数の場合は bindReference で参照情報だけ渡すべきなのかもしれない
			//要考察
			pEnv->bindNewValue(funcVal.arguments[i].name, callFunc.actualArguments[i]);

			//localVariables[funcVal.arguments[i].name] = argmentValues[i];
		}

		std::cout << "Function Environment:\n";
		pEnv->printEnvironment();

		std::cout << "Function Definition:\n";
		boost::apply_visitor(Printer(), funcVal.expr);

		Evaluated result = boost::apply_visitor(*this, funcVal.expr);

		//関数を抜ける時に、仮引数は全て解放される
		pEnv->pop();

		/*
		最後にローカル変数の環境を関数の実行前のものに戻す。
		*/
		pEnv = buckUp;

		//評価結果がreturn式だった場合はreturnを外して中身を返す
		//return以外のジャンプ命令は関数では効果を持たないのでそのまま上に返す
		if (IsType<Jump>(result))
		{
			auto& jump = As<Jump>(result);
			if (jump.isReturn())
			{
				if (jump.lhs)
				{
					return jump.lhs.value();
				}
				else
				{
					//return式の中身が入って無ければとりあえずエラー
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}
			}
		}

		return result;
	}

	Evaluated operator()(const Range& range)
	{
		return 0;
	}

	Evaluated operator()(const Lines& statement)
	{
		pEnv->pushNormal();

		Evaluated result;
		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			std::cout << "Evaluate expression(" << i << ")" << std::endl;
			result = boost::apply_visitor(*this, expr);

			//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
			if (IsLValue(result))
			{
				const Evaluated& resultValue = pEnv->dereference(result);
				const bool isMacro = IsType<Jump>(resultValue);
				if (isMacro)
				{
					result = As<Jump>(resultValue);
				}
			}
			
			//途中でジャンプ命令を読んだら即座に評価を終了する
			if (IsType<Jump>(result))
			{
				break;
			}

			++i;
		}

		//この後すぐ解放されるので dereference しておく
		result = pEnv->dereference(result);

		pEnv->pop();

		return result;
	}

	Evaluated operator()(const If& if_statement)
	{
		const Evaluated cond = boost::apply_visitor(*this, if_statement.cond_expr);
		if (!IsType<bool>(cond))
		{
			//条件は必ずブール値である必要がある
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
		}

		if (As<bool>(cond))
		{
			const Evaluated result = boost::apply_visitor(*this, if_statement.then_expr);
			return result;

		}
		else if(if_statement.else_expr)
		{
			const Evaluated result = boost::apply_visitor(*this, if_statement.else_expr.value());
			return result;
		}
		
		//else式が無いケースで cond = False であったら一応警告を出す
		std::cerr << "Warning(" << __LINE__ << ")\n";
		return 0;
	}

	Evaluated operator()(const For& forExpression)
	{
		const Evaluated startVal = pEnv->dereference(boost::apply_visitor(*this, forExpression.rangeStart));
		const Evaluated endVal = pEnv->dereference(boost::apply_visitor(*this, forExpression.rangeEnd));

		//startVal <= endVal なら 1
		//startVal > endVal なら -1
		//を適切な型に変換して返す
		const auto calcStepValue = [&](const Evaluated& a, const Evaluated& b)->boost::optional<std::pair<Evaluated, bool>>
		{
			const bool a_IsInt = IsType<int>(a);
			const bool a_IsDouble = IsType<double>(a);

			const bool b_IsInt = IsType<int>(b);
			const bool b_IsDouble = IsType<double>(b);

			if (!((a_IsInt || a_IsDouble) && (b_IsInt || b_IsDouble)))
			{
				//エラー：ループのレンジが不正な型（整数か実数に評価できる必要がある）
				std::cerr << "Error(" << __LINE__ << ")\n";
				return boost::none;
			}

			const bool result_IsDouble = a_IsDouble || b_IsDouble;
			const auto lessEq = LessEqual(a, b, *pEnv);
			if (!IsType<bool>(lessEq))
			{
				//エラー：aとbの比較に失敗した
				//一応確かめているだけでここを通ることはないはず
				//LessEqualの実装ミス？
				std::cerr << "Error(" << __LINE__ << ")\n";
				return boost::none;
			}

			const bool isInOrder = As<bool>(lessEq);

			const int sign = isInOrder ? 1 : -1;

			if (result_IsDouble)
			{
				return std::pair<Evaluated, bool>(Mul(1.0, sign, *pEnv), isInOrder);
			}

			return std::pair<Evaluated, bool>(Mul(1, sign, *pEnv), isInOrder);
		};

		const auto loopContinues = [&](const Evaluated& loopCount, bool isInOrder)->boost::optional<bool>
		{
			//isInOrder == true
			//loopCountValue <= endVal

			//isInOrder == false
			//loopCountValue > endVal
			
			const Evaluated result = LessEqual(loopCount, endVal, *pEnv);
			if (!IsType<bool>(result))
			{
				//ここを通ることはないはず
				std::cerr << "Error(" << __LINE__ << ")\n";
				return boost::none;
			}

			return As<bool>(result) == isInOrder;
		};

		const auto stepOrder = calcStepValue(startVal, endVal);
		if (!stepOrder)
		{
			//エラー：ループのレンジが不正
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
		}

		const Evaluated step = stepOrder.value().first;
		const bool isInOrder = stepOrder.value().second;

		Evaluated loopCountValue = startVal;

		Evaluated loopResult;

		pEnv->pushNormal();

		while (true)
		{
			const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
			if (!isLoopContinuesOpt)
			{
				//エラー：ここを通ることはないはず
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}

			//ループの継続条件を満たさなかったので抜ける
			if (!isLoopContinuesOpt.value())
			{
				break;
			}

			pEnv->bindNewValue(forExpression.loopCounter.name, loopCountValue);

			loopResult = boost::apply_visitor(*this, forExpression.doExpr);

			//ループカウンタの更新
			loopCountValue = Add(loopCountValue, step, *pEnv);
		}

		pEnv->pop();
		
		return loopResult;
	}

	Evaluated operator()(const Return& return_statement)
	{
		const Evaluated lhs = boost::apply_visitor(*this, return_statement.expr);
		
		return Jump::MakeReturn(lhs);
	}

	Evaluated operator()(const ListConstractor& listConstractor)
	{
		List list;
		for (const auto& expr : listConstractor.data)
		{
			const Evaluated value = boost::apply_visitor(*this, expr);
			list.append(value);
		}

		return list;
	}

	Evaluated operator()(const KeyExpr& keyExpr)
	{
		const Evaluated value = boost::apply_visitor(*this, keyExpr.expr);

		return KeyValue(keyExpr.name, value);
	}

	Evaluated operator()(const RecordConstractor& recordConsractor)
	{
		pEnv->pushRecord();

		std::vector<Identifier> keyList;
		/*
		レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
		レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアスとして定義（スコープを抜けたら元に戻る≒遮蔽）
		*/

		Record record;

		int i = 0;
		for (const auto& expr : recordConsractor.exprs)
		{
			//std::cout << "Evaluate expression(" << i << ")" << std::endl;
			Evaluated value = boost::apply_visitor(*this, expr);

			//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
			if (auto keyValOpt = AsOpt<KeyValue>(value))
			{
				const auto keyVal = keyValOpt.value();
				keyList.push_back(keyVal.name);

				Assign(keyVal.name, keyVal.value, *pEnv);
			}
			else if (auto declSatOpt = AsOpt<DeclSat>(value))
			{

				//Expr constraints;
				//std::vector<DeclFree::Ref> freeVariables;

				//record.constraint = declSatOpt.value().expr;
				if (record.constraint)
				{
					record.constraint = BinaryExpr(record.constraint.value(), declSatOpt.value().expr, BinaryOp::And);
				}
				else
				{
					record.constraint = declSatOpt.value().expr;
				}
			}
			else if (auto declFreeOpt = AsOpt<DeclFree>(value))
			{
				const auto& refs = declFreeOpt.value().refs;
				//record.freeVariables.push_back(declFreeOpt.value().refs);
				
				record.freeVariables.insert(record.freeVariables.end(), refs.begin(), refs.end());
			}

			//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
			if (IsLValue(value))
			{
				const Evaluated& resultValue = pEnv->dereference(value);
				const bool isMacro = IsType<Jump>(resultValue);
				if (isMacro)
				{
					value = As<Jump>(resultValue);
				}
			}

			//途中でジャンプ命令を読んだら即座に評価を終了する
			if (IsType<Jump>(value))
			{
				break;
			}

			++i;
		}

		for (const auto& key : keyList)
		{
			record.append(key.name, pEnv->dereference(key));
		}

		//auto Environment::Make(*pEnv);
		if(record.constraint)
		{
			const Expr& constraint = record.constraint.value();

			/*std::vector<double> xs(record.freeVariables.size());
			for (size_t i = 0; i < xs.size(); ++i)
			{
				if (!IsType<Identifier>(record.freeVariables[i]))
				{
					std::cerr << "未対応" << std::endl;
					return 0;
				}

				As<Identifier>(record.freeVariables[i]).name;

				const auto val = pEnv->dereference(As<Identifier>(record.freeVariables[i]));

				if (!IsType<int>(val))
				{
					std::cerr << "未対応" << std::endl;
					return 0;
				}

				xs[i] = As<int>(val);
			}*/

			/*
			a == b => Minimize(C * Abs(a - b))
			a != b => Minimize(C * (a == b ? 1 : 0))
			a < b  => Minimize(C * Max(a - b, 0))
			a > b  => Minimize(C * Max(b - a, 0))

			e : error
			a == b => Minimize(C * Max(Abs(a - b) - e, 0))
			a != b => Minimize(C * (a == b ? 1 : 0))
			a < b  => Minimize(C * Max(a - b - e, 0))
			a > b  => Minimize(C * Max(b - a - e, 0))
			*/

			const auto func = [&](const std::vector<double>& xs)->double
			{
				pEnv->pushNormal();

				for (size_t i = 0; i < xs.size(); ++i)
				{
					if (!IsType<Identifier>(record.freeVariables[i]))
					{
						std::cerr << "未対応" << std::endl;
						return 0;
					}

					As<Identifier>(record.freeVariables[i]).name;

					const auto val = pEnv->dereference(As<Identifier>(record.freeVariables[i]));

					pEnv->bindNewValue(As<Identifier>(record.freeVariables[i]).name, val);
				}

				EvalSat evaluator(pEnv);
				Evaluated errorVal = boost::apply_visitor(evaluator, constraint);

				pEnv->pop();

				const double d = As<double>(errorVal);
				return d;
			};
		}

		pEnv->pop();
		return record;
	}

	Evaluated operator()(const Accessor& accessor)
	{
		ObjectReference result;
		
		Evaluated lval = boost::apply_visitor(*this, accessor.head);
		if (!IsType<Identifier>(lval))
		{
			//エラー：Identifier以外（オブジェクトのリテラル値へのアクセス）には未対応
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
		}
		result.name = As<Identifier>(lval).name;

		for (const auto& access : accessor.accesses)
		{
			if (auto listOpt = AsOpt<ListAccess>(access))
			{
				Evaluated value = boost::apply_visitor(*this, listOpt.value().index);

				if (auto indexOpt = AsOpt<int>(value))
				{
					result.appendListRef(indexOpt.value());
				}
				else
				{
					//エラー：list[index] の index が int 型でない
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}
			}
			else if (auto recordOpt = AsOpt<RecordAccess>(access))
			{
				result.appendRecordRef(recordOpt.value().name.name);
			}
			else
			{
				auto funcAccess = As<FunctionAccess>(access);

				std::vector<Evaluated> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					args.push_back(boost::apply_visitor(*this, expr));
				}
				result.appendFunctionRef(std::move(args));
			}
		}

		return result;
	}

	Evaluated operator()(const DeclSat& decl)
	{
		return decl;
	}

	Evaluated operator()(const DeclFree& decl)
	{
		return decl;
	}

private:

	std::shared_ptr<Environment> pEnv;
};

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

inline Evaluated evalExpr(const Expr& expr)
{
	//auto env = std::make_shared<Environment>();
	auto env = Environment::Make();

	Eval evaluator(env);

	const Evaluated result = boost::apply_visitor(evaluator, expr);

	return result;
}

const Evaluated& Environment::dereference(const Evaluated& reference)
{
	//即値はValuesには持っているとは限らない
	/*
	if (!IsType<Identifier>(reference))
	{
	return reference;
	}

	const boost::optional<unsigned> valueIDOpt = findValueID(As<const Identifier&>(reference).name);
	if (!valueIDOpt)
	{
	std::cerr << "Error(" << __LINE__ << ")\n";
	return reference;
	}

	return m_values[valueIDOpt.value()];
	*/

	if (auto nameOpt = AsOpt<Identifier>(reference))
	{
		const boost::optional<unsigned> valueIDOpt = findValueID(nameOpt.value().name);
		if (!valueIDOpt)
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return reference;
		}

		return m_values[valueIDOpt.value()];
	}
	/*
	else if (auto listRefOpt = AsOpt<ListReference>(reference))
	{
	const auto& ref = listRefOpt.value();
	const auto& listVal = m_values[ref.localValueID];

	boost::optional<const Evaluated&> result = listVal;
	for (const int index : ref.indices)
	{
	if (auto listOpt = AsOpt<List>(result.value()))
	{
	result = listOpt.value().data[index];
	}
	else
	{
	//指定されたローカルIDの値がList型ではない
	//リストの参照に失敗
	std::cerr << "Error(" << __LINE__ << ")\n";
	return reference;
	}
	}

	//if (auto listOpt = AsOpt<List>(listVal))
	//{
	//	return listOpt.value().data[listRefOpt.value().index];
	//}
	//else
	//{
	//	//指定されたローカルIDの値がList型ではない
	//	std::cerr << "Error(" << __LINE__ << ")\n";
	//	return reference;
	//}

	if (result)
	{
	return result.value();
	}
	}
	*/
	else if (auto objRefOpt = AsOpt<ObjectReference>(reference))
	{
		const auto& referenceProcess = objRefOpt.value();
		const boost::optional<unsigned> valueIDOpt = findValueID(referenceProcess.name);
		if (!valueIDOpt)
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return reference;
		}

		boost::optional<const Evaluated&> result = m_values[valueIDOpt.value()];
		
		for (const auto& ref : referenceProcess.references)
		{
			if (auto listRefOpt = AsOpt<ObjectReference::ListRef>(ref))
			{
				const int index = listRefOpt.value().index;

				if (auto listOpt = AsOpt<List>(result.value()))
				{
					result = listOpt.value().data[index];
				}
				else
				{
					//リストとしてアクセスするのに失敗
					std::cerr << "Error(" << __LINE__ << ")\n";
					return reference;
				}
			}
			else if (auto recordRefOpt = AsOpt<ObjectReference::RecordRef>(ref))
			{
				const std::string& key = recordRefOpt.value().key;

				if (auto recordOpt = AsOpt<Record>(result.value()))
				{
					result = recordOpt.value().values.at(key);
				}
				else
				{
					//レコードとしてアクセスするのに失敗
					std::cerr << "Error(" << __LINE__ << ")\n";
					return reference;
				}
			}
			else if (auto funcRefOpt = AsOpt<ObjectReference::FunctionRef>(ref))
			{
				if (auto recordOpt = AsOpt<FuncVal>(result.value()))
				{
					Expr caller = FunctionCaller(recordOpt.value(), funcRefOpt.value().args);
					
					if (auto sharedThis = m_weakThis.lock())
					{
						Eval evaluator(sharedThis);
						
						const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
						result = m_values[ID];
					}
					else
					{
						//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
						std::cerr << "Error(" << __LINE__ << ")\n";
						return reference;
					}
				}
				else
				{
					//関数としてアクセスするのに失敗
					std::cerr << "Error(" << __LINE__ << ")\n";
					return reference;
				}
			}
		}

		return result.value();
		/*
		const auto& listVal = m_values[ref.localValueID];

		boost::optional<const Evaluated&> result = listVal;
		for (const int index : ref.indices)
		{
		if (auto listOpt = AsOpt<List>(result.value()))
		{
		result = listOpt.value().data[index];
		}
		else
		{
		//指定されたローカルIDの値がList型ではない
		//リストの参照に失敗
		std::cerr << "Error(" << __LINE__ << ")\n";
		return reference;
		}
		}

		if (result)
		{
		return result.value();
		}*/
	}

	return reference;
}

inline void Environment::assignToObject(const ObjectReference & objectRef, const Evaluated & newValue)
{
	const boost::optional<unsigned> valueIDOpt = findValueID(objectRef.name);
	if (!valueIDOpt)
	{
		std::cerr << "Error(" << __LINE__ << ")\n";
		//return reference;
		return;
	}

	boost::optional<Evaluated&> result = m_values[valueIDOpt.value()];
	
	for (const auto& ref : objectRef.references)
	{
		if (auto listRefOpt = AsOpt<ObjectReference::ListRef>(ref))
		{
			const int index = listRefOpt.value().index;

			if (auto listOpt = AsOpt<List>(result.value()))
			{
				result = listOpt.value().data[index];
			}
			else//リストとしてアクセスするのに失敗
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return;
			}
		}
		else if (auto recordRefOpt = AsOpt<ObjectReference::RecordRef>(ref))
		{
			const std::string& key = recordRefOpt.value().key;

			if (auto recordOpt = AsOpt<Record>(result.value()))
			{
				result = recordOpt.value().values.at(key);
			}
			else//レコードとしてアクセスするのに失敗
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return;
			}
		}
		else if (auto funcRefOpt = AsOpt<ObjectReference::FunctionRef>(ref))
		{
			if (auto recordOpt = AsOpt<FuncVal>(result.value()))
			{
				Expr caller = FunctionCaller(recordOpt.value(), funcRefOpt.value().args);

				if (auto sharedThis = m_weakThis.lock())
				{
					Eval evaluator(sharedThis);
					const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
					result = m_values[ID];
				}
				else//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}
			}
			else//関数としてアクセスするのに失敗
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return;
			}
		}
	}

	result.value() = newValue;

	/*
	const auto& listVal = m_values[ref.localValueID];

	boost::optional<const Evaluated&> result = listVal;
	for (const int index : ref.indices)
	{
	if (auto listOpt = AsOpt<List>(result.value()))
	{
	result = listOpt.value().data[index];
	}
	else
	{
	//指定されたローカルIDの値がList型ではない
	//リストの参照に失敗
	std::cerr << "Error(" << __LINE__ << ")\n";
	return reference;
	}
	}

	if (result)
	{
	return result.value();
	}*/
}

//inline ListReference ListAccess::fold(std::shared_ptr<Environment> pEnv)const
//{
//	Eval evaluator(pEnv);
//
//	const Evaluated indexVal = boost::apply_visitor(evaluator, index);
//	if (!IsType<int>(indexVal))
//	{
//		//エラー：インデックスが整数でない
//		std::cerr << "Error(" << __LINE__ << ")\n";
//		return ListReference();
//	}
//
//	ListReference result;
//
//	const int index = As<int>(indexVal);
//
//	if (auto listRefOpt = listRef)
//	{
//		const auto listRefVal = listRefOpt.value();
//
//		if (auto identifierOpt = AsOpt<Identifier>(listRefVal))
//		{
//			if (auto idOpt = pEnv->findValueID(identifierOpt.value().name))
//			{
//				//ID = idOptの値がリスト型であるかをチェックするべき
//				result = ListReference(idOpt.value(), index);
//			}
//			else
//			{
//				//エラー：識別子が定義されていない
//				std::cerr << "Error(" << __LINE__ << ")\n";
//				return ListReference();
//			}
//		}
//		else
//		{
//			std::cerr << "Error(" << __LINE__ << ")\n";
//			return ListReference();
//		}
//	}
//	else
//	{
//		std::cerr << "Error(" << __LINE__ << ")\n";
//		return ListReference();
//	}
//
//	if (!child.empty())
//	{
//		child.front().foldImpl(result, pEnv);
//	}
//
//	return result;
//}
//
//inline void ListAccess::foldImpl(ListReference& listReference, std::shared_ptr<Environment> pEnv)const
//{
//	Eval evaluator(pEnv);
//
//	const Evaluated indexVal = boost::apply_visitor(evaluator, index);
//	if (!IsType<int>(indexVal))
//	{
//		//エラー：インデックスが整数でない
//		std::cerr << "Error(" << __LINE__ << ")\n";
//		
//		listReference = ListReference();
//		return;
//	}
//
//	const int index = As<int>(indexVal);
//	listReference.add(index);
//
//	if (!child.empty())
//	{
//		child.front().foldImpl(listReference, pEnv);
//	}
//}

//inline void Concat(Expr& lines1, const Expr& lines2)
//{
//	if (!IsType<Lines>(lines1) || !IsType<Lines>(lines2))
//	{
//		std::cerr << "Error";
//		return;
//	}
//
//	As<Lines>(lines1).concat(As<const Lines>(lines2));
//}
//
//inline void Append(Expr& lines, const Expr& expr)
//{
//	if (!IsType<Lines>(lines))
//	{
//		std::cerr << "Error";
//		return;
//	}
//
//	As<Lines>(lines).add(expr);
//}

//inline DefFunc MakeDefFunc(Expr& lines, const Expr& expr)
//{
//	if (!IsType<Lines>(lines))
//	{
//		std::cerr << "Error";
//		return DefFunc();
//	}
//
//	auto& ls = As<Lines>(lines);
//	if (!DefFunc::IsValidArgs(ls, expr))
//	{
//		std::cerr << "Error";
//		return DefFunc();
//	}
//
//	return DefFunc(ls, expr);
//}
//
//inline DefFunc* NewDefFunc(Expr& lines, const Expr& expr)
//{
//	if (!IsType<Lines>(lines))
//	{
//		std::cerr << "Error";
//		return new DefFunc();
//	}
//
//	auto& ls = As<Lines>(lines);
//	if (!DefFunc::IsValidArgs(ls, expr))
//	{
//		std::cerr << "Error";
//		return new DefFunc();
//	}
//
//	return new DefFunc(ls, expr);
//}

inline void ValuePrinter::operator()(const FuncVal& node)const
{
	std::cout << indent() << "FuncVal(" << std::endl;

	{
		const auto child = ValuePrinter(m_indent + 1);
		std::cout << child.indent();
		for (size_t i = 0; i < node.arguments.size(); ++i)
		{
			std::cout << node.arguments[i].name;
			if (i + 1 != node.arguments.size())
			{
				std::cout << ", ";
			}
		}
		std::cout << std::endl;

		std::cout << child.indent() << "->";

		boost::apply_visitor(Printer(m_indent + 1), node.expr);
	}

	std::cout << indent() << ")" << std::endl;
}

inline bool IsEqual(const Evaluated& value1, const Evaluated& value2)
{
	if (!SameType(value1.type(), value2.type()))
	{
		std::cerr << "Values are not same type." << std::endl;
		return false;
	}

	if (IsType<bool>(value1))
	{
		return As<bool>(value1) == As<bool>(value2);
	}
	else if (IsType<int>(value1))
	{
		return As<int>(value1) == As<int>(value2);
	}
	else if (IsType<double>(value1))
	{
		return As<double>(value1) == As<double>(value2);
	}
	else if (IsType<Identifier>(value1))
	{
		return As<Identifier>(value1) == As<Identifier>(value2);
	}
	else if (IsType<ObjectReference>(value1))
	{
		return As<ObjectReference>(value1) == As<ObjectReference>(value2);
	}
	else if (IsType<List>(value1))
	{
		return As<List>(value1) == As<List>(value2);
	}
	else if (IsType<KeyValue>(value1))
	{
		return As<KeyValue>(value1) == As<KeyValue>(value2);
	}
	else if (IsType<Record>(value1))
	{
		return As<Record>(value1) == As<Record>(value2);
	}
	else if (IsType<FuncVal>(value1))
	{
		return As<FuncVal>(value1) == As<FuncVal>(value2);
	}
	else if (IsType<Jump>(value1))
	{
		return As<Jump>(value1) == As<Jump>(value2);
	}
	else if (IsType<DeclSat>(value1))
	{
		return As<DeclSat>(value1) == As<DeclSat>(value2);
	}
	else if (IsType<DeclFree>(value1))
	{
		return As<DeclFree>(value1) == As<DeclFree>(value2);
	};

	std::cerr << "IsEqual: Type Error" << std::endl;
	return false;
}