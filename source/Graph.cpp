#pragma warning(disable:4996)
#include <iostream>

#include <Pita/Evaluator.hpp>
#include <Pita/Graph.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/Printer.hpp>
#include <Pita/IntrinsicGeometricFunctions.hpp>
#include <Pita/Program.hpp>

namespace cgl
{
	inline std::string GraphvizEscaped(const std::string& str)
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
					os << GraphvizEscaped(ss.str());
					os << "|";
				}
				else
				{
					os << "Branch " << GraphvizEscaped(ss.str());
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


	boost::optional<Expr> ControlFlowGraphMaker::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			return Expr(node);
		}
		//図形などの場合は評価される式を展開する
		return Expr(node);
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const UnaryExpr& node)
	{
		auto expr = boost::apply_visitor(*this, node.lhs);
		if (!expr)
		{
			CGL_Error("エラー");
		}

		return Expr(UnaryExpr(expr.get(), node.op));
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const BinaryExpr& node)
	{
		auto expr1 = boost::apply_visitor(*this, node.lhs);
		auto expr2 = boost::apply_visitor(*this, node.rhs);
		if (!expr1 || !expr2)
		{
			CGL_Error("エラー");
		}

		return Expr(BinaryExpr(expr1.get(), expr2.get(), node.op));
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
		return Expr(node);
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
			return Expr(results.front());
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

			return Expr(Identifier(ss.str()));
		}
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const For& node)
	{
		/*boost::apply_visitor(*this, node.rangeStart);
		boost::apply_visitor(*this, node.rangeEnd);
		boost::apply_visitor(*this, node.doExpr);*/
		return Expr(node);
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const ListConstractor& node)
	{
		/*for (const auto& expr : node.data)
		{
		boost::apply_visitor(*this, expr);
		}*/
		return Expr(node);
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const KeyExpr& node)
	{
		//boost::apply_visitor(*this, node.expr);
		return Expr(node);
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const RecordConstractor& node)
	{
		/*for (const auto& expr : node.exprs)
		{
		boost::apply_visitor(*this, expr);
		}*/
		return Expr(node);
	}

	boost::optional<Expr> ControlFlowGraphMaker::operator()(const Accessor& node)
	{
		return Expr(node);
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

	size_t ConstraintGraph::addNode()
	{
		m_graphNodes.emplace_back();
		m_edgeIndices.emplace_back();
		return m_graphNodes.size() - 1;
	}

	size_t ConstraintGraph::addNode(const ConstraintGraphNode& lines)
	{
		m_graphNodes.push_back(lines);
		m_edgeIndices.emplace_back();
		return m_graphNodes.size() - 1;
	}

	ConstraintGraphNode& ConstraintGraph::node(size_t nodeIndex)
	{
		return m_graphNodes[nodeIndex];
	}

	const ConstraintGraphNode& ConstraintGraph::node(size_t nodeIndex)const
	{
		return m_graphNodes[nodeIndex];
	}

	/*void ConstraintGraph::addEdge(size_t fromNode, size_t toNode, bool isTrue)
	{
		m_edgeIndices[fromNode].emplace_back(toNode, isTrue);
	}*/
	void ConstraintGraph::addEdge(size_t fromNode, size_t toNode)
	{
		m_edgeIndices[fromNode].emplace(toNode, EdgeInfo());
	}

	void ConstraintGraph::migrateNode(size_t beforeNode, size_t afterNode)
	{
		for (size_t i = 0; i < m_edgeIndices.size(); ++i)
		{
			if (i == beforeNode)
			{
				continue;
			}

			//外部からbeforeNodeに入力されるエッジ
			auto it = m_edgeIndices[i].find(beforeNode);
			if (it != m_edgeIndices[i].end())
			{
				m_edgeIndices[i].erase(beforeNode);
				m_edgeIndices[i].emplace(afterNode, it->second);
			}
		}

		//beforeNodeから外部に出力されるエッジ
		for (const auto& edge : m_edgeIndices[beforeNode])
		{
			m_edgeIndices[afterNode].emplace(edge);
		}
		m_edgeIndices[beforeNode].clear();
	}

	void ConstraintGraph::outputGraphViz(std::ostream& os, std::shared_ptr<Context> pContext)const
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

		for (size_t i = 0; i < m_graphNodes.size(); ++i)
		{
			const auto& node = m_graphNodes[i];
			if (IsType<CFGNodeNull>(node))
			{
				continue;
			}

			//os << "  node" << i << R"( [label = "{<ptop>)";
			if (auto opt = AsOpt<CFGNodeBlock>(node))
			{
				os << "  node" << i << R"( [label = "{)";

				std::stringstream ss;
				if(IsType<LRValue>(opt.get().expr), false)
				{
					Printer printer(pContext, ss);
					boost::apply_visitor(printer, opt.get().expr);
				}
				else
				{
					Printer2 printer(pContext, ss);
					boost::apply_visitor(printer, opt.get().expr);
				}
				os << GraphvizEscaped(ss.str());
			}
			else if (auto opt = AsOpt<CFGNodeIf>(node))
			{
				os << "  node" << i << R"( [shape = diamond, label = "{)";
				os << GraphvizEscaped("If");
			}
			else if (auto opt = AsOpt<CFGNodeVarAddress>(node))
			{
				os << "  node" << i << R"( [shape = doublecircle, color = "#FF0000", label = "{)";
				const auto name = pContext->makeLabel(opt.get().address);
				os << GraphvizEscaped(name + ": Address(" + opt.get().address.toString() + ")");
			}
			else if (auto opt = AsOpt<CFGNodeRecordConstruction>(node))
			{
				os << "  node" << i << R"( [shape = trapezium, label = "{)";
				os << GraphvizEscaped("Record Constructor");
			}
			else if (auto opt = AsOpt<CFGNodeFunction>(node))
			{
				os << "  node" << i << R"( [shape = hexagon, label = "{)";
				if (opt.get().functionAddress)
				{
					const auto name = pContext->makeLabel(opt.get().functionAddress.get());
					os << GraphvizEscaped(name + "()");
				}
				else
				{
					os << GraphvizEscaped("Call Function");
				}
			}
			/*else if (auto opt = AsOpt<CFGNodeRecordBegin>(node))
			{
				os << "  node" << i << R"( [shape = trapezium, label = "{)";
				os << GraphvizEscaped("Record begin");
			}
			else if (auto opt = AsOpt<CFGNodeRecordEnd>(node))
			{
				os << "  node" << i << R"( [shape = invtrapezium, label = "{)";
				os << GraphvizEscaped("Record end");
			}*/
			else if (auto opt = AsOpt<CFGNodeConstraint>(node))
			{
				os << "  node" << i << R"( [shape = parallelogram, label = "{)";
				os << GraphvizEscaped(opt.get().name());
			}
			else if (auto opt = AsOpt<CFGNodeLogicalOp>(node))
			{
				os << "  node" << i << R"( [shape = triangle, label = "{)";
				os << GraphvizEscaped(opt.get().name());
			}

			/*for (size_t exprIndex = 0; exprIndex < lines.exprs.size(); ++exprIndex)
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
			}*/

			//os << R"(|{<ptrue>true|<pfalse>false})";
			//os << R"(<pbottom>)";

			os << R"(}"];)" << "\n";
		}

		os << "\n";

		for (size_t fromNodeIndex = 0; fromNodeIndex < m_edgeIndices.size(); ++fromNodeIndex)
		{
			for (const auto& edge : m_edgeIndices[fromNodeIndex])
			{
				os << "  node" << fromNodeIndex;
				os << " -> node" << edge.first << ";\n";
				/*os << "  node" << fromNodeIndex << ":pbottom";
				os << " -> node" << edge.first << ":ptop;\n";*/
				//os << "  node" << fromNodeIndex << ":" << (edge.isTrueBranch ? "ptrue" : "pfalse");
				//os << " -> node" << edge.toNodeIndex << ":ptop;\n";
			}
		}

		os << "}\n";
	}

	void ConstraintGraph::addVariableAddresses(const Context& context, const std::unordered_set<Address>& vars)
	{
		std::unordered_map<Address, size_t> addressNodeIndexMap;
		for (const Address address : vars)
		{
			addressNodeIndexMap.emplace(address, addNode(CFGNodeVarAddress(address)));
		}

		for(size_t nodeIndex = 0; nodeIndex < m_graphNodes.size(); ++nodeIndex)
		{
			const auto& node = m_graphNodes[nodeIndex];
			if (auto opt = AsOpt<CFGNodeBlock>(node))
			{
				std::unordered_set<Address> reachableAddressSet;
				std::unordered_set<Address> newAddressSet;
				RegionVariable::Attribute currentAttribute = RegionVariable::Other;
				
				std::vector<RegionVariable> outputs;
				OutputAddresses outputAddresses([&](Address address) {return vars.find(address) != vars.end(); }, outputs);

				CheckExpr(opt.get().expr, context, reachableAddressSet, newAddressSet, currentAttribute, outputAddresses);

				for (const auto& currentVar : outputs)
				{
					addEdge(addressNodeIndexMap[currentVar.address], nodeIndex);
				}
			}
		}
	}

	std::vector<size_t> ConstraintGraph::flowInEdgesTo(size_t nodeIndex)const
	{
		std::vector<size_t> flowInNodes;
		for (size_t i = 0; i < m_edgeIndices.size(); ++i)
		{
			if (i == nodeIndex)
			{
				continue;
			}

			auto it = m_edgeIndices[i].find(nodeIndex);
			if (it != m_edgeIndices[i].end())
			{
				flowInNodes.emplace_back(i);
			}
		}
		return flowInNodes;
	}

	std::vector<size_t> ConstraintGraph::flowOutEdgesFrom(size_t nodeIndex)const
	{
		std::vector<size_t> flowOutNodes;
		for (const auto& edge : m_edgeIndices[nodeIndex])
		{
			flowOutNodes.emplace_back(edge.first);
		}
		return flowOutNodes;
	}

	Identifier ConstraintGraph::NewVariable()
	{
		static size_t auxiliaryVariables = 0;
		++auxiliaryVariables;
		return Identifier(std::string("$") + ToS(auxiliaryVariables));
	}

	struct AdditionalInfo
	{
		size_t ifConditionExprIndex;
		size_t ifJudgeIndex;
		size_t ifThenIndex;
		size_t ifElseIndex;
	};

	class ConstraintGraphMaker : public boost::static_visitor<size_t>
	{
	public:
		ConstraintGraphMaker(const Context& context,
			ConstraintGraph& graph,
			size_t& currentNodeIndex,
			bool isRootConstraintNode,
			bool isAtomicConstraintNode) :
			context(context),
			graph(graph),
			currentNodeIndex(currentNodeIndex),
			isRootConstraintNode(isRootConstraintNode),
			isAtomicConstraintNode(isAtomicConstraintNode)
		{}

		//階層全体で共有
		const Context& context;
		ConstraintGraph& graph;
		size_t& currentNodeIndex;

		//深さごとに独立
		bool isRootConstraintNode;
		bool isAtomicConstraintNode;

		boost::optional<AdditionalInfo> additionalInfo;

		size_t operator()(const Identifier& node);
		size_t operator()(const LRValue& node);
		size_t operator()(const UnaryExpr& node);
		size_t operator()(const BinaryExpr& node);
		size_t operator()(const Lines& node);
		size_t operator()(const DefFunc& node);
		size_t operator()(const If& node);
		size_t operator()(const For& node);
		size_t operator()(const ListConstractor& node);
		size_t operator()(const KeyExpr& node);
		size_t operator()(const RecordConstractor& node);
		size_t operator()(const Accessor& node);

		size_t operator()(const DeclSat& node) { CGL_Error("未対応"); }
		size_t operator()(const DeclFree& node) { CGL_Error("未対応"); }
		size_t operator()(const Import& node) { CGL_Error("未対応"); }
		size_t operator()(const Range& node) { CGL_Error("未対応"); }
		size_t operator()(const Return& node) { CGL_Error("未対応"); }
	};

	size_t ConstraintGraphMaker::operator()(const Identifier& node)
	{
		return graph.addNode(CFGNodeBlock(node));
	}

	//図形などの場合は評価される式を展開する
	size_t ConstraintGraphMaker::operator()(const LRValue& node)
	{
		return graph.addNode(CFGNodeBlock(node));
		//if (node.isRValue())
		//{
		//	//return node;
		//}
		//return node;
	}

	size_t ConstraintGraphMaker::operator()(const UnaryExpr& node)
	{
		const size_t unaryOpIndex = graph.addNode();

		const size_t lhsIndex = boost::apply_visitor(*this, node.lhs);

		auto& lhsNode = graph.node(lhsIndex);
		if (!IsType<CFGNodeBlock>(lhsNode))
		{
			CGL_Error("エラー");
		}

		/*
		graph.migrateNode(lhsIndex, unaryOpIndex);

		lhsNode = CFGNodeNull();
		graph.node(unaryOpIndex) = CFGNodeBlock(node);
		return unaryOpIndex;
		//*/

		//*
		graph.addEdge(lhsIndex, unaryOpIndex);
		graph.node(unaryOpIndex) = CFGNodeBlock(Identifier(UnaryOpToStr(node.op)));

		return unaryOpIndex;
		//*/
	}

	size_t ConstraintGraphMaker::operator()(const BinaryExpr& node)
	{
		const auto convertOp = [](BinaryOp op)->boost::optional<CFGNodeConstraint::Op>
		{
			switch (op)
			{
			case cgl::Equal:        return CFGNodeConstraint::Equal;
			case cgl::NotEqual:     return CFGNodeConstraint::NotEqual;
			case cgl::LessThan:     return CFGNodeConstraint::LessThan;
			case cgl::LessEqual:    return CFGNodeConstraint::LessEqual;
			case cgl::GreaterThan:  return CFGNodeConstraint::GreaterThan;
			case cgl::GreaterEqual: return CFGNodeConstraint::GreaterEqual;
			default:
				return boost::none;
			}
		};

		//BinaryOpが論理演算子であれば子供はどちらも制約ノード
		if (isRootConstraintNode && (node.op == BinaryOp::And || node.op == BinaryOp::Or))
		{
			ConstraintGraphMaker child(context, graph, currentNodeIndex, true, true);

			const size_t binaryOpIndex = graph.addNode();

			const size_t lhsIndex = boost::apply_visitor(child, node.lhs);
			const size_t rhsIndex = boost::apply_visitor(child, node.rhs);

			graph.addEdge(lhsIndex, binaryOpIndex);
			graph.addEdge(rhsIndex, binaryOpIndex);

			graph.node(binaryOpIndex) = CFGNodeLogicalOp(node.op == BinaryOp::And ? CFGNodeLogicalOp::And : CFGNodeLogicalOp::Or);

			return binaryOpIndex;
		}

		//現在のノードは 原子制約ノード or ブロックノードだが、どちらにしろこれより下は全てブロックノード
		ConstraintGraphMaker child(context, graph, currentNodeIndex, false, false);

		//原子制約ノード
		auto atomicConstraintOpt = convertOp(node.op);
		if(isAtomicConstraintNode && atomicConstraintOpt)
		{
			std::cout << "isAtomicConstraintNode " << std::endl;
			const size_t binaryOpIndex = graph.addNode();

			const size_t lhsIndex = boost::apply_visitor(child, node.lhs);
			const size_t rhsIndex = boost::apply_visitor(child, node.rhs);

			graph.addEdge(lhsIndex, binaryOpIndex);
			graph.addEdge(rhsIndex, binaryOpIndex);

			graph.node(binaryOpIndex) = CFGNodeConstraint({ lhsIndex, rhsIndex }, atomicConstraintOpt.get());

			return binaryOpIndex;
		}
		//ブロックノード
		else
		{
			const size_t binaryOpIndex = graph.addNode();

			//ブロックノードの場合は基本的に分割する意味はないのでノードを結合したい
			const size_t lhsIndex = boost::apply_visitor(child, node.lhs);
			const size_t rhsIndex = boost::apply_visitor(child, node.rhs);

			const auto& lhsNode = graph.node(lhsIndex);
			const auto& rhsNode = graph.node(rhsIndex);

			/*if (!IsType<CFGNodeBlock>(lhsNode) || !IsType<CFGNodeBlock>(rhsNode))
			{
				CGL_Error("エラー");
			}*/

			if (IsType<CFGNodeBlock>(lhsNode) && IsType<CFGNodeBlock>(rhsNode), false)
			{
				//子供の持っているエッジを全て親に張り替える
				graph.migrateNode(lhsIndex, binaryOpIndex);
				graph.migrateNode(rhsIndex, binaryOpIndex);

				graph.node(lhsIndex) = CFGNodeNull();
				graph.node(rhsIndex) = CFGNodeNull();
				graph.node(binaryOpIndex) = CFGNodeBlock(node);

				return binaryOpIndex;
			}
			else
			{
				graph.addEdge(lhsIndex, binaryOpIndex);
				graph.addEdge(rhsIndex, binaryOpIndex);
				graph.node(binaryOpIndex) = CFGNodeBlock(Identifier(BinaryOpToStr(node.op)));

				return binaryOpIndex;
			}
		}
	}

