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

		boost::optional<Expr> operator()(const Identifier& node) { return Expr(node); }

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
	struct CFGNodeNull;
	struct CFGNodeBlock;
	struct CFGNodeIf;
	struct CFGNodeFunction;
	struct CFGNodeConstraint;
	struct CFGNodeLogicalOp;
	struct CFGNodeRecordConstruction;
	struct CFGNodeVarAddress;

	using ConstraintGraphNode = boost::variant<
		CFGNodeNull,
		CFGNodeVarAddress,
		CFGNodeBlock,
		CFGNodeIf,
		CFGNodeRecordConstruction,
		CFGNodeFunction,
		CFGNodeConstraint,
		CFGNodeLogicalOp>;

	//グラフを全てインデックスで管理しているため、
	//ノードを消す時もインデックスがずれないように空のノードに置き換える
	struct CFGNodeNull {};

	struct CFGNodeVarAddress
	{
		CFGNodeVarAddress() = default;
		CFGNodeVarAddress(Address address) :
			address(address)
		{}

		Address address;
	};

	struct CFGNodeBlock
	{
		CFGNodeBlock() = default;
		CFGNodeBlock(const Expr& expr) :
			expr(expr)
		{}

		Expr expr;
	};

	struct CFGNodeIf
	{
		CFGNodeIf() = default;
		CFGNodeIf(size_t yesIndex, size_t noIndex) :
			yesIndex(yesIndex), noIndex(noIndex)
		{}

		size_t yesIndex;
		size_t noIndex;
	};

	struct CFGNodeRecordConstruction
	{
		CFGNodeRecordConstruction() = default;
		CFGNodeRecordConstruction(const std::vector<size_t>& exprNodeIndices) :
			exprNodeIndices(exprNodeIndices)
		{}

		std::vector<size_t> exprNodeIndices;
	};

	struct FunctionInfo
	{
		//Distance, Touch など制約に関わりそうな関数はtrue
		//Printなど明らかに関係ないものはfalse
		bool isImportantFunction;

		//引数が図形を取るはずかどうか
		enum ExpectedType {Shape, Num};
		std::vector<ExpectedType> estimatedArgumentsTypes;

		//
	};

	//struct CFGNodeFunction
	//{
	//	CFGNodeFunction() = default;
	//	CFGNodeFunction(const std::vector<size_t>& argumentNodeIndices, size_t calleeFunctionNodeIndex) :
	//		argumentNodeIndices(argumentNodeIndices),
	//		calleeFunctionNodeIndex(calleeFunctionNodeIndex)
	//	{}

	//	//個々の引数ノードから伸びるエッジと同一だが、
	//	//エッジには引数の順序がないのでここで順序付けて保存しておく
	//	std::vector<size_t> argumentNodeIndices;
	//	size_t calleeFunctionNodeIndex;
	//};

	struct CFGNodeFunction
	{
		CFGNodeFunction() = default;
		CFGNodeFunction(const std::vector<size_t>& argumentNodeIndices,
			size_t calleeFunctionNodeIndex) :
			argumentNodeIndices(argumentNodeIndices),
			calleeFunctionNodeIndex(calleeFunctionNodeIndex)
		{}
		CFGNodeFunction(const std::vector<size_t>& argumentNodeIndices,
			size_t calleeFunctionNodeIndex,
			Address functionAddress) :
			argumentNodeIndices(argumentNodeIndices),
			calleeFunctionNodeIndex(calleeFunctionNodeIndex),
			functionAddress(functionAddress)
		{}

		//個々の引数ノードから伸びるエッジと同一だが、
		//エッジには引数の順序がないのでここで順序付けて保存しておく
		std::vector<size_t> argumentNodeIndices;
		size_t calleeFunctionNodeIndex;

		//グラフ可視化時になるべく名前を表示したいため、すぐアドレスを取れる場合だけ持っておく
		boost::optional<Address> functionAddress;
	};

	struct CFGNodeConstraint
	{
		enum Op {
			Equal,
			NotEqual,
			LessThan,
			LessEqual,
			GreaterThan,
			GreaterEqual
		};

		CFGNodeConstraint() = default;
		CFGNodeConstraint(const std::vector<size_t>& argumentNodeIndices, Op op):
			argumentNodeIndices(argumentNodeIndices),
			op(op)
		{}

		std::string name()const
		{
			switch (op)
			{
			case cgl::CFGNodeConstraint::Equal:        return "Equal";
			case cgl::CFGNodeConstraint::NotEqual:     return "NotEqual";
			case cgl::CFGNodeConstraint::LessThan:     return "LessThan";
			case cgl::CFGNodeConstraint::LessEqual:    return "LessEqual";
			case cgl::CFGNodeConstraint::GreaterThan:  return "GreaterThan";
			case cgl::CFGNodeConstraint::GreaterEqual: return "GreaterEqual";
			default: return "Unknown";
			}
		}

		//個々の引数ノードから伸びるエッジと同一だが、
		//エッジには引数の順序がないのでここで順序付けて保存しておく
		std::vector<size_t> argumentNodeIndices;
		Op op;
	};

	struct CFGNodeLogicalOp
	{
		enum Op { And, Or };

		CFGNodeLogicalOp() = default;
		CFGNodeLogicalOp(Op op) :
			op(op)
		{}

		std::string name()const
		{
			switch (op)
			{
			case cgl::CFGNodeLogicalOp::And: return "And";
			case cgl::CFGNodeLogicalOp::Or:  return "Or";
			default: return "Unknown";
			}
		}

		Op op;
	};

	class ConstraintGraph
	{
	public:
		ConstraintGraph() :
			m_graphNodes(1),
			m_edgeIndices(1)
		{}

		size_t addNode();
		size_t addNode(const ConstraintGraphNode& lines);

		ConstraintGraphNode& node(size_t nodeIndex);
		const ConstraintGraphNode& node(size_t nodeIndex)const;

		void addEdge(size_t fromNode, size_t toNode);

		void migrateNode(size_t beforeNode, size_t AfterNode);

		void outputGraphViz(std::ostream& os, std::shared_ptr<Context> pContext)const;

		void addVariableAddresses(const Context& context, const std::unordered_set<Address>& vars);

		std::vector<size_t> flowInEdgesTo(size_t nodeIndex)const;
		std::vector<size_t> flowOutEdgesFrom(size_t nodeIndex)const;

		static Identifier NewVariable();

	private:

		std::vector<ConstraintGraphNode> m_graphNodes;
		std::vector<std::unordered_map<size_t, EdgeInfo>> m_edgeIndices;
	};

	ConstraintGraph ConstructConstraintGraph(std::shared_ptr<Context> pContext, const Expr& constraintExpr);
}
