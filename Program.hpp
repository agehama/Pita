#pragma once
#include <Eigen/Core>

#include "Node.hpp"
#include "Printer.hpp"
#include "Environment.hpp"
#include "Evaluator.hpp"
#include "OptimizationEvaluator.hpp"
#include "Parser.hpp"
#include "Vectorizer.hpp"

extern std::ofstream ofs;
extern bool calculating;

namespace cgl
{
	class Program
	{
	public:
		Program() :
			pEnv(Environment::Make()),
			evaluator(pEnv)
		{}

		boost::optional<Expr> parse(const std::wstring& program)
		{
			using namespace cgl;

			Lines lines;

			SpaceSkipper<IteratorT> skipper;
			Parser<IteratorT, SpaceSkipperT> grammer;

			std::wstring::const_iterator it = program.begin();
			if (!boost::spirit::qi::phrase_parse(it, program.end(), grammer, skipper, lines))
			{
				//std::cerr << L"Syntax Error: parse failed\n";
				std::cout << L"Syntax Error: parse failed\n";
				return boost::none;
			}

			if (it != program.end())
			{
				//std::cerr << L"Syntax Error: ramains input\n" << std::wstring(it, program.end());
				std::cout << L"Syntax Error: ramains input\n" << std::wstring(it, program.end());
				return boost::none;
			}

			Expr result = lines;
			return result;
		}

		boost::optional<Evaluated> execute(const std::wstring& program)
		{
			if (auto exprOpt = parse(program))
			{
				try
				{
					return pEnv->expand(boost::apply_visitor(evaluator, exprOpt.value()));
				}
				catch (const cgl::Exception& e)
				{
					std::cerr << L"Exception: " << e.what() << std::endl;
				}
			}

			return boost::none;
		}

		bool draw(const std::wstring& program, bool logOutput = true)
		{
			if (logOutput) std::cout << L"parse..." << std::endl;
			
			if (auto exprOpt = parse(program))
			{
				try
				{
					if (logOutput)
					{
						std::cout << L"parse succeeded" << std::endl;
						printExpr(exprOpt.value());
					}

					if (logOutput) std::cout << L"execute..." << std::endl;
					const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
					const Evaluated result = pEnv->expand(lrvalue);
					if (logOutput) std::cout << L"execute succeeded" << std::endl;

					if (logOutput) std::cout << L"output SVG..." << std::endl;
					OutputSVG(std::cout, result, pEnv);
					if (logOutput) std::cout << L"output succeeded" << std::endl;
				}
				catch (const cgl::Exception& e)
				{
					std::cerr << L"Exception: " << e.what() << std::endl;
				}
			}

			return false;
		}

		void execute1(const std::wstring& program, bool logOutput = true)
		{
			clear();

			if (logOutput)
			{
				std::cout << L"parse..." << std::endl;
				std::cout << program << std::endl;
			}

			if (auto exprOpt = parse(program))
			{
				try
				{
					if (logOutput)
					{
						std::cout << L"parse succeeded" << std::endl;
						printExpr(exprOpt.value(), std::cout);
					}

					if (logOutput) std::cout << L"execute..." << std::endl;
					const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
					evaluated = pEnv->expand(lrvalue);
					if (logOutput)
					{
						std::cout << L"execute succeeded" << std::endl;
						printEvaluated(evaluated.value(), pEnv, std::cout, 0);

						std::cout << L"output SVG..." << std::endl;
						std::ofstream file(L"result.svg");
						OutputSVG(file, evaluated.value(), pEnv);
						file.close();
						std::cout << L"completed" << std::endl;
					}

					succeeded = true;
				}
				catch (const cgl::Exception& e)
				{
					//std::cerr << L"Exception: " << e.what() << std::endl;
					std::cout << L"Exception: " << e.what() << std::endl;

					succeeded = false;
				}	
			}
			else
			{
				succeeded = false;
			}

			calculating = false;
		}

		void clear()
		{
			pEnv = Environment::Make();
			evaluated = boost::none;
			evaluator = Eval(pEnv);
			succeeded = false;
		}

		bool test(const std::wstring& program, const Expr& expr)
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

		std::shared_ptr<Environment> getEnvironment()
		{
			return pEnv;
		}

		boost::optional<Evaluated>& getEvaluated()
		{
			return evaluated;
		}

		bool isSucceeded()const
		{
			return succeeded;
		}

	private:
		std::shared_ptr<Environment> pEnv;
		Eval evaluator;
		boost::optional<Evaluated> evaluated;
		bool succeeded;
	};
}
