#pragma once
#include <iomanip>
#include "Node.hpp"
#include "Environment.hpp"

extern std::wofstream ofs;

namespace cgl
{
	template<typename T>
	inline std::wstring ToS(T str)
	{
		return std::to_string(str);
	}

	template<typename T>
	inline std::wstring ToS(T str, int precision)
	{
		std::ostringstream os;
		os << std::setprecision(precision) << str;
		return os.str();
	}

	class ValuePrinter : public boost::static_visitor<void>
	{
	public:
		ValuePrinter(std::shared_ptr<Environment> pEnv, std::wostream& os, int indent, const std::wstring& header = L"") :
			pEnv(pEnv),
			os(os),
			m_indent(indent),
			m_header(header)
		{}

		std::shared_ptr<Environment> pEnv;
		int m_indent;
		std::wostream& os;
		mutable std::wstring m_header;
		
		std::wstring indent()const
		{
			const int tabSize = 4;
			std::wstring str;
			for (int i = 0; i < m_indent*tabSize; ++i)
			{
				str += ' ';
			}
			str += m_header;
			m_header.clear();
			return str;
		}

		void operator()(bool node)const
		{
			os << indent() << L"Bool(" << (node ? L"true" : L"false") << L")" << std::endl;
		}

		void operator()(int node)const
		{
			os << indent() << L"Int(" << node << L")" << std::endl;
		}

		void operator()(double node)const
		{
			os << indent() << L"Double(" << node << L")" << std::endl;
		}

		void operator()(const List& node)const
		{
			os << indent() << L"[" << std::endl;
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				/*const std::wstring header = std::wstring(L"(") + std::to_string(i) + L"): ";
				const auto child = ValuePrinter(m_indent + 1, header);
				const Evaluated currentData = data[i];
				boost::apply_visitor(child, currentData);*/

				//const std::wstring header = std::wstring(L"(") + std::to_string(i) + L"): ";
				const std::wstring header = std::wstring(L"Address(") + data[i].toString() + L"): ";
				const auto child = ValuePrinter(pEnv, os, m_indent + 1, header);
				//os << child.indent() << L"Address(" << data[i].toString() << L")" << std::endl;

				if (pEnv)
				{
					const Evaluated evaluated = pEnv->expand(data[i]);
					boost::apply_visitor(child, evaluated);
				}
				else
				{
					os << child.indent() << L"Address(" << data[i].toString() << L")" << std::endl;
				}
			}

			os << indent() << L"]" << std::endl;
		}

		void operator()(const KeyValue& node)const
		{
			const std::wstring header = static_cast<std::wstring>(node.name) + L": ";
			{
				const auto child = ValuePrinter(pEnv, os, m_indent, header);
				boost::apply_visitor(child, node.value);
			}
		}

		void operator()(const Record& node)const
		{
			os << indent() << L"{" << std::endl;

			for (const auto& value : node.values)
			{
				//const std::wstring header = value.first + L": ";
				const std::wstring header = value.first + std::wstring(L"(") + value.second.toString() + L"): ";

				const auto child = ValuePrinter(pEnv, os, m_indent + 1, header);

				if (pEnv)
				{
					const Evaluated evaluated = pEnv->expand(value.second);
					boost::apply_visitor(child, evaluated);
				}
				else
				{
					os << child.indent() << L"Address(" << value.second.toString() << L")" << std::endl;
				}
			}

			os << indent() << L"}" << std::endl;
		}

		void operator()(const FuncVal& node)const;

		void operator()(const Jump& node)const
		{
			os << indent() << L"Jump(" << node.op << L")" << std::endl;
		}

		/*void operator()(const DeclSat& node)const
		{
			os << indent() << L"DeclSat(" << L")" << std::endl;
		}*/

		//void operator()(const DeclFree& node)const;
	};

	inline void printEvaluated(const Evaluated& evaluated, std::shared_ptr<Environment> pEnv, std::wostream& os, int indent = 0)
	{
		ValuePrinter printer(pEnv, os, indent);
		boost::apply_visitor(printer, evaluated);
	}

#ifdef CGL_EnableLogOutput
	inline void printEvaluated(const Evaluated& evaluated, std::shared_ptr<Environment> pEnv, int indent = 0)
	{
		printEvaluated(evaluated, pEnv, ofs, indent);
	}