#ifdef commentout
	size_t ConstraintGraphMaker::operator()(const Lines& lines)
	{
		const Expr expr = ExpandedEmptyLines(lines);
		if (!IsType<Lines>(expr))
		{
			return boost::apply_visitor(*this, expr);
		}

		const Lines& node = As<Lines>(expr);

		//現在のLinesが最後に制約を持つような場合は、中身をちゃんと見る
		if (isRootConstraintNode)
		{
			Lines beforeLast;
			for (size_t i = 0; i + 1 < node.exprs.size(); ++i)
			{
				beforeLast.add(node.exprs[i]);
			}

			const size_t processIndex = graph.addNode();
			graph.node(processIndex) = CFGNodeBlock(beforeLast);

			ConstraintGraphMaker lastChild(context, graph, currentNodeIndex, false, true);
			if (!node.exprs.empty())
			{
				const size_t constraintIndex = boost::apply_visitor(lastChild, node.exprs.back());
				graph.addEdge(processIndex, constraintIndex);

				//ここで返すのはconstraintIndexで良いのか？
				//少なくともLinesはもう使わないと思うなのでそっちが必要ということはないはず
				return constraintIndex;
			}
			else
			{
				return processIndex;
			}
		}
		//そうでない場合はただブロックに入れて返すだけ
		else
		{
			return graph.addNode(CFGNodeBlock(node));
		}
	}
