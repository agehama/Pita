
#include <Eigen/Core>

#include <Pita/Program.hpp>
#include <Pita/Parser.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/Printer.hpp>

std::ofstream ofs;
bool calculating;
int constraintViolationCount;

namespace cgl
{
	//boost::optional<Expr> Program::parse(const std::string& program)
	//{
	//	using namespace cgl;

	//	boost::u8_to_u32_iterator<std::string::const_iterator> tbegin(program.begin()), tend(program.end());

	//	Lines lines;

	//	SpaceSkipper<IteratorT> skipper;
	//	Parser<IteratorT, SpaceSkipperT> grammer;

	//	auto it = tbegin;
	//	if (!boost::spirit::qi::phrase_parse(it, tend, grammer, skipper, lines))
	//	{
	//		//std::cerr << "Syntax Error: parse failed\n";
	//		std::cout << "Syntax Error: parse failed\n";
	//		return boost::none;
	//	}

	//	if (it != tend)
	//	{
	//		//std::cout << "Syntax Error: ramains input\n" << std::string(it, program.end());
	//		std::cout << "Syntax Error: ramains input\n";
	//		return boost::none;
	//	}

	//	Expr result = lines;
	//	return result;
	//}

	/*
	boost::optional<Val> Program::execute(const std::string& program)
	{
		if (auto exprOpt = Parse(program))
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

		if (auto exprOpt = Parse(program))
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
				const Val result = pEnv->expand(lrvalue);
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
	*/

	//void Program::execute1(const std::string& program, const std::string& output_filename, bool logOutput)
	void Program::execute1(const std::string& filepath, const std::string& output_filename, bool logOutput)
	{
		clear();

		if (logOutput)
		{
			std::cerr << "Parse \"" << filepath << "\" ..." << std::endl;
		}

		if (auto exprOpt = Parse1(filepath))
		{
			try
			{
				if (logOutput)
				{
					std::cerr << "parse succeeded" << std::endl;
					//printExpr2(exprOpt.value(), pEnv, std::cerr);
					printExpr(exprOpt.value(), pEnv, std::cerr);
				}

				if (logOutput) std::cerr << "execute..." << std::endl;
				const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
				evaluated = pEnv->expand(lrvalue, LocationInfo());
				if (logOutput)
				{
					std::cerr << "execute succeeded" << std::endl;
					//printVal(evaluated.value(), pEnv, std::cout, 0);
				}

				if (logOutput)
					std::cerr << "output SVG..." << std::endl;

				if (output_filename.empty())
				{
					OutputSVG2(std::cout, Packed(evaluated.value(), *pEnv), "shape");
				}
				else
				{
					std::ofstream file(output_filename);
					OutputSVG2(file, Packed(evaluated.value(), *pEnv), "shape");
					file.close();
				}

				if (logOutput)
				std::cerr << "completed" << std::endl;

				succeeded = true;
			}
			catch (const cgl::Exception& e)
			{
				std::cerr << e.what() << std::endl;
				if (e.hasInfo)
				{
					PrintErrorPos(filepath, e.info);
				}
				else
				{
					std::cerr << "Exception does not has any location info." << std::endl;
				}

				succeeded = false;
			}
			catch (const std::exception& other)
			{
				std::cerr << "Error: " << other.what() << std::endl;

				succeeded = false;
			}
		}
		else
		{
			succeeded = false;
		}

		calculating = false;
	}

	/*
	void Program::run(const std::string& program, bool logOutput)
	{
		clear();

		if (logOutput)
		{
			std::cout << "parse..." << std::endl;
			std::cout << program << std::endl;
		}

		if (auto exprOpt = Parse(program))
		{
			try
			{
				if (logOutput)
				{
					std::cout << "parse succeeded" << std::endl;
					printExpr(exprOpt.value(), pEnv, std::cout);
				}

				if (logOutput) std::cout << "execute..." << std::endl;
				const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
				evaluated = pEnv->expand(lrvalue);
				if (logOutput) std::cout << "completed" << std::endl;

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
	*/

	void Program::clear()
	{
		pEnv = Context::Make();
		evaluated = boost::none;
		evaluator = Eval(pEnv);
		succeeded = false;
	}

	bool Program::test(const std::string& input_filepath, const Expr& expr)
	{
		clear();

		/*if (auto result = execute1(input_filepath, "", false))
		{
			std::shared_ptr<Context> pEnv2 = Context::Make();
			Eval evaluator2(pEnv2);

			const Val answer = pEnv->expand(boost::apply_visitor(evaluator2, expr));

			return IsEqualVal(result.value(), answer);
		}*/

		return false;
	}

	boost::optional<int> Program::asIntOpt()
	{
		if (evaluated)
		{
			if (auto opt = AsOpt<int>(evaluated.value()))
			{
				return opt.value();
			}
		}

		return boost::none;
	}

	boost::optional<double> Program::asDoubleOpt()
	{
		if (evaluated)
		{
			if (auto opt = AsOpt<double>(evaluated.value()))
			{
				return opt.value();
			}
		}

		return boost::none;
	}
}
