#define NOMINMAX
#define _CRTDBG_MAP_ALLOC

#pragma warning(disable:4996)

#include <iostream>
#include <fstream>

#ifdef USE_CURSES
#include <signal.h>
#include <string.h>
#include <curses.h>
#include <stdlib.h>
#pragma comment(lib, "pdcurses.lib")
#endif

#ifdef _MSC_VER
#include <crtdbg.h> // for _CrtDumpMemoryLeaks
static void MEM_Init()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_DELAY_FREE_MEM_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
}
#else
static void MEM_Init() {}
#endif

#include <stdio.h>
#include <time.h>

#include <cxxopts.hpp>
#include <Pita/Program.hpp>
#include <Pita/Parser.hpp>

#include <Pita/Node.hpp>
#include <Pita/Printer.hpp>
#include <Pita/TreeLogger.hpp>

extern bool calculating;
extern bool isDebugMode;
extern bool isBlockingMode;
extern bool isContextFreeMode;
extern bool isDumpParseTree;

#ifdef USE_CURSES
/* Trap interrupt */
void trap(int sig)
{
	if (sig == SIGINT)
	{
		endwin();
		exit(0);
	}
}
#endif

int main(int argc, char* argv[])
{
	std::string input_file, output_file;

	MEM_Init();

	try
	{
		cxxopts::Options options("pita", "Pita - Constraint design language");
		options
			.positional_help("[input_path]")
			.show_positional_help();

		options.add_options()
			("i,input", "Input file path", cxxopts::value<std::string>())
			("o,output", "Output file path (if unspecified, result is printed to standard output)", cxxopts::value<std::string>())
			("d,debug", "Enable debug mode")
			("b,block", "Enable blocking mode")
			("h,help", "Show help")
			("licence", "Show licence")
			("logHTML", "Output log.html")
			("contextFree", "Enable Context Free Mode(experimental)")
			("preEvaluation", "Output eval result as JSON format (for PitaStd generation)")
			("dumpParseTree", "Dump parse tree")
			//("seed", "Random seed (if unspecified, seed is generated non-deterministically)", cxxopts::value<int>(), "N")
			//("constraint-timeout", "Set timeout seconds of each constraint solving", cxxopts::value<int>(), "N")
			;

		options.parse_positional({ "input" });

		auto result = options.parse(argc, argv);

		if(result.count("help"))
		{
			std::cout << options.help() << std::endl;
			return 0;
		}

		if (result.count("licence"))
		{
			const auto line = R"(
========================================================================
)";

			std::cout << line <<
#include <Pita/Licences/CppNumericalSolvers>
				;

			std::cout << line <<
#include <Pita/Licences/Eigen>
				;

			std::cout << line <<
#include <Pita/Licences/OpenSiv3D>
				;

			std::cout << line <<
#include <Pita/Licences/cereal>
				;

			std::cout << line <<
#include <Pita/Licences/cxxopts>
				;
			
			std::cout << line <<
#include <Pita/Licences/geos_1>
				<<
#include <Pita/Licences/geos_2>
				<<
#include <Pita/Licences/geos_3>
				<<
#include <Pita/Licences/geos_4>
				<<
#include <Pita/Licences/geos_5>
				<<
#include <Pita/Licences/geos_6>
				<<
#include <Pita/Licences/geos_7>
				<<
#include <Pita/Licences/geos_8>
				<<
#include <Pita/Licences/geos_9>
				<<
#include <Pita/Licences/geos_10>
				<<
#include <Pita/Licences/geos_11>
				<<
#include <Pita/Licences/geos_12>
				<<
#include <Pita/Licences/geos_13>
				<<
#include <Pita/Licences/geos_14>
				;

			std::cout << line <<
#include <Pita/Licences/libcmaes_1>
				<<
#include <Pita/Licences/libcmaes_2>
				<<
#include <Pita/Licences/libcmaes_3>
				<<
#include <Pita/Licences/libcmaes_4>
				<<
#include <Pita/Licences/libcmaes_5>
				;

			return 0;
		}

		if (result.count("input"))
		{
			input_file = result["input"].as<std::string>();
		}
		else
		{
			std::cout << "Input file is not specified.\n" << std::endl;
			std::cout << options.help() << std::endl;
			return 0;
		}

		if (result.count("output"))
		{
			output_file = result["output"].as<std::string>();
		}

		isDebugMode = false;
		isBlockingMode = false;
		isContextFreeMode = false;
		if (result.count("debug"))
		{
			isDebugMode = true;
		}
		if (result.count("block"))
		{
			isBlockingMode = true;
		}
		if (result.count("contextFree"))
		{
			isContextFreeMode = true;
		}
		if (result.count("dumpParseTree"))
		{
			isDumpParseTree = true;
		}

		if (result.count("logHTML"))
		{
			cgl::TreeLogger::instance().logStart("log.html");
		}

#ifdef USE_CURSES
		if(isDebugMode && isBlockingMode)
		{
#ifdef XCURSES
			Xinitscr(argc, argv);
#else
			initscr();
#endif

			start_color();
# if defined(NCURSES_VERSION) || (defined(PDC_BUILD) && PDC_BUILD > 3000)
			use_default_colors();
# endif
			cbreak();
			noecho();

			curs_set(0);

#if !defined(__TURBOC__) && !defined(OS2)
			signal(SIGINT, trap);
#endif
			noecho();

			/* refresh stdscr so that reading from it will not cause it to
			overwrite the other windows that are being created */

			refresh();

			const int PositiveColor = 1;
			const int NegativeColor = 2;
			init_pair(PositiveColor, COLOR_WHITE, COLOR_BLACK);
			init_pair(NegativeColor, COLOR_BLACK, COLOR_WHITE);

			bkgd(COLOR_PAIR(PositiveColor));

			int xpos = 0;

			/*for (;;)
			{
				clear();

				++xpos;
				attrset(COLOR_PAIR(PositiveColor));
				mvprintw(3, xpos, "TestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTestTest");

				attrset(COLOR_PAIR(NegativeColor));
				mvprintw(3, 9, "TestTestTest");

				refresh();
				napms(100);
			}*/
		}
#endif
		

		/*
		if (result.count("seed"))
		{
			//result["seed"].as<int>();
			std::cout << "TODO: Set seed\n" << std::endl;
		}

		if (result.count("constraint-timeout"))
		{
			//result["constraint-timeout"].as<int>();
			std::cout << "TODO: Set timeout\n" << std::endl;
		}
		*/

		//ofs.open("log.txt");

		calculating = true;
		cgl::Program program;

		if (result.count("preEvaluation"))
		{
			isDebugMode = true;
			const bool flag = program.preEvaluate(input_file, output_file);
			std::cout << std::boolalpha << "Result: " << flag << std::endl;
		}
		else
		{
			program.execute1(input_file, output_file, !isDebugMode);
		}

		/*if (isDebugMode)
		{
			endwin();
		}*/
	}
	catch (const cxxopts::OptionException& e)
	{
		std::cerr << "Error parsing options: " << e.what() << std::endl;
		exit(1);
	}

	return 0;
}
