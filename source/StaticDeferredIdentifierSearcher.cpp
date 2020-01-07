#include <Pita/Context.hpp>
#include <Pita/StaticDeferredIdentifierSearcher.hpp>
#include <Pita/ExprTransformer.hpp>
#include <Pita/TreeLogger.hpp>
#include <Pita/ClosureMaker.hpp>

namespace cgl
{
	void StaticDeferredIdentifierSearcher::addLocalVariable(const std::string& localVariable)
	{
		//std::cerr << "add local variable: \"" << localVariable << "\"" << std::endl;
		localVariables.insert(localVariable);
	}

	bool StaticDeferredIdentifierSearcher::isLocalVariable(const std::string& localVariable)const
	{
		return localVariables.find(localVariable) != localVariables.end();
	}

	ScopeAddress StaticDeferredIdentifierSearcher::makeChildScope()
	{
		ScopeAddress childScope = currentScopeInfo;
		childScope.push_back(currentScopeIndex);
		++currentScopeIndex;
		return childScope;
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const LRValue& node) { return ExprTransformer::operator()(node); }

	// 考察：識別子単体（アクセッサでない）での遅延呼び出しを許可するべきか？
	// 代入式の左辺は BinaryExpr で処理するのでこの Identifier では遅延呼び出しのみ考慮する
	//   再帰図形の場合：許可すべき
	// 　  -> ShapeName と ShapeName{} は同じ図形を表すべきなので
	//      （後者は単に前者のクローンであり両者で意味が変わるのはおかしい）
	//   再帰関数の場合：微妙
	//     -> Pitaでは引数の無い関数呼び出しの括弧省略には対応していない
	//        再帰の場合だけこれが許容されるのは微妙な気はする
	Expr StaticDeferredIdentifierSearcher::operator()(const Identifier& node)
	{
		//std::cerr << "Begin Identifier" << " \"" << static_cast<std::string>(node) << "\"" << std::endl;
		if (isLocalVariable(node))
		{
			return node;
		}

		//LocalVariableになければすべて遅延呼び出しか？
		//->Import元の環境、標準ライブラリなど事前に定義された変数とのチェックが済んでいれば遅延呼び出し

		//遅延呼び出し
		hasDeferredCall = true;
		Accessor result;
		result.head = node.asDeferredCall(currentScopeInfo);
		result.add(FunctionAccess());
		//std::cerr << "End   Identifier" << " \"" << static_cast<std::string>(node) << "\"" << std::endl;
		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const UnaryExpr& node) { return ExprTransformer::operator()(node); }

