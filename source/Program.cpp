#pragma warning(disable:4996)
#pragma comment(lib, "pdcurses.lib")

#include <signal.h>
#include <string.h>
#include <curses.h>
#include <stdlib.h>
#include <time.h>

#include <future>
#include <thread>
#include <chrono>

#include <Eigen/Core>

#include <Pita/Program.hpp>
#include <Pita/Parser.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/Printer.hpp>

//#include <glog/logging.h>
//#ifdef _DEBUG
////#  pragma comment(lib, "Debug/gflags_static.lib")
//#  pragma comment(lib, "Debug/glogd.lib")
//#else
////#  pragma comment(lib, "Release/gflags_static.lib")
//#  pragma comment(lib, "Release/glog.lib")
//#endif
//#pragma comment(lib, "shlwapi.lib")


#ifdef _DEBUG
#  pragma comment(lib, "Debug/StackWalker.lib")
#else
#  pragma comment(lib, "Release/StackWalker.lib")
#endif

#ifdef __has_include
#  if __has_include(<Pita/PitaStd>)
#    define CGL_HAS_STANDARD_FILE
#  endif
#endif

bool calculating;
int constraintViolationCount;
bool isDebugMode;
bool isBlockingMode;
bool isInConstraint = false;

bool printAddressInsertion = false;

namespace cgl
{
	struct LinePos
	{
		int x, y;
		LinePos() = default;
		LinePos(int x, int y) :x(x), y(y) {}
	};

	struct ScreenPos
	{
		int x, y;
		ScreenPos() = default;
		ScreenPos(int x, int y) :x(x), y(y) {}
	};

	LinePos currentBeginPos;
	LinePos currentEndPos;

	void UpdateCurrentLocation(const LocationInfo& info)
	{
		if (info.isValid())
		{
			currentBeginPos = LinePos(info.locInfo_posBegin, info.locInfo_lineBegin);
			currentEndPos = LinePos(info.locInfo_posEnd, info.locInfo_lineEnd);

			if (isDebugMode && isBlockingMode && !isInConstraint)
			{
				while (true)
				{
					if ('n' == getchar())
					{
						break;
					}
				}
			}
		}
	}

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

		/*google::InitGoogleLogging("pita");
		google::InstallFailureSignalHandler();*/

		_set_se_translator(TranslateInternalException);
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
		clearState();

