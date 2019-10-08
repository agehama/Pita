#include <Pita/Context.hpp>
#include <Pita/DynamicDeferredIdentifierSearcher.hpp>
#include <Pita/ExprTransformer.hpp>
#include <Pita/TreeLogger.hpp>
#include <Pita/ClosureMaker.hpp>

namespace cgl
{
	void DynamicDeferredIdentifierSearcher::addLocalVariable(const std::string& localVariable)
	{
		localVariables.insert(localVariable);
	}

	bool DynamicDeferredIdentifierSearcher::isLocalVariable(const std::string& localVariable)const
	{
		return localVariables.find(localVariable) != localVariables.end();
	}

	ScopeAddress DynamicDeferredIdentifierSearcher::makeChildScope()
	{
		ScopeAddress childScope = currentScopeInfo;
		childScope.push_back(currentScopeIndex);
		++currentScopeIndex;
		return childScope;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const LRValue& node) { return node; }

	Expr DynamicDeferredIdentifierSearcher::operator()(const Identifier& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const Identifier& node)");

		if (isLocalVariable(node))
		{
			return node;
		}

		// 遅延式の定義
		// 定義の中身が本当に遅延しなければならないか確認
		if (node.isMakeClosure())
		{
			const auto [scopeAddress, rawIdentifier] = node.decomposed();

			if (auto opt = context.getDeferredFunction(node))
			{
				DefFuncWithScopeInfo& func = opt.get();
				// ここは currentScopeInfo?
				// func.scopeInfo;
				DynamicDeferredIdentifierSearcher child(localVariables, currentScopeInfo, deferredIdentifiers, context);
				Expr originalFuncDef = boost::apply_visitor(child, func.generalDef.expr);
				hasDeferredCall = hasDeferredCall || child.hasDeferredCall;

				// 遅延しなければならない場合：参照可能なものはアドレスに変換して返す
				if (child.hasDeferredCall)
				{
					std::set<std::string> functionArguments;
					for (const auto& name : func.generalDef.arguments)
					{
						functionArguments.insert(name.toString());
					}
					return AsClosure(context, func.generalDef.expr, functionArguments);
				}
				// 遅延しなくてよい場合：元の式を即時評価して返す
				else
				{
					BinaryExpr originalExpr(rawIdentifier, originalFuncDef, BinaryOp::Assign);
					return originalExpr.setLocation(node);
				}
			}
			else
			{
				// この visitor は MakeClosure 内で呼ばれるはずなのでここで定義が context に存在しないのはおかしい
				CGL_Error("DynamicDeferredIdentifierSearcher エラー");
			}
		}

		// 遅延式の呼び出し
		// 呼び出される式が本当に遅延しなければならないか確認
		if (node.isDeferredCall())
		{
			const auto [scopeAddress, rawIdentifier] = node.decomposed();

			if (Address address = context.findAddress(rawIdentifier);
				isLocalVariable(rawIdentifier) || address.isValid())
			{
				return rawIdentifier;
			}
			else
			{
				hasDeferredCall = true;
				return node;
			}
		}

		// LocalVariable になければすべて遅延呼び出しか？
		// -> Import 元の環境、標準ライブラリなど事前に定義された変数とのチェックが済んでいれば遅延呼び出し