	// 遅延呼び出し式の定義
	Expr StaticDeferredIdentifierSearcher::operator()(const BinaryExpr& node)
	{
		//std::cerr << "Begin BinaryExpr" << std::endl;
		const bool isAssignExpr = IsType<Identifier>(node.lhs) && node.op == BinaryOp::Assign;
		if (!isAssignExpr)
		{
			//std::cerr << "End   BinaryExpr(0)" << std::endl;
			return ExprTransformer::operator()(node);
		}

		Identifier lhsName(As<Identifier>(node.lhs));

		Identifier functionBaseName = lhsName;

		//再帰深度における定義の特殊化
		boost::optional<int> specializationRecDepth;

		// 代入式が遅延識別子定義になる条件は以下のどれか：
		//   ・内部にそのスコープから参照できない識別子呼び出しを持つ
		//   ・内部に遅延識別子呼び出しを持つ
		//   ・識別子##数字 = Expr という形で宣言されている（再帰深度の特殊化）

		//特殊形の定義
		{
			const std::string lhsStr = lhsName.toString();
			const size_t index = lhsStr.find("##");
			if (index != std::string::npos)
			{
				functionBaseName = Identifier(std::string(lhsStr.begin(), lhsStr.begin() + index));

				//TODO:##以降がちゃんと数字になっているかの確認も必要

				specializationRecDepth = std::stoi(std::string(lhsStr.begin() + index + 2, lhsStr.end()));
			}
		}

		//一般形の定義
		if (!specializationRecDepth)
		{
			//ここでmakeChildScopeをする意味とは？
			//StaticDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers);

			StaticDeferredIdentifierSearcher child(localVariables, currentScopeInfo, deferredIdentifiers);
			//std::cerr << ">>>>  BinaryExpr child" << std::endl;
			boost::apply_visitor(child, node.rhs);
			//std::cerr << "<<<<  BinaryExpr child" << std::endl;
			if (!child.hasDeferredCall)
			{
				addLocalVariable(functionBaseName);
				//std::cerr << "End   BinaryExpr(1)" << std::endl;
				return ExprTransformer::operator()(node);
			}
		}

		// 遅延呼び出し式は三種類（呼び出し側は全て関数呼び出しとして呼ぶ）：
		//   ・関数の遅延呼び出し
		//   ・図形の遅延呼び出し
		//   ・その他の式の遅延呼び出し

		const auto getDefRef = [&]()->DefFuncWithScopeInfo &
		{
			std::vector<DefFuncWithScopeInfo>& eachScopeDefs = deferredIdentifiers[functionBaseName];

			auto it = std::find_if(eachScopeDefs.begin(), eachScopeDefs.end(),
				[&](const DefFuncWithScopeInfo& elem)
				{
					return elem.scopeInfo == currentScopeInfo;
				});

			if (it == eachScopeDefs.end())
			{
				eachScopeDefs.emplace_back(currentScopeInfo);
				return eachScopeDefs.back();
			}
			return *it;
		};

		{
			DefFuncWithScopeInfo& recFuncDef = getDefRef();
			//StaticDeferredIdentifierSearcher child2(localVariables, makeChildScope(), deferredIdentifiers);
			StaticDeferredIdentifierSearcher child2(localVariables, currentScopeInfo, deferredIdentifiers);
			DefFunc currentDef;
			if (IsType<DefFunc>(node.rhs))
			{
				currentDef = As<DefFunc>(node.rhs);
				for (const auto& argument : currentDef.arguments)
				{
					child2.addLocalVariable(argument);
				}

				// 右辺の評価は識別子をローカル環境に加えない状態で行う（右辺の中で再帰呼び出しできるようにするため）
				currentDef.expr = boost::apply_visitor(child2, currentDef.expr);
			}
			else
			{
				//std::cerr << "---------------------------------" << std::endl;
				//printExpr(node.rhs, nullptr, std::cerr);
				//std::cerr << "---------------------------------" << std::endl;

				currentDef.expr = boost::apply_visitor(child2, node.rhs);
				//std::cerr << "=================================" << std::endl;
				//printExpr(currentDef.expr, nullptr, std::cerr);
				//std::cerr << "=================================" << std::endl;
				currentDef.setLocation(node);
			}
			//std::cerr << "BinaryExpr deferredFunc.defFunc.arguments.size(): " << currentDef.arguments.size() << ", \"" << functionBaseName.toString() << "\"" << std::endl;
			//printExpr(currentDef.expr, nullptr, std::cerr);

			if (specializationRecDepth)
			{
				recFuncDef.setSpecialDef(currentDef, specializationRecDepth.get());
			}
			else
			{
				recFuncDef.setGeneralDef(currentDef);
			}
		}

		/*
		if (IsType<DefFunc>(node.rhs))
		{
			DefFuncWithScopeInfo deferredFunc(As<DefFunc>(node.rhs), currentScopeInfo);
			for (const auto& argument : deferredFunc.generalDef.arguments)
			{
				child2.addLocalVariable(argument);
			}

			// 右辺の評価は識別子をローカル環境に加えない状態で行う（右辺の中で再帰呼び出しできるようにするため）
			deferredFunc.generalDef.expr = boost::apply_visitor(child2, deferredFunc.generalDef.expr);
			std::cerr << "BinaryExpr deferredFunc.defFunc.arguments.size(): " <<
				deferredFunc.generalDef.arguments.size() << ", \"" << functionBaseName.toString() << "\"" << std::endl;
			printExpr(deferredFunc.generalDef.expr, nullptr, std::cerr);
			deferredIdentifiers[functionBaseName].push_back(deferredFunc);
		}
		else
		{
			DefFuncWithScopeInfo deferredFunc;
			deferredFunc.generalDef.expr = boost::apply_visitor(child2, node.rhs);
			deferredFunc.generalDef.setLocation(node);
			deferredFunc.scopeInfo = currentScopeInfo;
			std::cerr << "BinaryExpr deferredFunc.defFunc.arguments.size(): " <<
				deferredFunc.generalDef.arguments.size() << ", \"" << functionBaseName.toString() << "\"" << std::endl;
			printExpr(deferredFunc.generalDef.expr, nullptr, std::cerr);
			deferredIdentifiers[functionBaseName].push_back(deferredFunc);
		}
		*/

		// これ以降もこの識別子は遅延呼び出しできるようにしたいのでローカル環境には加えない

		return lhsName.asMakeClosure(currentScopeInfo);
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const Lines& node)
	{
		//std::cerr << "Begin Lines" << std::endl;
		Lines result;

		StaticDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& expr : node.exprs)
		{
			result.add(boost::apply_visitor(child, expr));
			//std::cerr << "     " << (child.hasDeferredCall ? "true" : "false") << std::endl;
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;

		//std::cerr << "End   Lines(0)" << std::endl;
		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const DefFunc& node)
	{
		//std::cerr << "Begin DefFunc" << std::endl;
		StaticDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& argument : node.arguments)
		{
			child.addLocalVariable(argument);
			//std::cerr << "     " << (child.hasDeferredCall ? "true" : "false") << std::endl;
		}

