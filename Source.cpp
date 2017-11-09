#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>
//#include <boost/spirit/home/x3.hpp>

#include "Node.hpp"

auto MakeUnaryExpr(UnaryOp op)
{
	return boost::phoenix::bind([](const auto & e, UnaryOp op) {
		return UnaryExpr(e, op);
	}, boost::spirit::_1, op);
}

auto MakeBinaryExpr(BinaryOp op)
{
	return boost::phoenix::bind([](const auto& lhs, const auto& rhs, BinaryOp op) {
		return BinaryExpr(lhs, rhs, op);
	}, boost::spirit::_val, boost::spirit::_1, op);
}

template <class F, class... Args>
auto Call(F func, Args... args)
{
	return boost::phoenix::bind(func, args...);
}

namespace cgl
{
	using namespace boost::spirit;

	template<typename Iterator>
	struct SpaceSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		SpaceSkipper() :SpaceSkipper::base_type(skip)
		{
			skip = +(lit(' ') ^ lit('\r') ^ lit('\t'));
		}
	};

	template<typename Iterator>
	struct LineSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		LineSkipper() :LineSkipper::base_type(skip)
		{
			skip = ascii::space;
		}
	};

	struct keywords_t : qi::symbols<char, qi::unused_type> {
		keywords_t() {
			add("for", qi::unused)
				("in", qi::unused)
				("while", qi::unused);
		}
	} const keywords;

	using IteratorT = std::string::const_iterator;
	using SpaceSkipperT = SpaceSkipper<IteratorT>;
	using LineSkipperT = LineSkipper<IteratorT>;

	SpaceSkipperT spaceSkipper;
	LineSkipperT lineSkipper;

	using qi::char_;

	template<typename Iterator, typename Skipper>
	struct Parser
		: qi::grammar<Iterator, Lines(), Skipper>
	{
		//qi::rule<Iterator, RecordConstractor(), Skipper> record_maker;
		qi::rule<Iterator, ListConstractor(), Skipper> list_maker;
		qi::rule<Iterator, ListAccess(), Skipper> list_access;
		qi::rule<Iterator, For(), Skipper> for_expr;
		qi::rule<Iterator, If(), Skipper> if_expr;
		qi::rule<Iterator, Return(), Skipper> return_expr;
		qi::rule<Iterator, DefFunc(), Skipper> def_func;
		qi::rule<Iterator, CallFunc(), Skipper> call_func;
		qi::rule<Iterator, Arguments(), Skipper> arguments;
		qi::rule<Iterator, Identifier(), Skipper> id;
		qi::rule<Iterator, Expr(), Skipper> general_expr, logic_expr, logic_term, logic_factor, compare_expr, arith_expr, basic_arith_expr, term, factor, pow_term, pow_term1;
		qi::rule<Iterator, Lines(), Skipper> expr_seq, statement;
		qi::rule<Iterator, Lines(), Skipper> program;

		//qi::rule<Iterator, Range(), Skipper> range;

		//qi::rule<Iterator, std::string(), Skipper> double_value;
		//qi::rule<Iterator, Identifier(), Skipper> double_value, double_value2;


		qi::rule<Iterator> s;
		qi::rule<Iterator> distinct_keyword;
		qi::rule<Iterator, std::string(), Skipper> unchecked_identifier;
		
		Parser() : Parser::base_type(program)
		{
			auto concatLines = [](Lines& lines, Expr& expr) { lines.concat(expr); };

			auto makeLines = [](Expr& expr) { return Lines(expr); };

			auto makeDefFunc = [](const Arguments& arguments, const Expr& expr) { return DefFunc(arguments, expr); };

			auto makeCallFunc = [](const Identifier& identifier) { return CallFunc(identifier); };

			auto addCharacter = [](Identifier& identifier, char c) { identifier.name.push_back(c); };

			auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };

			auto addArgument = [](CallFunc& callFunc, const Expr& expr) { callFunc.actualArguments.push_back(expr); };

			auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

			/*auto makeDouble = [](const std::string& str) { return std::stod(str); };
			auto makeString = [](char c) { return std::string({ c }); };
			auto appendString = [](std::string& str, char c) { str.push_back(c); };*/

			auto makeDouble = [](const Identifier& str) { return std::stod(str.name); };
			auto makeString = [](char c) { return Identifier(std::string({ c })); };
			auto appendString = [](Identifier& str, char c) { str.name.push_back(c); };
			auto appendString2 = [](Identifier& str, const Identifier& str2) { str.name.append(str2.name); };

			//auto makeBool = [](bool b) { return std::stod(str); };
			
			program = s >> -(expr_seq) >> s;

			/*expr_seq = general_expr[_val = Call(makeLines, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(concatLines, _val, _1)])
				| (+(lit('\n')) >> general_expr[Call(concatLines, _val, _1)])
				);*/

			expr_seq = statement[_val = _1] >> *(
				*(lit('\n')) >> statement[Call(Lines::Concat, _val, _1)]
				);

			statement = general_expr[_val = Call(Lines::Make, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(Lines::Append, _val, _1)])
				| (lit('\n') >> general_expr[Call(Lines::Append, _val, _1)])
				);

			//record_maker = (char_('{') >> char_('}'));

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
				>> s >> general_expr[Call(For::SetRangeEnd, _val, _1)] >> s >> lit("do")
				>> s >> general_expr[Call(For::SetDo, _val, _1)];

			return_expr = lit("return") >> s >> general_expr[_val = Call(Return::Make, _1)];

			//def_func = arguments[_val = _1] >> lit("->") >> s >> expr_seq[Call(applyFuncDef, _val, _1)];
			def_func = arguments[_val = _1] >> lit("->") >> s >> statement[Call(applyFuncDef, _val, _1)];

			call_func = id[_val = Call(makeCallFunc, _1)] >> '('
				>> -(s >> general_expr[Call(addArgument, _val, _1)])
				>> *(s >> ',' >> s >> general_expr[Call(addArgument, _val, _1)]) >> s >> ')';

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

			//= ^ -> �͉E����

			arith_expr = (basic_arith_expr[_val = _1] >> -(s >> '=' >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Assign)]));

			basic_arith_expr = term[_val = _1] >>
				*((s >> '+' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Add)]) |
				(s >> '-' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Sub)]))
				;

			term = pow_term[_val = _1]
				| (factor[_val = _1] >>
					*((s >> '*' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Mul)]) |
					(s >> '/' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Div)]))
					)
				;
			
			//�Œ�ł�1�͎󂯎��悤�ɂ��Ȃ��ƁA�P���factor���󗝂ł��Ă��܂��̂�Mul,Div�̕��ɍs���Ă���Ȃ�
			pow_term = factor[_val = _1] >> s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)];
			pow_term1 = factor[_val = _1] >> -(s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)]);

			//factor = double_[_val = _1]
			/*factor = double_value[_val = Call(makeDouble, _1)]
				| int_[_val = _1]
				| lit("true")[_val = true]
				| lit("false")[_val = false]
				| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
				| '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
				| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
				| call_func[_val = _1]
				| def_func[_val = _1]
				| id[_val = _1];*/

			list_maker = (char_('[') >> s >> general_expr[_val = Call(ListConstractor::Make, _1)] >>
				*(s >> char_(',') >> s >> general_expr[Call(ListConstractor::Append, _val, _1)]) >> s >> char_(']')
				)
				| (char_('[') >> s >> char_(']'));

			list_access = id[_val = Call(ListAccess::Make, _1)] >> s >> "[" >> s >> general_expr[Call(ListAccess::SetIndex, _val, _1)] >> s >> "]";

			factor = /*double_[_val = _1]
				|*/ int_[_val = _1]
				| lit("true")[_val = true]
				| lit("false")[_val = false]
				| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
				| '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
				| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
				| list_access[_val = _1]
				| call_func[_val = _1]
				| def_func[_val = _1]
				| for_expr[_val = _1]
				| list_maker[_val = _1]
				| id[_val = _1];

			//range = factor[_val = Call(Range::Make, _1)] >> s >> lit("..") >> s >> factor[Call(Range::SetRhs,_val, _1)];

			//id�̓r���ɂ͋󔒂��܂߂Ȃ�
			//id = lexeme[ascii::alpha[_val = _1] >> *(ascii::alnum[Call(addCharacter, _val, _1)])];
			//id = identifier_def[_val = _1];
			id = unchecked_identifier[_val = _1] - distinct_keyword;

			distinct_keyword = qi::lexeme[keywords >> !(qi::alnum | '_')];
			unchecked_identifier = qi::lexeme[(qi::alpha | qi::char_('_')) >> *(qi::alnum | qi::char_('_'))];

			/*auto const distinct_keyword = qi::lexeme[keywords >> !(qi::alnum | '_')];
			auto const unchecked_identifier = qi::lexeme[(qi::alpha | qi::char_('_')) >> *(qi::alnum | qi::char_('_'))];
			auto const identifier_def = unchecked_identifier - distinct_keyword;*/

			s = -(ascii::space);

			//double_ ���� 1. �Ƃ����p�[�X�ł��Ă��܂������Ń����W�̃p�[�X�Ɏx�Ⴊ�o��̂ŕʂɒ�`����
			//double_value = lexeme[qi::char_('1', '9') >> *(ascii::digit) >> lit(".") >> +(ascii::digit)  [_val = Call(makeDouble, _1)]];
			/*double_value = lexeme[qi::char_('1', '9')[_val = Call(makeString, _1)] >> *(ascii::digit[Call(appendString, _val, _1)])
				>> lit(".")[Call(appendString, _val, _1)] >> +(ascii::digit[Call(appendString, _val, _1)])];*/
			/*double_value = lexeme[qi::char_('1', '9')[_val = _1] >> *(ascii::digit[Call(appendString, _val, _1)])
				>> lit(".")[Call(appendString, _val, _1)] >> +(ascii::digit[Call(appendString, _val, _1)])];*/
			
			/*double_value = lexeme[qi::char_('1', '9')[_val = _1] >> *(ascii::digit[Call(appendString, _val, _1)]) >> double_value2[Call(appendString2, _val, _1)]];

			double_value2 = lexeme[lit(".")[_val = '.'] >> +(ascii::digit[Call(appendString, _val, _1)])];*/

		}
	};
}

