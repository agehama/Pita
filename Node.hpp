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

//using types =
//boost::mpl::vector26<
//	int,
//	double,
//	Identifier,
//	boost::recursive_wrapper<Statement>,
//	boost::recursive_wrapper<Lines>,
//	boost::recursive_wrapper<DefFunc>,
//	boost::recursive_wrapper<CallFunc>,
//
//	boost::recursive_wrapper<UnaryExpr<Not>>,
//	boost::recursive_wrapper<BinaryExpr<And>>,
//	boost::recursive_wrapper<BinaryExpr<Or>>,
//
//	boost::recursive_wrapper<BinaryExpr<Equal>>,
//	boost::recursive_wrapper<BinaryExpr<NotEqual>>,
//	boost::recursive_wrapper<BinaryExpr<LessThan>>,
//	boost::recursive_wrapper<BinaryExpr<LessEqual>>,
//	boost::recursive_wrapper<BinaryExpr<GreaterThan>>,
//	boost::recursive_wrapper<BinaryExpr<GreaterEqual>>,
//
//	boost::recursive_wrapper<UnaryExpr<Add>>,
//	boost::recursive_wrapper<UnaryExpr<Sub>>,
//
//	boost::recursive_wrapper<BinaryExpr<Add>>,
//	boost::recursive_wrapper<BinaryExpr<Sub>>,
//	boost::recursive_wrapper<BinaryExpr<Mul>>,
//	boost::recursive_wrapper<BinaryExpr<Div>>,
//
//	boost::recursive_wrapper<BinaryExpr<Pow>>,
//	boost::recursive_wrapper<BinaryExpr<Assign>>,
//
//	boost::recursive_wrapper<If>,
//	boost::recursive_wrapper<Return> >;

using types =
boost::mpl::vector18<
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

	/*boost::recursive_wrapper<ListAccess>,
	boost::recursive_wrapper<RecordAccess>,
	boost::recursive_wrapper<FunctionAccess>,*/

	boost::recursive_wrapper<Accessor>,

	boost::recursive_wrapper<FunctionCaller>
	>;

using Expr = boost::make_variant_over<types>::type;

void printExpr(const Expr& expr);

struct Jump;

//struct ListReference;

struct ObjectReference;

using Evaluated = boost::variant<
	bool,
	int,
	double,
	Identifier,
	//boost::recursive_wrapper<ListReference>,
	boost::recursive_wrapper<ObjectReference>,
	boost::recursive_wrapper<List>,
	boost::recursive_wrapper<KeyValue>,
	boost::recursive_wrapper<Record>,
	boost::recursive_wrapper<FuncVal>,
	boost::recursive_wrapper<Jump>
>;

bool IsLValue(const Evaluated& value)
{
	//return IsType<Identifier>(value) || IsType<ListReference>(value);
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
		std::cout << "general_expr" << std::endl;
		lines.add(expr);
	}

	static void Concat(Lines& lines1, const Lines& lines2)
	{
		std::cout << "statement" << std::endl;
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
	std::vector<Identifier> argments;
	Expr expr;

	FuncVal() = default;

	FuncVal(
		std::shared_ptr<Environment> environment,
		const std::vector<Identifier>& argments,
		const Expr& expr) :
		environment(environment),
		argments(argments),
		expr(expr)
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

//inline FuncVal GetFuncVal(const Identifier& funcName)
//{
//	const auto funcItOpt = findVariable(funcName.name);
//
//	if (!funcItOpt)
//	{
//		std::cerr << "Error(" << __LINE__ << "): function \"" << funcName.name << "\" was not found." << "\n";
//	}
//
//	const auto funcIt = funcItOpt.get();
//
//	if (!SameType(funcIt->second.type(), typeid(FuncVal)))
//	{
//		std::cerr << "Error(" << __LINE__ << "): function \"" << funcName.name << "\" is not a function." << "\n";
//	}
//
//	return boost::get<FuncVal>(funcIt->second);
//}

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

//struct ListReference
//{
//	unsigned localValueID;
//	std::vector<int> indices;
//
//	ListReference() = default;
//
//	ListReference(unsigned localValueID, int index) :
//		localValueID(localValueID),
//		indices({ index })
//	{}
//
//	void add(int index)
//	{
//		indices.push_back(index);
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
};

struct Record
{
	std::unordered_map<std::string, Evaluated> values;

