#pragma once
#pragma warning(disable:4996)

#define CGL_ENABLE_CURRYING

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cmath>
#include <cfloat>
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
#include <type_traits>

#include <boost/version.hpp>
#define CGL_BOOST_MAJOR_VERSION (BOOST_VERSION / 100000)
#define CGL_BOOST_MINOR_VERSION (BOOST_VERSION / 100 % 1000)

#include <boost/fusion/include/vector.hpp>
#include <boost/variant.hpp>
#include <boost/mpl/vector/vector30.hpp>
#include <boost/optional.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <boost/functional/hash.hpp>

#include <cereal/access.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/array.hpp>
#include <cereal/types/valarray.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/deque.hpp>
#include <cereal/types/forward_list.hpp>
#include <cereal/types/list.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/map.hpp>
#include <cereal/types/queue.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/stack.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/unordered_set.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/tuple.hpp>
#include <cereal/types/bitset.hpp>
#include <cereal/types/complex.hpp>
#include <cereal/types/chrono.hpp>
#include <cereal/types/polymorphic.hpp>

#include <cereal/types/boost_variant.hpp>
#include <cereal/types/boost_optional.hpp>

#include <cereal/archives/binary.hpp>
#include <cereal/archives/portable_binary.hpp>
#include <cereal/archives/xml.hpp>
#include <cereal/archives/json.hpp>

#include <Eigen/Core>

#include "Error.hpp"
#include "Util.hpp"

namespace cgl
{
	template<class T>
	using Vector = std::vector<T, Eigen::aligned_allocator<T>>;

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

#if (CGL_BOOST_MAJOR_VERSION == 1) && (CGL_BOOST_MINOR_VERSION <= 57)
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
#else
	template<class T1, class T2>
	inline boost::optional<T1&> AsOpt(T2& t2)
	{
		if (IsType<T1>(t2))
		{
			return boost::relaxed_get<T1>(t2);
		}
		return boost::none;
	}

	template<class T1, class T2>
	inline boost::optional<const T1&> AsOpt(const T2& t2)
	{
		if (IsType<T1>(t2))
		{
			return boost::relaxed_get<T1>(t2);
		}
		return boost::none;
	}

	template<class T1, class T2>
	inline T1& As(T2& t2)
	{
		return boost::relaxed_get<T1>(t2);
	}

	template<class T1, class T2>
	inline const T1& As(const T2& t2)
	{
		return boost::relaxed_get<T1>(t2);
	}
#endif

	//https://stackoverflow.com/questions/37626566/get-boostvariants-type-index-with-boostmpl
	template <typename T, typename ... Ts>
	struct type_index;

	template <typename T, typename ... Ts>
	struct type_index<T, T, Ts ...>
		: std::integral_constant<std::size_t, 0>
	{};

	template <typename T, typename U, typename ... Ts>
	struct type_index<T, U, Ts ...>
		: std::integral_constant<std::size_t, 1 + type_index<T, Ts...>::value>
	{};

	template <typename T, typename ... Ts>
	struct variant_first_same_type_idx;

	template <typename T, typename Head, typename ... Tail>
	struct variant_first_same_type_idx<T, boost::variant<Head, Tail ... >>
		: type_index<T, Head, Tail ...>
	{};

	enum UnaryOp
	{
		Not,

		Plus,
		Minus,

		Dynamic
	};

	enum BinaryOp
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

		Concat,

