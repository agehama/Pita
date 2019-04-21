#pragma once
#pragma warning(disable:4996)

#include "Node.hpp"

namespace cgl
{
	//Expr -> Expr の変換を行う処理全般のひな型
	class ExprTransformer : public boost::static_visitor<Expr>
	{
	public:
		ExprTransformer() = default;
		virtual ~ExprTransformer() = default;

		virtual Expr operator()(const LRValue& node);
		virtual Expr operator()(const Identifier& node);
		virtual Expr operator()(const UnaryExpr& node);
		virtual Expr operator()(const BinaryExpr& node);
		virtual Expr operator()(const Lines& node);
		virtual Expr operator()(const DefFunc& node);
		virtual Expr operator()(const If& node);
		virtual Expr operator()(const For& node);
		virtual Expr operator()(const ListConstractor& node);
		virtual Expr operator()(const KeyExpr& node);
		virtual Expr operator()(const RecordConstractor& node);
		virtual Expr operator()(const Accessor& node);
		virtual Expr operator()(const DeclSat& node);
		virtual Expr operator()(const DeclFree& node);
		virtual Expr operator()(const Import& node);
		virtual Expr operator()(const Range& node);
		virtual Expr operator()(const Return& node);
	};
}