	void append(const std::string& name, const Evaluated& value)
	{
		values[name] = value;
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

struct ObjectReference
{
	struct ListRef
	{
		int index;

		ListRef() = default;
		ListRef(int index) :index(index) {}
	};

	struct RecordRef
	{
		std::string key;

		RecordRef() = default;
		RecordRef(const std::string& key) :key(key) {}		
	};

	struct FunctionRef
	{
		std::vector<Evaluated> args;

		FunctionRef() = default;
		FunctionRef(const std::vector<Evaluated>& args) :args(args) {}
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
};

inline void printEvaluated(const Evaluated& evaluated)
{
	if (IsType<bool>(evaluated))
	{
		//std::cout << "Bool(" << As<bool>(evaluated) << ")";
		std::cout << "Bool(" << (As<bool>(evaluated) ? "true" : "false") << ")";
	}
	else if (IsType<int>(evaluated))
	{
		std::cout << "Int(" << As<int>(evaluated) << ")";
	}
	else if (IsType<double>(evaluated))
	{
		std::cout << "Double(" << As<double>(evaluated) << ")";
	}
	else if (IsType<Identifier>(evaluated))
	{
		std::cout << "Identifier(" << As<Identifier>(evaluated).name << ")";
	}
	else if (IsType<FuncVal>(evaluated))
	{
		std::cout << "FuncVal(" << ")";
	}
	else if (IsType<Jump>(evaluated))
	{
		std::cout << "Jump(" << As<Jump>(evaluated).op << ")";
	}
	else if (IsType<List>(evaluated))
	{
		std::cout << "List(";
		const auto& data = As<List>(evaluated).data;
		for (size_t i = 0; i < data.size(); ++i)
		{
			printEvaluated(data[i]);
			if (i + 1 != data.size())
			{
				std::cout << ", ";
			}
		}
		std::cout << ")";
	}
	//else if (IsType<ListReference>(evaluated))
	//{
	//	std::cout << "ListReference( ";
	//	auto ref = As<ListReference>(evaluated);
	//	//std::cout << "LocalEnv(" << ref.localValueID << ")[" << ref.index << "]";

	//	std::cout << "LocalEnv(" << ref.localValueID << ")";
	//	for (const int index : ref.indices)
	//	{
	//		std::cout << "[" << index << "]";
	//	}
	//	
	//	std::cout << " )";
	//}
	else if (IsType<ObjectReference>(evaluated))
	{
		std::cout << "ObjectReference( ";
		std::cout << " )";
	}
	else if (IsType<KeyValue>(evaluated))
	{
		std::cout << "KeyValue( ";
		auto ref = As<KeyValue>(evaluated);
		std::cout << ref.name.name << ":";
		printEvaluated(ref.value);
		std::cout << " )";
	}
	else if (IsType<Record>(evaluated))
	{
		std::cout << "Record( ";
		auto ref = As<Record>(evaluated);
		for (const auto& elem : ref.values)
		{
			std::cout << elem.first << ":";
			printEvaluated(elem.second);
		}
		std::cout << " )";
	}

	std::cout << std::endl;
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

	/*const Evaluated& derefID(unsigned valueID)const
	{
		return m_values[valueID];
	}*/

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
		for (auto& localEnv : m_bindingNames)
		{
			if (localEnv.find(name))
			{
				localEnv.bind(name, valueID);
			}
		}

		//現在の環境に変数が存在しなければ、
		//環境リストの末尾（＝最も内側のスコープ）に変数を追加する
		m_bindingNames.back().bind(name, valueID);
	}

	void push()
	{
		m_bindingNames.emplace_back();
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

	//void assignToList(const ListReference& listRef, const Evaluated& newValue)
	//{
	//	std::cout << "assignToList:";

	//	boost::optional<List&> listOpt;
	//	if (listOpt = AsOpt<List>(m_values[listRef.localValueID]))
	//	{
	//		for (size_t i = 0; i < listRef.indices.size(); ++i)
	//		{
	//			const int currentIndex = listRef.indices[i];
	//			Evaluated& result = listOpt.value().data[currentIndex];

	//			std::cout << "index deref:" << currentIndex << "\n";

	//			if (i + 1 == listRef.indices.size())
	//			{
	//				std::cout << "before assign:";
	//				printEvaluated(result);

	//				//最後まで読み終わったらここに来る
	//				result = newValue;

	//				std::cout << "after assign:";
	//				printEvaluated(result);
	//				return;
	//			}
	//			else if (!IsType<List>(result))
	//			{
	//				//まだインデックスが残っているのにデータがリストでなければ参照エラー
	//				std::cerr << "Error(" << __LINE__ << ")\n";
	//				return;
	//			}

	//			listOpt = As<List>(result);
	//		}

	//		//定義された識別子がリストでない
	//		std::cerr << "Error(" << __LINE__ << ")\n";
	//		return;
	//	}

	//	//指定されたIDのデータがList型ではない
	//	std::cerr << "Error(" << __LINE__ << ")\n";

	//	/*
	//	if (auto listOpt = AsOpt<List>(m_values[listRef.localValueID]))
	//	{
	//		listOpt.value().data[listRef.index] = newValue;
	//		return;
	//	}

	//	//指定されたIDのデータがList型ではない
	//	std::cerr << "Error(" << __LINE__ << ")\n";

	//	*/
	//}

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

private:

	//外側のスコープから順番に変数を探して返す
	boost::optional<unsigned> findValueID(const std::string& name)const
	{
		boost::optional<unsigned> valueIDOpt;

		for (auto it = m_bindingNames.rbegin(); it != m_bindingNames.rend(); ++it)
		{
			if (valueIDOpt = it->find(name))
			{
				break;
			}
		}

		/*for (const auto& localEnv : m_bindingNames)
		{
		if (valueIDOpt = localEnv.find(name))
		{
		break;
		}
		}*/

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

//inline Evaluated Dereference(const Evaluated& reference, const Environment& env)
//{
//	
//	if (!IsType<Identifier>(reference))
//	{
//		return reference;
//	}
//
//	const auto& name = As<Identifier>(reference).name;
//	const auto it = env.find(name);
//	if (it == env.end())
//	{
//		std::cerr << "Error(" << __LINE__ << ")\n";
//		return 0;
//	}
//
//	return Dereference(it->second, env);
//}

/*
Unary Operators:

Not,

Plus,
Minus
*/

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

/*
Binary Operators:

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
*/

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
	/*if (!IsType<Identifier>(lhs))
	{
		std::cerr << "Error(" << __LINE__ << ")\n";
		return 0;
	}*/
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


//extern std::map<std::string, Evaluated> globalVariables;
//extern std::map<std::string, Evaluated> localVariables;
//
//inline boost::optional<std::map<std::string, Evaluated>::iterator> findVariable(const std::string& variableName)
//{
//	std::map<std::string, Evaluated>::iterator itLocal = localVariables.find(variableName);
//	if (itLocal != localVariables.end())
//	{
//		return itLocal;
//	}
//
//	std::map<std::string, Evaluated>::iterator itGlobal = globalVariables.find(variableName);
//	if (itGlobal != globalVariables.end())
//	{
//		return itGlobal;
//	}
//
//	return boost::none;
//}


//struct EvalOpt
//{
//	bool m_bool;
//	int m_int;
//	double m_double;
//
//	int m_which;
//
//	static EvalOpt Bool(bool v)
//	{
//		return{ v,0,0,0 };
//	}
//	static EvalOpt Int(int v)
//	{
//		return{ 0,v,0,1 };
//	}
//	static EvalOpt Double(double v)
//	{
//		return{ 0,0,v,2 };
//	}
//};
//
//inline EvalOpt Ref(const Evaluated& lhs)
//{
//	if (IsType<bool>(lhs))
//	{
//		return EvalOpt::Bool(boost::get<bool>(lhs));
//	}
//	else if (IsType<int>(lhs))
//	{
//		return EvalOpt::Int(boost::get<int>(lhs));
//	}
//	else if (IsType<double>(lhs))
//	{
//		return EvalOpt::Double(boost::get<double>(lhs));
//	}
//	else if (IsType<Identifier>(lhs))
//	{
//		const auto name = boost::get<Identifier>(lhs).name;
//		const auto itOpt = findVariable(name);
//		if (!itOpt)
//		{
//			std::cerr << "Error(" << __LINE__ << ")\n";
//			return EvalOpt::Double(0);
//		}
//		return Ref(itOpt.get()->second);
//		//return EvalOpt::Double();
//	}
//
//	std::cerr << "Error(" << __LINE__ << ")\n";
//	return EvalOpt::Double(0);
//}

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

	/*auto operator()(const IntHolder& node)const -> void
	{
	std::cout << "IntHolder(" << node.n << ")";
	}*/

	void operator()(const UnaryExpr& node)const
	{
		std::cout << indent() << "Unary(" << std::endl;

		boost::apply_visitor(Printer(m_indent + 1), node.lhs);

		std::cout << indent() << ")" << std::endl;
	}

	/*void operator()(const UnaryExpr<Not>& node)const
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
	}*/

	void operator()(const BinaryExpr& node)const
	{
		std::cout << indent() << "Binary(" << std::endl;
		boost::apply_visitor(Printer(m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(m_indent + 1), node.rhs);
		std::cout << indent() << ")" << std::endl;
	}

	/*
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
	*/

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

	auto operator()(const FunctionCaller& callFunc)const -> void
	{
		std::cout << indent() << "CallFunc(" << std::endl;

		{
			const auto child = Printer(m_indent + 1);
			std::cout << child.indent() << "FuncRef(" << std::endl;

			if (IsType<Identifier>(callFunc.funcRef))
			{
				const auto& funcName = As<Identifier>(callFunc.funcRef);
				boost::apply_visitor(Printer(m_indent + 2), Expr(funcName));
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

		/*{
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
		}*/

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

	/*void operator()(const ListAccess& listAccess)const
	{
		std::cout << indent() << "ListConstractor(" << std::endl;

		const auto child = Printer(m_indent + 1);
		if (IsType<Identifier>(listAccess.listRef))
		{
			std::cout << child.indent() << As<Identifier>(listAccess.listRef).name << std::endl;
		}
		else
		{
			std::cout << child.indent() << "ListVal()" << std::endl;
		}

		boost::apply_visitor(Printer(m_indent + 1), listAccess.index);
		std::cout << indent() << ")" << std::endl;
	}*/

	void operator()(const ListAccess& listAccess)const
	{
		std::cout << indent() << "ListAccess(" << std::endl;

		/*if (auto listRefOpt = listAccess.listRef)
		{
			const auto listRef = listRefOpt.value();
			const auto child = Printer(m_indent + 1);
			if (IsType<Identifier>(listRef))
			{
				std::cout << child.indent() << As<Identifier>(listRef).name << std::endl;
			}
			else
			{
				std::cout << child.indent() << "ListVal()" << std::endl;
			}
		}
		if (!listAccess.child.empty())
		{
			const Expr child = listAccess.child.front();

			boost::apply_visitor(Printer(m_indent + 1), child);
		}

		boost::apply_visitor(Printer(m_indent + 1), listAccess.index);*/
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
};


class Eval : public boost::static_visitor<Evaluated>
{
public:

	Eval(std::shared_ptr<Environment> pEnv) :pEnv(pEnv) {}

	Evaluated operator()(bool node)
	{
		std::cout << "Begin-End bool expression(" << ")" << std::endl;

		return node;
	}

	Evaluated operator()(int node)
	{
		std::cout << "Begin-End int expression(" << ")" << std::endl;

		return node;
	}

	Evaluated operator()(double node)
	{
		std::cout << "Begin-End double expression(" << ")" << std::endl;

		return node;
	}

	Evaluated operator()(const Identifier& node)
	{
		std::cout << "Begin-End Identifier expression(" << ")" << std::endl;

		return node;
	}

	Evaluated operator()(const UnaryExpr& node)
	{
		std::cout << "Begin UnaryExpr expression(" << ")" << std::endl;

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

	/*Evaluated operator()(const UnaryExpr<Add>& node)const
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
	}*/

	Evaluated operator()(const BinaryExpr& node)
	{
		std::cout << "Begin BinaryExpr expression(" << ")" << std::endl;

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

	/*
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
	*/

	Evaluated operator()(const DefFunc& defFunc)
	{
		std::cout << "Begin DefFunc expression(" << ")" << std::endl;

		//auto val = FuncVal(globalVariables, defFunc.arguments, defFunc.expr);
		//auto val = FuncVal(pEnv, defFunc.arguments, defFunc.expr);

		//FuncVal val(std::make_shared<Environment>(*pEnv), defFunc.arguments, defFunc.expr);
		FuncVal val(Environment::Make(*pEnv), defFunc.arguments, defFunc.expr);

		std::cout << "End DefFunc expression(" << ")" << std::endl;

		return val;
	}

	Evaluated operator()(const CallFunc& callFunc)
	{
		std::cout << "Begin CallFunc expression(" << ")" << std::endl;
		
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
		assert(funcVal.argments.size() == callFunc.actualArguments.size());

		/*
		引数に与えられた式の評価
		この時点ではまだ関数の外なので、ローカル変数は変わらない。
		*/
		std::vector<Evaluated> argmentValues(funcVal.argments.size());

		for (size_t i = 0; i < funcVal.argments.size(); ++i)
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

		/*for (size_t i = 0; i < funcVal.argments.size(); ++i)
		{
			localVariables[funcVal.argments[i].name] = argmentValues[i];
		}*/
		
		//関数の引数用にスコープを一つ追加する
		pEnv->push();
		for (size_t i = 0; i < funcVal.argments.size(); ++i)
		{
			//現在は値も変数も全て値渡し（コピー）をしているので、単純に bindNewValue を行えばよい
			//本当は変数の場合は bindReference で参照情報だけ渡すべきなのかもしれない
			//要考察
			pEnv->bindNewValue(funcVal.argments[i].name, argmentValues[i]);
			
			//localVariables[funcVal.argments[i].name] = argmentValues[i];
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

		std::cout << "End CallFunc expression(" << ")" << std::endl;

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
		std::cout << "Begin CallFunc expression(" << ")" << std::endl;

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
		assert(funcVal.argments.size() == callFunc.actualArguments.size());
		
		/*
		関数の評価
		ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える。
		*/
		//localVariables = funcVal.environment;
		pEnv = funcVal.environment;

		/*for (size_t i = 0; i < funcVal.argments.size(); ++i)
		{
		localVariables[funcVal.argments[i].name] = argmentValues[i];
		}*/

		//関数の引数用にスコープを一つ追加する
		pEnv->push();
		for (size_t i = 0; i < funcVal.argments.size(); ++i)
		{
			//現在は値も変数も全て値渡し（コピー）をしているので、単純に bindNewValue を行えばよい
			//本当は変数の場合は bindReference で参照情報だけ渡すべきなのかもしれない
			//要考察
			pEnv->bindNewValue(funcVal.argments[i].name, callFunc.actualArguments[i]);

			//localVariables[funcVal.argments[i].name] = argmentValues[i];
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

		std::cout << "End CallFunc expression(" << ")" << std::endl;

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
		std::cout << "Begin Statement expression(" << ")" << std::endl;

		pEnv->push();

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

		std::cout << "End Statement expression(" << ")" << std::endl;

		return result;
	}

	Evaluated operator()(const If& if_statement)
	{
		std::cout << "Begin If expression(" << ")" << std::endl;

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
		std::cout << "Begin For expression(" << ")" << std::endl;

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

		pEnv->push();

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
		std::cout << "Begin Return expression(" << ")" << std::endl;

		const Evaluated lhs = boost::apply_visitor(*this, return_statement.expr);
		
		return Jump::MakeReturn(lhs);
	}

	Evaluated operator()(const ListConstractor& listConstractor)
	{
		std::cout << "Begin ListConstractor()" << std::endl;

		List list;
		for (const auto& expr : listConstractor.data)
		{
			const Evaluated value = boost::apply_visitor(*this, expr);
			list.append(value);
		}

		return list;
	}

	//リストの要素アクセスは全て左辺値になる
	//Evaluated operator()(const ListAccess& listAccess)
	//{
	//	std::cout << "Begin ListAccess()" << std::endl;
	//	
	//	/*
	//	const Evaluated indexVal = boost::apply_visitor(*this, listAccess.index);
	//	
	//	if (!IsType<int>(indexVal))
	//	{
	//		//エラー：インデックスが整数でない
	//		std::cerr << "Error(" << __LINE__ << ")\n";
	//		return 0;
	//	}

	//	const int index = As<int>(indexVal);

	//	if (auto identifierOpt = AsOpt<Identifier>(listAccess.listRef))
	//	{
	//		if (auto idOpt = pEnv->findValueID(identifierOpt.value().name))
	//		{
	//			//ID=idOptの値がリスト型であるかをチェックする
	//			return ListReference(idOpt.value(), index);
	//		}
	//		//エラー：識別子が定義されていない
	//		std::cerr << "Error(" << __LINE__ << ")\n";
	//		return 0;
	//	}

	//	//unknown error
	//	std::cerr << "Error(" << __LINE__ << ")\n";
	//	*/

	//	return listAccess.fold(pEnv);
	//}

	Evaluated operator()(const KeyExpr& keyExpr)
	{
		std::cout << "Begin KeyExpr()" << std::endl;
		
		const Evaluated value = boost::apply_visitor(*this, keyExpr.expr);

		return KeyValue(keyExpr.name, value);
	}

	Evaluated operator()(const RecordConstractor& recordConsractor)
	{
		std::cout << "Begin RecordConstractor()" << std::endl;

		pEnv->push();

		std::vector<Identifier> keyList;

		int i = 0;
		for (const auto& expr : recordConsractor.exprs)
		{
			std::cout << "Evaluate expression(" << i << ")" << std::endl;
			Evaluated value = boost::apply_visitor(*this, expr);

			//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
			if (auto keyValOpt = AsOpt<KeyValue>(value))
			{
				const auto keyVal = keyValOpt.value();
				keyList.push_back(keyVal.name);

				Assign(keyVal.name, keyVal.value, *pEnv);
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

		Record record;
		for (const auto& key : keyList)
		{
			record.append(key.name, pEnv->dereference(key));
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

private:

	//Environment& env;
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