		try
		{
			double parseSec = 0;
			double executeSec = 0;
			double outputSec = 0;

			const double parseBegin = GetSec();
			if (auto exprOpt = Parse1(filepath))
			{
				parseSec = GetSec() - parseBegin;

				if (logOutput)
				{
					std::cerr << "parse succeeded" << std::endl;
					//printExpr2(exprOpt.get(), pEnv, std::cerr);
					printExpr(exprOpt.get(), pEnv, std::cerr);
				}

				const auto executeAndOutputSVG = [&]() {
					if (logOutput) std::cerr << "execute ..." << std::endl;

					//_set_se_translator(TranslateInternalException);

					const double executeBegin = GetSec();
					const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.get());
					executeSec = GetSec() - executeBegin;

					evaluated = pEnv->expand(lrvalue, LocationInfo());
					if (logOutput)
					{
						std::cerr << "execute succeeded" << std::endl;
						//printVal(evaluated.get(), pEnv, std::cout, 0);
					}

					if (logOutput)
						std::cerr << "output SVG ..." << std::endl;

					const double outputBegin = GetSec();
					if (output_filename.empty())
					{
						OutputSVG2(std::cout, Packed(evaluated.get(), *pEnv), "shape", pEnv);
					}
					else
					{
						std::ofstream file(output_filename);
						OutputSVG2(file, Packed(evaluated.get(), *pEnv), "shape", pEnv);
						file.close();
					}
					outputSec = GetSec() - outputBegin;

					if (logOutput)
					{
						std::cerr << "completed" << std::endl;
					}

					std::cerr << "parse     : " << parseSec << "[sec]" << std::endl;
					std::cerr << "execute   : " << executeSec << "[sec]" << std::endl;
					std::cerr << "output    : " << outputSec << "[sec]" << std::endl;

					succeeded = true;
				};

				const int windowWidth = COLS;
				const int windowHeight = LINES;

				const auto LinePosToAbsolutePos = [](const std::vector<std::string>& lines, const LinePos& pos, int windowWidth)
				{
					if (lines.size() <= pos.y)
					{
						return ScreenPos(-1, -1);
					}

					ScreenPos currentPos(0, 0);
					for (int ly = 0; ly < pos.y; ++ly)
					{
						currentPos.y += (std::max(0, static_cast<int>(lines[ly].length()) - 1) / windowWidth) + 1;
					}

					currentPos.y += pos.x / windowWidth;
					currentPos.x = pos.x % windowWidth;

					return currentPos;
				};

				const auto AbsolutePosToScreenPos = [](const ScreenPos& pos, int focusLine, int windowHeight)
				{
					const int focusLineBegin = std::max(focusLine - windowHeight / 2, 0);
					return ScreenPos(pos.x, pos.y - focusLineBegin);
				};

				const auto LinePosToScreenPos = [&](const std::vector<std::string>& lines, const LinePos& pos, int windowWidth, int windowHeight, int focusLine)
				{
					return AbsolutePosToScreenPos(LinePosToAbsolutePos(lines, pos, windowWidth), focusLine, windowHeight);
				};

				const auto splitLines = [](std::ifstream& ifs)->std::vector<std::string>
				{
					std::stringstream tabRemovedStr;
					char c;
					while (ifs.get(c))
					{
						if (c == '\t')
							tabRemovedStr << "    ";
						else
							tabRemovedStr << c;
					}
					std::stringstream is;
					is << tabRemovedStr.str();

					std::vector<std::string> result;
					std::string line;
					while (std::getline(is, line))
					{
						result.push_back(line);
					}

					return result;
				};

				int focusLine = 0;
				const auto draw = [&](const std::vector<std::string>& lines, const LinePos& focusBegin, const LinePos& focusEnd)
				{
					int locInfo_lineBegin = focusBegin.y, locInfo_lineEnd = focusEnd.y;
					int locInfo_posBegin = focusBegin.x, locInfo_posEnd = focusEnd.x;

					if (locInfo_lineBegin != 1)
					{
						locInfo_posBegin = std::max(static_cast<int>(locInfo_posBegin) - 1, 0);
					}
					if (locInfo_lineEnd != 1)
					{
						locInfo_posEnd = std::max(static_cast<int>(locInfo_posEnd) - 1, 0);
					}

					const int focusLineBegin = std::max(static_cast<int>(locInfo_lineBegin) - 1, 0);
					const int focusLineEnd = std::max(static_cast<int>(locInfo_lineEnd) - 1, 0);

					while(true)
					{
						const auto focusBeginDrawPos = LinePosToScreenPos(lines, LinePos(locInfo_posBegin, focusLineBegin), windowWidth, windowHeight, focusLine);

						if (focusBeginDrawPos.y < 0)
						{
							--focusLine;
						}
						else if(windowHeight <= focusBeginDrawPos.y)
						{
							++focusLine;
						}
						else
						{
							break;
						}
					}

					const auto searchTargetLineY = [&](const ScreenPos& targetPos)->int
					{
						LinePos left(0, 0), right(0, static_cast<int>(lines.size()) - 1);
						{
							const auto leftResult = LinePosToScreenPos(lines, left, windowWidth, windowHeight, focusLine);
							if (targetPos.y <= leftResult.y)
							{
								return left.y;
							}
							const auto rightResult = LinePosToScreenPos(lines, right, windowWidth, windowHeight, focusLine);
							if (rightResult.y <= targetPos.y)
							{
								return right.y;
							}
						}

						while (true)
						{
							const LinePos center(0, (left.y + right.y) / 2);
							const auto result = LinePosToScreenPos(lines, center, windowWidth, windowHeight, focusLine);
							if (result.y == targetPos.y)
							{
								return center.y;
							}
							else if (result.y < targetPos.y)
							{
								left = center;
							}
							else
							{
								right = center;
							}

							if (left.y == right.y)
							{
								return left.y;
							}
						}
					};

					const int printLineBegin = searchTargetLineY(ScreenPos(0, 0));
					const int printLineEnd = searchTargetLineY(ScreenPos(0, windowHeight - 1));

					const int PositiveColor = 1;
					const int NegativeColor = 2;

					attrset(COLOR_PAIR(PositiveColor));
					for (int ly = printLineBegin; ly <= printLineEnd; ++ly)
					{
						LinePos currentLinePos(0, ly);
						const auto drawPos = LinePosToScreenPos(lines, currentLinePos, windowWidth, windowHeight, focusLine);
						mvprintw(drawPos.y, drawPos.x, lines[ly].c_str());
					}

					attrset(COLOR_PAIR(NegativeColor));
					if (focusLineBegin == focusLineEnd)
					{
						LinePos currentLinePos(locInfo_posBegin, focusLineBegin);
						const auto str = lines[focusLineBegin].substr(locInfo_posBegin, locInfo_posEnd - locInfo_posBegin);
						const auto drawPos = LinePosToScreenPos(lines, currentLinePos, windowWidth, windowHeight, focusLine);
						mvprintw(drawPos.y, drawPos.x, str.c_str());
					}
					else
					{
						{
							int ly = focusLineBegin;
							LinePos currentLinePos(locInfo_posBegin, ly);
							const auto str = std::string(lines[ly].begin() + locInfo_posBegin, lines[ly].end());
							const auto drawPos = LinePosToScreenPos(lines, currentLinePos, windowWidth, windowHeight, focusLine);
							mvprintw(drawPos.y, drawPos.x, str.c_str());
						}
						for (int ly = focusLineBegin + 1; ly < focusLineEnd; ++ly)
						{
							LinePos currentLinePos(0, ly);
							const auto drawPos = LinePosToScreenPos(lines, currentLinePos, windowWidth, windowHeight, focusLine);
							mvprintw(drawPos.y, drawPos.x, lines[ly].c_str());
						}
						{
							int ly = focusLineEnd;
							LinePos currentLinePos(0, ly);
							const auto str = std::string(lines[ly].begin(), lines[ly].begin() + locInfo_posEnd);
							const auto drawPos = LinePosToScreenPos(lines, currentLinePos, windowWidth, windowHeight, focusLine);
							mvprintw(drawPos.y, drawPos.x, str.c_str());
						}
					}
				};

				if (isDebugMode)
				{
					using namespace std::chrono_literals;

					std::ifstream ifs(filepath);
					const auto lines = splitLines(ifs);

					auto future = std::async(std::launch::async, executeAndOutputSVG);

					const int PositiveColor = 1;
					const int NegativeColor = 2;

					int xpos = 0;

					while(true)
					{
						auto status = future.wait_for(0ms);
						if (status == std::future_status::ready && (!isBlockingMode || 'q' == getchar()))
						{
							break;
						}

						clear();

						draw(lines, currentBeginPos, currentEndPos);

						refresh();
						napms(10);
					}
				}
				else
				{
					executeAndOutputSVG();
				}
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
			std::cerr << "Program Error: " << other.what() << std::endl;

			succeeded = false;
			throw;
		}

		calculating = false;
	}

	void Program::executeInline(const std::string& source, bool logOutput)
	{
		clearState();

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

				OutputSVG2(std::cout, Packed(evaluated.get(), *pEnv), "shape", pEnv);

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
					PrintErrorPosSource(source, e.info);
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

			throw;
		}

		calculating = false;
	}

	void Program::clearState()
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
		clearState();

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
		clearState();

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

					std::string reducedStr;
					for (char c : ss.str())
					{
						if (c != '\n' && c != '\r' && c != '\t' && c != ' ')
						{
							reducedStr.push_back(c);
						}
					}

					std::ofstream ofs(output_filename);
					const auto splittedStr = SplitStringVSCompatible(std::string(reducedStr.begin(), reducedStr.end()));
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
