#pragma once
#pragma warning(disable:4996)
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <exception>
#include <chrono>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

namespace cgl
{
	struct LocationInfo
	{
		unsigned locInfo_lineBegin = 0, locInfo_lineEnd = 0;
		unsigned locInfo_posBegin = 0, locInfo_posEnd = 0;
		std::string getInfo() const;

		bool isValid()const
		{
			return !(locInfo_lineBegin == 0 && locInfo_lineEnd == 0 && locInfo_posBegin == 0 && locInfo_posEnd == 0);
		}
	};

	class Exception : public std::exception
	{
	public:
		explicit Exception(const std::string& message) :message(message) {}
		Exception(const std::string& message, const LocationInfo& info) :message(message), info(info), hasInfo(true) {}
		const char* what() const noexcept override { return message.c_str(); }

		LocationInfo info;
		bool hasInfo = false;

	private:
		std::string message;
	};

	class InternalException : public std::exception
	{
	public:
		InternalException() = default;
		InternalException(unsigned int exceptionCode, EXCEPTION_POINTERS* exceptionPointers)
			:exceptionCode(exceptionCode), exceptionPointers(exceptionPointers)
		{
			PEXCEPTION_RECORD p = exceptionPointers->ExceptionRecord;

			/*std::stringstream ss;
			ss << "Internal Error:\n";
			ss << "Exception Address: " << static_cast<void*>(p->ExceptionAddress) << "\n";

			ss << std::hex << std::setfill('0') << std::setw(8);
			ss << "Exception Code:    " << p->ExceptionCode << "\n";
			ss << "Exception Flags:   " << p->ExceptionFlags << "\n";*/

			std::cout << "Internal Error:" << std::endl;
			std::cout << "Exception Address: " << static_cast<void*>(p->ExceptionAddress) << std::endl;

			std::cout << std::hex << std::setfill('0') << std::setw(8);
			std::cout << "Exception Code:    " << p->ExceptionCode << std::endl;
			std::cout << "Exception Flags:   " << p->ExceptionFlags << std::endl;
		}

		const char* what() const noexcept override
		{
			return message.c_str();
		}

	private:
		unsigned int exceptionCode;
		EXCEPTION_POINTERS* exceptionPointers;
		std::string message;
	};

	inline void Log(std::ostream& os, const std::string& str)
	{
		std::regex regex("\n");
		os << std::regex_replace(str, regex, "\n          |> ") << "\n";
	}

	inline double GetSec() {
		return static_cast<double>(
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()
			) / 1000000000.0;
	}

	inline void TranslateInternalException(unsigned int ExceptionCode, PEXCEPTION_POINTERS ExceptionPointers) 
	{
		throw InternalException(ExceptionCode, ExceptionPointers);
	}
}

//extern std::ofstream ofs;

#define CGL_FileName (strchr(__FILE__, '\\') ? strchr(__FILE__, '\\') + 1 : __FILE__)
#define CGL_FileDesc (std::string(CGL_FileName) + "(" + std::to_string(__LINE__) + ")")
#define CGL_TagError (std::string("[Error]   |> "))
#define CGL_TagWarn  (std::string("[Warning] |> "))
#define CGL_TagDebug (std::string("[Debug]   |> "))

#define OutputFileDesc

#ifdef OutputFileDesc
#define CGL_Error(message) (throw cgl::Exception(std::string("Error") + message + " | " + CGL_FileDesc))
#define CGL_ErrorInternal(message) (throw cgl::Exception(std::string("Error") + std::string("内部エラー: ") + message + " | " + CGL_FileDesc))
#define CGL_ErrorNode(info, message) (throw cgl::Exception(std::string("Error") + info.getInfo() + ": " + message + " | " + CGL_FileDesc, info))
#define CGL_ErrorNodeInternal(info, message) (throw cgl::Exception(std::string("Error") + std::string("内部エラー: ") + message + ", " + info.getInfo() + " | " + CGL_FileDesc, info))
#else
#define CGL_Error(message) (throw cgl::Exception(std::string("Error") + message))
#define CGL_ErrorInternal(message) (throw cgl::Exception(std::string("Error") + std::string("内部エラー: ") + message))
#define CGL_ErrorNode(info, message) (throw cgl::Exception(std::string("Error") + info.getInfo() + ": " + message, info))
#define CGL_ErrorNodeInternal(info, message) (throw cgl::Exception(std::string("Error") + std::string("内部エラー: ") + message + ", " + info.getInfo(), info))
#endif

#define CGL_DBG (std::cout << std::string(CGL_FileName) << " - "<< __FUNCTION__ << "(): " << __LINE__ << std::endl)
#define CGL_DBG1(message) (std::cout << message << " | " << std::string(CGL_FileName) << " - "<< __FUNCTION__ << "(): " << __LINE__ << std::endl)

#ifdef CGL_EnableLogOutput
#define CGL_ErrorLog(message) (cgl::Log(std::cerr, CGL_TagError + CGL_FileDesc + message))
#define CGL_WarnLog(message)  (cgl::Log(std::cerr, CGL_TagWarn  + CGL_FileDesc + message))
//#define CGL_DebugLog(message) (cgl::Log(std::cout, CGL_TagDebug + CGL_FileDesc + message))
#define CGL_DebugLog(message) (cgl::Log(ofs, CGL_TagDebug + CGL_FileDesc + message))
#else
#define CGL_ErrorLog(message) 
#define CGL_WarnLog(message)  
#define CGL_DebugLog(message) 
#endif


namespace cgl
{
	namespace msgs
	{
		inline std::string Undefined(const std::string& name)
		{
			return std::string() + "識別子\"" + name + "\"が定義されていません。";
		}
	}
	
}