#pragma once
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	//class DynamicDeferredIdentifierSearcher 
	//
	// 代入式を置き換えた #MakeClosure() 命令の中で呼ばれる
	// 主にレコード継承の中で呼び出した関数が遅延呼び出し式なのか、もしくは継承元で定義された関数呼び出しなのかを調べるために使う
	// 後者の場合はここで DeferredCall を持っていない（つまり即時呼び出し可能である）ことが決定できる
	//
	// 即時呼び出し可能な場合の評価方法について：
	// 　部分式に DeferredCall が入っている場合があるためそのまま Eval には渡せない
	// 　この場合の評価方法は2通り考えられる
	// 　　1. 即時呼び出し可能か判定すると同時に DeferredCall を取り除いた式を返す
	// 　　2. Eval を拡張して DeferredCall を無視するオプションを付ける
	// 　1と2では1の方が評価される式が明示的に決まるため1の方が良い
	//
	// 1の方法で正しく評価が行えるか：
	// 　1で対応するためには、即時呼び出しの中で呼ばれ得る全ての関数は定義に DeferredCall を持たないことを保証できる必要がある
	// 　これは、即時呼び出しの中で呼び出せる関数は全て評価済みの関数である（逆に DeferredCall を持った関数に Address が振られることはない）ことから保証できる
	// 　
	// したがって、ここでは即時呼び出し可能かの判定を行い、可能な場合は即時呼び出し用の式を返すことにする
	class DynamicDeferredIdentifierSearcher : public boost::static_visitor<Expr>
	{
	public:
		//全ての外部環境を予め与える必要がある
		DynamicDeferredIdentifierSearcher() = delete;

		DynamicDeferredIdentifierSearcher(const std::set<std::string>& localVariables, const ScopeAddress& parentScopeInfo, DeferredIdentifiers& deferredIdentifiers, Context& context)
			: localVariables(localVariables)
			, currentScopeInfo(parentScopeInfo)
			, deferredIdentifiers(deferredIdentifiers)
			, context(context)
		{}

		std::set<std::string> localVariables;
		ScopeAddress currentScopeInfo;
		ScopeIndex currentScopeIndex = 0;

		bool hasDeferredCall = false;

		Context& context;

		DeferredIdentifiers& deferredIdentifiers;

		void addLocalVariable(const std::string& localVariable);
		bool isLocalVariable(const std::string& localVariable)const;
		ScopeAddress makeChildScope();

		Expr operator()(const LRValue& node);
		Expr operator()(const Identifier& node);
		Expr operator()(const UnaryExpr& node);
		Expr operator()(const BinaryExpr& node);
		Expr operator()(const Lines& node);
		Expr operator()(const DefFunc& node);
		Expr operator()(const If& node);
		Expr operator()(const For& node);
		Expr operator()(const ListConstractor& node);
		Expr operator()(const KeyExpr& node);
		Expr operator()(const RecordConstractor& node);
		Expr operator()(const Accessor& node);
		Expr operator()(const DeclSat& node);
		Expr operator()(const DeclFree& node);
		Expr operator()(const Import& node);
		Expr operator()(const Range& node);
		Expr operator()(const Return& node);
	};
}
