#pragma once
#include <stack>
#include <set>
#include <filesystem>

#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3
#define BOOST_SPIRIT_UNICODE

#include <boost/optional.hpp>
#include <boost/regex/pending/unicode_iterator.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <boost/spirit/include/support_line_pos_iterator.hpp>

#include "Node.hpp"

namespace cgl
{
	namespace filesystem = std::experimental::filesystem;

	//パース時のみ使用
	extern std::stack<filesystem::path> workingDirectories;

	extern std::unordered_map<size_t, boost::optional<Expr>> importedParseTrees;

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
	//using IteratorT = std::u32string::const_iterator;
	using SourceT = boost::spirit::line_pos_iterator<std::string::const_iterator>;
	//using IteratorT = boost::u8_to_u32_iterator<std::string::const_iterator>;
	using IteratorT = boost::u8_to_u32_iterator<SourceT>;

	struct Annotator
	{
		//typedef void result_type;

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
		void static doAnnotate(LocationInfo& li, IteratorT f, IteratorT l, SourceT first, SourceT last)
		{
			auto sourceBeginIt = f.base();
			auto sourceEndIt = l.base();

			auto lowerBound = get_line_start(first, sourceBeginIt);

			li.locInfo_lineBegin = get_line(sourceBeginIt);
			li.locInfo_lineEnd = get_line(sourceEndIt);

			auto line = get_current_line(lowerBound, sourceBeginIt, last);

			size_t cur_pos = 0, start_pos = 0, end_pos = 0;
			for (IteratorT it = line.begin(), _eol = line.end(); ; ++it, ++cur_pos)
			{
				if (it.base() == sourceBeginIt) start_pos = cur_pos;
				if (it.base() == sourceEndIt) end_pos = cur_pos;

				if (*(it.base()) == '\n')
					cur_pos = 0;

				if (it == _eol)
					break;
			}

			li.locInfo_posBegin = start_pos;
			li.locInfo_posEnd = end_pos;
		}
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

	template<typename Skipper>
	struct Parser
		: qi::grammar<IteratorT, Lines(), Skipper>
	{
		qi::rule<IteratorT, DeclSat(), Skipper> constraints;
		qi::rule<IteratorT, DeclFree(), Skipper> freeVals;

		qi::rule<IteratorT, FunctionAccess(), Skipper> functionAccess;
		qi::rule<IteratorT, RecordAccess(), Skipper> recordAccess;
		qi::rule<IteratorT, ListAccess(), Skipper> listAccess;
		qi::rule<IteratorT, Accessor(), Skipper> accessor;

		qi::rule<IteratorT, Access(), Skipper> access;

		qi::rule<IteratorT, KeyExpr(), Skipper> record_keyexpr;
		qi::rule<IteratorT, RecordConstractor(), Skipper> record_maker;
		qi::rule<IteratorT, RecordInheritor(), Skipper> record_inheritor;

		qi::rule<IteratorT, ListConstractor(), Skipper> list_maker;
		qi::rule<IteratorT, Import(), Skipper> import_expr;
		qi::rule<IteratorT, For(), Skipper> for_expr;
		qi::rule<IteratorT, If(), Skipper> if_expr;
		qi::rule<IteratorT, Return(), Skipper> return_expr;
		qi::rule<IteratorT, DefFunc(), Skipper> def_func;
		qi::rule<IteratorT, Arguments(), Skipper> arguments;
		qi::rule<IteratorT, Identifier(), Skipper> id;
		qi::rule<IteratorT, KeyExpr(), Skipper> key_expr;
		qi::rule<IteratorT, std::u32string(), Skipper> char_string;
		qi::rule<IteratorT, Expr(), Skipper> general_expr, logic_expr, logic_term, logic_factor, compare_expr, arith_expr, basic_arith_expr, term, factor, pow_term, pow_term1;
		qi::rule<IteratorT, Lines(), Skipper> expr_seq, statement;
		qi::rule<IteratorT, Lines(), Skipper> program;

		qi::rule<IteratorT> s, s1;
		qi::rule<IteratorT> distinct_keyword;
		qi::rule<IteratorT, std::u32string(), Skipper> unchecked_identifier;
		qi::rule<IteratorT, std::u32string(), Skipper> float_value;

		boost::phoenix::function<Annotator> annotate;
		
		Parser(SourceT first, SourceT last) :
			Parser::base_type(program),
			annotate(Annotator(first, last))
		{
			auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };
			auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

			program = s >> -(expr_seq) >> s;

			expr_seq = statement[_val = _1] >> *(
				+(lit('\n')) >> statement[Call(Lines::Concat, _val, _1)]
				);

			statement = general_expr[_val = Call(Lines::Make, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(Lines::Append, _val, _1)])
				| (lit('\n') >> general_expr[Call(Lines::Append, _val, _1)])
				);

			general_expr =
				if_expr[_val = _1]
				| return_expr[_val = _1]
				| logic_expr[_val = _1];

			if_expr = lit("if") >> s >> general_expr[_val = Call(If::Make, _1)]
				>> s >> lit("then") >> s >> general_expr[Call(If::SetThen, _val, _1)]
				>> -(s >> lit("else") >> s >> general_expr[Call(If::SetElse, _val, _1)])
				;

