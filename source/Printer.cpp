#include <fstream>
#include <memory>

#include <Unicode.hpp>

#include <Pita/Printer.hpp>

namespace cgl
{
	std::string ValuePrinter::indent()const
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

	void ValuePrinter::operator()(bool node)const
	{
		os << indent() << "Bool(" << (node ? "true" : "false") << ")" << std::endl;
	}

	void ValuePrinter::operator()(int node)const
	{
		os << indent() << "Int(" << node << ")" << std::endl;
	}

	void ValuePrinter::operator()(double node)const
	{
		os << indent() << "Double(" << node << ")" << std::endl;
	}

	void ValuePrinter::operator()(const CharString& node)const
	{
		os << indent() << "String(" << Unicode::UTF32ToUTF8(node.toString()) << ")" << std::endl;
		//os << indent() << "String(" << UTF8ToString(Unicode::UTF32ToUTF8(node.toString())) << ")" << std::endl;
	}

	void ValuePrinter::operator()(const List& node)const
	{
		os << indent() << "[" << std::endl;

		const auto& data = node.data;
		for (size_t i = 0; i < data.size(); ++i)
		{
			/*const std::string header = std::string("(") + std::to_string(i) + "): ";
			const auto child = ValuePrinter(m_indent + 1, header);
			const Val currentData = data[i];
			boost::apply_visitor(child, currentData);*/

			//const std::string header = std::string("(") + std::to_string(i) + "): ";
			const std::string header = std::string("Address(") + data[i].toString() + "): ";
			const auto child = ValuePrinter(pEnv, os, m_indent + 1, header);
			//os << child.indent() << "Address(" << data[i].toString() << ")" << std::endl;

			if (pEnv && data[i].isValid())
			{
				//const Val evaluated = pEnv->expand(data[i]);
				if (auto opt = pEnv->expandOpt(data[i]))
				{
					boost::apply_visitor(child, opt.get());
				}
			}
			else
			{
				os << child.indent() << "Address(" << data[i].toString() << ")" << std::endl;
			}
		}

		os << indent() << "]" << std::endl;
	}

	void ValuePrinter::operator()(const KeyValue& node)const
	{
		const std::string header = static_cast<std::string>(node.name) + ": ";
		{
			const auto child = ValuePrinter(pEnv, os, m_indent, header);
			boost::apply_visitor(child, node.value);
		}
	}

	void ValuePrinter::operator()(const Record& node)const
	{
		os << indent() << "{" << std::endl;

		for (const auto& value : node.values)
		{
			//const std::string header = value.first + ": ";
			const std::string header = value.first + std::string("(") + value.second.toString() + "): ";

			const auto child = ValuePrinter(pEnv, os, m_indent + 1, header);

			if (pEnv && value.second.isValid())
			{
				//const Val evaluated = pEnv->expand(value.second);
				if (auto opt = pEnv->expandOpt(value.second))
				{
					boost::apply_visitor(child, opt.get());
				}
			}
			else
			{
				os << child.indent() << "Address(" << value.second.toString() << ")" << std::endl;
			}
		}

		os << indent() << "}" << std::endl;
	}

	void ValuePrinter::operator()(const FuncVal& node)const
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

