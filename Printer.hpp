#pragma once
#include <iomanip>
#include "Node.hpp"
#include "Environment.hpp"

extern std::ofstream ofs;

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

	class ValuePrinter : public boost::static_visitor<void>
	{
	public:
		ValuePrinter(std::shared_ptr<Environment> pEnv, std::ostream& os, int indent, const std::string& header = "") :
			pEnv(pEnv),
			os(os),
			m_indent(indent),
			m_header(header)
		{}

		std::shared_ptr<Environment> pEnv;
		int m_indent;
		std::ostream& os;
		mutable std::string m_header;
		
		std::string indent()const
		{
			const int tabSize = 4;
			std::string str;
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
			os << indent() << "Bool(" << (node ? "true" : "false") << ")" << std::endl;
		}

		void operator()(int node)const
		{
			os << indent() << "Int(" << node << ")" << std::endl;
		}

		void operator()(double node)const
		{
			os << indent() << "Double(" << node << ")" << std::endl;
		}

		void operator()(const List& node)const
		{
			os << indent() << "[" << std::endl;
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				/*const std::string header = std::string("(") + std::to_string(i) + "): ";
				const auto child = ValuePrinter(m_indent + 1, header);
				const Evaluated currentData = data[i];
				boost::apply_visitor(child, currentData);*/

				//const std::string header = std::string("(") + std::to_string(i) + "): ";
				const std::string header = std::string("Address(") + data[i].toString() + "): ";
				const auto child = ValuePrinter(pEnv, os, m_indent + 1, header);
				//os << child.indent() << "Address(" << data[i].toString() << ")" << std::endl;

				if (pEnv)
				{
					const Evaluated evaluated = pEnv->expand(data[i]);
					boost::apply_visitor(child, evaluated);
				}
				else
				{
					os << child.indent() << "Address(" << data[i].toString() << ")" << std::endl;
				}
			}

			os << indent() << "]" << std::endl;
		}

		void operator()(const KeyValue& node)const
		{
			const std::string header = static_cast<std::string>(node.name) + ": ";
			{
				const auto child = ValuePrinter(pEnv, os, m_indent, header);
				boost::apply_visitor(child, node.value);
			}
		}

		void operator()(const Record& node)const
		{
			os << indent() << "{" << std::endl;

			for (const auto& value : node.values)
			{
				//const std::string header = value.first + ": ";
				const std::string header = value.first + std::string("(") + value.second.toString() + "): ";

				const auto child = ValuePrinter(pEnv, os, m_indent + 1, header);

				if (pEnv)
				{
					const Evaluated evaluated = pEnv->expand(value.second);
					boost::apply_visitor(child, evaluated);
				}
				else
				{
					os << child.indent() << "Address(" << value.second.toString() << ")" << std::endl;
				}
			}

			os << indent() << "}" << std::endl;
		}

		void operator()(const FuncVal& node)const;

		void operator()(const Jump& node)const
		{
			os << indent() << "Jump(" << node.op << ")" << std::endl;
		}

		/*void operator()(const DeclSat& node)const
		{
			os << indent() << "DeclSat(" << ")" << std::endl;
		}*/

		void operator()(const DeclFree& node)const;
	};

	inline void printEvaluated(const Evaluated& evaluated, std::shared_ptr<Environment> pEnv, std::ostream& os, int indent = 0)
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
		PrintSatExpr(const std::vector<double>& data, std::ostream& os) :
			data(data),
			os(os)
		{}

		const std::vector<double>& data;
		std::ostream& os;

		void operator()(double node)const
		{
			//os << node;
			//CGL_DebugLog(ToS(node));
			os << node;
		}

		void operator()(const SatReference& node)const
		{
			//os << "$" << node.refID;
			//CGL_DebugLog(ToS("$") + ToS(node.refID));
			os << "$" << node.refID;
		}

		void operator()(const SatUnaryExpr& node)const
		{
			switch (node.op)
			{
				//case UnaryOp::Not:   return lhs;
			case UnaryOp::Minus: os << "-( "; break;
			case UnaryOp::Plus:  os << "+( "; break;
			}

			boost::apply_visitor(*this, node.lhs);
			os << " )";
		}

		void operator()(const SatBinaryExpr& node)const
		{
			os << "( ";
			boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case BinaryOp::And: os << " & "; break;

			case BinaryOp::Or:  os << " | "; break;

			case BinaryOp::Equal:        os << " == "; break;
			case BinaryOp::NotEqual:     os << " != "; break;
			case BinaryOp::LessThan:     os << " < "; break;
			case BinaryOp::LessEqual:    os << " <= "; break;
			case BinaryOp::GreaterThan:  os << " > "; break;
			case BinaryOp::GreaterEqual: os << " >= "; break;

			case BinaryOp::Add: os << " + "; break;
			case BinaryOp::Sub: os << " - "; break;
			case BinaryOp::Mul: os << " * "; break;
			case BinaryOp::Div: os << " / "; break;

			case BinaryOp::Pow: os << " ^ "; break;
			}

			boost::apply_visitor(*this, node.rhs);
			os << " )";
		}

		void operator()(const SatFunctionReference& node)const
		{
		}
	};

	class Printer : public boost::static_visitor<void>
	{
	public:
		Printer(std::ostream& os, int indent = 0) :
			os(os),
			m_indent(indent)
		{}

		std::ostream& os;
		int m_indent;

		std::string indent()const
		{
			const int tabSize = 4;
			std::string str;
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
				//os << indent() << "Address(" << node.address() << ")" << std::endl;
				os << indent() << "Address(" << node.address().toString() << ")" << std::endl;
			}
			else
			{
				printEvaluated(node.evaluated(), nullptr, os, m_indent);
			}
			//printEvaluated(node.value, m_indent);
			//os << indent() << "Double(" << node << ")" << std::endl;
		}

		void operator()(const Identifier& node)const
		{
			os << indent() << "Identifier(" << static_cast<std::string>(node) << ")" << std::endl;
		}

		void operator()(const SatReference& node)const
		{
			os << indent() << "SatReference(" << node.refID << ")" << std::endl;
		}

		void operator()(const UnaryExpr& node)const
		{
			os << indent() << "Unary(" << std::endl;

			boost::apply_visitor(Printer(os, m_indent + 1), node.lhs);

			os << indent() << ")" << std::endl;
		}

		void operator()(const BinaryExpr& node)const
		{
			os << indent() << "Binary(" << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), node.lhs);
			boost::apply_visitor(Printer(os, m_indent + 1), node.rhs);
			os << indent() << ")" << std::endl;
		}

		void operator()(const DefFunc& defFunc)const
		{
			os << indent() << "DefFunc(" << std::endl;

			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << "Arguments(" << std::endl;
				{
					const auto grandChild = Printer(os, child.m_indent + 1);

					const auto& args = defFunc.arguments;
					for (size_t i = 0; i < args.size(); ++i)
					{
						os << grandChild.indent() << static_cast<std::string>(args[i]) << (i + 1 == args.size() ? "\n" : ",\n");
					}
				}
				os << child.indent() << ")" << std::endl;
			}

			boost::apply_visitor(Printer(os, m_indent + 1), defFunc.expr);

			os << indent() << ")" << std::endl;
		}

		void operator()(const Range& range)const
		{
			os << indent() << "Range(" << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), range.lhs);
			boost::apply_visitor(Printer(os, m_indent + 1), range.rhs);
			os << indent() << ")" << std::endl;
		}

		void operator()(const Lines& statement)const
		{
			os << indent() << "Statement begin" << std::endl;

			int i = 0;
			for (const auto& expr : statement.exprs)
			{
				os << indent() << "(" << i << "): " << std::endl;
				boost::apply_visitor(Printer(os, m_indent + 1), expr);
				++i;
			}

			os << indent() << "Statement end" << std::endl;
		}

		void operator()(const If& if_statement)const
		{
			os << indent() << "If(" << std::endl;

			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << "Cond(" << std::endl;

				boost::apply_visitor(Printer(os, m_indent + 2), if_statement.cond_expr);

				os << child.indent() << ")" << std::endl;
			}

			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << "Then(" << std::endl;

				boost::apply_visitor(Printer(os, m_indent + 2), if_statement.then_expr);

				os << child.indent() << ")" << std::endl;
			}

			if (if_statement.else_expr)
			{
				const auto child = Printer(os, m_indent + 1);
				os << child.indent() << "Else(" << std::endl;

				boost::apply_visitor(Printer(os, m_indent + 2), if_statement.else_expr.value());

				os << child.indent() << ")" << std::endl;
			}

			os << indent() << ")" << std::endl;
		}

		void operator()(const For& forExpression)const
		{
			os << indent() << "For(" << std::endl;

			os << indent() << ")" << std::endl;
		}

		void operator()(const Return& return_statement)const
		{
			os << indent() << "Return(" << std::endl;

			boost::apply_visitor(Printer(os, m_indent + 1), return_statement.expr);

			os << indent() << ")" << std::endl;
		}

		void operator()(const ListConstractor& listConstractor)const
		{
			os << indent() << "ListConstractor(" << std::endl;

			int i = 0;
			for (const auto& expr : listConstractor.data)
			{
				os << indent() << "(" << i << "): " << std::endl;
				boost::apply_visitor(Printer(os, m_indent + 1), expr);
				++i;
			}
			os << indent() << ")" << std::endl;
		}

		void operator()(const KeyExpr& keyExpr)const
		{
			os << indent() << "KeyExpr(" << std::endl;

			const auto child = Printer(os, m_indent + 1);

			os << child.indent() << static_cast<std::string>(keyExpr.name) << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), keyExpr.expr);
			os << indent() << ")" << std::endl;
		}

		void operator()(const RecordConstractor& recordConstractor)const
		{
			os << indent() << "RecordConstractor(" << std::endl;

			int i = 0;
			for (const auto& expr : recordConstractor.exprs)
			{
				os << indent() << "(" << i << "): " << std::endl;
				boost::apply_visitor(Printer(os, m_indent + 1), expr);
				++i;
			}
			os << indent() << ")" << std::endl;
		}

		void operator()(const RecordInheritor& record)const
		{
			os << indent() << "RecordInheritor(" << std::endl;

			const auto child = Printer(os, m_indent + 1);
			Expr expr = record.adder;
			boost::apply_visitor(child, record.original);
			boost::apply_visitor(child, expr);

			os << indent() << ")" << std::endl;
		}

		void operator()(const DeclSat& node)const
		{
			os << indent() << "DeclSat(" << std::endl;
			boost::apply_visitor(Printer(os, m_indent + 1), node.expr);
			os << indent() << ")" << std::endl;
		}

		void operator()(const Accessor& accessor)const
		{
			os << indent() << "Accessor(" << std::endl;

			Printer child(os, m_indent + 1);
			boost::apply_visitor(child, accessor.head);
			for (const auto& access : accessor.accesses)
			{
				if (auto opt = AsOpt<ListAccess>(access))
				{
					os << child.indent() << "[" << std::endl;
					boost::apply_visitor(child, opt.value().index);
					os << child.indent() << "]" << std::endl;
				}
				else if (auto opt = AsOpt<RecordAccess>(access))
				{
					os << child.indent() << "." << std::string(opt.value().name) << std::endl;
				}
				else if (auto opt = AsOpt<FunctionAccess>(access))
				{
					os << child.indent() << "(" << std::endl;
					for (const auto& arg : opt.value().actualArguments)
					{
						boost::apply_visitor(child, arg);
					}
					os << child.indent() << ")" << std::endl;
				}
			}
			os << indent() << ")" << std::endl;
		}
	};

	/*inline void printLines(const Lines& statement)
	{
		os << "Lines begin" << std::endl;

		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			os << "Expr(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(), expr);
			++i;
		}

		os << "Lines end" << std::endl;
	}*/

	inline void printExpr(const Expr& expr, std::ostream& os)
	{
		os << "PrintExpr(\n";
		boost::apply_visitor(Printer(os), expr);
		os << ") " << std::endl;
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
			os << indent() << "Built-in function()" << std::endl;
		}
		else
		{
			os << indent() << "FuncVal(" << std::endl;

			{
				const auto child = ValuePrinter(pEnv, os, m_indent + 1);
				os << child.indent();
				for (size_t i = 0; i < node.arguments.size(); ++i)
				{
					os << static_cast<std::string>(node.arguments[i]);
					if (i + 1 != node.arguments.size())
					{
						os << ", ";
					}
				}
				os << std::endl;

				os << child.indent() << "->";

				boost::apply_visitor(Printer(os, m_indent + 1), node.expr);
			}

			os << indent() << ")" << std::endl;
		}
	}

	inline void ValuePrinter::operator()(const DeclFree& node)const
	{
		os << indent() << "DeclFree(" << std::endl;
		for (size_t i = 0; i < node.accessors.size(); ++i)
		{
			Expr expr = node.accessors[i];
			boost::apply_visitor(Printer(os, m_indent + 1), expr);
		}
		os << indent() << ")" << std::endl;
		//boost::apply_visitor(Printer(m_indent + 1), accessor.head);
	}

#ifdef CGL_EnableLogOutput
	void Environment::printEnvironment(bool flag)const
	{
		if (!flag)
		{
			return;
		}

		std::ostream& os = ofs;

		os << "Print Environment Begin:\n";

		os << "Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			os << keyval.first.toString() << " : ";

			printEvaluated(val, m_weakThis.lock(), os);
		}

		os << "References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			os << "Depth : " << d << "\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names)
			{
				os << keyval.first << " : " << keyval.second.toString() << "\n";
			}
		}

		os << "Print Environment End:\n";
	}
#else
	void Environment::printEnvironment(bool flag)const {}
#endif
}