#endif

	size_t ConstraintGraphMaker::operator()(const Lines& lines)
	{
		const Expr expr = ExpandedEmptyLines(lines);
		if (!IsType<Lines>(expr))
		{
			return boost::apply_visitor(*this, expr);
		}

		const Lines& node = As<Lines>(expr);

		size_t lastNodeIndex = currentNodeIndex;
		if (!node.exprs.empty())
		{
			lastNodeIndex = boost::apply_visitor(*this, node.exprs.front());
		}

		for (size_t i = 1; i < node.exprs.size(); ++i)
		{
			auto& lastNode = graph.node(lastNodeIndex);
			const size_t currentNodeIndex = boost::apply_visitor(*this, node.exprs[i]);
			auto& currentNode = graph.node(currentNodeIndex);

			graph.addEdge(lastNodeIndex, currentNodeIndex);
			lastNodeIndex = currentNodeIndex;
		}

		return lastNodeIndex;
	}

	size_t ConstraintGraphMaker::operator()(const DefFunc& node)
	{
		return graph.addNode(CFGNodeBlock(node));
	}

	size_t ConstraintGraphMaker::operator()(const If& node)
	{
		ConstraintGraphMaker child(context, graph, currentNodeIndex, false, false);

		CFGNodeIf ifNode;
		AdditionalInfo info;

		const size_t conditionNodeIndex = boost::apply_visitor(child, node.cond_expr);
		const size_t ifNodeIndex = graph.addNode();

		info.ifConditionExprIndex = conditionNodeIndex;
		info.ifJudgeIndex = ifNodeIndex;

		graph.addEdge(conditionNodeIndex, ifNodeIndex);

		const size_t thenNodeIndex = boost::apply_visitor(child, node.then_expr);
		ifNode.yesIndex = thenNodeIndex;
		info.ifThenIndex = thenNodeIndex;
		graph.addEdge(ifNodeIndex, thenNodeIndex);

		const size_t elseNodeIndex = (node.else_expr
			? boost::apply_visitor(child, node.else_expr.get())
			: graph.addNode(CFGNodeNull())
			);
		ifNode.noIndex = elseNodeIndex;
		info.ifElseIndex = elseNodeIndex;
		graph.addEdge(ifNodeIndex, elseNodeIndex);

		graph.node(ifNodeIndex) = ifNode;
		additionalInfo = info;

		return thenNodeIndex;
		//return ifNodeIndex;
	}

	size_t ConstraintGraphMaker::operator()(const For& node)
	{
		ConstraintGraphMaker child(context, graph, currentNodeIndex, false, false);

		const Expr startExpr = BinaryExpr(node.loopCounter, node.rangeStart, BinaryOp::Assign);
		const size_t startNodeIndex = boost::apply_visitor(child, startExpr);

		If ifexpr;
		ifexpr.cond_expr = BinaryExpr(node.loopCounter, node.rangeEnd, BinaryOp::LessEqual);
		ifexpr.then_expr = node.doExpr;

		const Expr expr = ifexpr;
		boost::apply_visitor(child, expr);
		const size_t conditionNodeIndex = child.additionalInfo.get().ifConditionExprIndex;
		const size_t doNodeIndex = child.additionalInfo.get().ifThenIndex;
		const size_t breakNodeIndex = child.additionalInfo.get().ifElseIndex;

		additionalInfo = boost::none;

		graph.addEdge(startNodeIndex, conditionNodeIndex);

		Expr addExpr = BinaryExpr(node.loopCounter, BinaryExpr(node.loopCounter, LRValue(1), BinaryOp::Add), BinaryOp::Assign);
		const size_t continuousNodeIndex = boost::apply_visitor(child, addExpr);

		graph.addEdge(doNodeIndex, continuousNodeIndex);
		graph.addEdge(continuousNodeIndex, conditionNodeIndex);

		return breakNodeIndex;
	}

	size_t ConstraintGraphMaker::operator()(const ListConstractor& node)
	{
		return graph.addNode(CFGNodeBlock(node));
	}

	size_t ConstraintGraphMaker::operator()(const KeyExpr& node)
	{
		return graph.addNode(CFGNodeBlock(node));
	}

	size_t ConstraintGraphMaker::operator()(const RecordConstractor& node)
	{
		const size_t recordNodeIndex = graph.addNode();

		std::vector<size_t> recordExprIndices;
		for (size_t i = 0; i < node.exprs.size(); ++i)
		{
			const size_t currentExprIndex = graph.addNode(CFGNodeBlock(node.exprs[i]));
			graph.addEdge(currentExprIndex, recordNodeIndex);
			recordExprIndices.push_back(currentExprIndex);
		}

		graph.node(recordNodeIndex) = CFGNodeRecordConstruction(recordExprIndices);

		return recordNodeIndex;

		/*
		const size_t recordBeginIndex = graph.addNode(CFGNodeRecordBegin());
		const size_t recordEndIndex = graph.addNode(CFGNodeRecordEnd());

		for (size_t i = 0; i < node.exprs.size(); ++i)
		{
			const size_t currentExprIndex = graph.addNode(CFGNodeBlock(node.exprs[i]));

			if (i == 0)
			{
				graph.addEdge(recordBeginIndex, currentExprIndex);
			}
			if (i + 1 == node.exprs.size())
			{
				graph.addEdge(currentExprIndex, recordEndIndex);
			}
		}

		if (node.exprs.empty())
		{
			graph.addEdge(recordBeginIndex, recordEndIndex);
		}

		return recordEndIndex;
		*/
	}

	size_t ConstraintGraphMaker::operator()(const Accessor& node)
	{
		const size_t headNodeIndex = boost::apply_visitor(*this, node.head);

		if (node.accesses.empty())
		{
			return headNodeIndex;
		}

		size_t prevControlNodeIndex = headNodeIndex;

		for (const auto& access : node.accesses)
		{
			/*if (auto opt = AsOpt<FunctionAccess>(access))
			{
				const size_t callerNodeIndex = graph.addNode();
				graph.addEdge(prevControlNode, callerNodeIndex);

				std::vector<size_t> argumentIndices;
				const auto& arguments = opt.get().actualArguments;
				for (size_t argIndex = 0; argIndex < arguments.size(); ++argIndex)
				{
					const size_t currentArgumentIndex = boost::apply_visitor(*this, arguments[argIndex]);
					graph.addEdge(currentArgumentIndex, callerNodeIndex);
					argumentIndices.push_back(currentArgumentIndex);
				}

				graph.node(callerNodeIndex) = CFGNodeFunction(argumentIndices, prevControlNode);
				prevControlNode = callerNodeIndex;
			}*/
			if (auto opt = AsOpt<FunctionAccess>(access))
			{
				const size_t callerNodeIndex = graph.addNode();
				graph.addEdge(prevControlNodeIndex, callerNodeIndex);

				std::vector<size_t> argumentIndices;
				const auto& arguments = opt.get().actualArguments;
				for (size_t argIndex = 0; argIndex < arguments.size(); ++argIndex)
				{
					const size_t currentArgumentIndex = boost::apply_visitor(*this, arguments[argIndex]);
					graph.addEdge(currentArgumentIndex, callerNodeIndex);
					argumentIndices.push_back(currentArgumentIndex);
				}

				CFGNodeFunction functionNode(argumentIndices, prevControlNodeIndex);
				{
					auto& prevControlNode = graph.node(prevControlNodeIndex);
					if (IsType<CFGNodeBlock>(prevControlNode) && graph.flowInEdgesTo(prevControlNodeIndex).empty())
					{
						Eval evaluator(context.m_weakThis.lock());
						LRValue lrvalue = boost::apply_visitor(evaluator, As<CFGNodeBlock>(prevControlNode).expr);
						if (auto opt = lrvalue.deref(context))
						{
							functionNode.functionAddress = opt.get();
						}
					}
				}

				graph.node(callerNodeIndex) = functionNode;
				prevControlNodeIndex = callerNodeIndex;
			}
		}

		return prevControlNodeIndex;
	}

	ConstraintGraph ConstructConstraintGraph(std::shared_ptr<Context> pContext, const Expr& constraintExpr)
	{
		ConstraintGraph graph;
		size_t currentNodeIndex = 0;
		ConstraintGraphMaker graphMaker(*pContext, graph, currentNodeIndex, true, true);

		std::cout << "constraint expr:" << std::endl;
		Printer printer(nullptr, std::cout);
		boost::apply_visitor(printer, constraintExpr);

		std::cout << "build graph:" << std::endl;
		boost::apply_visitor(graphMaker, constraintExpr);

		return graph;
	}
}
