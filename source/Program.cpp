#include <Eigen/Core>

#include <Pita/Program.hpp>
#include <Pita/Parser.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/Printer.hpp>

#ifdef __has_include
#  if __has_include(<Pita/PitaStd>)
#    define CGL_HAS_STANDARD_FILE
#  endif
#endif

bool calculating;
int constraintViolationCount;

namespace cgl
{
#ifdef CGL_HAS_STANDARD_FILE
	Program::Program() :
		pEnv(Context::Make()),
		evaluator(pEnv),
		isInitialized(true)
	{
		std::cout << "load PitaStd ..." << std::endl;
		std::vector<std::string> pitaStdSplitted({
#include <Pita/PitaStd>
			});

		std::stringstream ss;
		for (const auto& str : pitaStdSplitted)
		{
			ss << str;
		}

		cereal::JSONInputArchive ar(ss);
		Context& context = *pEnv;
		ar(context);
	}
#else
	Program::Program() :
		pEnv(Context::Make()),
		evaluator(pEnv),
		isInitialized(false)
	{}
#endif

	inline std::vector<std::string> SplitStringVSCompatible(const std::string& str)
	{
		std::vector<std::string> result;

		const int divLength = 16380;
		while (result.size() * divLength < str.length())
		{
			const size_t offset = result.size() * divLength;
			result.push_back(std::string(str.begin() + offset, str.begin() + std::min(offset + divLength, str.length())));
		}

		return result;
	}

	void Program::execute1(const std::string& filepath, const std::string& output_filename, bool logOutput)
	{
		clear();

		try
		{
			if (auto exprOpt = Parse1(filepath))
			{
				if (logOutput)
				{
					std::cerr << "parse succeeded" << std::endl;
					//printExpr2(exprOpt.get(), pEnv, std::cerr);
					printExpr(exprOpt.get(), pEnv, std::cerr);
				}

				if (logOutput) std::cerr << "execute ..." << std::endl;
				const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.get());
				evaluated = pEnv->expand(lrvalue, LocationInfo());
				if (logOutput)
				{
					std::cerr << "execute succeeded" << std::endl;
					//printVal(evaluated.get(), pEnv, std::cout, 0);
				}

				if (logOutput)
					std::cerr << "output SVG ..." << std::endl;

				if (output_filename.empty())
				{
					OutputSVG2(std::cout, Packed(evaluated.get(), *pEnv), "shape");
				}
				else
				{
					std::ofstream file(output_filename);
					OutputSVG2(file, Packed(evaluated.get(), *pEnv), "shape");
					file.close();
				}

				if (logOutput)
					std::cerr << "completed" << std::endl;

				succeeded = true;
			}
			else
			{
				std::cout << "Parse failed" << std::endl;
				succeeded = false;
			}
		}
		catch (const cgl::Exception& e)
		{
			if (!errorMessagePrinted)
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
			}

