#pragma once
#include <stack>
#include <set>

#ifdef USE_IMPORT
#  ifdef USE_BOOST_LIB
#    include <boost/filesystem.hpp>
#  else
#    ifdef __has_include
#      if __has_include(<filesystem>)
#        include <filesystem>
#      elif __has_include(<experimental/filesystem>)
#        include <experimental/filesystem>
#        define CGL_EXPERIMENTAL_FILESYSTEM
#      else
#        error "filesystem does not exists"
#      endif
#    endif
#  endif
#endif

//#define BOOST_SPIRIT_DEBUG
#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_UNICODE

#include <boost/optional.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <boost/spirit/include/support_line_pos_iterator.hpp>

#include "Node.hpp"

extern bool isDebugMode;

namespace cgl
{
#ifdef USE_IMPORT
#  ifdef USE_BOOST_LIB
	namespace filesystem = boost::filesystem;
#  else
#    if defined(CGL_EXPERIMENTAL_FILESYSTEM) || (defined(_MSC_VER) && _MSC_VER <= 1900)
	namespace filesystem = std::experimental::filesystem;
#    else
	namespace filesystem = std::filesystem;
#    endif
#  endif

	//パース時のみ使用
	extern std::stack<filesystem::path> workingDirectories;

	extern std::unordered_map<size_t, boost::optional<Expr>> importedParseTrees;
#endif

	extern bool errorMessagePrinted;

	inline auto MakeUnaryExpr(UnaryOp op)
	{
		return boost::phoenix::bind([](const auto & e, UnaryOp op) {return UnaryExpr(e, op); }, boost::spirit::_1, op);
	}

	inline auto MakeBinaryExpr(BinaryOp op)
	{
		return boost::phoenix::bind([&](const auto& lhs, const auto& rhs, BinaryOp op) {return BinaryExpr(lhs, rhs, op); }, boost::spirit::_val, boost::spirit::_1, op);
	}

	template <class F, class... Args>
	inline auto Call(F func, Args... args)
	{
		return boost::phoenix::bind(func, args...);
	}

	template<class FromT, class ToT>
	inline auto Cast()
	{
		return boost::phoenix::bind([&](const FromT& a) {return static_cast<ToT>(a); }, boost::spirit::_1);
	}

	using namespace boost::spirit;

	//using IteratorT = std::string::const_iterator;
	using SourceT = boost::spirit::line_pos_iterator<std::string::const_iterator>;
	using IteratorT = boost::u8_to_u32_iterator<SourceT>;

	struct Annotator
	{
		Annotator(SourceT first, SourceT last) :
			first(first),
			last(last)
		{}

		const SourceT first;
		const SourceT last;

		template<typename Val, typename First, typename Last>
		void operator()(Val& v, First f, Last l)const
		{
			doAnnotate(v, f, l, first, last);
		}

	private:
		static void doAnnotate(LocationInfo& li, IteratorT f, IteratorT l, SourceT first, SourceT last);
		static void doAnnotate(Expr& li, IteratorT f, IteratorT l, SourceT first, SourceT last);
		static void doAnnotate(...) {}
	};

	struct ErrorHandler
	{
		ErrorHandler(SourceT first, SourceT last, const std::string& sourcePath) :
			first(first),
			last(last),
			sourcePath(sourcePath)
		{}

		const SourceT first;
		const SourceT last;
		const std::string sourcePath;

		typedef qi::error_handler_result result_type;
		template<typename First, typename Last, typename T>
		qi::error_handler_result operator()(First f, Last l, T const& what) const
		{
			std::stringstream ss;
			ss << what;
			doAnnotate(f, l, first, last, sourcePath, ss.str());
			return qi::fail;
		}

	private:
		static void doAnnotate(IteratorT f, IteratorT l, SourceT first, SourceT last, const std::string& sourcePath, const std::string& what);
		static void doAnnotate(...) {}
	};

	template<typename Iterator>
	struct SpaceSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		SpaceSkipper() :SpaceSkipper::base_type(skip)
		{
			skip = +(lit(' ') ^ lit('\r') ^ lit('\t'));
		}
	};

	namespace encode = qi::unicode;

	template<typename Iterator>
	struct LineSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		LineSkipper() :LineSkipper::base_type(skip)
		{
			skip = encode::space;
		}
	};

	struct keywords_t : qi::symbols<char, qi::unused_type> {
		keywords_t() {
			add("for", qi::unused)
				("in", qi::unused)
				("do", qi::unused)
				("sat", qi::unused)
				("var", qi::unused)
				("if", qi::unused)
				("then", qi::unused)
				("else", qi::unused);
		}
	} const keywords;

	using SpaceSkipperT = SpaceSkipper<IteratorT>;
	using LineSkipperT = LineSkipper<IteratorT>;

	static SpaceSkipperT spaceSkipper;
	static LineSkipperT lineSkipper;

	using SkipperT = LineSkipperT;

	struct Parser
		: qi::grammar<IteratorT, Lines(), SkipperT>
	{
		Parser(SourceT first, SourceT last, const std::string& sourcePath);

		qi::rule<IteratorT, DeclSat(), SkipperT> constraints;
		qi::rule<IteratorT, DeclFree(), SkipperT> freeVals;

		qi::rule<IteratorT, FunctionAccess(), SkipperT> functionAccess;
		qi::rule<IteratorT, RecordAccess(), SkipperT> recordAccess;
		qi::rule<IteratorT, ListAccess(), SkipperT> listAccess;
		qi::rule<IteratorT, InheritAccess(), SkipperT> inheritAccess;
		qi::rule<IteratorT, Accessor(), SkipperT> accessor;

		qi::rule<IteratorT, Access(), SkipperT> access;

		qi::rule<IteratorT, KeyExpr(), SkipperT> record_keyexpr;
		qi::rule<IteratorT, RecordConstractor(), SkipperT> record_maker;

		qi::rule<IteratorT, ListConstractor(), SkipperT> list_maker;
		qi::rule<IteratorT, Import(), SkipperT> import_expr;
		qi::rule<IteratorT, For(), SkipperT> for_expr;
		qi::rule<IteratorT, If(), SkipperT> if_expr;
		qi::rule<IteratorT, Return(), SkipperT> return_expr;
		qi::rule<IteratorT, DefFunc(), SkipperT> def_func;
		qi::rule<IteratorT, Arguments(), SkipperT> arguments;
		qi::rule<IteratorT, Identifier(), SkipperT> id;
		qi::rule<IteratorT, KeyExpr(), SkipperT> key_expr;
		qi::rule<IteratorT, std::u32string(), SkipperT> char_string;
		qi::rule<IteratorT, Expr(), SkipperT> general_expr, logic_expr, logic_term, logic_factor, compare_expr, arith_expr, basic_arith_expr, term, factor, pow_term, pow_term1;
		qi::rule<IteratorT, Lines(), SkipperT> expr_seq;
		qi::rule<IteratorT, Lines(), SkipperT> program;

		qi::rule<IteratorT> distinct_keyword;
		qi::rule<IteratorT, std::u32string(), SkipperT> unchecked_identifier;
		qi::rule<IteratorT, std::u32string(), SkipperT> float_value;

		boost::phoenix::function<Annotator> annotate;
		boost::phoenix::function<ErrorHandler> errorHandler;
	};

	boost::optional<Expr> Parse1(const std::string& filepath);
	boost::optional<Expr> ParseFromSourceCode(const std::string& sourceCode);
	void PrintErrorPos(const std::string& input_filepath, const LocationInfo& locationInfo);
	void PrintErrorPosSource(const std::string& sourceCode, const LocationInfo& locationInfo);
}
