#pragma warning(disable:4996)
#include <iostream>

#include <Pita/Graph.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/Printer.hpp>
#include <Pita/IntrinsicGeometricFunctions.hpp>
#include <Pita/Program.hpp>

namespace cgl
{
	size_t ControlFlowGraph::addNode()
	{
		m_blockNodes.emplace_back();
		m_edgeIndices.emplace_back();
		return m_blockNodes.size() - 1;
	}

	size_t ControlFlowGraph::addNode(const Lines& lines)
	{
		m_blockNodes.push_back(lines);
		m_edgeIndices.emplace_back();
		return m_blockNodes.size() - 1;
	}

	Lines& ControlFlowGraph::node(size_t nodeIndex)
	{
		return m_blockNodes[nodeIndex];
	}

	const Lines& ControlFlowGraph::node(size_t nodeIndex)const
	{
		return m_blockNodes[nodeIndex];
	}

	void ControlFlowGraph::addEdge(size_t fromNode, size_t toNode, bool isTrue)
	{
		m_edgeIndices[fromNode].emplace_back(toNode, isTrue);
	}

	void ControlFlowGraph::outputGraphViz(std::ostream& os)const
	{
		os << R"(digraph constraint_flow_graph {
  graph [
    charset = "UTF-8",
    bgcolor = "#EDEDED",
    rankdir = TB,
    nodesep = 1.1,
    ranksep = 1.05
  ];

  node [
    shape = record,
    fontname = "Migu 1M",
    fontsize = 12,
  ];)";
		os << "\n\n";

