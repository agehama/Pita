#pragma once
#pragma warning(disable:4996)
#include <cmath>
#include <vector>
#include <memory>
#include <functional>
#include <string>
#include <exception>
#include <iostream>
#include <fstream>
#include <regex>
#include <set>
#include <map>
#include <unordered_map>
#include <wchar.h>

#include <boost/fusion/include/vector.hpp>

#include <boost/variant.hpp>
#include <boost/mpl/vector/vector30.hpp>
#include <boost/optional.hpp>

#include <cmaes.h>

#include <cppoptlib/meta.h>
#include <cppoptlib/problem.h>
#include <cppoptlib/solver/bfgssolver.h>

namespace cgl
{
	class Exception : public std::exception
	{
	public:
		explicit Exception(const std::string& message) :message(message) {}
		const char* what() const noexcept override { return message.c_str(); }

	private:
		std::string message;
	};

	inline void Log(std::ostream& os, const std::string& str)
	{
		//std::wregex regex("\n");
		//os << std::regex_replace(str, regex, "\n          |> ") << "\n";
		os << str << "\n";
	}

	const double pi = 3.1415926535;
	const double deg2rad = pi / 180.0;
	const double rad2deg = 180.0 / pi;
}

extern std::ofstream ofs;

#define CGL_FileName (strchr(__FILE__, '\\') ? strchr(__FILE__, '\\') + 1 : __FILE__)
#define CGL_FileDesc (std::string(CGL_FileName) + "(" + std::to_string(__LINE__) + ") : ")
#define CGL_TagError (std::string("[Error]   |> "))
#define CGL_TagWarn  (std::string("[Warning] |> "))
#define CGL_TagDebug (std::string("[Debug]   |> "))
#define CGL_Error(message) (throw cgl::Exception(CGL_FileDesc + message))

#ifdef CGL_EnableLogOutput
#define CGL_ErrorLog(message) (cgl::Log(std::cerr, CGL_TagError + CGL_FileDesc + message))
#define CGL_WarnLog(message)  (cgl::Log(std::cerr, CGL_TagWarn  + CGL_FileDesc + message))
//#define CGL_DebugLog(message) (cgl::Log(std::cout, CGL_TagDebug + CGL_FileDesc + message))
#define CGL_DebugLog(message) (cgl::Log(ofs, CGL_TagDebug + CGL_FileDesc + message))
#else
#define CGL_ErrorLog(message) 
#define CGL_WarnLog(message)  
#define CGL_DebugLog(message) 
#endif

namespace std
{
	template<class T> struct hash;
}

namespace cgl
{
	//std::string UTF8ToString(const std::string& str);

	template<class T1, class T2>
	inline bool SameType(const T1& t1, const T2& t2)
	{
		return t1 == t2;
	}

	template<class T1, class T2>
	inline bool IsType(const T2& t2)
	{
		return typeid(T1) == t2.type();
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
		Assign,

		Concat
	};

	inline std::string BinaryOpToStr(BinaryOp op)
	{
		switch (op)
		{
		case BinaryOp::And: return "And";
		case BinaryOp::Or:  return "Or";

		case BinaryOp::Equal:        return "Equal";
		case BinaryOp::NotEqual:     return "NotEqual";
		case BinaryOp::LessThan:     return "LessThan";
		case BinaryOp::LessEqual:    return "LessEqual";
		case BinaryOp::GreaterThan:  return "GreaterThan";
		case BinaryOp::GreaterEqual: return "GreaterEqual";

		case BinaryOp::Add: return "Add";
		case BinaryOp::Sub: return "Sub";
		case BinaryOp::Mul: return "Mul";
		case BinaryOp::Div: return "Div";

		case BinaryOp::Pow:    return "Pow";
		case BinaryOp::Assign: return "Assign";

		case BinaryOp::Concat: return "Concat";
		}

		return "Unknown";
	}

	struct DefFunc;

	struct Identifier
	{
	public:
		Identifier() = default;

		Identifier(const std::string& name_) :
			name(name_)
		{}

		bool operator==(const Identifier& other)const
		{
			return name == other.name;
		}

		bool operator!=(const Identifier& other)const
		{
			return !(*this == other);
		}

		operator const std::string&()const
		{
			return name;
		}

	private:
		std::string name;
	};

	struct Address
	{
	public:
		Address() :valueID(0) {}

