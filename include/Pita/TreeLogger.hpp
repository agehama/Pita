#pragma once
#pragma warning(disable:4996)
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <exception>
#include <chrono>
#include <stack>

namespace cgl
{
	class TreeLogger
	{
	public:
		TreeLogger() = default;
		~TreeLogger()
		{
			logFinish();
		}

		static TreeLogger& instance()
		{
			static TreeLogger logger;
			return logger;
		}

		void push(const std::string& message)
		{
			if (ofs.is_open())
			{
				messages.push(message);
				ofs << std::string("<details><summary>") + escaped(message) + "</summary>\n<div style=\"padding-left:1em\">\n";
			}
		}

		void pop()
		{
			if (ofs.is_open())
			{
				messages.pop();
				ofs << "</div>\n</details>\n";
			}
		}

		void write(const std::string& message)
		{
			if (ofs.is_open())
			{
				ofs << std::string("<p>") + escaped(message) + "</p>\n";
			}
		}

		void logStart(const std::string& filePath)
		{
			logFinish();

			ofs.open(filePath);
			printHead();
		}

		void logFinish()
		{
			if (ofs.is_open())
			{
				while (!messages.empty())
				{
					pop();
				}

				printTail();
				ofs.close();
			}
		}

		bool isEnable()const { return ofs.is_open(); }

	private:
		void printHead();
		void printTail();
		std::string escaped(const std::string& str);

		std::ofstream ofs;
		std::stack<std::string> messages;
	};

	struct ScopeLog
	{
		ScopeLog() = delete;

		ScopeLog(const std::string& message)
		{
			TreeLogger::instance().push(message);
		}

		~ScopeLog()
		{
			TreeLogger::instance().pop();
		}

		void write(const std::string& message)
		{
			TreeLogger::instance().write(message);
		}
	};
}