		SetDiff
	};

	std::string UnaryOpToStr(UnaryOp op);
	std::string BinaryOpToStr(BinaryOp op);

	using ScopeIndex = unsigned;
	using ScopeAddress = std::deque<ScopeIndex>;

	struct Identifier : public LocationInfo
	{
	public:
		Identifier() = default;

		explicit Identifier(const std::string& name) :
			name(name)
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

		bool isDeferredCall()const;
		Identifier asDeferredCall(const ScopeAddress& identifierScopeInfo)const;

		bool isMakeClosure()const;
		Identifier asMakeClosure(const ScopeAddress& identifierScopeInfo)const;
		
		//rawIdentifierの形式：再帰深度の情報は除去しないため##の付いた形で返る
		std::pair<ScopeAddress, Identifier> decomposed()const;

		/*friend std::ostream& operator<<(std::ostream& os, const Identifier& node)
		{
			return os;
		}*/

	//private:
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

		bool operator<(const Address other)const
		{
			return valueID < other.valueID;
		}

		bool operator<=(const Address other)const
		{
			return valueID <= other.valueID;
		}

		static Address Null()
		{
			return Address();
		}

		std::string toString()const
		{
			return std::to_string(valueID);
		}

	//private:
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

	//private:
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

	template <>
	struct hash<cgl::Identifier>
	{
	public:
		size_t operator()(const cgl::Identifier& identifier)const
		{
			return hash<std::string>()(static_cast<std::string>(identifier));
		}
	};
}

namespace cgl
{
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