		for (size_t i = 0; i < m_blockNodes.size(); ++i)
		{
			os << "  node" << i << R"( [label = "{<ptop>)";
			const auto& lines = m_blockNodes[i];

			for (size_t exprIndex = 0; exprIndex < lines.exprs.size(); ++exprIndex)
			{
				std::stringstream ss;
				Printer2 printer(nullptr, ss);
				boost::apply_visitor(printer, lines.exprs[exprIndex]);

				if (exprIndex + 1 != lines.exprs.size())
				{
					os << escaped(ss.str());
					os << "|";
				}
				else
				{
					os << "Branch " << escaped(ss.str());
				}
			}

			os << R"(|{<ptrue>true|<pfalse>false})";

			os << R"(}"];)" << "\n";
		}

		os << "\n";

		for (size_t fromNodeIndex = 0; fromNodeIndex < m_edgeIndices.size(); ++fromNodeIndex)
		{
			for (const auto& edge : m_edgeIndices[fromNodeIndex])
			{
				os << "  node" << fromNodeIndex << ":" << (edge.isTrueBranch ? "ptrue" : "pfalse");
				os << " -> node" << edge.toNodeIndex << ":ptop;\n";
			}
		}

		os << "}\n";
	}

	Identifier ControlFlowGraph::NewVariable()
	{
		static size_t auxiliaryVariables = 0;
		++auxiliaryVariables;
		return Identifier(std::string("$") + ToS(auxiliaryVariables));
	}

	std::string ControlFlowGraph::escaped(const std::string& str)const
	{
		std::stringstream ss;
		for (char c : str)
		{
			switch (c)
			{
			case '|':
				ss << "\\|";
				break;
			case '<':
				ss << "\\<";
				break;
			case '>':
				ss << "\\>";
				break;
			case '{':
				ss << "\\{";
				break;
			case '}':
				ss << "\\}";
				break;
			case '\n':
				ss << "\\n\n";
				break;
			default:
				ss << c;
				break;
			}
		}

		return ss.str();
	}


	boost::optional<Expr> ControlFlowGraphMaker::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			return node;
		}
		//図形などの場合は評価される式を展開する
		return node;
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const UnaryExpr& node)
	{
		auto expr = boost::apply_visitor(*this, node.lhs);
		if (!expr)
		{
			CGL_Error("エラー");
		}

		return UnaryExpr(expr.get(), node.op);
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const BinaryExpr& node)
	{
		auto expr1 = boost::apply_visitor(*this, node.lhs);
		auto expr2 = boost::apply_visitor(*this, node.rhs);
		if (!expr1 || !expr2)
		{
			CGL_Error("エラー");
		}

		return BinaryExpr(expr1.get(), expr2.get(), node.op);
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const Lines& node)
	{
		for (size_t i = 0; i + 1 < node.exprs.size(); ++i)
		{
			const auto& expr = node.exprs[i];
			if (auto resultExpr = boost::apply_visitor(*this, expr))
			{
				graph.node(currentNodeIndex).add(resultExpr.get());
			}
		}

		if (!node.exprs.empty())
		{
			return boost::apply_visitor(*this, node.exprs.back());
		}

		return boost::none;
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const DefFunc& node)
	{
		//boost::apply_visitor(*this, node.expr);
		return node;
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const If& node)
	{
		if (auto expr = boost::apply_visitor(*this, node.cond_expr))
		{
			graph.node(currentNodeIndex).add(expr.get());
		}

		const size_t nextNodeIndex = graph.addNode();

		std::vector<Identifier> results;
		{
			const size_t thenNodeIndex = graph.addNode();
			graph.addEdge(currentNodeIndex, thenNodeIndex, true);

			ControlFlowGraphMaker child(context, graph, thenNodeIndex);
			if (auto thenResultExpr = boost::apply_visitor(child, node.then_expr))
			{
				results.push_back(ControlFlowGraph::NewVariable());
				graph.node(thenNodeIndex).add(BinaryExpr(results.back(), thenResultExpr.get(), BinaryOp::Assign));
			}

			//then式が実行し終わったら自動的にif式の次のノードに処理が飛ぶようにする
			graph.node(thenNodeIndex).add(LRValue(true));
			graph.addEdge(thenNodeIndex, nextNodeIndex, true);
		}

		if (node.else_expr)
		{
			const size_t elseNodeIndex = graph.addNode();
			graph.addEdge(currentNodeIndex, elseNodeIndex, false);

			ControlFlowGraphMaker child(context, graph, elseNodeIndex);
			if (auto elseResultExpr = boost::apply_visitor(child, node.else_expr.get()))
			{
				results.push_back(ControlFlowGraph::NewVariable());
				graph.node(elseNodeIndex).add(BinaryExpr(results.back(), elseResultExpr.get(), BinaryOp::Assign));
			}

			//else式が実行し終わったら自動的にif式の次のノードに処理が飛ぶようにする
			graph.node(elseNodeIndex).add(LRValue(true));
			graph.addEdge(elseNodeIndex, nextNodeIndex, true);
		}
		else
		{
			//else式がない場合は、condが満たされないと自動で次のノードへ行く
			graph.addEdge(currentNodeIndex, nextNodeIndex, false);
		}

		currentNodeIndex = nextNodeIndex;

		if (results.empty())
		{
			return boost::none;
		}
		else if (results.size() == 1)
		{
			return results.front();
		}
		else
		{
			std::stringstream ss;
			ss << "Phi(";
			for (size_t i = 0; i + 1 < results.size(); ++i)
			{
				ss << results[i].toString() << ", ";
			}
			ss << results.back().toString() << ")";

			return Identifier(ss.str());
		}
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const For& node)
	{
		/*boost::apply_visitor(*this, node.rangeStart);
		boost::apply_visitor(*this, node.rangeEnd);
		boost::apply_visitor(*this, node.doExpr);*/
		return node;
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const ListConstractor& node)
	{
		/*for (const auto& expr : node.data)
		{
		boost::apply_visitor(*this, expr);
		}*/
		return node;
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const KeyExpr& node)
	{
		//boost::apply_visitor(*this, node.expr);
		return node;
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const RecordConstractor& node)
	{
		/*for (const auto& expr : node.exprs)
		{
		boost::apply_visitor(*this, expr);
		}*/
		return node;
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const Accessor& node)
	{
		node;
		return node;
	}

	void MakeControlFlowGraph(const Context& context, const Expr& constraintExpr, std::ostream& os)
	{
		ControlFlowGraph graph;
		ControlFlowGraphMaker graphMaker(context, graph, 0);

		std::cout << "constraint expr:" << std::endl;
		Printer printer(nullptr, std::cout);
		boost::apply_visitor(printer, constraintExpr);

		std::cout << "build graph:" << std::endl;

		if (auto resultExpr = boost::apply_visitor(graphMaker, constraintExpr))
		{
			graph.node(graphMaker.currentNodeIndex).add(resultExpr.get());
		}

		std::cout << "output graphviz:" << std::endl;

		graph.outputGraphViz(os);

		std::cout << "completed MakeControlFlowGraph:" << std::endl;
	}
}
