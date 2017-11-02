#include <iostream>
#include <string>
#include <cmath>

#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <boost/variant/variant.hpp>
#include <boost/variant/recursive_wrapper.hpp>
#include <boost/variant/static_visitor.hpp>
#include <boost/variant/apply_visitor.hpp>
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

//namespace test_parser {
//	using namespace boost::spirit;
//
//	template<typename Iterator>
//	struct expr_grammer
//		: qi::grammar<Iterator, Lines(), ascii::space_type>
//	{
//		qi::rule<Iterator, Expr(), ascii::space_type> expr, term, factor, pow_term, pow_term1;
//		qi::rule<Iterator, Lines(), ascii::space_type> expr_seq;
//
//		expr_grammer() : expr_grammer::base_type(expr_seq)
//		{
//			auto concat = [](Lines& lines, Expr& expr) {
//				lines.concat(expr);
//				return lines;
//			};
//
//			auto makeLines = [](Expr& expr) {
//				return Lines(expr);
//			};
//
//			expr_seq = expr[_val = Call(makeLines, _1)] >> *(
//				(',' >> expr[_val = Call(concat, _val, _1)])
//				| ('\n' >> expr[_val = Call(concat, _val, _1)])
//				);
//
//			expr = term[_val = _1] >> *(
//				('+' >> term[_val = MakeBinaryExpr<Add>()])
//				| ('-' >> term[_val = MakeBinaryExpr<Sub>()])
//				| ('=' >> term[_val = MakeBinaryExpr<Assign>()])
//				);
//
//			term = pow_term[_val = _1]
//				| (factor[_val = _1] >> *(
//				    ('*' >> factor[_val = MakeBinaryExpr<Mul>()])
//				  | ('/' >> factor[_val = MakeBinaryExpr<Div>()])
//				  ))
//				;
//
//			pow_term = factor[_val = _1] >> -('^' >> pow_term[_val = MakeBinaryExpr<Pow>()]);
//
//			factor = double_[_val = _1]
//				| '(' >> expr[_val = _1] >> ')'
//				| '+' >> factor[_val = MakeUnaryExpr<Add>()]
//				| '-' >> factor[_val = MakeUnaryExpr<Sub>()];
//		}
//	};
//}

namespace test_parser {
	using namespace boost::spirit;

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

		expr_grammer() : expr_grammer::base_type(expr_seq)
		{
			auto concatLines = [](Lines& lines, Expr& expr) { lines.concat(expr); };

			auto makeLines = [](Expr& expr) { return Lines(expr); };

			auto makeDefFunc = [](const Arguments& arguments, const Expr& expr) { return DefFunc(arguments, expr); };

			auto makeCallFunc = [](const Identifier& identifier) { return CallFunc(identifier); };

			auto addCharacter = [](Identifier& identifier, char c) { identifier.name.push_back(c); };

			auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };

			auto addArgument = [](CallFunc& callFunc, const Expr& expr) { callFunc.actualArguments.push_back(expr); };

			auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

			expr_seq = general_expr[_val = Call(makeLines, _1)] >> *(
				(',' >> general_expr[Call(concatLines, _val, _1)])
				| ('\n' >> general_expr[Call(concatLines, _val, _1)])
				);

			//= ^ -> は右結合

			general_expr = def_func[_val = _1]
				| call_func[_val = _1]
				| if_expr[_val = _1]
				| return_expr[_val = _1]
				| logic_expr[_val = _1];

			if_expr = lit("if") >> general_expr[_val = Call(If::Make, _1)]
				>> lit("then") >> general_expr[Call(If::SetThen, _val, _1)]
				>> -(lit("else") >> general_expr[Call(If::SetElse, _val, _1)])
				;

			return_expr = lit("return") >> general_expr[_val = Call(Return::Make, _1)];

			def_func = arguments[_val = _1] >> lit("->") >> expr_seq[Call(applyFuncDef, _val, _1)];

			call_func = id[_val = Call(makeCallFunc, _1)] >> '(' 
				>> -(general_expr[Call(addArgument, _val, _1)]) 
				>> *(',' >> general_expr[Call(addArgument, _val, _1)]) >> ')';

			arguments = -(id[_val = _1] >> *(',' >> arguments[Call(concatArguments, _val, _1)]));

			logic_expr = logic_term[_val = _1] >> *('|' >> logic_term[_val = MakeBinaryExpr<Or>()]);