#else
	inline void printEvaluated(const Evaluated& evaluated, std::shared_ptr<Environment> pEnv, int indent = 0) {}
#endif

	class PrintSatExpr : public boost::static_visitor<void>
	{
	public:
		PrintSatExpr(const std::vector<double>& data, std::wostream& os) :
			data(data),
			os(os)
		{}

		const std::vector<double>& data;
		std::wostream& os;

		void operator()(double node)const
		{
			//os << node;
			//CGL_DebugLog(ToS(node));
			os << node;
		}

		void operator()(const SatReference& node)const
		{
			//os << L"$" << node.refID;
			//CGL_DebugLog(ToS(L"$") + ToS(node.refID));
			os << L"$" << node.refID;
		}

		void operator()(const SatUnaryExpr& node)const
		{
			switch (node.op)
			{
				//case UnaryOp::Not:   return lhs;
			case UnaryOp::Minus: os << L"-( "; break;
			case UnaryOp::Plus:  os << L"+( "; break;
			}

			boost::apply_visitor(*this, node.lhs);
			os << L" )";
		}

		void operator()(const SatBinaryExpr& node)const
		{
			os << L"( ";
			boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case BinaryOp::And: os << L" & "; break;

			case BinaryOp::Or:  os << L" | "; break;

			case BinaryOp::Equal:        os << L" == "; break;
			case BinaryOp::NotEqual:     os << L" != "; break;
			case BinaryOp::LessThan:     os << L" < "; break;
			case BinaryOp::LessEqual:    os << L" <= "; break;
			case BinaryOp::GreaterThan:  os << L" > "; break;
			case BinaryOp::GreaterEqual: os << L" >= "; break;

			case BinaryOp::Add: os << L" + "; break;
			case BinaryOp::Sub: os << L" - "; break;
			case BinaryOp::Mul: os << L" * "; break;
			case BinaryOp::Div: os << L" / "; break;

			case BinaryOp::Pow: os << L" ^ "; break;

			case BinaryOp::Concat: os << L" @ "; break;
			}

			boost::apply_visitor(*this, node.rhs);
			os << L" )";
		}

		void operator()(const SatFunctionReference& node)const
		{
		}
	};

	class Printer : public boost::static_visitor<void>
	{
	public:
		Printer(std::wostream& os, int indent = 0) :
			os(os),
			m_indent(indent)
		{}

		std::wostream& os;
		int m_indent;

		std::wstring indent()const
		{
			const int tabSize = 4;
			std::wstring str;
			for (int i = 0; i < m_indent*tabSize; ++i)
			{
				str += ' ';
			}
			return str;
		}

		void operator()(const LRValue& node)const
		{
			if (node.isLValue())
			{
				//os << indent() << L"Address(" << node.address() << L")" << std::endl;
				os << indent() << L"Address(" << node.address().toString() << L")" << std::endl;
			}
			else
			{
				printEvaluated(node.evaluated(), nullptr, os, m_indent);
			}
			//printEvaluated(node.value, m_indent);
			//os << indent() << L"Double(" << node << L")" << std::endl;
		}

		void operator()(const Identifier& node)const
		{
			os << indent() << L"Identifier(" << static_cast<std::wstring>(node) << L")" << std::endl;
		}

		void operator()(const SatReference& node)const
		{
			os << indent() << L"SatReference(" << node.refID << L")" << std::endl;
		}

		void operator()(const UnaryExpr& node)const
		{
			os << indent() << L"Unary(" << std::endl;

			boost::apply_visitor(Printer(os, m_indent + 1), node.lhs);

			os << indent() << L")" << std::endl;
		}

		void operator()(const BinaryExpr& node)const
		{
			os << indent() << L"Binary(" << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), node.lhs);
			boost::apply_visitor(Printer(os, m_indent + 1), node.rhs);
			os << indent() << L")" << std::endl;
		}

		void operator()(const DefFunc& defFunc)const
		{
			os << indent() << L"DefFunc(" << std::endl;

			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << L"Arguments(" << std::endl;
				{
					const auto grandChild = Printer(os, child.m_indent + 1);

					const auto& args = defFunc.arguments;
					for (size_t i = 0; i < args.size(); ++i)
					{
						os << grandChild.indent() << static_cast<std::wstring>(args[i]) << (i + 1 == args.size() ? L"\n" : L",\n");
					}
				}
				os << child.indent() << L")" << std::endl;
			}

			boost::apply_visitor(Printer(os, m_indent + 1), defFunc.expr);

			os << indent() << L")" << std::endl;
		}

		void operator()(const Range& range)const
		{
			os << indent() << L"Range(" << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), range.lhs);
			boost::apply_visitor(Printer(os, m_indent + 1), range.rhs);
			os << indent() << L")" << std::endl;
		}

		void operator()(const Lines& statement)const
		{
			os << indent() << L"Statement begin" << std::endl;

			int i = 0;
			for (const auto& expr : statement.exprs)
			{
				os << indent() << L"(" << i << L"): " << std::endl;
				boost::apply_visitor(Printer(os, m_indent + 1), expr);
				++i;
			}

			os << indent() << L"Statement end" << std::endl;
		}

		void operator()(const If& if_statement)const
		{
			os << indent() << L"If(" << std::endl;

			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << L"Cond(" << std::endl;

				boost::apply_visitor(Printer(os, m_indent + 2), if_statement.cond_expr);

				os << child.indent() << L")" << std::endl;
			}

			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << L"Then(" << std::endl;

				boost::apply_visitor(Printer(os, m_indent + 2), if_statement.then_expr);

				os << child.indent() << L")" << std::endl;
			}

			if (if_statement.else_expr)
			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << L"Else(" << std::endl;

				boost::apply_visitor(Printer(os, m_indent + 2), if_statement.else_expr.value());

				os << child.indent() << L")" << std::endl;
			}

			os << indent() << L")" << std::endl;
		}

		void operator()(const For& forExpression)const
		{
			os << indent() << L"For(" << std::endl;

			os << indent() << L")" << std::endl;
		}

		void operator()(const Return& return_statement)const
		{
			os << indent() << L"Return(" << std::endl;

			boost::apply_visitor(Printer(os, m_indent + 1), return_statement.expr);

			os << indent() << L")" << std::endl;
		}

		void operator()(const ListConstractor& listConstractor)const
		{
			os << indent() << L"ListConstractor(" << std::endl;

			int i = 0;
			for (const auto& expr : listConstractor.data)
			{
				os << indent() << L"(" << i << L"): " << std::endl;
				boost::apply_visitor(Printer(os, m_indent + 1), expr);
				++i;
			}
			os << indent() << L")" << std::endl;
		}

		void operator()(const KeyExpr& keyExpr)const
		{
			os << indent() << L"KeyExpr(" << std::endl;

			const auto child = Printer(os, m_indent + 1);

			os << child.indent() << static_cast<std::wstring>(keyExpr.name) << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), keyExpr.expr);
			os << indent() << L")" << std::endl;
		}

		void operator()(const RecordConstractor& recordConstractor)const
		{
			os << indent() << L"RecordConstractor(" << std::endl;

			int i = 0;
			for (const auto& expr : recordConstractor.exprs)
			{
				os << indent() << L"(" << i << L"): " << std::endl;
				boost::apply_visitor(Printer(os, m_indent + 1), expr);
				++i;
			}
			os << indent() << L")" << std::endl;
		}

		void operator()(const RecordInheritor& record)const
		{
			os << indent() << L"RecordInheritor(" << std::endl;

			const auto child = Printer(os, m_indent + 1);
			Expr expr = record.adder;
			boost::apply_visitor(child, record.original);
			boost::apply_visitor(child, expr);

			os << indent() << L")" << std::endl;
		}

		void operator()(const DeclSat& node)const
		{
			os << indent() << L"DeclSat(" << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), node.expr);
			os << indent() << L")" << std::endl;
		}

		void operator()(const DeclFree& node)const
		{
			os << indent() << L"DeclFree(" << std::endl;
			for (const auto& accessor : node.accessors)
			{
				Expr expr = accessor;
				boost::apply_visitor(Printer(os, m_indent + 1), expr);
			}
			os << indent() << L")" << std::endl;
		}

		void operator()(const Accessor& accessor)const
		{
			os << indent() << L"Accessor(" << std::endl;

			Printer child(os, m_indent + 1);
			boost::apply_visitor(child, accessor.head);
			for (const auto& access : accessor.accesses)
			{
				if (auto opt = AsOpt<ListAccess>(access))
				{
					os << child.indent() << L"[" << std::endl;
					boost::apply_visitor(child, opt.value().index);
					os << child.indent() << L"]" << std::endl;
				}
				else if (auto opt = AsOpt<RecordAccess>(access))
				{
					os << child.indent() << L"." << std::wstring(opt.value().name) << std::endl;
				}
				else if (auto opt = AsOpt<FunctionAccess>(access))
				{
					os << child.indent() << L"(" << std::endl;
					for (const auto& arg : opt.value().actualArguments)
					{
						boost::apply_visitor(child, arg);
					}
					os << child.indent() << L")" << std::endl;
				}
			}
			os << indent() << L")" << std::endl;
		}
	};

	/*inline void printLines(const Lines& statement)
	{
		os << L"Lines begin" << std::endl;

		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			os << L"Expr(" << i << L"): " << std::endl;
			boost::apply_visitor(Printer(), expr);
			++i;
		}

		os << L"Lines end" << std::endl;
	}*/

	inline void printExpr(const Expr& expr, std::wostream& os)
	{
		os << L"PrintExpr(\n";
		boost::apply_visitor(Printer(os), expr);
		os << L") " << std::endl;
	}