		explicit Address(unsigned valueID) :
			valueID(valueID)
		{}

		bool isValid()const
		{
			return valueID != 0;
		}

		bool operator==(const Address other)const
		{
			return valueID == other.valueID;
		}

		bool operator!=(const Address other)const
		{
			return valueID != other.valueID;
		}

		static Address Null()
		{
			return Address();
		}

		std::string toString()const
		{
			return std::to_string(valueID);
		}

	private:
		friend struct std::hash<Address>;

		unsigned valueID;
	};
}

namespace std
{
	template <>
	struct hash<cgl::Address>
	{
	public:
		size_t operator()(const cgl::Address& address)const
		{
			return hash<size_t>()(static_cast<size_t>(address.valueID));
		}
	};
}

namespace cgl
{
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
	struct RecordInheritor;

	struct Character;

	struct KeyValue;
	struct Record;

	struct RecordAccess;
	struct FunctionAccess;

	struct Accessor;
	
	struct DeclSat;
	struct DeclFree;

	struct Jump;

	struct SatReference
	{
		int refID;

		SatReference() = default;

		explicit SatReference(int refID)
			:refID(refID)
		{}

		bool operator==(const SatReference& other)const
		{
			return refID == other.refID;
		}

		bool operator!=(const SatReference& other)const
		{
			return refID != other.refID;
		}
	};

	struct CharString
	{
	public:
		CharString() = default;
		CharString(const std::u32string& str) :str(str) {}

		bool operator==(const CharString& other)const
		{
			if (str.size() != other.str.size())
			{
				return false;
			}

			for (size_t i = 0; i < str.size(); ++i)
			{
				if (str[i] != other.str[i])
				{
					return false;
				}
			}

			return true;
		}

		bool operator!=(const CharString& other)const
		{
			return !(*this == other);
		}

		std::u32string toString()const
		{
			return str;
		}

	private:
		std::u32string str;
	};

	using Evaluated = boost::variant<
		bool,
		int,
		double,
		CharString,
		boost::recursive_wrapper<List>,
		boost::recursive_wrapper<KeyValue>,
		boost::recursive_wrapper<Record>,
		boost::recursive_wrapper<FuncVal>,
		boost::recursive_wrapper<Jump>
	>;

	struct RValue;
	struct LRValue;

	using Expr = boost::variant<
		boost::recursive_wrapper<LRValue>,
		Identifier,
		SatReference,

		boost::recursive_wrapper<UnaryExpr>,
		boost::recursive_wrapper<BinaryExpr>,

		boost::recursive_wrapper<DefFunc>,
		boost::recursive_wrapper<Range>,

		boost::recursive_wrapper<Lines>,

		boost::recursive_wrapper<If>,
		boost::recursive_wrapper<For>,
		boost::recursive_wrapper<Return>,

		boost::recursive_wrapper<ListConstractor>,

		boost::recursive_wrapper<KeyExpr>,
		boost::recursive_wrapper<RecordConstractor>,
		boost::recursive_wrapper<RecordInheritor>,
		boost::recursive_wrapper<DeclSat>,
		boost::recursive_wrapper<DeclFree>,

		boost::recursive_wrapper<Accessor>
	>;

	bool IsEqualEvaluated(const Evaluated& value1, const Evaluated& value2);
	bool IsEqual(const Expr& value1, const Expr& value2);

	void printExpr(const Expr& expr);

	struct RValue
	{
		RValue() = default;
		explicit RValue(const Evaluated& value) :value(value) {}

		bool operator==(const RValue& other)const
		{
			return IsEqualEvaluated(value, other.value);
		}

		bool operator!=(const RValue& other)const
		{
			return !IsEqualEvaluated(value, other.value);
		}
		
		Evaluated value;
	};

	struct LRValue
	{
		LRValue() = default;

		LRValue(const Evaluated& value) :value(RValue(value)) {}
		LRValue(const RValue& value) :value(value) {}
		LRValue(Address value) :value(value) {}

		static LRValue Bool(bool a) { return LRValue(a); }
		static LRValue Int(int a) { return LRValue(a); }
		static LRValue Double(double a) { return LRValue(a); }
		static LRValue Float(const std::string& str) { return LRValue(std::stod(str)); }
		//static LRValue Sat(const DeclSat& a) { return LRValue(a); }
		//static LRValue Free(const DeclFree& a) { return LRValue(a); }