				boost::apply_visitor(Printer(pEnv, os, m_indent + 1), node.expr);
			}

			os << indent() << ")" << std::endl;
		}
	}

	void ValuePrinter::operator()(const Jump& node)const
	{
		os << indent() << "Jump(" << node.op << ")" << std::endl;
	}

	std::string ValuePrinter2::indent()const
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

	std::string ValuePrinter2::footer()const
	{
		return m_indent == 0 ? "" : "\n";
	}

	std::string ValuePrinter2::delimiter()const
	{
		return m_indent == 0 ? ", " : "\n";
	}

	void ValuePrinter2::operator()(bool node)const
	{
		os << indent() << (node ? "true" : "false") << footer();
	}

	void ValuePrinter2::operator()(int node)const
	{
		os << indent() << node << footer();
	}

	void ValuePrinter2::operator()(double node)const
	{
		os << indent() << node << footer();
	}

	void ValuePrinter2::operator()(const CharString& node)const
	{
		os << indent() << "\"" << Unicode::UTF32ToUTF8(node.toString()) << "\"" << footer();
		//os << indent() << "String(" << UTF8ToString(Unicode::UTF32ToUTF8(node.toString())) << ")" << footer();
	}

	void ValuePrinter2::operator()(const List& node)const
	{
		std::stringstream ss;
		const auto& data = node.data;
		for (size_t i = 0; i < data.size(); ++i)
		{
			const std::string header = std::string("Address(") + data[i].toString() + "): ";
			const auto child = ValuePrinter2(pEnv, ss, 0, header);

			const std::string lf = i + 1 == data.size() ? "" : delimiter();
			
			if (pEnv && data[i].isValid())
			{
				//const Val evaluated = pEnv->expand(data[i]);
				if (auto opt = pEnv->expandOpt(data[i]))
				{
					boost::apply_visitor(child, opt.get());
					ss << lf;
				}
			}
			else
			{
				ss << child.indent() << "Address(" << data[i].toString() << ")" << lf;
			}
		}

		os << indent() << "[" << ss.str() << "]" << footer();
	}

	void ValuePrinter2::operator()(const KeyValue& node)const
	{
		std::stringstream ss;
		const std::string header = static_cast<std::string>(node.name) + ": ";
		{
			const auto child = ValuePrinter2(pEnv, ss, 0, header);
			boost::apply_visitor(child, node.value);

			os << indent() << ss.str() << footer();
		}
	}

	void ValuePrinter2::operator()(const Record& node)const
	{
		std::stringstream ss;
		int i = 0;
		for (const auto& value : node.values)
		{
			const std::string header = value.first + std::string("(") + value.second.toString() + "): ";

			const auto child = ValuePrinter2(pEnv, ss, 0, header);

			const std::string lf = i + 1 == node.values.size() ? "" : delimiter();

			if (pEnv && value.second.isValid())
			{
				//const Val evaluated = pEnv->expand(value.second);
				if (auto opt = pEnv->expandOpt(value.second))
				{
					boost::apply_visitor(child, opt.get());
					ss << lf;
				}
			}
			else
			{
				os << child.indent() << "Address(" << value.second.toString() << ")" << lf;
			}
			++i;
		}

		os << indent() << "{" << ss.str() << "}" << footer();
	}

	void ValuePrinter2::operator()(const FuncVal& node)const
	{
		if (node.builtinFuncAddress)
		{
			os << indent() << "Built-in function()" << footer();
		}
		else
		{
			os << indent() << "FuncVal(" << footer();

			{
				const auto child = ValuePrinter2(pEnv, os, m_indent + 1);
				os << child.indent();
				for (size_t i = 0; i < node.arguments.size(); ++i)
				{
					os << static_cast<std::string>(node.arguments[i]);
					if (i + 1 != node.arguments.size())
					{
						os << ", ";
					}
				}
				os << footer();

				os << child.indent() << "->";

				boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), node.expr);
			}

			os << indent() << ")" << footer();
		}
	}

	void ValuePrinter2::operator()(const Jump& node)const
	{
		os << indent() << "Jump(" << node.op << ")" << footer();
	}

	void PrintSatExpr::operator()(double node)const
	{
		//os << node;
		//CGL_DebugLog(ToS(node));
		os << node;
	}

	void PrintSatExpr::operator()(const SatUnaryExpr& node)const
	{
		switch (node.op)
		{
			//case UnaryOp::Not:   return lhs;
		case UnaryOp::Minus:    os << "-( "; break;
		case UnaryOp::Plus:     os << "+( "; break;
		case UnaryOp::Dynamic:  os << "@( "; break;
		}

		boost::apply_visitor(*this, node.lhs);
		os << " )";
	}

	void PrintSatExpr::operator()(const SatBinaryExpr& node)const
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

		case BinaryOp::Concat: os << " @ "; break;

		case BinaryOp::SetDiff: os << " \\ "; break;
		}

		boost::apply_visitor(*this, node.rhs);
		os << " )";
	}

	void PrintSatExpr::operator()(const SatFunctionReference& node)const
	{
	}

	std::string Printer::indent()const
	{
		const int tabSize = 4;
		std::string str;
		for (int i = 0; i < m_indent*tabSize; ++i)
		{
			str += ' ';
		}
		return str;
	}

	void Printer::operator()(const LRValue& node)const
	{
		if (node.isLValue())
		{
			if (node.isAddress())
			{
				if (pEnv)
				{
					auto opt = pEnv->expandOpt(node);
					os << indent() << node.toString() << ": " << (opt ? "val" : "none") << std::endl;
					if (opt)
					{
						printVal(opt.get(), pEnv, os, m_indent);
					}
				}
				else
				{
					os << indent() << node.toString() << std::endl;
				}
			}
			else
			{
				/*if (pEnv)
				{
					os << indent() << "Reference" << static_cast<LocationInfo>(node).getInfo() << "(";
					pEnv->printReference(node.reference(), os);
					os << ")" << std::endl;
				}
				else*/
				{
					os << indent() << "Reference" << static_cast<LocationInfo>(node).getInfo() << "(" << node.toString() << ")" << std::endl;
				}
			}
		}
		else
		{
			printVal(node.evaluated(), nullptr, os, m_indent);
		}
		//printVal(node.value, m_indent);
		//os << indent() << "Double(" << node << ")" << std::endl;
	}

	void Printer::operator()(const Identifier& node)const
	{
		os << indent() << "Identifier" << static_cast<LocationInfo>(node).getInfo() << "(" << static_cast<std::string>(node) << ")" << std::endl;
	}

	void Printer::operator()(const Import& node)const
	{
		os << indent() << "Import" << static_cast<LocationInfo>(node).getInfo() << "(" << ")" << std::endl;
	}

	void Printer::operator()(const UnaryExpr& node)const
	{
		os << indent() << UnaryOpToStr(node.op) << static_cast<LocationInfo>(node).getInfo() << "(" << std::endl;

		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), node.lhs);

		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const BinaryExpr& node)const
	{
		os << indent() << BinaryOpToStr(node.op) << static_cast<LocationInfo>(node).getInfo() << "(" << std::endl;
		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), node.lhs);
		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), node.rhs);
		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const DefFunc& defFunc)const
	{
		os << indent() << "DefFunc" << defFunc.getInfo() << "(" << std::endl;

		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "Arguments(" << std::endl;
			{
				const auto grandChild = Printer(pEnv, os, child.m_indent + 1);

				const auto& args = defFunc.arguments;
				for (size_t i = 0; i < args.size(); ++i)
				{
					os << grandChild.indent() << static_cast<std::string>(args[i]) << (i + 1 == args.size() ? "\n" : ",\n");
				}
			}
			os << child.indent() << ")" << std::endl;
		}

		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), defFunc.expr);

		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const Range& range)const
	{
		os << indent() << "Range" << range.getInfo() << "(" << std::endl;
		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), range.lhs);
		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), range.rhs);
		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const Lines& statement)const
	{
		os << indent() << "Statement begin" << statement.getInfo() << "(" << std::endl;

		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			os << indent() << "(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(pEnv, os, m_indent + 1), expr);
			++i;
		}

		os << indent() << ") Statement end" << std::endl;
	}

	void Printer::operator()(const If& if_statement)const
	{
		os << indent() << "If" << if_statement.getInfo() << "(" << std::endl;

		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "Cond(" << std::endl;

			boost::apply_visitor(Printer(pEnv, os, m_indent + 2), if_statement.cond_expr);

			os << child.indent() << ")" << std::endl;
		}

		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "Then(" << std::endl;

			boost::apply_visitor(Printer(pEnv, os, m_indent + 2), if_statement.then_expr);

			os << child.indent() << ")" << std::endl;
		}

		if (if_statement.else_expr)
		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "Else(" << std::endl;

			boost::apply_visitor(Printer(pEnv, os, m_indent + 2), if_statement.else_expr.get());

			os << child.indent() << ")" << std::endl;
		}

		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const For& forExpression)const
	{
		os << indent() << "For" << forExpression.getInfo() << "(" << std::endl;

		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "loopCounter: " << forExpression.loopCounter.toString() << std::endl;
		}

		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "RangeStart(" << std::endl;

			boost::apply_visitor(Printer(pEnv, os, m_indent + 2), forExpression.rangeStart);

			os << child.indent() << ")" << std::endl;
		}

		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "RangeEnd(" << std::endl;

			boost::apply_visitor(Printer(pEnv, os, m_indent + 2), forExpression.rangeEnd);

			os << child.indent() << ")" << std::endl;
		}

		{
			const auto child = Printer(pEnv, os, m_indent + 1);
			os << child.indent() << "Do(" << std::endl;

			boost::apply_visitor(Printer(pEnv, os, m_indent + 2), forExpression.doExpr);

			os << child.indent() << ")" << std::endl;
		}

		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const Return& return_statement)const
	{
		os << indent() << "Return" << return_statement.getInfo() << "(" << std::endl;

		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), return_statement.expr);

		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const ListConstractor& listConstractor)const
	{
		os << indent() << "ListConstractor" << listConstractor.getInfo() << "(" << std::endl;

		int i = 0;
		for (const auto& expr : listConstractor.data)
		{
			os << indent() << "(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(pEnv, os, m_indent + 1), expr);
			++i;
		}
		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const KeyExpr& keyExpr)const
	{
		os << indent() << "KeyExpr" << keyExpr.getInfo() << "(" << std::endl;

		const auto child = Printer(pEnv, os, m_indent + 1);

		os << child.indent() << static_cast<std::string>(keyExpr.name) << std::endl;
		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), keyExpr.expr);
		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const RecordConstractor& recordConstractor)const
	{
		os << indent() << "RecordConstractor" << recordConstractor.getInfo() << "(" << std::endl;

		int i = 0;
		for (const auto& expr : recordConstractor.exprs)
		{
			os << indent() << "(" << i << "): " << std::endl;
			boost::apply_visitor(Printer(pEnv, os, m_indent + 1), expr);
			++i;
		}
		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const DeclSat& node)const
	{
		os << indent() << "DeclSat" << static_cast<LocationInfo>(node).getInfo() << "(" << std::endl;
		boost::apply_visitor(Printer(pEnv, os, m_indent + 1), node.expr);
		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const DeclFree& node)const
	{
		os << indent() << "DeclFree" << static_cast<LocationInfo>(node).getInfo() << "(" << std::endl;
		for (const auto& varRange : node.accessors)
		{
			Expr expr = varRange.accessor;
			boost::apply_visitor(Printer(pEnv, os, m_indent + 1), expr);
		}
		os << indent() << ")" << std::endl;
	}

	void Printer::operator()(const Accessor& accessor)const
	{
		os << indent() << "Accessor" << accessor.getInfo() << "(" << std::endl;

		Printer child(pEnv, os, m_indent + 1);
		boost::apply_visitor(child, accessor.head);
		for (const auto& access : accessor.accesses)
		{
			if (auto opt = AsOpt<ListAccess>(access))
			{
				os << child.indent() << "[" << std::endl;
				boost::apply_visitor(child, opt.get().index);
				os << child.indent() << "]" << std::endl;
			}
			else if (auto opt = AsOpt<RecordAccess>(access))
			{
				os << child.indent() << "." << std::string(opt.get().name) << std::endl;
			}
			else if (auto opt = AsOpt<FunctionAccess>(access))
			{
				os << child.indent() << "(" << std::endl;
				for (const auto& arg : opt.get().actualArguments)
				{
					boost::apply_visitor(child, arg);
				}
				os << child.indent() << ")" << std::endl;
			}
			else if (auto opt = AsOpt<InheritAccess>(access))
			{
				os << child.indent() << "{" << std::endl;
				for (const auto& arg : opt.get().adder.exprs)
				{
					boost::apply_visitor(child, arg);
				}
				os << child.indent() << "}" << std::endl;
			}
		}
		os << indent() << ")" << std::endl;
	}

	std::string Printer2::indent()const
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

	std::string Printer2::footer()const
	{
		return m_indent == 0 ? "" : "\n";
	}

	std::string Printer2::delimiter()const
	{
		return m_indent == 0 ? ", " : "\n";
	}

	void Printer2::operator()(const LRValue& node)const
	{
		/*os << indent();*/

		if (node.isLValue())
		{
			if (node.isEitherReference() && node.eitherReference().local)
			{
				os << node.eitherReference().local.get().toString() << footer();
			}
			else
			{
				os << node.toString() << footer();
			}
		}
		else
		{
			//printVal(node.evaluated(), nullptr, os, m_indent);
			printVal2(node.evaluated(), nullptr, os, 0);
		}
		//printVal(node.value, m_indent);
		//os << indent() << "Double(" << node << ")" << footer();
	}

	void Printer2::operator()(const Identifier& node)const
	{
		//os << indent() << "Identifier(" << static_cast<std::string>(node) << ")" << footer();
		os /*<< indent()*/ << static_cast<std::string>(node);
	}

	void Printer2::operator()(const Import& node)const
	{}

	void Printer2::operator()(const UnaryExpr& node)const
	{
		/*os << indent() << UnaryOpToStr(node.op) << "(" << footer();

		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), node.lhs);

		os << indent() << ")" << footer();*/

		std::stringstream ss;

		UnaryOp op = node.op;
		switch (op)
		{
		case cgl::UnaryOp::Not: ss << "!"; break;
		case cgl::UnaryOp::Plus: ss << "+"; break;
		case cgl::UnaryOp::Minus: ss << "-"; break;
		case cgl::UnaryOp::Dynamic: ss << "@"; break;
		default: break;
		}

		boost::apply_visitor(Printer2(pEnv, ss, 0), node.lhs);

		os << indent() << ss.str() << footer();
	}

	void Printer2::operator()(const BinaryExpr& node)const
	{
		/*os << indent() << BinaryOpToStr(node.op) << "(" << footer();
		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), node.lhs);
		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), node.rhs);
		os << indent() << ")" << footer();*/

		if (node.op == BinaryOp::Assign)
		{
			std::stringstream sss;
			boost::apply_visitor(Printer2(pEnv, sss, 0), node.rhs);
			if (sss.str().find('\n') != std::string::npos)
			{
				std::stringstream ss;
				boost::apply_visitor(Printer2(pEnv, ss, 0), node.lhs);

				ss << " = \n";

				boost::apply_visitor(Printer2(pEnv, ss, m_indent + 1), node.rhs);

				os << indent() << ss.str() << footer();
				return;
			}
		}

		std::stringstream ss;
		boost::apply_visitor(Printer2(pEnv, ss, 0), node.lhs);

		switch (node.op)
		{
		case cgl::BinaryOp::And: ss << " & "; break;
		case cgl::BinaryOp::Or: ss << " | "; break;
		case cgl::BinaryOp::Equal: ss << " == "; break;
		case cgl::BinaryOp::NotEqual: ss << " != "; break;
		case cgl::BinaryOp::LessThan: ss << " < "; break;
		case cgl::BinaryOp::LessEqual: ss << " <= "; break;
		case cgl::BinaryOp::GreaterThan: ss << " > "; break;
		case cgl::BinaryOp::GreaterEqual: ss << " >= "; break;
		case cgl::BinaryOp::Add: ss << " + "; break;
		case cgl::BinaryOp::Sub: ss << " - "; break;
		case cgl::BinaryOp::Mul: ss << " * "; break;
		case cgl::BinaryOp::Div: ss << " / "; break;
		case cgl::BinaryOp::Pow: ss << " ^ "; break;
		case cgl::BinaryOp::Assign: ss << " = "; break;
		case cgl::BinaryOp::Concat: ss << " @ "; break;
		case cgl::BinaryOp::SetDiff: ss << " \\ "; break;
		default: ss << ""; break;
		}

		boost::apply_visitor(Printer2(pEnv, ss, 0), node.rhs);

		os << indent() << ss.str() << footer();
	}

	void Printer2::operator()(const DefFunc& defFunc)const
	{
		/*os << indent() << "DefFunc(" << footer();

		{
			const auto child = Printer2(pEnv, os, m_indent + 1);
			os << child.indent() << "Arguments(" << footer();
			{
				const auto grandChild = Printer2(pEnv, os, child.m_indent + 1);

				const auto& args = defFunc.arguments;
				for (size_t i = 0; i < args.size(); ++i)
				{
					os << grandChild.indent() << static_cast<std::string>(args[i]) << (i + 1 == args.size() ? "\n" : ",\n");
				}
			}
			os << child.indent() << ")" << footer();
		}

		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), defFunc.expr);

		os << indent() << ")" << footer();*/

		//std::cout << "m_indent: " << m_indent << ", " << __LINE__ << std::endl;
		{
			std::stringstream ss;
			ss << "(";
			const auto& args = defFunc.arguments;
			for (size_t i = 0; i < args.size(); ++i)
			{
				ss << static_cast<std::string>(args[i]) << (i + 1 == args.size() ? "" : ", ");
			}
			ss << " -> ";
			//os << indent() << ss.str() << footer();
			os << indent() << ss.str() << "\n";
		}

		//std::cout << "m_indent: " << m_indent << ", " << __LINE__ << std::endl;
		{
			std::stringstream ss;

			boost::apply_visitor(Printer2(pEnv, ss, m_indent == 0 ? 0 : m_indent + 1), defFunc.expr);
			os << indent() << ss.str() << "\n";
		}
		

		os << indent() << ")" << footer();
	}

	void Printer2::operator()(const Range& range)const
	{
		os << indent() << "Range(" << footer();
		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), range.lhs);
		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), range.rhs);
		os << indent() << ")" << footer();
	}

	void Printer2::operator()(const Lines& statement)const
	{
		//os << indent() << "Statement begin" << footer();

		if (statement.exprs.size() == 1)
		{
			std::stringstream ss;
			boost::apply_visitor(Printer2(pEnv, ss, 0), statement.exprs.front());
			os << indent() << "(" << ss.str() << ")" << footer();
		}
		else
		{
			os << indent() << "(" << footer();
			for (const auto& expr : statement.exprs)
			{
				/*os << indent() << "(" << i << "): " << footer();
				boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), expr);
				++i;*/
				//boost::apply_visitor(*this, expr);
				boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), expr);
			}
			os << indent() << ")" << footer();
		}

		//os << indent() << "Statement end" << footer();
	}

	void Printer2::operator()(const If& if_statement)const
	{
		/*
		os << indent() << "If(" << footer();

		{
			const auto child = Printer2(pEnv, os, m_indent + 1);
			os << child.indent() << "Cond(" << footer();

			boost::apply_visitor(Printer2(pEnv, os, m_indent + 2), if_statement.cond_expr);

			os << child.indent() << ")" << footer();
		}

		{
			const auto child = Printer2(pEnv, os, m_indent + 1);
			os << child.indent() << "Then(" << footer();

			boost::apply_visitor(Printer2(pEnv, os, m_indent + 2), if_statement.then_expr);

			os << child.indent() << ")" << footer();
		}

		if (if_statement.else_expr)
		{
			const auto child = Printer2(pEnv, os, m_indent + 1);
			os << child.indent() << "Else(" << footer();

			boost::apply_visitor(Printer2(pEnv, os, m_indent + 2), if_statement.else_expr.get());

			os << child.indent() << ")" << footer();
		}

		os << indent() << ")" << footer();
		*/

		{
			std::stringstream ss;
			const auto child = Printer2(pEnv, ss, 0);
			boost::apply_visitor(child, if_statement.cond_expr);
			os << indent() << "if(" << ss.str() << ")" << footer();
		}

		{
			std::stringstream ss;
			const auto child = Printer2(pEnv, ss, m_indent + 1);
			boost::apply_visitor(child, if_statement.then_expr);

			os << indent() << "then(" << footer();
			os << ss.str();
			os << indent() << ")" << footer();
		}

		if (if_statement.else_expr)
		{
			std::stringstream ss;
			const auto child = Printer2(pEnv, ss, m_indent + 1);
			boost::apply_visitor(child, if_statement.else_expr.get());

			os << indent() << "else(" << footer();
			os << ss.str();
			os << indent() << ")" << footer();
		}
	}

	void Printer2::operator()(const For& forExpression)const
	{
		os << indent() << "For(" << footer();

		os << indent() << ")" << footer();
	}

	void Printer2::operator()(const Return& return_statement)const
	{
		os << indent() << "Return(" << footer();

		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), return_statement.expr);

		os << indent() << ")" << footer();
	}

	void Printer2::operator()(const ListConstractor& listConstractor)const
	{
		os << indent() << "[" << footer();

		int i = 0;
		for (const auto& expr : listConstractor.data)
		{
			//os << indent() << "(" << i << "): " << footer();
			boost::apply_visitor(Printer2(pEnv, os, m_indent == 0 ? 0 : m_indent + 1), expr);
			if (m_indent == 0 && i + 1 != listConstractor.data.size())
			{
				os << delimiter();
			}
			++i;
		}
		os << indent() << "]" << footer();
	}

	void Printer2::operator()(const KeyExpr& keyExpr)const
	{
		//boost::apply_visitor(Printer2(pEnv, os, m_indent, static_cast<std::string>(keyExpr.name) + ": "), keyExpr.expr);

		os << indent() << keyExpr.name.toString() << ": ";
		boost::apply_visitor(Printer2(pEnv, os, 0), keyExpr.expr);
		os << footer();
	}

	void Printer2::operator()(const RecordConstractor& recordConstractor)const
	{
		os << indent() << "{" << footer();

		int i = 0;
		for (const auto& expr : recordConstractor.exprs)
		{
			//boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), expr);

			boost::apply_visitor(Printer2(pEnv, os, m_indent == 0 ? 0 : m_indent + 1), expr);
			if (m_indent == 0 && i + 1 != recordConstractor.exprs.size())
			{
				os << delimiter();
			}
			++i;
		}
		os << indent() << "}" << footer();
	}

	void Printer2::operator()(const DeclSat& node)const
	{
		os << indent() << "DeclSat(" << footer();
		boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), node.expr);
		os << indent() << ")" << footer();
	}

	void Printer2::operator()(const DeclFree& node)const
	{
		os << indent() << "DeclFree(" << footer();
		for (const auto& varRange : node.accessors)
		{
			Expr expr = varRange.accessor;
			boost::apply_visitor(Printer2(pEnv, os, m_indent + 1), expr);
		}
		os << indent() << ")" << footer();
	}

	void Printer2::operator()(const Accessor& accessor)const
	{
		/*os << indent() << "Accessor(" << footer();

		Printer2 child(pEnv, os, m_indent + 1);
		boost::apply_visitor(child, accessor.head);
		for (const auto& access : accessor.accesses)
		{
			if (auto opt = AsOpt<ListAccess>(access))
			{
				os << child.indent() << "[" << footer();
				boost::apply_visitor(child, opt.get().index);
				os << child.indent() << "]" << footer();
			}
			else if (auto opt = AsOpt<RecordAccess>(access))
			{
				os << child.indent() << "." << std::string(opt.get().name) << footer();
			}
			else if (auto opt = AsOpt<FunctionAccess>(access))
			{
				os << child.indent() << "(" << footer();
				for (const auto& arg : opt.get().actualArguments)
				{
					boost::apply_visitor(child, arg);
				}
				os << child.indent() << ")" << footer();
			}
		}
		os << indent() << ")" << footer();*/

		std::stringstream ss;
		Printer2 child(pEnv, ss, 0);
		boost::apply_visitor(child, accessor.head);
		for (const auto& access : accessor.accesses)
		{
			if (auto opt = AsOpt<ListAccess>(access))
			{
				ss << "[";
				boost::apply_visitor(child, opt.get().index);
				ss << "]";
			}
			else if (auto opt = AsOpt<RecordAccess>(access))
			{
				ss << "." << std::string(opt.get().name);
			}
			else if (auto opt = AsOpt<FunctionAccess>(access))
			{
				ss << "(";
				int argIndex = 0;
				const auto& args = opt.get().actualArguments;
				for (const auto& arg : args)
				{
					boost::apply_visitor(child, arg);
					if (argIndex+1 != args.size())
					{
						ss << ", ";
					}
					++argIndex;
				}
				ss << ")";
			}
			else if (auto opt = AsOpt<InheritAccess>(access))
			{
				ss << "{";
				int argIndex = 0;
				const auto& exprs = opt.get().adder.exprs;
				for (const auto& arg : exprs)
				{
					boost::apply_visitor(child, arg);
					if (argIndex + 1 != exprs.size())
					{
						ss << ", ";
					}
					++argIndex;
				}
				ss << "}";
			}
		}

		os << indent() << ss.str() << footer();
	}

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
	
	//inline void ValuePrinter::operator()(const DeclFree& node)const
	//{
	//	os << indent() << "DeclFree(" << std::endl;
	//	for (size_t i = 0; i < node.accessors.size(); ++i)
	//	{
	//		Expr expr = node.accessors[i];
	//		boost::apply_visitor(Printer(os, m_indent + 1), expr);
	//	}
	//	os << indent() << ")" << std::endl;
	//	//boost::apply_visitor(Printer(m_indent + 1), accessor.head);
	//}
}