			succeeded = false;
		}
		catch (const std::exception& other)
		{
			std::cerr << "Error: " << other.what() << std::endl;

			succeeded = false;
		}

		calculating = false;
	}

	void Program::executeInline(const std::string& source, bool logOutput)
	{
		clear();

		try
		{
			if (auto exprOpt = ParseFromSourceCode(source))
			{
				if (logOutput)
				{
					std::cerr << "parse succeeded" << std::endl;
					//printExpr2(exprOpt.get(), pEnv, std::cerr);
					printExpr(exprOpt.get(), pEnv, std::cerr);
				}

				if (logOutput) std::cerr << "execute ..." << std::endl;
				const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.get());
				evaluated = pEnv->expand(lrvalue, LocationInfo());
				if (logOutput)
				{
					std::cerr << "execute succeeded" << std::endl;
					//printVal(evaluated.get(), pEnv, std::cout, 0);
				}

				if (logOutput)
					std::cerr << "output SVG ..." << std::endl;

				OutputSVG2(std::cout, Packed(evaluated.get(), *pEnv), "shape");

				if (logOutput)
					std::cerr << "completed" << std::endl;

				succeeded = true;
			}
			else
			{
				std::cout << "Parse failed" << std::endl;
				succeeded = false;
			}
		}
		catch (const cgl::Exception& e)
		{
			if (!errorMessagePrinted)
			{
				std::cerr << e.what() << std::endl;
				if (e.hasInfo)
				{
					PrintErrorPos("empty source", e.info);
				}
				else
				{
					std::cerr << "Exception does not has any location info." << std::endl;
				}
			}

			succeeded = false;
		}
		catch (const std::exception& other)
		{
			std::cerr << "Error: " << other.what() << std::endl;

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
					printExpr(exprOpt.get(), pEnv, std::cout);
				}

				if (logOutput) std::cout << "execute..." << std::endl;
				const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.get());
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
		if (!isInitialized)
		{
			pEnv = Context::Make();
			isInitialized = true;
		}
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

			return IsEqualVal(result.get(), answer);
		}*/

		return false;
	}

	boost::optional<int> Program::asIntOpt()
	{
		if (evaluated)
		{
			if (auto opt = AsOpt<int>(evaluated.get()))
			{
				return opt.get();
			}
		}

		return boost::none;
	}

	boost::optional<double> Program::asDoubleOpt()
	{
		if (evaluated)
		{
			if (auto opt = AsOpt<double>(evaluated.get()))
			{
				return opt.get();
			}
		}

		return boost::none;
	}

	bool Program::preEvaluate(const std::string& input_filename, const std::string& output_filename, bool logOutput)
	{
		clear();

		if (logOutput)
		{
			std::cerr << "Parse \"" << input_filename << "\" ..." << std::endl;
		}

		try
		{
			if (auto exprOpt = Parse1(input_filename))
			{
				if (logOutput)
				{
					std::cerr << "parse succeeded" << std::endl;
					printExpr(exprOpt.get(), pEnv, std::cerr);
				}

				if (logOutput) std::cerr << "execute ..." << std::endl;

				const Expr expr = exprOpt.get();
				if (IsType<Lines>(expr))
				{
					const Lines& lines = As<Lines>(expr);
					pEnv->enterScope();

					Val result;
					for (const auto& expr : lines.exprs)
					{
						result = pEnv->expand(boost::apply_visitor(evaluator, expr), lines);

						if (IsType<Jump>(result))
						{
							break;
						}
					}

					if (logOutput)
					{
						std::cerr << "execute succeeded" << std::endl;
					}

					std::stringstream ss;
					{
						cereal::JSONOutputArchive ar(ss, cereal::JSONOutputArchive::Options::NoIndent());
						Context& context = *pEnv;
						ar(context);
					}
					const auto serializedStr = ss.str();

					std::ofstream ofs(output_filename);
					const auto splittedStr = SplitStringVSCompatible(std::string(serializedStr.begin(), serializedStr.end()));
					for (size_t i = 0; i < splittedStr.size(); ++i)
					{
						ofs << "std::string(R\"(" << splittedStr[i] << ")\")";
						if (i + 1 < splittedStr.size())
						{
							ofs << ',';
						}
					}

					return true;
				}
				else
				{
					std::cout << "Execute failed" << std::endl;
				}
			}
			else
			{
				std::cout << "Parse failed" << std::endl;
			}
		}
		catch (const cgl::Exception& e)
		{
			if (!errorMessagePrinted)
			{
				std::cerr << e.what() << std::endl;
				if (e.hasInfo)
				{
					PrintErrorPos(input_filename, e.info);
				}
				else
				{
					std::cerr << "Exception does not has any location info." << std::endl;
				}
			}
		}
		catch (const std::exception& other)
		{
			std::cerr << "Error: " << other.what() << std::endl;
		}

		return false;
	}
}
