#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "Node.hpp"

template<class Op>
auto MakeUnaryExpr()
{
	return boost::phoenix::bind([](const auto & e) { return UnaryExpr<Op>(e); }, boost::spirit::_1);
}

template<class Op>
auto MakeBinaryExpr()
{
	return boost::phoenix::bind([](const auto& lhs, const auto& rhs) {
		return BinaryExpr<Op>(lhs, rhs);
	}, boost::spirit::_val, boost::spirit::_1);
}

template <class F, class... Args>
auto Call(F func, Args... args)
{
	return boost::phoenix::bind(func, args...);
}

namespace test_parser {
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

	using IteratorT = std::string::const_iterator;
	using SpaceSkipperT = SpaceSkipper<IteratorT>;
	using LineSkipperT = LineSkipper<IteratorT>;

	SpaceSkipperT spaceSkipper;
	LineSkipperT lineSkipper;

	template<typename Iterator, typename Skipper>
	struct expr_grammer
		: qi::grammar<Iterator, Lines(), Skipper>
	{
		qi::rule<Iterator, If(), Skipper> if_expr;
		qi::rule<Iterator, Return(), Skipper> return_expr;
		qi::rule<Iterator, DefFunc(), Skipper> def_func;
		qi::rule<Iterator, CallFunc(), Skipper> call_func;
		qi::rule<Iterator, Arguments(), Skipper> arguments;
		qi::rule<Iterator, Identifier(), Skipper> id;
		qi::rule<Iterator, Expr(), Skipper> general_expr, logic_expr, logic_term, logic_factor, compare_expr, arith_expr, basic_arith_expr, term, factor, pow_term, pow_term1;
		qi::rule<Iterator, Lines(), Skipper> expr_seq;
		qi::rule<Iterator, Lines(), Skipper> program;

		qi::rule<Iterator> s;

		expr_grammer() : expr_grammer::base_type(program)
		{
			auto concatLines = [](Lines& lines, Expr& expr) { lines.concat(expr); };

			auto makeLines = [](Expr& expr) { return Lines(expr); };

			auto makeDefFunc = [](const Arguments& arguments, const Expr& expr) { return DefFunc(arguments, expr); };

			auto makeCallFunc = [](const Identifier& identifier) { return CallFunc(identifier); };

			auto addCharacter = [](Identifier& identifier, char c) { identifier.name.push_back(c); };

			auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };

			auto addArgument = [](CallFunc& callFunc, const Expr& expr) { callFunc.actualArguments.push_back(expr); };

			auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

			program = s >> -(expr_seq) >> s;

			expr_seq = general_expr[_val = Call(makeLines, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(concatLines, _val, _1)])
				| (+(lit('\n')) >> general_expr[Call(concatLines, _val, _1)])
				);

			//= ^ -> は右結合

			general_expr =
				if_expr[_val = _1]
				| return_expr[_val = _1]
				| logic_expr[_val = _1];

			if_expr = lit("if") >> s >> general_expr[_val = Call(If::Make, _1)]
				>> s >> lit("then") >> s >> general_expr[Call(If::SetThen, _val, _1)]
				>> -(s >> lit("else") >> s >> general_expr[Call(If::SetElse, _val, _1)])
				;

			return_expr = lit("return") >> s >> general_expr[_val = Call(Return::Make, _1)];

			def_func = arguments[_val = _1] >> lit("->") >> s >> expr_seq[Call(applyFuncDef, _val, _1)];

			call_func = id[_val = Call(makeCallFunc, _1)] >> '('
				>> -(s >> general_expr[Call(addArgument, _val, _1)])
				>> *(s >> ',' >> s >> general_expr[Call(addArgument, _val, _1)]) >> s >> ')';

			arguments = -(id[_val = _1] >> *(s >> ',' >> s >> arguments[Call(concatArguments, _val, _1)]));

			logic_expr = logic_term[_val = _1] >> *(s >> '|' >> s >> logic_term[_val = MakeBinaryExpr<Or>()]);

			logic_term = logic_factor[_val = _1] >> *(s >> '&' >> s >> logic_factor[_val = MakeBinaryExpr<And>()]);

			logic_factor = ('!' >> s >> compare_expr[_val = MakeUnaryExpr<Not>()])
				| compare_expr[_val = _1]
				;

			compare_expr = arith_expr[_val = _1] >> *(
				(s >> lit("==") >> s >> arith_expr[_val = MakeBinaryExpr<Equal>()])
				| (s >> lit("!=") >> s >> arith_expr[_val = MakeBinaryExpr<NotEqual>()])
				| (s >> lit("<") >> s >> arith_expr[_val = MakeBinaryExpr<LessThan>()])
				| (s >> lit("<=") >> s >> arith_expr[_val = MakeBinaryExpr<LessEqual>()])
				| (s >> lit(">") >> s >> arith_expr[_val = MakeBinaryExpr<GreaterThan>()])
				| (s >> lit(">=") >> s >> arith_expr[_val = MakeBinaryExpr<GreaterEqual>()])
				)
				;

			arith_expr = (basic_arith_expr[_val = _1] >> -(s >> '=' >> s >> arith_expr[_val = MakeBinaryExpr<Assign>()]));

			basic_arith_expr = term[_val = _1] >>
				*((s >> '+' >> s >> term[_val = MakeBinaryExpr<Add>()]) |
				(s >> '-' >> s >> term[_val = MakeBinaryExpr<Sub>()]))
				;

			term = pow_term[_val = _1]
				| (factor[_val = _1] >>
					*((s >> '*' >> s >> pow_term1[_val = MakeBinaryExpr<Mul>()]) |
					(s >> '/' >> s >> pow_term1[_val = MakeBinaryExpr<Div>()]))
					)
				;

			//最低でも1つは受け取るようにしないと、単一のfactorを受理できてしまうのでMul,Divの方に行ってくれない
			pow_term = factor[_val = _1] >> s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr<Pow>()];
			pow_term1 = factor[_val = _1] >> -(s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr<Pow>()]);

			factor = double_[_val = _1]
				| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
				| '+' >> s >> factor[_val = MakeUnaryExpr<Add>()]
				| '-' >> s >> factor[_val = MakeUnaryExpr<Sub>()]
				| call_func[_val = _1]
				| def_func[_val = _1]
				| id[_val = _1];


			//idの途中には空白を含めない
			id = lexeme[ascii::alpha[_val = _1] >> *(ascii::alnum[Call(addCharacter, _val, _1)])];

			s = -(ascii::space);
		}
	};
}

#define DO_TEST

int main()
{
	const auto parse = [](const std::string& str, Lines& lines)->bool
	{
		using namespace test_parser;

		SpaceSkipper<IteratorT> skipper;
		expr_grammer<IteratorT, SpaceSkipperT> grammer;

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
)"
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
	while (std::getline(std::cin, buffer))
	{
		Lines lines;
		const bool succeed = parse(buffer, lines);

		if (!succeed)
		{
			std::cerr << "Parse error!!\n";
		}

		printLines(lines);
	}

	return 0;
}