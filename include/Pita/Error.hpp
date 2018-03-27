#pragma once
#pragma warning(disable:4996)
#include <iostream>
#include <fstream>
#include <string>
#include <exception>

namespace cgl
{
	struct LocationInfo {
		unsigned locInfo_lineBegin = 0, locInfo_lineEnd = 0;
		unsigned locInfo_posBegin = 0, locInfo_posEnd = 0;
		std::string getInfo() const;
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

	inline void Log(std::ostream& os, const std::string& str)
	{
		std::regex regex("\n");
		os << std::regex_replace(str, regex, "\n          |> ") << "\n";
	}
}

extern std::ofstream ofs;

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