	//private:
		std::u32string str;
	};

	struct KeyValue;
	struct Jump;
	struct FuncVal;
	struct Record;
	struct List;
	using Val = boost::variant<
		CharString,
		int,
		bool,
		double,
		boost::recursive_wrapper<KeyValue>,
		boost::recursive_wrapper<List>,
		boost::recursive_wrapper<Record>,
		boost::recursive_wrapper<FuncVal>,
		boost::recursive_wrapper<Jump>
	>;

	struct PackedList;
	struct PackedRecord;
	using PackedVal = boost::variant<
		CharString,
		int,
		bool,
		double,
		boost::recursive_wrapper<KeyValue>,
		boost::recursive_wrapper<PackedList>,
		boost::recursive_wrapper<PackedRecord>,
		boost::recursive_wrapper<FuncVal>,
		boost::recursive_wrapper<Jump>
	>;

	class Context;
	size_t GetHash(const Val& val, const Context& context);
	size_t GetHash(const PackedVal& val);
	
	PackedVal Packed(const Val& value, const Context& context);
	Val Unpacked(const PackedVal& packedValue, Context& context);

	inline bool IsNum(const Val& value)
	{
		return IsType<double>(value) || IsType<int>(value);
	}

	inline bool IsNum(const PackedVal& value)
	{
		return IsType<double>(value) || IsType<int>(value);
	}

	bool IsVec2(const Val& value);

	bool IsShape(const Val& value);

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

	Eigen::Vector2d AsVec2(const Val& value, const Context& context);

	struct LRValue;
	struct Import;
	struct UnaryExpr;
	struct BinaryExpr;
	struct DefFunc;
	struct Range;
	struct Lines;
	struct If;
	struct For;
	struct Return;
	struct ListConstractor;
	struct KeyExpr;
	struct RecordConstractor;
	struct DeclSat;
	struct DeclFree;
	struct Accessor;
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
		boost::recursive_wrapper<DeclSat>,
		boost::recursive_wrapper<DeclFree>,

		boost::recursive_wrapper<Accessor>
	>;

	template<typename T>
	inline constexpr int ExprIndex()
	{
		return variant_first_same_type_idx<T, Expr>::value;
	}

	bool IsEqualVal(const Val& value1, const Val& value2);
	bool IsEqual(const Expr& value1, const Expr& value2);

	void printExpr(const Expr& expr);

	struct EitherReference
	{
		//std::shared_ptr<Expr> local;
		boost::optional<Identifier> local;
		Address replaced;

		EitherReference() = default;

		/*EitherReference(const Expr& original)
			: local(std::make_shared<Expr>(original))
		{}

		EitherReference(const Expr& original, Address address)
			: local(std::make_shared<Expr>(original))
			, replaced(address)
		{}

		EitherReference(std::shared_ptr<Expr> original, Address address)
			: local(original)
			, replaced(address)
		{}*/
		explicit EitherReference(const boost::optional<Identifier>& original)
			: local(original)
		{}

		EitherReference(const boost::optional<Identifier>& original, Address address)
			: local(original)
			, replaced(address)
		{}

		bool localReferenciable(const Context& context)const;

		std::string toString()const;
	};

	struct RValue
	{
		RValue() = default;
		explicit RValue(const Val& value) :value(value) {}

		bool operator==(const RValue& other)const { return IsEqualVal(value, other.value); }
		bool operator!=(const RValue& other)const { return !IsEqualVal(value, other.value); }

		Val value;
	};

	struct LRValue : public LocationInfo
	{
		LRValue() = default;

		LRValue(const RValue& value) :value(value) {}
		explicit LRValue(const Val& value) :value(RValue(value)) {}
		explicit LRValue(Address value) :value(value) {}
		explicit LRValue(Reference value) :value(value) {}
		explicit LRValue(const EitherReference& value) :value(value) {}

		static LRValue Bool(bool a);
		static LRValue Int(int a);
		static LRValue Double(double a) { return LRValue(Val(a)); }
		static LRValue Float(const std::u32string& str);

		LRValue& setLocation(const LocationInfo& info);

		bool isRValue()const { return IsType<RValue>(value); }
		bool isLValue()const { return !isRValue(); }

		bool isAddress()const { return IsType<Address>(value); }
		bool isReference()const { return IsType<Reference>(value); }
		bool isEitherReference()const { return IsType<EitherReference>(value); }

		bool isValid()const;

		std::string toString()const;
		std::string toString(Context& context)const;

		Reference reference()const { return As<Reference>(value); }
		const EitherReference& eitherReference()const
		{
			const auto& result = As<EitherReference>(value);
			return result;
		}

		const Val& evaluated()const { return As<RValue>(value).value; }
		Val& mutableVal() { return As<RValue>(value).value; }

		template<class T>
		void push_back(T& data, Context& context)const
		{
			if (isEitherReference())
			{
				data.push_back(eitherReference().replaced);
			}
			else if (isReference())
			{
				data.push_back(getReference(context));
			}
			else if (isAddress())
			{
				data.push_back(address(context));
			}
			else
			{
				data.push_back(makeTemporaryValue(context));
			}
		}

		boost::optional<Address> deref(const Context& env)const;

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

		boost::variant<boost::recursive_wrapper<RValue>, Address, Reference, EitherReference> value;

		private:
			friend class Context;
			friend class ExprAddressCheker;
			friend class AddressReplacer;

			Address address(const Context& context)const;

			Address getReference(const Context& context)const;

			Address makeTemporaryValue(Context& context)const;
	};

	struct Interval
	{
		double minimum = -DBL_MAX;
		double maximum = +DBL_MAX;
		Interval() = default;
		Interval(double minimum, double maximum) :
			minimum(minimum),
			maximum(maximum)
		{}
	};

	struct RegionVariable
	{
		Address address;

		enum Attribute {
			Position = 1 << 0,
			Scale = 1 << 1,
			Angle = 1 << 2,
			Other = 1 << 3
			//Position
			//Scale
			//Angle
			//Other
			//X
			//Y
			//Shape
		};
		std::set<Attribute> attributes;

		RegionVariable() = default;
		RegionVariable(Address address, Attribute attribute) :
			address(address),
			attributes({ attribute })
		{}

		bool has(Attribute attribute)const
		{
			return attributes.find(attribute) != attributes.end();
		}

		std::string toString()const
		{
			std::string result = "Address(" + address.toString() + "): ";
			if (!attributes.empty())
			{
				switch (*attributes.begin())
				{
				case cgl::RegionVariable::Position:
					result += "Position";
					break;
				case cgl::RegionVariable::Scale:
					result += "Scale";
					break;
				case cgl::RegionVariable::Angle:
					result += "Angle";
					break;
				case cgl::RegionVariable::Other:
					result += "Other";
					break;
				default:
					result += "unknown";
					break;
				}
				return result;
			}
			return result + "undefined";
		}
	};

	struct OptimizeRegion
	{
		boost::variant<Interval, PackedVal> region;
		boost::optional<double> eps;

		//一つのregionはvariableの集合に対してかかる
		//したがって、variableのリストのうちどこからどこまでに対応するかというインデックス情報を保存しておく
		//->制約が分解されたときにインデックスがずれるのでこれではだめだった。普通にアドレスの集合を持つ
		//->また、freeVariablesに対応した順序でAddressが取り出せないといけないので、setではなくvectorで管理する
		std::vector<Address> addresses;

		bool has(Address address)const
		{
			return std::find(addresses.begin(), addresses.end(), address) != addresses.end();
		}
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

	//private:
		void updateHash();

		std::string importPath;
		std::string importName;
		size_t seed;
	};

	struct OptimizationProblemSat
	{
	public:
		boost::optional<Expr> expr;

		//制約式に含まれる全ての参照にIDを振る(=参照ID)
		//参照IDはdouble型の値に紐付けられる

		std::vector<double> data;//参照ID->data
		std::vector<Address> refs;//参照ID->Address

		std::unordered_map<Address, int> invRefs;//Address->参照ID

		//freeVariablesから辿れる全てのアドレス
		std::vector<RegionVariable> freeVariableRefs;//変数ID->Address
		std::vector<OptimizeRegion> optimizeRegions;

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

	struct UnaryExpr : public LocationInfo
	{
		Expr lhs;
		UnaryOp op;

		UnaryExpr() = default;

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

	// 遅延呼び出しされる識別子の管理に必要
	struct DefFuncWithScopeInfo
	{
		DefFuncWithScopeInfo() = default;
		DefFuncWithScopeInfo(const ScopeAddress& scopeInfo)
			: scopeInfo(scopeInfo)
		{}
		DefFuncWithScopeInfo(const DefFunc& generalDef, const ScopeAddress& scopeInfo)
			: generalDef(generalDef)
			, scopeInfo(scopeInfo)
		{}
		DefFuncWithScopeInfo(const DefFunc& specialDef, int recDepth, const ScopeAddress& scopeInfo)
			: specialDefs({ {recDepth, specialDef} })
			, scopeInfo(scopeInfo)
		{}

		void setGeneralDef(const DefFunc& defFunc)
		{
			generalDef = defFunc;
		}
		void setSpecialDef(const DefFunc& defFunc, int recDepth)
		{
			specialDefs[recDepth] = defFunc;
		}

		// GC用
		void collectReferenceableAddresses(std::unordered_set<Address>& addresses)const
		{
			for (const auto& keyval : memo)
			{
				addresses.emplace(keyval.second);
			}
		}

		DefFunc generalDef;
		std::unordered_map<int, DefFunc> specialDefs;
		ScopeAddress scopeInfo;

	private:
		friend class Eval;
		int callDepth = 0;
		std::unordered_map<size_t, Address> memo;
	};
	using DeferredIdentifiers = std::unordered_map<Identifier, std::vector<DefFuncWithScopeInfo>>;

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
					&& IsEqual(else_expr.get(), other.else_expr.get());
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

		template <typename... Args>
		ListConstractor& adds(const Args&... args)
		{
			addsImpl(args...);
			return *this;
		}

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
	static Expr MakeListConstractor(const Args&... args)
	{
		ListConstractor instance;
		instance.adds(args...);
		Expr result = instance;
		return result;
	}

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

		List& push_back(const Address& address)
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

		template <typename... Args>
		RecordConstractor& adds(const Args&... args)
		{
			addsImpl(args...);
			return *this;
		}

	private:

		template<typename HeadName, typename HeadVal>
		void addsImpl(const HeadName& headName, const HeadVal& headVal)
		{
			add(KeyExpr(headName, headVal));
		}

		template<typename HeadName, typename HeadVal, typename... Tail>
		void addsImpl(const HeadName& headName, const HeadVal& headVal, const Tail&... tail)
		{
			add(KeyExpr(headName, headVal));
			addsImpl(tail...);
		}
	};

	template <typename... Args>
	static Expr MakeRecordConstructor(const Args&... args)
	{
		RecordConstractor instance;
		instance.adds(args...);
		Expr result = instance;
		return result;
	}

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

	struct PackedKeyValue
	{
		Identifier name;
		PackedVal value;
		Address address;

		PackedKeyValue() = default;

		PackedKeyValue(const Identifier& name, const PackedVal& value) :
			name(name),
			value(value)
		{}
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

	struct DeclFree : public LocationInfo
	{
		struct DeclVar
		{
			Expr accessor;
			boost::optional<Expr> range;

			DeclVar() = default;
			DeclVar(const Expr& accessor) :accessor(accessor) {}
		};
		std::vector<DeclVar> accessors;

		DeclFree() = default;

		DeclFree& setLocation(const LocationInfo& info)
		{
			locInfo_lineBegin = info.locInfo_lineBegin;
			locInfo_lineEnd = info.locInfo_lineEnd;
			locInfo_posBegin = info.locInfo_posBegin;
			locInfo_posEnd = info.locInfo_posEnd;
			return *this;
		}

		void addAccessor(const Expr& accessor)
		{
			accessors.emplace_back(accessor);
		}

		void addRange(const Expr& range)
		{
			accessors.back().range = range;
		}

		static void AddAccessor(DeclFree& decl, const Accessor& accessor)
		{
			decl.accessors.emplace_back(accessor);
		}

		static void AddAccessorDynamic(DeclFree& decl, const Accessor& accessor)
		{
			decl.accessors.push_back(DeclVar(UnaryExpr(accessor, UnaryOp::Dynamic)));
		}

		static void AddRange(DeclFree& decl, const Expr& expr)
		{
			decl.accessors.back().range = expr;
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

	//enum class RecordType { Normal, Path, Text, ShapePath };
	enum RecordType { RecordTypeNormal, RecordTypePath, RecordTypeText, RecordTypeShapePath };

	//using FreeVariable = std::pair<Address, VariableRange>;

	//using FreeVariableAddress = std::pair<Address, VariableRange>;

	using FreeVarType = boost::variant<boost::recursive_wrapper<Accessor>, Reference>;

	struct BoundedFreeVar
	{
		FreeVarType freeVariable;// Accessor | Reference
		boost::optional<Expr> freeRange;

		BoundedFreeVar() = default;
		BoundedFreeVar(const FreeVarType& freeVariable) :freeVariable(freeVariable) {}
	};

	//unitConstraintに出現するfree変数のAddress
	using ConstraintAppearance = std::unordered_set<Address>;

	//同じfree変数への依存性を持つ制約IDの組
	using ConstraintGroup = std::unordered_set<size_t>;

	//レコード継承時に用いるデータ
	struct OldRecordData
	{
		OldRecordData() = default;

		//変数ID->変数・範囲
		std::vector<RegionVariable> regionVars;
		std::vector<OptimizeRegion> optimizeRegions;

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

		//var宣言で指定されたアクセッサとその範囲
		std::vector<BoundedFreeVar> freeVariables;

		OldRecordData original;

		RecordType type = RecordType::RecordTypeNormal;
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

	static PackedRecord MakeVec2(double x, double y)
	{
		return MakeRecord("x", x, "y", y);
	}

	struct Record
	{
		std::unordered_map<std::string, Address> values;

		std::vector<OptimizationProblemSat> problems;
		boost::optional<Expr> constraint;

		//var宣言で指定されたアクセッサ
		//std::vector<Accessor> freeVariables;
		//std::vector<FreeVarType> freeVariables;// Accessor | Reference

		//var宣言で指定されたアクセッサの範囲
		//std::vector<Expr> freeRanges;

		//var宣言で指定されたアクセッサとその範囲
		std::vector<BoundedFreeVar> boundedFreeVariables;

		OldRecordData original;

		RecordType type = RecordType::RecordTypeNormal;
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
				constraint = BinaryExpr(constraint.get(), logicExpr, BinaryOp::And);
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
		FunctionAccess(const Expr& argument)
			:actualArguments({ argument })
		{}

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

	struct InheritAccess
	{
		RecordConstractor adder;

		InheritAccess() = default;
		InheritAccess(const RecordConstractor& record)
			:adder(record)
		{}

		static InheritAccess Make(const RecordConstractor& record)
		{
			return InheritAccess(record);
		}

		bool operator==(const InheritAccess& other)const
		{
			return IsEqual(adder, other.adder);
		}
	};

	using Access = boost::variant<ListAccess, RecordAccess, FunctionAccess, InheritAccess>;

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

		static Accessor MakeBinaryFunctionCall(const Identifier& functionName, const Expr& arg1, const Expr& arg2)
		{
			Accessor accessor;
			accessor.head = functionName;

			FunctionAccess caller;
			caller.add(arg1).add(arg2);
			AppendFunction(accessor, caller);

			return accessor;
		}

		static Accessor MakeUnaryFunctionCall(const Expr& function, const Expr& arg)
		{
			Accessor accessor;
			accessor.head = function;

			FunctionAccess caller;
			caller.add(arg);
			AppendFunction(accessor, caller);

			return accessor;
		}

		static void AppendList(Accessor& obj, const ListAccess& access)
		{
			obj.accesses.push_back(access);
		}

		static void AppendRecord(Accessor& obj, const RecordAccess& access)
		{
			obj.accesses.push_back(access);
		}

#ifdef CGL_ENABLE_CURRYING
		static void AppendFunction(Accessor& obj, const FunctionAccess& access)
		{
			if (access.actualArguments.empty())
			{
				obj.accesses.push_back(access);
			}
			else
			{
				//複数引数の呼び出しはここで1引数の複数回呼び出しに置き換える
				for (const auto& arg : access.actualArguments)
				{
					obj.accesses.push_back(FunctionAccess(arg));
				}
			}
		}
#else
		static void AppendFunction(Accessor& obj, const FunctionAccess& access)
		{
			obj.accesses.push_back(access);
		}
#endif

		static void AppendInherit(Accessor& obj, const InheritAccess& access)
		{
			obj.accesses.push_back(access);
		}

		static void Append(Accessor& obj, const Access& access)
		{
			if (auto listAccessOpt = AsOpt<ListAccess>(access))
			{
				AppendList(obj, listAccessOpt.get());
			}
			else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
			{
				AppendRecord(obj, recordAccessOpt.get());
			}
			else if (auto functionAccessOpt = AsOpt<FunctionAccess>(access))
			{
				AppendFunction(obj, functionAccessOpt.get());
			}
			else if (auto inheritAccessOpt = AsOpt<InheritAccess>(access))
			{
				AppendInherit(obj, inheritAccessOpt.get());
			}
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

				return IsEqualVal(lhs.get(), other.lhs.get());
			}
			else
			{
				return !other.lhs;
			}
		}
	};

	Expr BuildString(const std::u32string& str);
}

namespace cereal
{
	template<class Archive>
	inline void save(Archive& ar, const cgl::CharString& node)
	{
		const std::string str = cgl::AsUtf8(node.str);
		ar(str);
	}

	template<class Archive>
	inline void load(Archive& ar, cgl::CharString& node)
	{
		std::string str;
		ar(str);
		node.str = cgl::AsUtf32(str);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::List& node)
	{
		ar(node.data);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::PackedList::Data& node)
	{
		ar(node.value);
		ar(node.address);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::PackedList& node)
	{
		ar(node.data);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::KeyValue& node)
	{
		ar(node.name);
		ar(node.value);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Record& node)
	{
		ar(node.values);
		ar(node.problems);
		ar(node.constraint);
		ar(node.boundedFreeVariables);
		ar(node.original);
		ar(node.type);
		ar(node.isSatisfied);
		ar(node.pathPoints);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::PackedRecord::Data& node)
	{
		ar(node.value);
		ar(node.address);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::PackedRecord& node)
	{
		ar(node.values);
		ar(node.problems);
		ar(node.constraint);
		ar(node.freeVariables);
		ar(node.type);
		ar(node.isSatisfied);
		ar(node.pathPoints);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::FuncVal& node)
	{
		ar(node.arguments);
		ar(node.expr);
		ar(node.builtinFuncAddress);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Jump& node)
	{
		ar(node.lhs);
		ar(node.op);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Address& address)
	{
		ar(address.valueID);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Reference& reference)
	{
		ar(reference.referenceID);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::EitherReference& reference)
	{
		ar(reference.local);
		ar(reference.replaced);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Lines& node)
	{
		ar(node.exprs);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::BinaryExpr& node)
	{
		ar(node.lhs);
		ar(node.rhs);
		ar(node.op);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::RValue& node)
	{
		ar(node.value);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::LRValue& node)
	{
		ar(node.value);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Identifier& node)
	{
		ar(node.name);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Import& node)
	{
		ar(node.importPath);
		ar(node.importName);
		ar(node.seed);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::UnaryExpr& node)
	{
		ar(node.lhs);
		ar(node.op);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Range& node)
	{
		ar(node.lhs);
		ar(node.rhs);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::DefFunc& node)
	{
		ar(node.arguments);
		ar(node.expr);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::If& node)
	{
		ar(node.cond_expr);
		ar(node.then_expr);
		ar(node.else_expr);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::For& node)
	{
		ar(node.loopCounter);
		ar(node.rangeStart);
		ar(node.rangeEnd);
		ar(node.doExpr);
		ar(node.asList);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Return& node)
	{
		ar(node.expr);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::ListConstractor& node)
	{
		ar(node.data);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::KeyExpr& node)
	{
		ar(node.name);
		ar(node.expr);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::RecordConstractor& node)
	{
		ar(node.exprs);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::ListAccess& node)
	{
		ar(node.index);
		ar(node.isArbitrary);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::RecordAccess& node)
	{
		ar(node.name);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::FunctionAccess& node)
	{
		ar(node.actualArguments);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::InheritAccess& node)
	{
		ar(node.adder);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Accessor& node)
	{
		ar(node.head);
		ar(node.accesses);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::DeclSat& node)
	{
		ar(node.expr);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::DeclFree::DeclVar& node)
	{
		ar(node.accessor);
		ar(node.range);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::DeclFree& node)
	{
		ar(node.accessors);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Interval& node)
	{
		ar(node.minimum);
		ar(node.maximum);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::RegionVariable& node)
	{
		ar(node.address);
		ar(node.attributes);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::OptimizeRegion& node)
	{
		ar(node.region);
		ar(node.eps);
		ar(node.addresses);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::OptimizationProblemSat& node)
	{
		ar(node.expr);
		ar(node.data);
		ar(node.refs);
		ar(node.invRefs);
		ar(node.freeVariableRefs);
		ar(node.hasPlateausFunction);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::BoundedFreeVar& node)
	{
		ar(node.freeVariable);
		ar(node.freeRange);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::OldRecordData& node)
	{
		ar(node.regionVars);
		ar(node.optimizeRegions);
		ar(node.unitConstraints);
		ar(node.variableAppearances);
		ar(node.groupConstraints);
		ar(node.constraintGroups);
	}

	template<class Archive>
	inline void save(Archive& ar, const Eigen::Vector2d& node)
	{
		double x = node.x(), y = node.y();
		ar(x);
		ar(y);
	}

	template<class Archive>
	inline void load(Archive& ar, Eigen::Vector2d& node)
	{
		double x, y;
		ar(x);
		ar(y);
		node << x, y;
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::DeepReference& reference)
	{
		ar(reference.head);
		ar(reference.tail);
	}
}
