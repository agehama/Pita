#pragma once
#include <iomanip>
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	template<typename T>
	inline std::string ToS(T str)
	{
		return std::to_string(str);
	}

	template<typename T>
	inline std::string ToS(T str, int precision)
	{
		std::ostringstream os;
		os << std::setprecision(precision) << str;
		return os.str();
	}

	template<>
	inline std::string ToS<Eigen::Vector2d>(Eigen::Vector2d v)
	{
		return std::string("(") + std::to_string(v.x()) + ", " + std::to_string(v.y()) + ")";
	}

	class ValuePrinter : public boost::static_visitor<void>
	{
	public:
		ValuePrinter(std::shared_ptr<Context> pEnv, std::ostream& os, int indent, const std::string& header = "") :
			pEnv(pEnv),
			os(os),
			m_indent(indent),
			m_header(header)
		{}

		std::shared_ptr<Context> pEnv;
		int m_indent;
		std::ostream& os;
		mutable std::string m_header;
		
		std::string indent()const;

		void operator()(bool node)const;

		void operator()(int node)const;

		void operator()(double node)const;

		void operator()(const CharString& node)const;

		void operator()(const List& node)const;

		void operator()(const KeyValue& node)const;

		void operator()(const Record& node)const;

		void operator()(const FuncVal& node)const;

		void operator()(const Jump& node)const;
	};

	class ValuePrinter2 : public boost::static_visitor<void>
	{
	public:
		ValuePrinter2(std::shared_ptr<Context> pEnv, std::ostream& os, int indent, const std::string& header = "") :
			pEnv(pEnv),
			os(os),
			m_indent(indent),
			m_header(header)
		{}

		std::shared_ptr<Context> pEnv;
		int m_indent;
		std::ostream& os;
		mutable std::string m_header;

		std::string indent()const;
		std::string footer()const;
		std::string delimiter()const;

		void operator()(bool node)const;

		void operator()(int node)const;

		void operator()(double node)const;

		void operator()(const CharString& node)const;

		void operator()(const List& node)const;

		void operator()(const KeyValue& node)const;

		void operator()(const Record& node)const;

		void operator()(const FuncVal& node)const;

		void operator()(const Jump& node)const;
	};

	inline void printVal(const Val& evaluated, std::shared_ptr<Context> pEnv, std::ostream& os, int indent = 0)
	{
		ValuePrinter printer(pEnv, os, indent);
		boost::apply_visitor(printer, evaluated);
	}

	inline void printVal2(const Val& evaluated, std::shared_ptr<Context> pEnv, std::ostream& os, int indent = 0)
	{
		ValuePrinter2 printer(pEnv, os, indent);
		boost::apply_visitor(printer, evaluated);
	}

#ifdef CGL_EnableLogOutput
	inline void printVal(const Val& evaluated, std::shared_ptr<Context> pEnv, int indent = 0)
	{
		printVal(evaluated, pEnv, ofs, indent);
	}
#else
	inline void printVal(const Val& evaluated, std::shared_ptr<Context> pEnv, int indent = 0) {}
#endif

	class Printer : public boost::static_visitor<void>
	{
	public:
		Printer(std::shared_ptr<Context> pEnv, std::ostream& os, int indent = 0) :
			pEnv(pEnv),
			os(os),
			m_indent(indent)
		{}

		std::shared_ptr<Context> pEnv;
		std::ostream& os;
		int m_indent;

		std::string indent()const;

		void operator()(const LRValue& node)const;

		void operator()(const Identifier& node)const;

		void operator()(const Import& node)const;

		void operator()(const UnaryExpr& node)const;

		void operator()(const BinaryExpr& node)const;

		void operator()(const DefFunc& defFunc)const;

		void operator()(const Range& range)const;

		void operator()(const Lines& statement)const;

		void operator()(const If& if_statement)const;
		
		void operator()(const For& forExpression)const;

		void operator()(const Return& return_statement)const;

		void operator()(const ListConstractor& listConstractor)const;

		void operator()(const KeyExpr& keyExpr)const;

		void operator()(const RecordConstractor& recordConstractor)const;

		void operator()(const DeclSat& node)const;

		void operator()(const DeclFree& node)const;

		void operator()(const Accessor& accessor)const;
	};

	class Printer2 : public boost::static_visitor<void>
	{
	public:
		Printer2(std::shared_ptr<Context> pEnv, std::ostream& os, int indent = 0, const std::string& header = "") :
			pEnv(pEnv),
			os(os),
			m_indent(indent),
			m_header(header)
		{}

		std::shared_ptr<Context> pEnv;
		std::ostream& os;
		int m_indent;
		mutable std::string m_header;

		std::string indent()const;
		std::string footer()const;
		std::string delimiter()const;

		void operator()(const LRValue& node)const;

		void operator()(const Identifier& node)const;

		void operator()(const Import& node)const;

		void operator()(const UnaryExpr& node)const;

		void operator()(const BinaryExpr& node)const;

		void operator()(const DefFunc& defFunc)const;

		void operator()(const Range& range)const;

		void operator()(const Lines& statement)const;

		void operator()(const If& if_statement)const;

		void operator()(const For& forExpression)const;

		void operator()(const Return& return_statement)const;

		void operator()(const ListConstractor& listConstractor)const;

		void operator()(const KeyExpr& keyExpr)const;

		void operator()(const RecordConstractor& recordConstractor)const;

		void operator()(const DeclSat& node)const;

		void operator()(const DeclFree& node)const;

		void operator()(const Accessor& accessor)const;
	};

	inline void printExpr(const Expr& expr, std::shared_ptr<Context> pEnv, std::ostream& os)
	{
		os << "PrintExpr(\n";
		boost::apply_visitor(Printer(pEnv, os), expr);
		os << ") " << std::endl;
	}

	inline void printExpr2(const Expr& expr, std::shared_ptr<Context> pEnv, std::ostream& os)
	{
		boost::apply_visitor(Printer2(pEnv, os), expr);
		os << std::endl;
	}

#ifdef CGL_EnableLogOutput
	inline void printExpr(const Expr& expr)
	{
		printExpr(expr, ofs);
	}
#else
	inline void printExpr(const Expr& expr) {}
#endif

	inline std::string exprVal(const Expr& expr, std::shared_ptr<Context> pEnv)
	{
		std::stringstream ss;
		printExpr(expr, pEnv, ss);
		return ss.str();
	}

	inline std::string exprStr2(const Expr& expr, std::shared_ptr<Context> pEnv)
	{
		std::stringstream ss;
		printExpr2(expr, pEnv, ss);
		return ss.str();
	}

	inline std::string valStr(const Val& val, std::shared_ptr<Context> pEnv)
	{
		std::stringstream ss;
		printVal(val, pEnv, ss);
		return ss.str();
	}

	inline std::string valStr2(const Val& val, std::shared_ptr<Context> pEnv)
	{
		std::stringstream ss;
		printVal2(val, pEnv, ss);
		return ss.str();
	}
}