		bool isRValue()const
		{
			return IsType<RValue>(value);
		}

		bool isLValue()const
		{
			return !isRValue();
		}

		Address address()const
		{
			return As<Address>(value);
		}

		const Evaluated& evaluated()const
		{
			return As<RValue>(value).value;
		}

		bool operator==(const LRValue& other)const
		{
			if (isLValue() && other.isLValue())
			{
				return address() == other.address();
			}
			else if (isRValue() && other.isRValue())
			{
				return IsEqualEvaluated(evaluated(), other.evaluated());
			}

			return false;
		}

		boost::variant<boost::recursive_wrapper<RValue>, Address> value;
	};

	struct OptimizationProblemSat;

	struct SatUnaryExpr;
	struct SatBinaryExpr;
	struct SatLines;

	struct SatFunctionReference;

	using SatExpr = boost::variant<
		double,
		SatReference,
		boost::recursive_wrapper<SatUnaryExpr>,
		boost::recursive_wrapper<SatBinaryExpr>,
		boost::recursive_wrapper<SatFunctionReference>
	>;

	class Context;

	struct OptimizationProblemSat
	{
		boost::optional<Expr> candidateExpr;

		boost::optional<Expr> expr;

		//制約式に含まれる全ての参照にIDを振る(=参照ID)
		//参照IDはdouble型の値に紐付けられる
		//std::unordered_map<int, int> refs;//参照ID -> dataのインデックス

		std::vector<double> data;//参照ID->data
		std::vector<Address> refs;//参照ID->Address

		std::unordered_map<Address, int> invRefs;//Address->参照ID

		void addConstraint(const Expr& logicExpr);
		void constructConstraint(std::shared_ptr<Context> pEnv, std::vector<Address>& freeVariables);

		bool initializeData(std::shared_ptr<Context> pEnv);

		void update(int index, double x)
		{
			data[index] = x;
		}

		double eval(std::shared_ptr<Context> pEnv);
	};

	struct SatUnaryExpr
	{
		SatExpr lhs;
		UnaryOp op;

		SatUnaryExpr() = default;

		SatUnaryExpr(const SatExpr& lhs, UnaryOp op) :
			lhs(lhs), 
			op(op)
		{}
	};

	struct SatBinaryExpr
	{
		SatExpr lhs;
		SatExpr rhs;
		BinaryOp op;

		SatBinaryExpr() = default;

		SatBinaryExpr(const SatExpr& lhs, const SatExpr& rhs, BinaryOp op) :
			lhs(lhs),
			rhs(rhs),
			op(op)
		{}
	};

	struct SatLines
	{
		std::vector<SatExpr> exprs;

		SatLines() = default;

		SatLines(const SatExpr& expr) :
			exprs({ expr })
		{}

		SatLines(const std::vector<SatExpr>& exprs_) :
			exprs(exprs_)
		{}

		void add(const SatExpr& expr)
		{
			exprs.push_back(expr);
		}

		void concat(const SatLines& lines)
		{
			exprs.insert(exprs.end(), lines.exprs.begin(), lines.exprs.end());
		}

		SatLines& operator+=(const SatLines& lines)
		{
			exprs.insert(exprs.end(), lines.exprs.begin(), lines.exprs.end());
			return *this;
		}

		size_t size()const
		{
			return exprs.size();
		}

		const SatExpr& operator[](size_t index)const
		{
			return exprs[index];
		}

		SatExpr& operator[](size_t index)
		{
			return exprs[index];
		}
	};

	struct UnaryExpr
	{
		Expr lhs;
		UnaryOp op;

		UnaryExpr(const Expr& lhs, UnaryOp op) :
			lhs(lhs), op(op)
		{}

		bool operator==(const UnaryExpr& other)const
		{
			if (op != other.op)
			{
				return false;
			}

			return IsEqual(lhs, other.lhs);
		}
	};

	struct BinaryExpr
	{
		Expr lhs;
		Expr rhs;
		BinaryOp op;

		BinaryExpr(const Expr& lhs, const Expr& rhs, BinaryOp op) :
			lhs(lhs), rhs(rhs), op(op)
		{}

		bool operator==(const BinaryExpr& other)const
		{
			if (op != other.op)
			{
				return false;
			}

			return IsEqual(lhs, other.lhs) && IsEqual(rhs, other.rhs);
		}
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

