#include <Eigen/Core>

#include <Pita/Node.hpp>
#include <Pita/Printer.hpp>
#include <Pita/Environment.hpp>
#include <Pita/Evaluator.hpp>
#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/Parser.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/Program.hpp>

extern std::ofstream ofs;
extern bool calculating;

namespace cgl
{
	boost::optional<Expr> Program::parse(const std::string& program)
	{
		using namespace cgl;

		Lines lines;

		SpaceSkipper<IteratorT> skipper;
		Parser<IteratorT, SpaceSkipperT> grammer;

		std::string::const_iterator it = program.begin();
		if (!boost::spirit::qi::phrase_parse(it, program.end(), grammer, skipper, lines))
		{
			//std::cerr << "Syntax Error: parse failed\n";
			std::cout << "Syntax Error: parse failed\n";
			return boost::none;
		}

		if (it != program.end())
		{
			//std::cerr << "Syntax Error: ramains input\n" << std::string(it, program.end());
			std::cout << "Syntax Error: ramains input\n" << std::string(it, program.end());
			return boost::none;
		}

		Expr result = lines;
		return result;
	}

	boost::optional<Evaluated> Program::execute(const std::string& program)
	{
		if (auto exprOpt = parse(program))
		{
			try
			{
				return pEnv->expand(boost::apply_visitor(evaluator, exprOpt.value()));
			}
			catch (const cgl::Exception& e)
			{
				std::cerr << "Exception: " << e.what() << std::endl;
			}
		}

		return boost::none;
	}

	bool Program::draw(const std::string& program, bool logOutput)
	{
		if (logOutput) std::cout << "parse..." << std::endl;

		if (auto exprOpt = parse(program))
		{
			try
			{
				if (logOutput)
				{
					std::cout << "parse succeeded" << std::endl;
					printExpr(exprOpt.value());
				}

				if (logOutput) std::cout << "execute..." << std::endl;
				const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
				const Evaluated result = pEnv->expand(lrvalue);
				if (logOutput) std::cout << "execute succeeded" << std::endl;

				if (logOutput) std::cout << "output SVG..." << std::endl;
				OutputSVG(std::cout, result, pEnv);
				if (logOutput) std::cout << "output succeeded" << std::endl;
			}
			catch (const cgl::Exception& e)
			{
				std::cerr << "Exception: " << e.what() << std::endl;
			}
		}

		return false;
	}

	void Program::execute1(const std::string& program, bool logOutput)
	{
		clear();

		if (logOutput)
		{
			std::cout << "parse..." << std::endl;
			std::cout << program << std::endl;
		}

		if (auto exprOpt = parse(program))
		{
			try
			{
				if (logOutput)
				{
					std::cout << "parse succeeded" << std::endl;
					printExpr(exprOpt.value(), std::cout);
				}

				if (logOutput) std::cout << "execute..." << std::endl;
				const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
				evaluated = pEnv->expand(lrvalue);
				if (logOutput)
				{
					std::cout << "execute succeeded" << std::endl;
					printEvaluated(evaluated.value(), pEnv, std::cout, 0);

					std::cout << "output SVG..." << std::endl;
					std::ofstream file("result.svg");
					OutputSVG(file, evaluated.value(), pEnv);
					file.close();
					std::cout << "completed" << std::endl;
				}

				succeeded = true;
			}
			catch (const cgl::Exception& e)
			{
				//std::cerr << "Exception: " << e.what() << std::endl;
				std::cout << "Exception: " << e.what() << std::endl;

				succeeded = false;
			}
		}
		else
		{
			succeeded = false;
		}

		calculating = false;
	}

	void Program::clear()
	{
		pEnv = Environment::Make();
		evaluated = boost::none;
		evaluator = Eval(pEnv);
		succeeded = false;
	}

	bool Program::test(const std::string& program, const Expr& expr)
	{
		clear();

		if (auto result = execute(program))
		{
			std::shared_ptr<Environment> pEnv2 = Environment::Make();
			Eval evaluator2(pEnv2);

			const Evaluated answer = pEnv->expand(boost::apply_visitor(evaluator2, expr));

			return IsEqualEvaluated(result.value(), answer);
		}

		return false;
	}
}