		return node;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const UnaryExpr& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const UnaryExpr& node)");

		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op).setLocation(node);
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const BinaryExpr& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const BinaryExpr& node)");

		// 遅延式の定義は既に Identifier に置き換え済みなのでここでは考慮する必要無い
		// LocalVariables の追加だけ行う
		const bool isDeclaration = IsType<Identifier>(node.lhs) && node.op == BinaryOp::Assign;
		if (isDeclaration)
		{
			addLocalVariable(As<Identifier>(node.lhs));
		}

		return BinaryExpr(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs),
			node.op).setLocation(node);
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const Lines& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const Lines& node)");

		Lines result = node;
		result.exprs.clear();
		DynamicDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers, context);
		for (const auto& expr : node.exprs)
		{
			result.add(boost::apply_visitor(child, expr));
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		return result;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const DefFunc& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const DefFunc& node)");

		DefFunc result = node;
		DynamicDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers, context);
		for (const auto& argument : node.arguments)
		{
			child.addLocalVariable(argument);
		}
		result.expr = boost::apply_visitor(child, node.expr);
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		return result;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const If& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const If& node)");

		If result = node;
		result.cond_expr = boost::apply_visitor(*this, node.cond_expr);
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (result.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.get());
		}
		return result;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const For& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const For& node)");

		For result = node;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
		{
			DynamicDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers, context);
			child.addLocalVariable(node.loopCounter);
			result.doExpr = boost::apply_visitor(child, node.doExpr);
			hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		}
		return result;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const ListConstractor& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const ListConstractor& node)");

		ListConstractor result = node;
		result.data.clear();
		DynamicDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers, context);
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(child, expr));
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		return result;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const KeyExpr& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const KeyExpr& node)");

		KeyExpr result = node;
		addLocalVariable(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const RecordConstractor& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const RecordConstractor& node)");

		RecordConstractor result = node;
		result.exprs.clear();
		DynamicDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers, context);
		for (const auto& expr : node.exprs)
		{
			result.exprs.push_back(boost::apply_visitor(child, expr));
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		return result;
	}

	// アクセッサの場合
	// どこかに参照できない呼び出しがあれば全体はdeferredcallのまま
	// head も tail も通常呼び出しならそのまま返していい
	//
	// 考えられるケースは次の4つ
	// 　head も tail も通常呼び出し
	// 　head のみ遅延呼び出し
	// 　tail のみ遅延呼び出し
	// 　head も tail も遅延呼び出し
	Expr DynamicDeferredIdentifierSearcher::operator()(const Accessor& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const Accessor& node)");

		std::set<std::string> baseRecordElements;
		Accessor result;
		if (IsType<Identifier>(node.head))
		{
			const Identifier& head = As<Identifier>(node.head);
			if (head.isDeferredCall())
			{
				const auto [scopeAddress, rawIdentifier] = head.decomposed();

				if (isLocalVariable(rawIdentifier))
				{
					scopeLog.write(rawIdentifier.toString() + " is local variable(deferred call)");
					result.head = rawIdentifier;
				}
				else if (Address address = context.findAddress(rawIdentifier); address.isValid())
				{
					scopeLog.write(rawIdentifier.toString() + " is evaluated value(deferred call)");
					result.head = rawIdentifier;
					//result.head = LRValue(EitherReference(rawIdentifier, address)).setLocation(node);

					// head がレコードの場合はその後に続くのは InheritAccess か RecordAccess であり
					// どちらにしろレコードのメンバをローカル環境に追加する必要がある
					const Val& val = context.expand(LRValue(address), node);
					if (auto recordOpt = AsOpt<Record>(val))
					{
						const Record& record = recordOpt.get();
						for (const auto& keyval : record.values)
						{
							baseRecordElements.insert(keyval.first);
						}
					}
				}
				else
				{
					scopeLog.write(rawIdentifier.toString() + " is currently undefined value");
					hasDeferredCall = true;
					result.head = head;
				}
			}
			// 外部で定義済み（評価済み）の変数の場合
			else if (Address address = context.findAddress(head); address.isValid())
			{
				//result.head = head;
				scopeLog.write(head.toString() + " is evaluated value(direct call)");
				result.head = LRValue(EitherReference(head, address)).setLocation(node);
				//result.head = Identifier("efusehf");

				const Val& val = context.expand(LRValue(address), node);
				if (auto recordOpt = AsOpt<Record>(val))
				{
					const Record& record = recordOpt.get();
					for (const auto& keyval : record.values)
					{
						baseRecordElements.insert(keyval.first);
					}
				}
			}
			// 局所変数の場合
			else
			{
				scopeLog.write(head.toString() + " is local variable(direct call)");
				result.head = head;
			}

			// head が MakeClosure であるケースはあり得るか：
			// MakeClosure は Identifier 単体にしか置き換えられないはずなので Accessor に来ることはない
		}

		size_t accessIndex = 0;
		// head がもともと DeferredCall に置き換えられていたが不要だと分かったときは tail の不要な FunctionAccess もはずす

		/*if (IsType<Identifier>(node.head) && As<Identifier>(node.head).isDeferredCall() && !hasDeferredCall
			&& !node.accesses.empty() && IsType<FunctionAccess>(node.accesses.front()) && As<FunctionAccess>(node.accesses.front()).actualArguments.empty())
		{
			accessIndex = 1;
		}*/

		for (; accessIndex < node.accesses.size(); ++accessIndex)
		{
			const auto& access = node.accesses[accessIndex];
			if (auto opt = AsOpt<ListAccess>(access))
			{
				DynamicDeferredIdentifierSearcher child(localVariables, currentScopeInfo, deferredIdentifiers, context);
				const ListAccess& listAccess = opt.get();
				ListAccess replacedAccess(boost::apply_visitor(child, listAccess.index));
				result.accesses.push_back(replacedAccess);
				hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
			}
			else if (auto opt = AsOpt<FunctionAccess>(access))
			{
				DynamicDeferredIdentifierSearcher child(localVariables, currentScopeInfo, deferredIdentifiers, context);
				const FunctionAccess& funcAccess = opt.get();
				FunctionAccess replacedAccess;
				for (const auto& arg : funcAccess.actualArguments)
				{
					replacedAccess.actualArguments.push_back(boost::apply_visitor(child, arg));
				}
				result.accesses.push_back(replacedAccess);
				hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
			}
			else if (auto opt = AsOpt<InheritAccess>(access))
			{
				baseRecordElements.insert(localVariables.begin(), localVariables.end());

				/*static int num = 0;
				std::cerr << num << ":";
				printExpr2(node, nullptr, std::cerr);
				if (num == 0)
				{
					std::cerr << "localVars:\n";
					for (const auto& str : baseRecordElements)
					{
						std::cerr << str << std::endl;
					}
				}
				++num;*/

				DynamicDeferredIdentifierSearcher child(baseRecordElements, currentScopeInfo, deferredIdentifiers, context);
				const InheritAccess& inheritAccess = opt.get();
				const Expr adderExpr = inheritAccess.adder;
				InheritAccess replacedAccess(As<RecordConstractor>(boost::apply_visitor(child, adderExpr)));
				result.accesses.push_back(replacedAccess);
				hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
			}
			else
			{
				result.accesses.push_back(access);
			}
		}

		return result.setLocation(node);
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const DeclSat& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const DeclSat& node)");

		return DeclSat(boost::apply_visitor(*this, node.expr)).setLocation(node);
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const DeclFree& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const DeclFree& node)");

		DeclFree result = node;
		result.accessors.clear();
		for (const auto& var : node.accessors)
		{
			DeclFree::DeclVar declVar;
			declVar.accessor = boost::apply_visitor(*this, var.accessor);
			if (var.range)
			{
				declVar.range = boost::apply_visitor(*this, var.range.get());
			}
			result.accessors.push_back(declVar);
		}
		return result;
	}

	//Import対応はTODO
	Expr DynamicDeferredIdentifierSearcher::operator()(const Import& node) { return node; }

	Expr DynamicDeferredIdentifierSearcher::operator()(const Range& node)
	{
		auto scopeLog = ScopeLog("DynamicDeferredIdentifierSearcher::operator()(const Range& node)");

		Range result = node;
		result.lhs = boost::apply_visitor(*this, node.lhs);
		result.rhs = boost::apply_visitor(*this, node.rhs);
		return result;
	}

	Expr DynamicDeferredIdentifierSearcher::operator()(const Return& node)
	{
		return Return(boost::apply_visitor(*this, node.expr)).setLocation(node);
	}
}