			for_expr = lit("for") >> s >> id[_val = Call(For::Make, _1)] >> s >> lit("in")
				>> s >> general_expr[Call(For::SetRangeStart, _val, _1)] >> s >> lit(":")
				>> s >> general_expr[Call(For::SetRangeEnd, _val, _1)] >> s >>
				((lit("do") >> s >> general_expr[Call(For::SetDo, _val, _1, false)]) |
				(lit("list") >> s >> general_expr[Call(For::SetDo, _val, _1, true)]));

			import_expr = lit("import") >> s >> '\"' >> char_string[_val = Call(Import::Make, _1)] >> '\"' >> -(s >> lit("as") >> s >> id[Call(Import::SetName, _val, _1)]);

			return_expr = lit("return") >> s >> general_expr[_val = Call(Return::Make, _1)];

			//def_func = arguments[_val = _1] >> lit("->") >> s >> statement[Call(applyFuncDef, _val, _1)];
			def_func = arguments[_val = _1] >> lit("->") >> s >> expr_seq[Call(applyFuncDef, _val, _1)];

			constraints = lit("sat") >> '(' >> s >> statement[_val = Call(DeclSat::Make, _1)] >> s >> ')';

			/*freeVals = lit("var") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())]) >> *(
				s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())])
				) >> s >> ')';*/
			freeVals = lit("var") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())]) >>
				-(s >> lit("in") >> s >> factor[Call(DeclFree::AddRange, _val, _1)]) >> *(
					s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())]) >>
					-(s >> lit("in") >> s >> factor[Call(DeclFree::AddRange, _val, _1)])
					) >> s >> ')';

			arguments = -(id[_val = _1] >> *(s >> ',' >> s >> arguments[Call(concatArguments, _val, _1)]));

			logic_expr = logic_term[_val = _1] >> *(s >> '|' >> s >> logic_term[_val = MakeBinaryExpr(BinaryOp::Or)]);

			logic_term = logic_factor[_val = _1] >> *(s >> '&' >> s >> logic_factor[_val = MakeBinaryExpr(BinaryOp::And)]);

			logic_factor = ('!' >> s >> compare_expr[_val = MakeUnaryExpr(UnaryOp::Not)])
				| compare_expr[_val = _1]
				;

			compare_expr = arith_expr[_val = _1] >> *(
				(s >> lit("==") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Equal)])
				| (s >> lit("!=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::NotEqual)])
				| (s >> lit("<") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessThan)])
				| (s >> lit("<=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessEqual)])
				| (s >> lit(">") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterThan)])
				| (s >> lit(">=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterEqual)])
				)
				;

			//= ^ -> は右結合
			arith_expr = (key_expr[_val = _1] | basic_arith_expr[_val = _1]) >> -(
				(s >> '=' >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Assign)])
				);

			key_expr = id[_val = Call(KeyExpr::Make, _1)] >> s >> ':' >> s >> basic_arith_expr[Call(KeyExpr::SetExpr, _val, _1)];

			basic_arith_expr = term[_val = _1] >>
				*(
				(s >> '+' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Add)]) |
				(s >> '-' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Sub)]) |
				(s >> '@' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Concat)])
					)
				;

			term = pow_term[_val = _1]
				| (factor[_val = _1] >>
					*((s >> '*' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Mul)]) |
					(s >> '/' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Div)]))
					)
				;
			
			//最低でも1つは受け取るようにしないと、単一のfactorを受理できてしまうのでMul,Divの方に行ってくれない
			pow_term = factor[_val = _1] >> s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)];
			pow_term1 = factor[_val = _1] >> -(s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)]);

			//record{} の間には改行は挟めない（record,{}と区別できなくなるので）
			record_inheritor = (accessor[_val = Call(RecordInheritor::MakeAccessor, _1)] | id[_val = Call(RecordInheritor::MakeIdentifier, _1)]) >> record_maker[Call(RecordInheritor::AppendRecord, _val, _1)];

			record_maker = (
				encode::char_('{') >> s >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]) >>
				*(
				(s >> ',' >> s >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]))
					| (+(encode::char_('\n')) >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]))
					)
				>> s >> encode::char_('}')
				)
				| (encode::char_('{') >> s >> encode::char_('}'));

			//レコードの name:val の name と : の間に改行を許すべきか？ -> 許しても解析上恐らく問題はないが、意味があまりなさそう
			record_keyexpr = id[_val = Call(KeyExpr::Make, _1)] >> encode::char_(':') >> s >> general_expr[Call(KeyExpr::SetExpr, _val, _1)];

			list_maker = (encode::char_('[') >> s >> general_expr[_val = Call(ListConstractor::Make, _1)] >>
				*(
					(s >> encode::char_(',') >> s >> general_expr[Call(ListConstractor::Append, _val, _1)])
					| (+(encode::char_('\n')) >> general_expr[Call(ListConstractor::Append, _val, _1)])
					) >> s >> encode::char_(']')
				)
				| (encode::char_('[') >> s >> encode::char_(']'));
			
			accessor = (id[_val = Call(Accessor::Make, _1)] >> +(access[Call(Accessor::Append, _val, _1)]))
				| (list_maker[_val = Call(Accessor::Make, _1)] >> listAccess[Call(Accessor::AppendList, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]))
				| (record_maker[_val = Call(Accessor::Make, _1)] >> recordAccess[Call(Accessor::AppendRecord, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]));
			
			access = functionAccess[_val = Cast<FunctionAccess, Access>()]
				| listAccess[_val = Cast<ListAccess, Access>()]
				| recordAccess[_val = Cast<RecordAccess, Access>()];

			recordAccess = encode::char_('.') >> s >> id[_val = Call(RecordAccess::Make, _1)];

			//listAccess = encode::char_('[') >> s >> general_expr[Call(ListAccess::SetIndex, _val, _1)] >> s >> encode::char_(']');
			listAccess = encode::char_('[') >> s >> (encode::char_('*')[Call(ListAccess::SetIndexArbitrary, _val)] | general_expr[Call(ListAccess::SetIndex, _val, _1)]) >> s >> encode::char_(']');

			functionAccess = encode::char_('(')
				>> -(s >> general_expr[Call(FunctionAccess::Append, _val, _1)])
				>> *(s >> encode::char_(',') >> s >> general_expr[Call(FunctionAccess::Append, _val, _1)]) >> s >> encode::char_(')');

			factor = 
				  '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
				| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
				| '@' >> s >> (accessor[_val = MakeUnaryExpr(UnaryOp::Dynamic)] | id[_val = MakeUnaryExpr(UnaryOp::Dynamic)])
				| float_value[_val = Call(LRValue::Float, _1)]
				| int_[_val = Call(LRValue::Int, _1)]
				| lit("true")[_val = Call(LRValue::Bool, true)]
				| lit("false")[_val = Call(LRValue::Bool, false)]
				| import_expr[_val = _1]
				| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
				| '\"' >> char_string[_val = Call(BuildString, _1)] >> '\"'
				| constraints[_val = _1]
				| record_inheritor[_val = _1]
				| freeVals[_val = _1]
				| accessor[_val = _1]
				| def_func[_val = _1]
				| for_expr[_val = _1]
				| list_maker[_val = _1]
				| record_maker[_val = _1]
				| id[_val = _1];

			id = unchecked_identifier[_val = Call(Identifier::MakeIdentifier, _1)] - distinct_keyword;

			char_string = lexeme[*(encode::char_ - encode::char_('\"'))];

			distinct_keyword = lexeme[keywords >> !(encode::alnum | '_')];
			unchecked_identifier = lexeme[(encode::alpha | encode::char_('_')) >> *(encode::alnum | encode::char_('_'))];

			float_value = lexeme[+encode::char_('0', '9') >> encode::char_('.') >> +encode::char_('0', '9')];

			s = *(encode::space);

			auto setLocationInfo = annotate(_val, _1, _3);
			qi::on_success(constraints, setLocationInfo);
			qi::on_success(freeVals, setLocationInfo);
			qi::on_success(functionAccess, setLocationInfo);
			qi::on_success(recordAccess, setLocationInfo);
			qi::on_success(listAccess, setLocationInfo);
			qi::on_success(accessor, setLocationInfo);
			qi::on_success(access, setLocationInfo);
			qi::on_success(record_keyexpr, setLocationInfo);
			qi::on_success(record_maker, setLocationInfo);
			qi::on_success(record_inheritor, setLocationInfo);
			qi::on_success(list_maker, setLocationInfo);
			qi::on_success(import_expr, setLocationInfo);
			qi::on_success(for_expr, setLocationInfo);
			qi::on_success(if_expr, setLocationInfo);
			qi::on_success(return_expr, setLocationInfo);
			qi::on_success(def_func, setLocationInfo);
			qi::on_success(arguments, setLocationInfo);
			qi::on_success(id, setLocationInfo);
			qi::on_success(key_expr, setLocationInfo);
			qi::on_success(char_string, setLocationInfo);
			qi::on_success(general_expr, setLocationInfo);
			qi::on_success(logic_expr, setLocationInfo);
			qi::on_success(logic_term, setLocationInfo);
			qi::on_success(logic_factor, setLocationInfo);
			qi::on_success(compare_expr, setLocationInfo);
			qi::on_success(arith_expr, setLocationInfo);
			qi::on_success(basic_arith_expr, setLocationInfo);
			qi::on_success(term, setLocationInfo);
			qi::on_success(factor, setLocationInfo);
			qi::on_success(pow_term, setLocationInfo);
			qi::on_success(pow_term1, setLocationInfo);
			qi::on_success(expr_seq, setLocationInfo);
			qi::on_success(statement, setLocationInfo);
			qi::on_success(program, setLocationInfo);
		}
	};

	//boost::optional<Expr> Parse(const std::string& program);
	boost::optional<Expr> Parse1(const std::string& filename);
	void PrintErrorPos(const std::string& input_filepath, const LocationInfo& locationInfo);
}