#ifdef CGL_EnableLogOutput
	inline void printExpr(const Expr& expr)
	{
		printExpr(expr, ofs);
	}
#else
	inline void printExpr(const Expr& expr) {}
#endif

	inline void ValuePrinter::operator()(const FuncVal& node)const
	{
		if (node.builtinFuncAddress)
		{
			os << indent() << L"Built-in function()" << std::endl;
		}
		else
		{
			os << indent() << L"FuncVal(" << std::endl;

			{
				const auto child = ValuePrinter(pEnv, os, m_indent + 1);
				os << child.indent();
				for (size_t i = 0; i < node.arguments.size(); ++i)
				{
					os << static_cast<std::wstring>(node.arguments[i]);
					if (i + 1 != node.arguments.size())
					{
						os << L", ";
					}
				}
				os << std::endl;

				os << child.indent() << L"->";

				boost::apply_visitor(Printer(os, m_indent + 1), node.expr);
			}

			os << indent() << L")" << std::endl;
		}
	}

	//inline void ValuePrinter::operator()(const DeclFree& node)const
	//{
	//	os << indent() << L"DeclFree(" << std::endl;
	//	for (size_t i = 0; i < node.accessors.size(); ++i)
	//	{
	//		Expr expr = node.accessors[i];
	//		boost::apply_visitor(Printer(os, m_indent + 1), expr);
	//	}
	//	os << indent() << L")" << std::endl;
	//	//boost::apply_visitor(Printer(m_indent + 1), accessor.head);
	//}

#ifdef CGL_EnableLogOutput
	void Environment::printEnvironment(bool flag)const
	{
		if (!flag)
		{
			return;
		}

		std::wostream& os = ofs;

		os << L"Print Environment Begin:\n";

		os << L"Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			os << keyval.first.toString() << L" : ";

			printEvaluated(val, m_weakThis.lock(), os);
		}

		os << L"References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			os << L"Depth : " << d << L"\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names)
			{
				os << keyval.first << L" : " << keyval.second.toString() << L"\n";
			}
		}

		os << L"Print Environment End:\n";
	}
#else
	void Environment::printEnvironment(bool flag)const {}
#endif

	void Environment::printEnvironment(std::wostream& os)const
	{
		os << L"Print Environment Begin:\n";

		os << L"Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			os << keyval.first.toString() << L" : ";

			printEvaluated(val, m_weakThis.lock(), os);
		}

		os << L"References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			os << L"Depth : " << d << L"\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names)
			{
				os << keyval.first << L" : " << keyval.second.toString() << L"\n";
			}
		}

		os << L"Print Environment End:\n";
	}
}