		auto result = DefFunc(node.arguments, boost::apply_visitor(child, node.expr)).setLocation(node);
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		//std::cerr << "End   DefFunc(0)" << std::endl;
		return result;
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const If& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher::operator()(const For& node)
	{
		//std::cerr << "Begin For" << std::endl;
		For result;

		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

		StaticDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers);
		child.addLocalVariable(node.loopCounter);
		result.doExpr = boost::apply_visitor(child, node.doExpr);
		result.loopCounter = node.loopCounter;
		result.asList = node.asList;
		//std::cerr << "     " << (child.hasDeferredCall ? "true" : "false") << std::endl;
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;

		//std::cerr << "End   For(0)" << std::endl;
		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const ListConstractor& node)
	{
		//std::cerr << "Begin ListConstractor" << std::endl;
		ListConstractor result;
		StaticDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(child, expr));
			//std::cerr << "     " << (child.hasDeferredCall ? "true" : "false") << std::endl;
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;

		//std::cerr << "End   ListConstractor(0)" << std::endl;
		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const KeyExpr& node)
	{
		//std::cerr << "Begin KeyExpr" << std::endl;
		addLocalVariable(node.name);

		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		//std::cerr << "End   KeyExpr(0)" << std::endl;
		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const RecordConstractor& node)
	{
		//std::cerr << "Begin RecordConstractor" << std::endl;
		RecordConstractor result;
		StaticDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& expr : node.exprs)
		{
			result.exprs.push_back(boost::apply_visitor(child, expr));
			//std::cerr << "     " << (child.hasDeferredCall ? "true" : "false") << std::endl;
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		//std::cerr << "End   RecordConstractor(0)" << std::endl;
		return result.setLocation(node);
	}

	// 遅延呼び出しは単一の識別子の場合のみ考慮する（静的に決定したいため）ので
	// アクセッサの場合は先頭要素のみ考慮して決める
	Expr StaticDeferredIdentifierSearcher::operator()(const Accessor& node)
	{
		std::vector<Access> replacedAccesses;
		for (const auto& access : node.accesses)
		{
			//StaticDeferredIdentifierSearcher child(localVariables, makeChildScope(), deferredIdentifiers);
			StaticDeferredIdentifierSearcher child(localVariables, currentScopeInfo, deferredIdentifiers);
			if (auto opt = AsOpt<ListAccess>(access))
			{
				const ListAccess& listAccess = opt.get();
				ListAccess replaced = listAccess;
				replaced.index = boost::apply_visitor(child, listAccess.index);
				replacedAccesses.push_back(replaced);
			}
			else if (auto opt = AsOpt<FunctionAccess>(access))
			{
				const FunctionAccess& funcAccess = opt.get();
				FunctionAccess replaced;
				for (const auto& arg : funcAccess.actualArguments)
				{
					replaced.actualArguments.push_back(boost::apply_visitor(child, arg));
				}
				replacedAccesses.push_back(replaced);
			}
			else if (auto opt = AsOpt<InheritAccess>(access))
			{
				const InheritAccess& inheritAccess = opt.get();
				InheritAccess replaced;

				const Expr adderExpr = inheritAccess.adder;
				replaced.adder = As<RecordConstractor>(boost::apply_visitor(child, adderExpr));

				// A = Shape{a: A{}} のような式で代入式を遅延識別子定義とするには
				// 継承アクセスの中身が遅延呼び出しを行うかどうかを上に伝える必要がある
				hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
				replacedAccesses.push_back(replaced);
			}
			else
			{
				replacedAccesses.push_back(access);
			}
		}

		Accessor result = node;
		result.accesses = replacedAccesses;

		//std::cerr << "Begin Accessor" << std::endl;
		if (!IsType<Identifier>(node.head))
		{
			//std::cerr << "End   Accessor(0)" << std::endl;

			return result;
		}

		// ここで head は Identifier に投げない -> boost::apply_visitor(*this, node.head)
		// Identifier に投げると返り値は Identifier と CallFunc(Accessor) の二種類があり得る
		// また、関数呼び出しの場合 Identifier は引数なしの呼び出しに置き換えるがアクセッサの場合は
		// 引数を持っている( FunctionAccess )の可能性があるため

		// ローカル変数ならそのまま返す
		const Identifier& head = As<Identifier>(node.head);
		if (isLocalVariable(head))
		{
			//std::cerr << "End   Accessor(1)" << std::endl;
			return result;
		}

		// 直後で最初の要素にアクセスしたいので一応チェックするが、
		// パース段階でアクセスが空のアクセッサは作られないはずなので
		// Qi のパーサを使っていればこのチェックは不要のはず
		if (node.accesses.empty())
		{
			//std::cerr << "End   Accessor(2)" << std::endl;
			return result;
		}

		// ローカル変数でないなら遅延呼び出し
		hasDeferredCall = true;

		// 既に置き換えられた遅延識別子呼び出しはそのまま返す
		if (head.isDeferredCall())
		{
			//std::cerr << "End   Accessor(3)" << std::endl;
			return result;
		}

		result.head = head.asDeferredCall(currentScopeInfo);

		// 外すときに簡単にするために機械的に()を付けることにする
		/*
		//関数呼び出しの場合は呼び出す関数名を変えるだけでよい
		const Access& access = result.accesses.front();
		if (IsType<FunctionAccess>(access))
		{
			//std::cerr << "End   Accessor(4)" << std::endl;
			return result;
		}
		*/

		result.accesses.clear();

		const Access& access = result.accesses.front();

		//関数以外の場合は関数呼び出しを間に挟む必要がある
		if (!IsType<FunctionAccess>(access))
		{
			result.add(FunctionAccess());
		}

		//result.accesses.insert(result.accesses.end(), node.accesses.begin(), node.accesses.end());
		result.accesses.insert(result.accesses.end(), replacedAccesses.begin(), replacedAccesses.end());

		//std::cerr << "End   Accessor(5)" << std::endl;
		return result;
	}

	Expr StaticDeferredIdentifierSearcher::operator()(const DeclSat& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher::operator()(const DeclFree& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher::operator()(const Import& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher::operator()(const Range& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher::operator()(const Return& node) { return ExprTransformer::operator()(node); }


	void StaticDeferredIdentifierSearcher2::addLocalVariable(const std::string& localVariable)
	{
		localVariables.insert(localVariable);
	}

	bool StaticDeferredIdentifierSearcher2::isLocalVariable(const std::string& localVariable)const
	{
		return localVariables.find(localVariable) != localVariables.end();
	}

	ScopeAddress StaticDeferredIdentifierSearcher2::makeChildScope()
	{
		ScopeAddress childScope = currentScopeInfo;
		childScope.push_back(currentScopeIndex);
		++currentScopeIndex;
		return childScope;
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const LRValue& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher2::operator()(const Identifier& node)
	{
		if (isLocalVariable(node))
		{
			return node;
		}

		//遅延呼び出し
		hasDeferredCall = true;
		Accessor result;
		result.head = node.asDeferredCall(currentScopeInfo);
		result.add(FunctionAccess());
		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const UnaryExpr& node) { return ExprTransformer::operator()(node); }

	// 遅延呼び出し式の定義
	Expr StaticDeferredIdentifierSearcher2::operator()(const BinaryExpr& node)
	{
		const bool isAssignExpr = IsType<Identifier>(node.lhs) && node.op == BinaryOp::Assign;
		if (!isAssignExpr)
		{
			return ExprTransformer::operator()(node);
		}

		Identifier lhsName(As<Identifier>(node.lhs));

		Identifier functionBaseName = lhsName;

		//再帰深度における定義の特殊化
		boost::optional<int> specializationRecDepth;

		// 代入式が遅延識別子定義になる条件は以下のどれか：
		//   ・内部にそのスコープから参照できない識別子呼び出しを持つ
		//   ・内部に遅延識別子呼び出しを持つ
		//   ・識別子##数字 = Expr という形で宣言されている（再帰深度の特殊化）

		//特殊形の定義
		{
			const std::string lhsStr = lhsName.toString();
			const size_t index = lhsStr.find("##");
			if (index != std::string::npos)
			{
				functionBaseName = Identifier(std::string(lhsStr.begin(), lhsStr.begin() + index));

				//TODO:##以降がちゃんと数字になっているかの確認も必要

				specializationRecDepth = std::stoi(std::string(lhsStr.begin() + index + 2, lhsStr.end()));
			}
		}

		// 遅延呼び出し式は三種類（呼び出し側は全て関数呼び出しとして呼ぶ）：
		//   ・関数の遅延呼び出し
		//   ・図形の遅延呼び出し
		//   ・その他の式の遅延呼び出し

		const auto getDefRef = [&]()->DefFuncWithScopeInfo &
		{
			std::vector<DefFuncWithScopeInfo>& eachScopeDefs = deferredIdentifiers[functionBaseName];

			auto it = std::find_if(eachScopeDefs.begin(), eachScopeDefs.end(),
				[&](const DefFuncWithScopeInfo& elem)
				{
					return elem.scopeInfo == currentScopeInfo;
				});

			if (it == eachScopeDefs.end())
			{
				eachScopeDefs.emplace_back(currentScopeInfo);
				return eachScopeDefs.back();
			}
			return *it;
		};

		{
			DefFuncWithScopeInfo& recFuncDef = getDefRef();
			StaticDeferredIdentifierSearcher2 child2(localVariables, currentScopeInfo, deferredIdentifiers);
			DefFunc currentDef;
			if (IsType<DefFunc>(node.rhs))
			{
				currentDef = As<DefFunc>(node.rhs);
				for (const auto& argument : currentDef.arguments)
				{
					child2.addLocalVariable(argument);
				}

				// 右辺の評価は識別子をローカル環境に加えない状態で行う（右辺の中で再帰呼び出しできるようにするため）
				currentDef.expr = boost::apply_visitor(child2, currentDef.expr);
			}
			else
			{
				currentDef.expr = boost::apply_visitor(child2, node.rhs);
				currentDef.setLocation(node);
			}

			if (specializationRecDepth)
			{
				recFuncDef.setSpecialDef(currentDef, specializationRecDepth.get());
			}
			else
			{
				recFuncDef.setGeneralDef(currentDef);
			}
		}

		return lhsName.asMakeClosure(currentScopeInfo);
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const Lines& node)
	{
		Lines result;

		StaticDeferredIdentifierSearcher2 child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& expr : node.exprs)
		{
			result.add(boost::apply_visitor(child, expr));
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;

		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const DefFunc& node)
	{
		StaticDeferredIdentifierSearcher2 child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& argument : node.arguments)
		{
			child.addLocalVariable(argument);
		}

		auto result = DefFunc(node.arguments, boost::apply_visitor(child, node.expr)).setLocation(node);
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		return result;
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const If& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher2::operator()(const For& node)
	{
		For result;

		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

		StaticDeferredIdentifierSearcher2 child(localVariables, makeChildScope(), deferredIdentifiers);
		child.addLocalVariable(node.loopCounter);
		result.doExpr = boost::apply_visitor(child, node.doExpr);
		result.loopCounter = node.loopCounter;
		result.asList = node.asList;
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;

		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const ListConstractor& node)
	{
		ListConstractor result;
		StaticDeferredIdentifierSearcher2 child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(child, expr));
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;

		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const KeyExpr& node)
	{
		addLocalVariable(node.name);

		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const RecordConstractor& node)
	{
		RecordConstractor result;
		StaticDeferredIdentifierSearcher2 child(localVariables, makeChildScope(), deferredIdentifiers);
		for (const auto& expr : node.exprs)
		{
			result.exprs.push_back(boost::apply_visitor(child, expr));
		}
		hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
		return result.setLocation(node);
	}

	// 遅延呼び出しは単一の識別子の場合のみ考慮する（静的に決定したいため）ので
	// アクセッサの場合は先頭要素のみ考慮して決める
	Expr StaticDeferredIdentifierSearcher2::operator()(const Accessor& node)
	{
		std::vector<Access> replacedAccesses;
		for (const auto& access : node.accesses)
		{
			StaticDeferredIdentifierSearcher2 child(localVariables, currentScopeInfo, deferredIdentifiers);
			if (auto opt = AsOpt<ListAccess>(access))
			{
				const ListAccess& listAccess = opt.get();
				ListAccess replaced = listAccess;
				replaced.index = boost::apply_visitor(child, listAccess.index);
				replacedAccesses.push_back(replaced);
			}
			else if (auto opt = AsOpt<FunctionAccess>(access))
			{
				const FunctionAccess& funcAccess = opt.get();
				FunctionAccess replaced;
				for (const auto& arg : funcAccess.actualArguments)
				{
					replaced.actualArguments.push_back(boost::apply_visitor(child, arg));
				}
				replacedAccesses.push_back(replaced);
			}
			else if (auto opt = AsOpt<InheritAccess>(access))
			{
				const InheritAccess& inheritAccess = opt.get();
				InheritAccess replaced;

				const Expr adderExpr = inheritAccess.adder;
				replaced.adder = As<RecordConstractor>(boost::apply_visitor(child, adderExpr));

				// A = Shape{a: A{}} のような式で代入式を遅延識別子定義とするには
				// 継承アクセスの中身が遅延呼び出しを行うかどうかを上に伝える必要がある
				hasDeferredCall = hasDeferredCall || child.hasDeferredCall;
				replacedAccesses.push_back(replaced);
			}
			else
			{
				replacedAccesses.push_back(access);
			}
		}

		Accessor result = node;
		result.accesses = replacedAccesses;

		if (!IsType<Identifier>(node.head))
		{
			return result;
		}

		// ローカル変数ならそのまま返す
		const Identifier& head = As<Identifier>(node.head);
		if (isLocalVariable(head))
		{
			return result;
		}

		// 直後で最初の要素にアクセスしたいので一応チェックするが、
		// パース段階でアクセスが空のアクセッサは作られないはずなので
		// Qi のパーサを使っていればこのチェックは不要のはず
		if (node.accesses.empty())
		{
			return result;
		}

		// ローカル変数でないなら遅延呼び出し
		hasDeferredCall = true;

		// 既に置き換えられた遅延識別子呼び出しはそのまま返す
		if (head.isDeferredCall())
		{
			return result;
		}

		result.head = head.asDeferredCall(currentScopeInfo);

		result.accesses.clear();

		const Access& access = result.accesses.front();

		//関数以外の場合は関数呼び出しを間に挟む必要がある
		if (!IsType<FunctionAccess>(access))
		{
			result.add(FunctionAccess());
		}

		result.accesses.insert(result.accesses.end(), replacedAccesses.begin(), replacedAccesses.end());

		return result;
	}

	Expr StaticDeferredIdentifierSearcher2::operator()(const DeclSat& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher2::operator()(const DeclFree& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher2::operator()(const Import& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher2::operator()(const Range& node) { return ExprTransformer::operator()(node); }

	Expr StaticDeferredIdentifierSearcher2::operator()(const Return& node) { return ExprTransformer::operator()(node); }
}
