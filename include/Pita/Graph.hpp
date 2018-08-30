#pragma once
#pragma warning(disable:4996)
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	struct EdgeInfo
	{
		EdgeInfo() = default;
		EdgeInfo(size_t toNodeIndex, bool isTrueBranch) :
			toNodeIndex(toNodeIndex),
			isTrueBranch(isTrueBranch)
		{}

		size_t toNodeIndex;
		bool isTrueBranch;
	};

	class ControlFlowGraph
	{
	public:
		ControlFlowGraph() :
			m_blockNodes(1),
			m_edgeIndices(1)
		{}

		size_t addNode();
		size_t addNode(const Lines& lines);

		Lines & node(size_t nodeIndex);
		const Lines & node(size_t nodeIndex)const;

		void addEdge(size_t fromNode, size_t toNode, bool isTrue);

		void outputGraphViz(std::ostream& os)const;

		static Identifier NewVariable();

	private:
		std::string escaped(const std::string& str)const;

		std::vector<Lines> m_blockNodes;
		std::vector<std::vector<EdgeInfo>> m_edgeIndices;
	};

	class ControlFlowGraphMaker : public boost::static_visitor<boost::optional<Expr>>
	{
	public:
		ControlFlowGraphMaker(const Context& context, ControlFlowGraph& graph, size_t currentNodeIndex) :
			context(context),
			graph(graph),
			currentNodeIndex(currentNodeIndex)
		{}

		const Context& context;
		ControlFlowGraph& graph;
		size_t currentNodeIndex;

		boost::optional<Expr> operator()(const Identifier& node) { return node; }

		boost::optional<Expr> operator()(const LRValue& node);
		boost::optional<Expr> operator()(const UnaryExpr& node);
		boost::optional<Expr> operator()(const BinaryExpr& node);
		boost::optional<Expr> operator()(const Lines& node);
		boost::optional<Expr> operator()(const DefFunc& node);
		boost::optional<Expr> operator()(const If& node);
		boost::optional<Expr> operator()(const For& node);
		boost::optional<Expr> operator()(const ListConstractor& node);
		boost::optional<Expr> operator()(const KeyExpr& node);
		boost::optional<Expr> operator()(const RecordConstractor& node);
		boost::optional<Expr> operator()(const Accessor& node);

		boost::optional<Expr> operator()(const DeclSat& node) { CGL_Error("未対応"); }
		boost::optional<Expr> operator()(const DeclFree& node) { CGL_Error("未対応"); }
		boost::optional<Expr> operator()(const Import& node) { CGL_Error("未対応"); }
		boost::optional<Expr> operator()(const Range& node) { CGL_Error("未対応"); }
		boost::optional<Expr> operator()(const Return& node) { CGL_Error("未対応"); }
	};

	void MakeControlFlowGraph(const Context& context, const Expr& constraintExpr, std::ostream& os);

	//struct CFGNodeConstant;
	//struct CFGNodeVariable;
	struct CFGNodeBlock;
	//struct CFGNodeFunction;
	struct CFGNodeConstraint;
	struct CFGNodeLogicalOp;

	using ConstraintGraphNode = boost::variant<
		CFGNodeBlock,
		CFGNodeConstraint,
		CFGNodeLogicalOp>;

	struct CFGNodeBlock
	{

	};

	struct CFGNodeConstraint
	{

	};

	struct CFGNodeLogicalOp
	{
		enum Op { And, Or };
	};

	/*class ConstraintGraph
	{
	public:
	private:
	};*/
}