#define DO_TEST

int main()
{
	const auto parse = [](const std::string& str, Lines& lines)->bool
	{
		using namespace cgl;

		SpaceSkipper<IteratorT> skipper;
		Parser<IteratorT, SpaceSkipperT> grammer;

		std::string::const_iterator it = str.begin();
		if (!boost::spirit::qi::phrase_parse(it, str.end(), grammer, skipper, lines))
		{
			std::cerr << "error: parse failed\n";
			return false;
		}

		if (it != str.end())
		{
			std::cerr << "error: ramains input\n" << std::string(it, str.end());
			return false;
		}

		return true;
	};

#ifdef DO_TEST

	std::vector<std::string> test_ok({
		"(1*2 + -3*-(4 + 5/6))",
		"1/(3+4)^6^7",
		"1 + 2, 3 + 4",
		"\n 4*5",
		"1 + 1 \n 2 + 3",
		"1 + 2 \n 3 + 4 \n",
		"1 + 1, \n 2 + 3",
		"1 + 1 \n \n \n 2 + 3",
		"1 + 2 \n , 4*5",
		"1 + 3 * \n 4 + 5",
		"(-> 1 + 2)",
		"(-> 1 + 2 \n 3)",
		"x, y -> x + y",
		"fun = (a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i)))",
		"fun = a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i))",
		"gcd1 = (m, n ->\n if m == 0\n then return m\n else self(mod(m, n), m)\n)",
		"gcd2 = (m, n ->\r\n if m == 0\r\n then return m\r\n else self(mod(m, n), m)\r\n)",
		"gcd3 = (m, n ->\n\tif m == 0\n\tthen return m\n\telse self(mod(m, n), m)\n)",
		"gcd4 = (m, n ->\r\n\tif m == 0\r\n\tthen return m\r\n\telse self(mod(m, n), m)\r\n)",
		R"(
gcd5 = (m, n ->
	if m == 0
	then return m
	else self(mod(m, n), m)
)
)",
R"(
func = x ->  
    x + 1
    return x

func2 = x, y -> x + y)"
	});

	std::vector<std::string> test_ng({
		", 3*4",
		"1 + 1 , , 2 + 3",
		"1 + 2, 3 + 4,",
		"1 + 2, \n , 3 + 4",
		"1 + 3 * , 4 + 5",
		"(->)"
	});

	int ok_wrongs = 0;
	int ng_wrongs = 0;

	std::cout << "==================== Test Case OK ====================" << std::endl;
	for (size_t i = 0; i < test_ok.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ok[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool succeed = parse(test_ok[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (succeed)
		{
			/*
			std::cout << "eval:\n";
			printEvaluated(evalExpr(expr));
			std::cout << "\n";
			*/
		}
		else
		{
			std::cout << "[Wrong]\n";
			++ok_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "==================== Test Case NG ====================" << std::endl;
	for (size_t i = 0; i < test_ng.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ng[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool failed = !parse(test_ng[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (failed)
		{
			std::cout << "no result\n";
		}
		else
		{
			//std::cout << "eval:\n";
			//printEvaluated(evalExpr(expr));
			//std::cout << "\n";
			std::cout << "[Wrong]\n";
			++ng_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "Result:\n";
	std::cout << "Correct programs: (Wrong / All) = (" << ok_wrongs << " / " << test_ok.size() << ")\n";
	std::cout << "Wrong   programs: (Wrong / All) = (" << ng_wrongs << " / " << test_ng.size() << ")\n";

#endif

	std::string buffer;
	while (std::cout << ">>", std::getline(std::cin, buffer))
	{
		Lines lines;
		const bool succeed = parse(buffer, lines);

		if (!succeed)
		{
			std::cerr << "Parse error!!\n";
		}

		printLines(lines);

		if (succeed)
		{
			Evaluated result = evalExpr(lines);
			std::cout << "Result Evaluation:\n";
			printEvaluated(result);
		}
	}

	return 0;
}