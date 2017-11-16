#pragma once
#pragma warning(disable:4996)
#include <cmath>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <map>
#include <unordered_map>

#include <boost/fusion/include/vector.hpp>

#include <boost/variant.hpp>
#include <boost/mpl/vector/vector30.hpp>
#include <boost/optional.hpp>

#include <cmaes.h>

#ifdef _DEBUG
#pragma comment(lib,"Debug/cmaes.lib")
#else
#pragma comment(lib,"Release/cmaes.lib")
#endif

namespace cgl
{
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
		if (IsType<T1>(t2))
		{
			return boost::get<T1>(t2);
		}
		return boost::none;
	}

	template<class T1, class T2>
	inline boost::optional<const T1&> AsOpt(const T2& t2)
	{
		if (IsType<T1>(t2))
		{
			return boost::get<T1>(t2);
		}
		return boost::none;
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
		boost::mpl::vector19<
		bool,
		int,
		double,
		Identifier,

		boost::recursive_wrapper<Range>,

		boost::recursive_wrapper<Lines>,
		boost::recursive_wrapper<DefFunc>,

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

		List& append(const Evaluated& value)
		{
			data.push_back(value);
			return *this;
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

		KeyValue(const Identifier& name, const Evaluated& value) :
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

			std::string asString()const
			{
				return std::string("[") + std::to_string(index) + "]";
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

			std::string asString()const
			{
				return std::string(".") + key;
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

			std::string asString()const
			{
				return std::string("( ") + std::to_string(args.size()) + "args" + " )";
			}
		};

		using Ref = boost::variant<ListRef, RecordRef, FunctionRef>;

		using ObjectT = boost::variant<Identifier, boost::recursive_wrapper<Record>, boost::recursive_wrapper<List>, boost::recursive_wrapper<FuncVal>>;

		ObjectT headValue;

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
			if (!(headValue == other.headValue))
			{
				return false;
			}

			if (references.size() != other.references.size())
			{
				return false;
			}

			for (size_t i = 0; i<references.size(); ++i)
			{
				if (!(references[i] == other.references[i]))
				{
					return false;
				}
			}

			return true;
		}

		std::string asString()const
		{
			//std::string str = name;
			std::string str = "objName";

			for (const auto& r : references)
			{
				if (auto opt = AsOpt<ListRef>(r))
				{
					str += opt.value().asString();
				}
				else if (auto opt = AsOpt<RecordRef>(r))
				{
					str += opt.value().asString();
				}
				else if (auto opt = AsOpt<FunctionRef>(r))
				{
					str += opt.value().asString();
				}
			}

			return str;
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

		RecordAccess(const Identifier& name) :
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

	using Access = boost::variant<ListAccess, RecordAccess, FunctionAccess>;

	struct Accessor
	{
		//�擪�̃I�u�W�F�N�g(���ʎq���������͊֐��E���X�g�E���R�[�h�̃��e����)
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

		static void Append(Accessor& obj, const Access& access)
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
}