		bool operator==(const Range& other)const
		{
			return IsEqual(lhs, rhs);
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

		bool operator==(const Lines& other)const
		{
			if (exprs.size() != other.exprs.size())
			{
				return false;
			}

			for (size_t i = 0; i < exprs.size(); ++i)
			{
				if (!IsEqual(exprs[i], other.exprs[i]))
				{
					return false;
				}
			}

			return true;
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
		std::vector<Identifier> arguments;
		Expr expr;
		boost::optional<Address> builtinFuncAddress;
		
		FuncVal() = default;

		explicit FuncVal(Address builtinFuncAddress) :
			builtinFuncAddress(builtinFuncAddress)
		{}

		FuncVal(
			const std::vector<Identifier>& arguments,
			const Expr& expr) :
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

		bool operator==(const DefFunc& other)const
		{
			if (arguments.size() != other.arguments.size())
			{
				return false;
			}

			for (size_t i = 0; i < arguments.size(); ++i)
			{
				if (!(arguments[i] == other.arguments[i]))
				{
					return false;
				}
			}

			return IsEqual(expr, other.expr);
		}
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

		bool operator==(const If& other)const
		{
			const bool b1 = static_cast<bool>(else_expr);
			const bool b2 = static_cast<bool>(other.else_expr);

			if (b1 != b2)
			{
				return false;
			}

			if (b1)
			{
				return IsEqual(cond_expr, other.cond_expr)
					&& IsEqual(then_expr, other.then_expr) 
					&& IsEqual(else_expr.value(), other.else_expr.value());
			}

			return IsEqual(cond_expr, other.cond_expr)
				&& IsEqual(then_expr, other.then_expr);
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

		bool operator==(const For& other)const
		{
			return loopCounter == other.loopCounter
				&& IsEqual(rangeStart, other.rangeStart)
				&& IsEqual(rangeEnd, other.rangeEnd)
				&& IsEqual(doExpr, other.doExpr);
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

		bool operator==(const Return& other)const
		{
			return IsEqual(expr, other.expr);
		}
	};

	struct ListConstractor
	{
		std::vector<Expr> data;

		ListConstractor() = default;

		ListConstractor(const Expr& expr) :
			data({ expr })
		{}

		ListConstractor& add(const Expr& expr)
		{
			data.push_back(expr);
			return *this;
		}

		ListConstractor& operator()(const Expr& expr)
		{
			return add(expr);
		}

		static ListConstractor Make(const Expr& expr)
		{
			return ListConstractor(expr);
		}

		static void Append(ListConstractor& list, const Expr& expr)
		{
			list.data.push_back(expr);
		}

		bool operator==(const ListConstractor& other)const
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

	struct List
	{
		std::vector<Address> data;

		List() = default;

		/*
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
		*/

		List& append(const Address& address)
		{
			data.push_back(address);
			return *this;
		}

		List& concat(const List& tail)
		{
			data.insert(data.end(), tail.data.begin(), tail.data.end());
			return *this;
		}

		static List Concat(const List& a, const List& b)
		{
			List result(a);
			return result.concat(b);
		}

		/*std::vector<unsigned>::iterator get(int index)
		{
			return data.begin() + index;
		}*/
		Address get(int index)const
		{
			return data[index];
		}

		bool operator==(const List& other)const
		{
			if (data.size() != other.data.size())
			{
				return false;
			}

			for (size_t i = 0; i < data.size(); ++i)
			{
				//if (!IsEqual(data[i], other.data[i]))

				//TODO: アドレス比較ではなく値の比較にすべき(リストが環境の参照を持つ？)
				//if (data[i].valueID == other.data[i].valueID)
				if (data[i] == other.data[i])
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

		bool operator==(const KeyExpr& other)const
		{
			return name == other.name
				&& IsEqual(expr, other.expr);
		}
	};

	struct RecordConstractor
	{
		std::vector<Expr> exprs;

		RecordConstractor() = default;

		RecordConstractor& add(const Expr& expr)
		{
			exprs.push_back(expr);
			return *this;
		}

		static void AppendKeyExpr(RecordConstractor& rec, const KeyExpr& KeyExpr)
		{
			rec.exprs.push_back(KeyExpr);
		}

		static void AppendExpr(RecordConstractor& rec, const Expr& expr)
		{
			rec.exprs.push_back(expr);
		}

		bool operator==(const RecordConstractor& other)const
		{
			if (exprs.size() != other.exprs.size())
			{
				return false;
			}

			for (size_t i = 0; i < exprs.size(); ++i)
			{
				if (!IsEqual(exprs[i], other.exprs[i]))
				{
					return false;
				}
			}

			return true;
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

			return IsEqualEvaluated(value, other.value);
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

	struct SatFunctionReference
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
			//std::vector<boost::variant<SatReference, Evaluated>> args;
			std::vector<SatExpr> args;

			FunctionRef() = default;
			FunctionRef(const std::vector<SatExpr>& args) :args(args) {}

			bool operator==(const FunctionRef& other)const
			{
				if (args.size() != other.args.size())
				{
					return false;
				}

				for (size_t i = 0; i < args.size(); ++i)
				{
					/*if (!IsEqual(args[i], other.args[i]))
					{
						return false;
					}*/
				}

				return true;
			}

			std::string asString()const
			{
				return std::string("( ") + std::to_string(args.size()) + "args" + " )";
			}

			/*
			void appendRef(const SatReference& ref)
			{
				args.push_back(ref);
			}

			void appendValue(const Evaluated& value)
			{
				args.push_back(value);
			}
			*/
			
			void appendExpr(const SatExpr& value)
			{
				args.push_back(value);
			}
		};

		using Ref = boost::variant<ListRef, RecordRef, FunctionRef>;

		//using ObjectT = boost::variant<boost::recursive_wrapper<FuncVal>>;

		//ObjectT headValue;

		//std::string funcName;
		Address headAddress; //funcName -> headAddress に変更

		std::vector<Ref> references;

		SatFunctionReference() = default;

		SatFunctionReference(Address address)
			:headAddress(address)
		{}

		void appendListRef(int index)
		{
			references.push_back(ListRef(index));
		}

		void appendRecordRef(const std::string& key)
		{
			references.push_back(RecordRef(key));
		}

		//void appendFunctionRef(const std::vector<Evaluated>& args)
		void appendFunctionRef(const FunctionRef& ref)
		{
			//references.push_back(FunctionRef(args));
			references.push_back(ref);
		}

		bool operator==(const SatFunctionReference& other)const
		{
			//if (!(headValue == other.headValue))
			/*if (!(funcName == other.funcName))
			{
				return false;
			}*/
			if (!(headAddress == other.headAddress))
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
		//using Ref = boost::variant<Identifier, boost::recursive_wrapper<ObjectReference>>;
		//using Ref = ObjectReference;

		//std::vector<Ref> refs;

		std::vector<Accessor> accessors;

		DeclFree() = default;

		void add(const Accessor& accessor)
		{
			accessors.push_back(accessor);
		}

		static void AddAccessor(DeclFree& decl, const Accessor& accessor)
		{
			decl.accessors.push_back(accessor);
		}

		bool operator==(const DeclFree& other)const
		{
			/*if (refs.size() != other.refs.size())
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
			}*/

			std::cerr << "Warning: IsEqual<DeclFree>() don't care about accessors" << std::endl;
			//accessors;

			return true;
		}
	};

	struct Record
	{
		//std::unordered_map<std::string, Evaluated> values;
		std::unordered_map<std::string, Address> values;
		OptimizationProblemSat problem;
		std::vector<Accessor> freeVariables;
		//std::vector<Address> freeVariables;//var宣言で指定された変数のアドレス
		std::vector<Address> freeVariableRefs;//var宣言で指定された変数から辿れる全てのアドレス
		enum Type { Normal, Path, Text };
		Type type = Normal;
		std::vector<Eigen::Vector2d> pathPoints;

		Record() = default;
		
		/*
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
		*/

		Record(const std::string& name, const Address& address)
		{
			append(name, address);
		}

		Record& append(const std::string& name, const Address& address)
		{
			values[name] = address;
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

				//if (!IsEqualEvaluated(keyval.second, otherIt->second))
				if (keyval.second != otherIt->second)
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
		bool isArbitrary = false;

		ListAccess() = default;

		ListAccess(const Expr& index) :index(index) {}

		static void SetIndex(ListAccess& listAccess, const Expr& index)
		{
			listAccess.index = index;
		}

		static void SetIndexArbitrary(ListAccess& listAccess)
		{
			listAccess.isArbitrary = true;
		}
		
		bool operator==(const ListAccess& other)const
		{
			return index == other.index;
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

		bool operator==(const RecordAccess& other)const
		{
			return name == other.name;
		}
	};

	struct FunctionAccess
	{
		std::vector<Expr> actualArguments;

		FunctionAccess() = default;

		FunctionAccess& add(const Expr& argument)
		{
			actualArguments.push_back(argument);
			return *this;
		}

		static void Append(FunctionAccess& obj, const Expr& argument)
		{
			obj.add(argument);
		}

		bool operator==(const FunctionAccess& other)const
		{
			if (actualArguments.size() != other.actualArguments.size())
			{
				return false;
			}

			for (size_t i = 0; i < actualArguments.size(); ++i)
			{
				if (!IsEqual(actualArguments[i], other.actualArguments[i]))
				{
					return false;
				}
			}
			
			return true;
		}
	};

	using Access = boost::variant<ListAccess, RecordAccess, FunctionAccess>;

	struct Accessor
	{
		//先頭のオブジェクト(識別子かもしくは関数・リスト・レコードのリテラル)
		Expr head;

		std::vector<Access> accesses;

		Accessor() = default;

		Accessor(const Expr& head) :
			head(head)
		{}

		Accessor& add(const Access& access)
		{
			accesses.push_back(access);
			return *this;
		}

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

		//std::pair<FunctionCaller, std::vector<Access>> getFirstFunction(std::shared_ptr<Context> pEnv);

		bool hasFunctionCall()const
		{
			for (const auto& a : accesses)
			{
				if (IsType<FunctionAccess>(a))
				{
					return true;
				}
			}

			return false;
		}

		bool operator==(const Accessor& other)const
		{
			if (accesses.size() != other.accesses.size())
			{
				return false;
			}

			for (size_t i = 0; i < accesses.size(); ++i)
			{
				if (!(accesses[i] == other.accesses[i]))
				{
					return false;
				}
			}

			return IsEqual(head, other.head);
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

				return IsEqualEvaluated(lhs.value(), other.lhs.value());
			}
			else
			{
				return !other.lhs;
			}
		}
	};

	struct RecordInheritor
	{
		//using OriginalRecord = boost::variant<Identifier, boost::recursive_wrapper<Record>>;
		//OriginalRecord original;
		//OriginalRecordがRecordになるのはあり得なくない？
		//それよりも関数の返り値がレコードの場合もあるのでoriginalには一般の式を取るべきだと思う

		Expr original;
		//std::vector<Expr> exprs;
		RecordConstractor adder;

		RecordInheritor() = default;

		RecordInheritor(const Expr& original) :
			original(original)
		{}

		static RecordInheritor MakeIdentifier(const Identifier& original)
		{
			return RecordInheritor(original);
		}

		static RecordInheritor MakeAccessor(const Accessor& original)
		{
			return RecordInheritor(original);
		}

		static void AppendKeyExpr(RecordInheritor& rec, const KeyExpr& KeyExpr)
		{
			rec.adder.exprs.push_back(KeyExpr);
		}

		static void AppendExpr(RecordInheritor& rec, const Expr& expr)
		{
			rec.adder.exprs.push_back(expr);
		}

		static void AppendRecord(RecordInheritor& rec, const RecordConstractor& rec2)
		{
			auto& exprs = rec.adder.exprs;
			exprs.insert(exprs.end(), rec2.exprs.begin(), rec2.exprs.end());
		}

		static RecordInheritor MakeRecord(const Identifier& original, const RecordConstractor& rec2)
		{
			RecordInheritor obj(original);
			AppendRecord(obj, rec2);
			return obj;
		}

		bool operator==(const RecordInheritor& other)const
		{
			/*if (IsType<Identifier>(original) && IsType<Identifier>(other.original))
			{
				return As<Identifier>(original) == As<Identifier>(other.original) && adder == other.adder;
			}
			if (IsType<Record>(original) && IsType<Record>(other.original))
			{
				return As<const Record&>(original) == As<const Record&>(other.original) && adder == other.adder;
			}*/

			return IsEqual(original, other.original) && adder == other.adder;

			std::cerr << "Error(" << __LINE__ << ")\n";
			return false;
		}
	};

	Expr BuildString(const std::string& str);
	
}
