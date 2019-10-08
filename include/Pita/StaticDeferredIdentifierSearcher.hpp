#pragma once
#include "Node.hpp"
#include "Context.hpp"
#include "ExprTransformer.hpp"

namespace cgl
{
	// 基本的にはローカル変数を拾ってくるだけなので大部分はClosureMakerのサブセットになる（->共通化できない？）
	// ClosureMakerと異なるのは返す式の形（遅延識別子を置き換えたものが返る）
	class StaticDeferredIdentifierSearcher : public ExprTransformer
	{
	public:
		//全ての外部環境を予め与える必要がある
		StaticDeferredIdentifierSearcher() = delete;

		StaticDeferredIdentifierSearcher(const std::set<std::string>& localVariables, const ScopeAddress& parentScopeInfo, DeferredIdentifiers& deferredIdentifiers)
			: localVariables(localVariables)
			, currentScopeInfo(parentScopeInfo)
			, deferredIdentifiers(deferredIdentifiers)
		{}

		std::set<std::string> localVariables;
		ScopeAddress currentScopeInfo;
		ScopeIndex currentScopeIndex = 0;

		bool hasDeferredCall = false;

		DeferredIdentifiers& deferredIdentifiers;

		void addLocalVariable(const std::string& localVariable);
		bool isLocalVariable(const std::string& localVariable)const;
		ScopeAddress makeChildScope();

		Expr operator()(const LRValue& node)override;
		Expr operator()(const Identifier& node)override;
		Expr operator()(const UnaryExpr& node)override;
		Expr operator()(const BinaryExpr& node)override;
		Expr operator()(const Lines& node)override;
		Expr operator()(const DefFunc& node)override;
		Expr operator()(const If& node)override;
		Expr operator()(const For& node)override;
		Expr operator()(const ListConstractor& node)override;
		Expr operator()(const KeyExpr& node)override;
		Expr operator()(const RecordConstractor& node)override;
		Expr operator()(const Accessor& node)override;
		Expr operator()(const DeclSat& node)override;
		Expr operator()(const DeclFree& node)override;
		Expr operator()(const Import& node)override;
		Expr operator()(const Range& node)override;
		Expr operator()(const Return& node)override;
	};

	//即時評価可能な式も遅延呼び出し可能な形で返す
	class StaticDeferredIdentifierSearcher2 : public ExprTransformer
	{
	public:
		//全ての外部環境を予め与える必要がある
		StaticDeferredIdentifierSearcher2() = delete;

		StaticDeferredIdentifierSearcher2(const std::set<std::string>& localVariables, const ScopeAddress& parentScopeInfo, DeferredIdentifiers& deferredIdentifiers)
			: localVariables(localVariables)
			, currentScopeInfo(parentScopeInfo)
			, deferredIdentifiers(deferredIdentifiers)
		{}

		std::set<std::string> localVariables;
		ScopeAddress currentScopeInfo;
		ScopeIndex currentScopeIndex = 0;

		bool hasDeferredCall = false;

		DeferredIdentifiers& deferredIdentifiers;

		void addLocalVariable(const std::string& localVariable);
		bool isLocalVariable(const std::string& localVariable)const;
		ScopeAddress makeChildScope();

		Expr operator()(const LRValue& node)override;
		Expr operator()(const Identifier& node)override;
		Expr operator()(const UnaryExpr& node)override;
		Expr operator()(const BinaryExpr& node)override;
		Expr operator()(const Lines& node)override;
		Expr operator()(const DefFunc& node)override;
		Expr operator()(const If& node)override;
		Expr operator()(const For& node)override;
		Expr operator()(const ListConstractor& node)override;
		Expr operator()(const KeyExpr& node)override;
		Expr operator()(const RecordConstractor& node)override;
		Expr operator()(const Accessor& node)override;
		Expr operator()(const DeclSat& node)override;
		Expr operator()(const DeclFree& node)override;
		Expr operator()(const Import& node)override;
		Expr operator()(const Range& node)override;
		Expr operator()(const Return& node)override;
	};
}