			logic_term = logic_factor[_val = _1] >> *('&' >> logic_factor[_val = MakeBinaryExpr<And>()]);

			logic_factor = ('!' >> compare_expr[_val = MakeUnaryExpr<Not>()])
				| compare_expr[_val = _1]
				;

			compare_expr = arith_expr[_val = _1] >> *(
				(lit("==") >> arith_expr[_val = MakeBinaryExpr<Equal>()])
				| (lit("!=") >> arith_expr[_val = MakeBinaryExpr<NotEqual>()])
				| (lit("<") >> arith_expr[_val = MakeBinaryExpr<LessThan>()])
				| (lit("<=") >> arith_expr[_val = MakeBinaryExpr<LessEqual>()])
				| (lit(">") >> arith_expr[_val = MakeBinaryExpr<GreaterThan>()])
				| (lit(">=") >> arith_expr[_val = MakeBinaryExpr<GreaterEqual>()])
				)
				;

			arith_expr = (basic_arith_expr[_val = _1] >> -('=' >> arith_expr[_val = MakeBinaryExpr<Assign>()]));

			basic_arith_expr = term[_val = _1] >>
				*(('+' >> term[_val = MakeBinaryExpr<Add>()]) |
				('-' >> term[_val = MakeBinaryExpr<Sub>()]))
				;

			term = pow_term[_val = _1]
				| (factor[_val = _1] >> 
					*(('*' >> pow_term1[_val = MakeBinaryExpr<Mul>()]) | 
					('/' >> pow_term1[_val = MakeBinaryExpr<Div>()]))
				)
				;

			//最低でも1つは受け取るようにしないと、単一のfactorを受理できてしまうのでMul,Divの方に行ってくれない
			pow_term = factor[_val = _1] >> '^' >> pow_term1[_val = MakeBinaryExpr<Pow>()];
			pow_term1 = factor[_val = _1] >> -('^' >> pow_term1[_val = MakeBinaryExpr<Pow>()]);

			factor = double_[_val = _1]
				| '(' >> expr_seq[_val = _1] >> ')'
				| '+' >> factor[_val = MakeUnaryExpr<Add>()]
				| '-' >> factor[_val = MakeUnaryExpr<Sub>()]
				| id[_val = _1];

			//idの途中には空白を含めない
			id = lexeme[ascii::alpha[_val = _1] >> *(ascii::alnum[Call(addCharacter, _val, _1)])];
		}
	};

	template<typename Iterator>
	struct SpaceSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		SpaceSkipper() :SpaceSkipper::base_type(skip)
		{
			skip = ascii::blank;
		}
	};

	using IteratorT = std::string::const_iterator;
	using SkipperT = SpaceSkipper<IteratorT>;
}

#define TEST_MODE

int main()
{
	//std::string buffer;
	//while (std::getline(std::cin, buffer)) {
	//	Lines lines;
	//	test_parser::expr_grammer<decltype(buffer.begin())> p;
	//	//if (boost::spirit::qi::phrase_parse(buffer.begin(), buffer.end(), p, boost::spirit::ascii::space, expr)) {
	//	if (boost::spirit::qi::phrase_parse(buffer.begin(), buffer.end(), p, boost::spirit::ascii::space, lines)) {
	//		printLines(lines);
	//	}
	//	else {
	//		std::cerr << "Parse error!!\n";
	//	}
	//}

	const auto parse = [](const std::string& str, Lines& lines)->bool
	{
		using namespace test_parser;

		SpaceSkipper<IteratorT> skipper;
		expr_grammer<IteratorT, SkipperT> grammer;

		if (!boost::spirit::qi::phrase_parse(str.begin(), str.end(), grammer, skipper, lines))
		{
			std::cerr << "Parse error!!\n";
			return false;
		}

		return true;
	};

#ifdef TEST_MODE

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
		"(-> 1 + 2)",
		"(-> 1 + 2 \n 3)",
		"x, y -> x + y",
		"fun = a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i))",
R"(
gcd = m, n ->
	if m == 0
	then return m
	else self(mod(m, n), m)
)"
	});

	std::vector<std::string> test_ng({
		", 3*4",
		"1 + 1 , , 2 + 3",
		"1 + 2, 3 + 4,",
		"1 + 2, \n , 3 + 4",
		"1 + 3 * , 4 + 5",
		"1 + 3 * \n 4 + 5",
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
		const bool failed = !parse(test_ok[i], expr);

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