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
#include <unordered_set>
#include <unordered_map>

#include <boost/fusion/include/vector.hpp>

#include <boost/variant.hpp>
#include <boost/mpl/vector/vector30.hpp>
#include <boost/optional.hpp>

#include <Eigen/Core>

#include "Error.hpp"

namespace std
{
	template<class T> struct hash;
}

namespace cgl
{
	const double pi = 3.1415926535;
	const double deg2rad = pi / 180.0;
	const double rad2deg = 180.0 / pi;

	template<class T>
	using Vector = std::vector<T, Eigen::aligned_allocator<T>>;

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
		Minus,

		Dynamic
	};

	inline std::string UnaryOpToStr(UnaryOp op)
	{
		switch (op)
		{
		case UnaryOp::Not:     return "Not";
		case UnaryOp::Plus:    return "Plus";
		case UnaryOp::Minus:   return "Minus";
		case UnaryOp::Dynamic: return "Dynamic";
		}

		return "UnknownUnaryOp";
	}

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

		return "UnknownBinaryOp";
	}

	struct DefFunc;

	struct Identifier : public LocationInfo
	{
	public:
		Identifier() = default;

		explicit Identifier(const std::string& name_) :
			name(name_)
		{}

		Identifier& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		static Identifier MakeIdentifier(const std::u32string& name_);

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

		const std::string& toString()const
		{
			return name;
		}

		friend std::ostream& operator<<(std::ostream& os, const Identifier& node) { return os; }

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

	struct Reference
	{
	public:
		Reference() {}

		explicit Reference(unsigned referenceID) :
			referenceID(referenceID)
		{}

		bool operator==(const Reference other)const
		{
			return referenceID == other.referenceID;
		}

		bool operator!=(const Reference other)const
		{
			return referenceID != other.referenceID;
		}

		std::string toString()const
		{
			return std::to_string(referenceID);
		}

	private:
		friend struct std::hash<Reference>;

		unsigned referenceID;
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

	template <>
	struct hash<cgl::Reference>
	{
	public:
		size_t operator()(const cgl::Reference& reference)const
		{
			return hash<size_t>()(static_cast<size_t>(reference.referenceID));
		}
	};
}

namespace cgl
{
	class Context;

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
	struct PackedList;

	struct ListAccess;

	struct KeyExpr;
	struct RecordConstractor;
	struct RecordInheritor;

	struct Character;

	struct KeyValue;
	struct Record;
	struct PackedRecord;

	struct RecordAccess;
	struct FunctionAccess;

	struct Accessor;

	struct DeclSat;
	struct DeclFree;

	struct Jump;

	struct Import;

	/*struct SatReference
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
	};*/

	struct CharString
	{
	public:
		CharString() = default;

		explicit CharString(const std::u32string& str) :str(str) {}

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

		bool empty()const
		{
			return str.empty();
		}

	private:
		std::u32string str;
	};

	using Val = boost::variant<
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

	using PackedVal = boost::variant<
		bool,
		int,
		double,
		CharString,
		boost::recursive_wrapper<PackedList>,
		boost::recursive_wrapper<KeyValue>,
		boost::recursive_wrapper<PackedRecord>,
		boost::recursive_wrapper<FuncVal>,
		boost::recursive_wrapper<Jump>
	>;

	PackedVal Packed(const Val& value, const Context& context);
	Val Unpacked(const PackedVal& packedValue, Context& context);

	inline double IsNum(const Val& value)
	{
		return IsType<double>(value) || IsType<int>(value);
	}

	inline double IsNum(const PackedVal& value)
	{
		return IsType<double>(value) || IsType<int>(value);
	}

	inline double AsDouble(const Val& value)
	{
		if (IsType<double>(value))
			return As<double>(value);
		else if (IsType<int>(value))
			return static_cast<double>(As<int>(value));
		else {
			CGL_Error("不正な値です");
			return 0;
		}
	}

	inline double AsDouble(const PackedVal& value)
	{
		if (IsType<double>(value))
			return As<double>(value);
		else if (IsType<int>(value))
			return static_cast<double>(As<int>(value));
		else {
			CGL_Error("不正な値です");
			return 0;
		}
	}

	struct RValue;
	struct LRValue;

	using Expr = boost::variant<
		boost::recursive_wrapper<LRValue>,
		Identifier,

		boost::recursive_wrapper<Import>,

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

	bool IsEqualVal(const Val& value1, const Val& value2);
	bool IsEqual(const Expr& value1, const Expr& value2);

	void printExpr(const Expr& expr);

	struct RValue
	{
		RValue() = default;
		explicit RValue(const Val& value) :value(value) {}

		bool operator==(const RValue& other)const
		{
			return IsEqualVal(value, other.value);
		}

		bool operator!=(const RValue& other)const
		{
			return !IsEqualVal(value, other.value);
		}

		Val value;
	};

	struct LRValue : public LocationInfo
	{
		LRValue() = default;

		LRValue(const Val& value) :value(RValue(value)) {}
		LRValue(const RValue& value) :value(value) {}
		LRValue(Address value) :value(value) {}
		LRValue(Reference value) :value(value) {}

		static LRValue Bool(bool a) { return LRValue(Val(a)); }
		static LRValue Int(int a) { return LRValue(Val(a)); }
		static LRValue Double(double a) { return LRValue(Val(a)); }
		//static LRValue Float(const std::string& str) { return LRValue(std::stod(str)); }
		static LRValue Float(const std::u32string& str);
		//static LRValue Sat(const DeclSat& a) { return LRValue(a); }
		//static LRValue Free(const DeclFree& a) { return LRValue(a); }

		LRValue& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		bool isRValue()const
		{
			return IsType<RValue>(value);
		}

		bool isLValue()const
		{
			return !isRValue();
		}

		bool isAddress()const
		{
			return IsType<Address>(value);
		}

		bool isReference()const
		{
			return IsType<Reference>(value);
		}

		bool isValid()const;

		std::string toString()const;

		Address address(const Context& env)const;

		Reference reference()const
		{
			return As<Reference>(value);
		}

		const Val& evaluated()const
		{
			return As<RValue>(value).value;
		}

		Val& mutableVal()
		{
			return As<RValue>(value).value;
		}

		bool operator==(const LRValue& other)const
		{
			/*if (isLValue() && other.isLValue())
			{
			return address() == other.address();
			}
			else if (isRValue() && other.isRValue())
			{
			return IsEqualVal(evaluated(), other.evaluated());
			}*/

			if (isRValue() && other.isRValue())
			{
				return IsEqualVal(evaluated(), other.evaluated());
			}

			return false;
		}

		friend std::ostream& operator<<(std::ostream& os, const LRValue& node)
		{
			return os;
		}

		boost::variant<boost::recursive_wrapper<RValue>, Address, Reference> value;
	};

	struct OptimizationProblemSat;

	struct SatUnaryExpr;
	struct SatBinaryExpr;
	struct SatLines;

	struct SatFunctionReference;

	using SatExpr = boost::variant<
		double,
		boost::recursive_wrapper<SatUnaryExpr>,
		boost::recursive_wrapper<SatBinaryExpr>,
		boost::recursive_wrapper<SatFunctionReference>
	>;

	struct VariableRange
	{
		double minimum = -100.0;
		double maximum = +100.0;
		VariableRange() = default;
		VariableRange(double minimum, double maximum) :
			minimum(minimum),
			maximum(maximum)
		{}
	};

	struct Import : public LocationInfo
	{
	public:
		Import() = default;

		explicit Import(const std::u32string& filePath);

		Import(const std::u32string& file, const Identifier& name);

		Import& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		LRValue eval(std::shared_ptr<Context> pContext)const;

		static Import Make(const std::u32string& filePath)
		{
			return Import(filePath);
		}

		static void SetName(Import& node, const Identifier& name);

		bool operator==(const Import& other)const
		{
			return false;
		}

		const std::string& getImportPath()const
		{
			return importPath;
		}

		const std::string& getImportName()const
		{
			return importName;
		}

		size_t getSeed()const
		{
			return seed;
		}

		friend std::ostream& operator<<(std::ostream& os, const Import& node) { return os; }

	private:
		void updateHash();

		std::string importPath;
		std::string importName;
		size_t seed;
	};

	struct OptimizationProblemSat
	{
	public:
		//boost::optional<Expr> candidateExpr;
		boost::optional<Expr> expr;

		//制約式に含まれる全ての参照にIDを振る(=参照ID)
		//参照IDはdouble型の値に紐付けられる

		std::vector<double> data;//参照ID->data
		std::vector<Address> refs;//参照ID->Address

		std::unordered_map<Address, int> invRefs;//Address->参照ID

												 //freeVariablesから辿れる全てのアドレス
		std::vector<std::pair<Address, VariableRange>> freeVariableRefs;//変数ID->Address

		bool hasPlateausFunction = false;

		void addUnitConstraint(const Expr& logicExpr);
		//void constructConstraint(std::shared_ptr<Context> pEnv, std::vector<std::pair<Address, VariableRange>>& freeVariables);

		std::vector<double> solve(std::shared_ptr<Context> pEnv, const LocationInfo& info, const Record currentRecord, const std::vector<Identifier>& currentKeyList);

	private:
		void constructConstraint(std::shared_ptr<Context> pEnv);

		bool initializeData(std::shared_ptr<Context> pEnv);

		void update(int index, double x)
		{
			data[index] = x;
		}

		double eval(std::shared_ptr<Context> pEnv, const LocationInfo& info);
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

	struct UnaryExpr : public LocationInfo
	{
		Expr lhs;
		UnaryOp op;

		UnaryExpr(const Expr& lhs, UnaryOp op) :
			lhs(lhs), op(op)
		{}

		UnaryExpr& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		bool operator==(const UnaryExpr& other)const
		{
			if (op != other.op)
			{
				return false;
			}

			return IsEqual(lhs, other.lhs);
		}

		friend std::ostream& operator<<(std::ostream& os, const UnaryExpr& node) { return os; }
	};

	struct BinaryExpr : public LocationInfo
	{
		Expr lhs;
		Expr rhs;
		BinaryOp op;

		BinaryExpr() = default;

		BinaryExpr(const Expr& lhs, const Expr& rhs, BinaryOp op) :
			lhs(lhs), rhs(rhs), op(op)
		{}

		static BinaryExpr Make(const Expr& lhs, const Expr& rhs, BinaryOp op)
		{
			return BinaryExpr(lhs, rhs, op);
		}

		BinaryExpr& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		bool operator==(const BinaryExpr& other)const
		{
			if (op != other.op)
			{
				return false;
			}

			return IsEqual(lhs, other.lhs) && IsEqual(rhs, other.rhs);
		}

		friend std::ostream& operator<<(std::ostream& os, const BinaryExpr& node) { return os; }
	};

	struct Range : public LocationInfo
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

		Range& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const Range& node) { return os; }
	};

	struct Lines : public LocationInfo
	{
		std::vector<Expr> exprs;

		Lines() = default;

		Lines(const Expr& expr) :
			exprs({ expr })
		{}

		Lines(const std::vector<Expr>& exprs_) :
			exprs(exprs_)
		{}

		Lines& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const Lines& node) { return os; }
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

	struct DefFunc : public LocationInfo
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

		DefFunc& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
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

		friend std::ostream& operator<<(std::ostream& os, const DefFunc& node) { return os; }
	};

	struct If : public LocationInfo
	{
		Expr cond_expr;
		Expr then_expr;
		boost::optional<Expr> else_expr;

		If() = default;

		If(const Expr& cond) :
			cond_expr(cond)
		{}

		If& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const If& node) { return os; }
	};

	struct For : public LocationInfo
	{
		Identifier loopCounter;
		Expr rangeStart;
		Expr rangeEnd;
		Expr doExpr;

		bool asList = false;

		For() = default;

		For(const Identifier& loopCounter, const Expr& rangeStart, const Expr& rangeEnd, const Expr& doExpr) :
			loopCounter(loopCounter),
			rangeStart(rangeStart),
			rangeEnd(rangeEnd),
			doExpr(doExpr)
		{}

		For& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		static void SetDo(For& forExpression, const Expr& doExpr, bool isList)
		{
			forExpression.doExpr = doExpr;

			forExpression.asList = isList;
		}

		bool operator==(const For& other)const
		{
			return loopCounter == other.loopCounter
				&& IsEqual(rangeStart, other.rangeStart)
				&& IsEqual(rangeEnd, other.rangeEnd)
				&& IsEqual(doExpr, other.doExpr);
		}

		friend std::ostream& operator<<(std::ostream& os, const For& node) { return os; }
	};

	struct Return : public LocationInfo
	{
		Expr expr;

		Return() = default;

		Return(const Expr& expr) :
			expr(expr)
		{}

		Return& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		static Return Make(const Expr& expr)
		{
			return Return(expr);
		}

		bool operator==(const Return& other)const
		{
			return IsEqual(expr, other.expr);
		}

		friend std::ostream& operator<<(std::ostream& os, const Return& node) { return os; }
	};

	struct ListConstractor : public LocationInfo
	{
		std::vector<Expr> data;

		ListConstractor() = default;

		ListConstractor(const Expr& expr) :
			data({ expr })
		{}

		ListConstractor& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const ListConstractor& node) { return os; }
	};

	struct PackedList
	{
		struct Data
		{
			PackedVal value;
			Address address;
			Data() = default;
			Data(const PackedVal& value, const Address address) :
				value(value),
				address(address)
			{}
		};

		std::vector<Data> data;
		PackedList() = default;

		void add(const Address address, const PackedVal& value)
		{
			data.emplace_back(value, address);
		}

		void add(const PackedVal& value)
		{
			data.emplace_back(value, Address());
		}

		template <typename... Args>
		PackedList& adds(const Args&... args)
		{
			addsImpl(args...);
			return *this;
		}

		Val unpacked(Context& context)const;

	private:

		template<typename Head>
		void addsImpl(const Head& head)
		{
			add(head);
		}

		template<typename Head, typename... Tail>
		void addsImpl(const Head& head, const Tail&... tail)
		{
			add(head);
			addsImpl(tail...);
		}
	};

	template <typename... Args>
	static PackedList MakeList(const Args&... args)
	{
		PackedList instance;
		instance.adds(args...);
		return instance;
	}

	struct List
	{
		std::vector<Address> data;

		List() = default;
		explicit List(const std::vector<Address>& data) :
			data(data)
		{}

		void add(const Address address)
		{
			data.push_back(address);
		}

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

		PackedVal packed(const Context& context)const;
	};

	struct KeyExpr : public LocationInfo
	{
		Identifier name;
		Expr expr;

		KeyExpr() = default;

		KeyExpr(const Identifier& name) :
			name(name)
		{}

		KeyExpr(const Identifier& name, const Expr& expr) :
			name(name), expr(expr)
		{}

		KeyExpr& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		static KeyExpr Make(const Identifier& name)
		{
			return KeyExpr(name);
		}

		/*static KeyExpr Make2(const Identifier& name, const Expr& expr)
		{
			return KeyExpr(name, expr);
		}*/

		static void SetExpr(KeyExpr& keyval, const Expr& expr)
		{
			keyval.expr = expr;
		}

		bool operator==(const KeyExpr& other)const
		{
			return name == other.name
				&& IsEqual(expr, other.expr);
		}

		friend std::ostream& operator<<(std::ostream& os, const KeyExpr& node) { return os; }
	};

	struct RecordConstractor : public LocationInfo
	{
		std::vector<Expr> exprs;

		RecordConstractor() = default;

		RecordConstractor& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const RecordConstractor& node) { return os; }
	};

	struct KeyValue
	{
		Identifier name;
		Val value;

		KeyValue() = default;

		KeyValue(const Identifier& name, const Val& value) :
			name(name),
			value(value)
		{}

		bool operator==(const KeyValue& other)const
		{
			if (name != other.name)
			{
				return false;
			}

			return IsEqualVal(value, other.value);
		}
	};

	struct DeclSat : public LocationInfo
	{
		Expr expr;

		DeclSat() = default;

		DeclSat(const Expr& expr) :
			expr(expr)
		{}

		DeclSat& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		static DeclSat Make(const Expr& expr)
		{
			return DeclSat(expr);
		}

		bool operator==(const DeclSat& other)const
		{
			std::cerr << "Warning: IsEqual<DeclSat>() don't care about expr" << std::endl;
			return true;
		}

		friend std::ostream& operator<<(std::ostream& os, const DeclSat& node) { return os; }
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

		//void appendFunctionRef(const std::vector<Val>& args)
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

	struct DeclFree : public LocationInfo
	{
		std::vector<Accessor> accessors;
		std::vector<Expr> ranges;

		DeclFree() = default;

		DeclFree& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		void addAccessor(const Accessor& accessor)
		{
			accessors.push_back(accessor);
		}

		void addRange(const Expr& range)
		{
			ranges.push_back(range);
		}

		static void AddAccessor(DeclFree& decl, const Accessor& accessor)
		{
			decl.accessors.push_back(accessor);
		}

		static void AddRange(DeclFree& decl, const Expr& expr)
		{
			decl.ranges.push_back(expr);
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

		friend std::ostream& operator<<(std::ostream& os, const DeclFree& node) { return os; }
	};

	enum class RecordType { Normal, Path, Text, ShapePath };

	using FreeVariable = std::pair<Address, VariableRange>;

	//unitConstraintに出現するfree変数のAddress
	using ConstraintAppearance = std::unordered_set<Address>;

	//同じfree変数への依存性を持つ制約IDの組
	using ConstraintGroup = std::unordered_set<size_t>;

	//レコード継承時に用いるデータ
	struct OldRecordData
	{
		OldRecordData() = default;

		//変数ID->アドレス
		std::vector<FreeVariable> freeVariableAddresses;

		//分解された単位制約
		std::vector<Expr> unitConstraints;

		//単位制約ごとの依存するfree変数の集合
		std::vector<ConstraintAppearance> variableAppearances;

		//同じfree変数への依存性を持つ単位制約の組
		std::vector<OptimizationProblemSat> groupConstraints;

		std::vector<ConstraintGroup> constraintGroups;
	};

	struct PackedRecord
	{
		struct Data
		{
			PackedVal value;
			Address address;
			Data() = default;
			Data(const PackedVal& value, const Address address) :
				value(value),
				address(address)
			{}
		};
		std::unordered_map<std::string, Data> values;

		std::vector<OptimizationProblemSat> problems;
		boost::optional<Expr> constraint;

		//var宣言で指定されたアクセッサ
		std::vector<Accessor> freeVariables;

		//var宣言で指定されたアクセッサの範囲
		std::vector<Expr> freeRanges;

		OldRecordData original;

		RecordType type = RecordType::Normal;
		bool isSatisfied = true;
		Vector<Eigen::Vector2d> pathPoints;

		PackedRecord() = default;

		void add(const std::string& key, const Address address, const PackedVal& value)
		{
			values.insert({ key, Data(value, address) });
		}

		void add(const std::string& key, const PackedVal& value)
		{
			values.insert({ key, Data(value, Address()) });
		}

		template <typename... Args>
		PackedRecord& adds(const Args&... args)
		{
			addsImpl(args...);
			return *this;
		}

		Val unpacked(Context& context)const;

	private:

		template<typename HeadName, typename HeadVal>
		void addsImpl(const HeadName& headName, const HeadVal& headVal)
		{
			add(headName, headVal);
		}

		template<typename HeadName, typename HeadVal, typename... Tail>
		void addsImpl(const HeadName& headName, const HeadVal& headVal, const Tail&... tail)
		{
			add(headName, headVal);
			addsImpl(tail...);
		}
	};

	template <typename... Args>
	static PackedRecord MakeRecord(const Args&... args)
	{
		PackedRecord instance;
		instance.adds(args...);
		return instance;
	}

	struct Record
	{
		std::unordered_map<std::string, Address> values;

		std::vector<OptimizationProblemSat> problems;
		boost::optional<Expr> constraint;

		//var宣言で指定されたアクセッサ
		std::vector<Accessor> freeVariables;
		//std::vector<std::pair<Address, VariableRange>> freeVariableRefs;//freeVariablesから辿れる全てのアドレス

		//var宣言で指定されたアクセッサの範囲
		std::vector<Expr> freeRanges;

		OldRecordData original;

		RecordType type = RecordType::Normal;
		bool isSatisfied = true;
		Vector<Eigen::Vector2d> pathPoints;

		Record() = default;
		explicit Record(const std::unordered_map<std::string, Address>& values) :
			values(values)
		{}

		Record(const std::string& name, const Address& address)
		{
			append(name, address);
		}

		void add(const std::string& key, const Address address)
		{
			values.insert({ key, address });
		}

		Record& append(const std::string& name, const Address& address)
		{
			values[name] = address;
			return *this;
		}

		void addConstraint(const Expr& logicExpr)
		{
			if (constraint)
			{
				constraint = BinaryExpr(constraint.value(), logicExpr, BinaryOp::And);
			}
			else
			{
				constraint = logicExpr;
			}
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

				//if (!IsEqualVal(keyval.second, otherIt->second))
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

		PackedVal packed(const Context& context)const;
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

		friend std::ostream& operator<<(std::ostream& os, const ListAccess& node) { return os; }
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

		friend std::ostream& operator<<(std::ostream& os, const RecordAccess& node) { return os; }
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

		friend std::ostream& operator<<(std::ostream& os, const FunctionAccess& node) { return os; }
	};

	using Access = boost::variant<ListAccess, RecordAccess, FunctionAccess>;

	struct Accessor : public LocationInfo
	{
		//先頭のオブジェクト(識別子かもしくは関数・リスト・レコードのリテラル)
		Expr head;

		std::vector<Access> accesses;

		Accessor() = default;

		Accessor(const Expr& head) :
			head(head)
		{}

		Accessor& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const Accessor& node) { return os; }
	};

	struct DeepReference
	{
		DeepReference() = default;

		using Key = boost::variant<int, std::string>;

		Address head;
		std::vector<std::pair<Key, Address>> tail;

		DeepReference& addList(int index, Address address)
		{
			tail.emplace_back(index, address);
			return *this;
		}

		DeepReference& addRecord(const std::string& name, Address address)
		{
			tail.emplace_back(name, address);
			return *this;
		}

		DeepReference& add(const Key& key, Address address)
		{
			tail.emplace_back(key, address);
			return *this;
		}
	};

	struct Jump
	{
		enum Op { Return, Break, Continue };

		boost::optional<Val> lhs;
		Op op;

		Jump() = default;

		Jump(Op op) :op(op) {}

		Jump(Op op, const Val& lhs) :
			op(op), lhs(lhs)
		{}

		static Jump MakeReturn(const Val& value)
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

				return IsEqualVal(lhs.value(), other.lhs.value());
			}
			else
			{
				return !other.lhs;
			}
		}
	};

	struct RecordInheritor : public LocationInfo
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

		RecordInheritor& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

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

		friend std::ostream& operator<<(std::ostream& os, const RecordInheritor& node) { return os; }
	};

	Expr BuildString(const std::u32string& str);

}
