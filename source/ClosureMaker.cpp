#include <Pita/Context.hpp>
#include <Pita/ClosureMaker.hpp>
#include <Pita/ExprTransformer.hpp>
#include <Pita/TreeLogger.hpp>

namespace cgl
{
	class ClosureMaker : public boost::static_visitor<Expr>
	{
	public:
		//関数内部で閉じているローカル変数
		std::set<std::string> localVariables;

		//std::shared_ptr<Context> pEnv;
		Context& context;

		//レコード継承の構文を扱うために必要
		bool isInnerRecord;

		//内側の未評価式における参照変数の展開を防ぐために必要
		bool isInnerClosure;

		ClosureMaker(Context& context, const std::set<std::string>& functionArguments, bool isInnerRecord = false, bool isInnerClosure = false) :
			context(context),
			localVariables(functionArguments),
			isInnerRecord(isInnerRecord),
			isInnerClosure(isInnerClosure)
		{}

		ClosureMaker& addLocalVariable(const std::string& name);

		bool isLocalVariable(const std::string& name)const;

		Expr operator()(const LRValue& node) { return node; }
		Expr operator()(const Identifier& node);
		Expr operator()(const Import& node) { return node; }
		Expr operator()(const UnaryExpr& node);
		Expr operator()(const BinaryExpr& node);
		Expr operator()(const Range& node);
		Expr operator()(const Lines& node);
		Expr operator()(const DefFunc& node);
		Expr operator()(const If& node);
		Expr operator()(const For& node);
		Expr operator()(const Return& node);
		Expr operator()(const ListConstractor& node);
		Expr operator()(const KeyExpr& node);
		Expr operator()(const RecordConstractor& node);
		Expr operator()(const Accessor& node);
		Expr operator()(const DeclSat& node);
		Expr operator()(const DeclFree& node);
	};

	ClosureMaker& ClosureMaker::addLocalVariable(const std::string& name)
	{
		localVariables.insert(name);
		return *this;
	}

	bool ClosureMaker::isLocalVariable(const std::string& name)const
	{
		return localVariables.find(name) != localVariables.end();
	}

	Expr ClosureMaker::operator()(const Identifier& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const Identifier& node)");

		//その関数のローカル変数であれば関数の実行時に評価すればよいので、名前を残す
		if (isLocalVariable(node))
		{
			return node;
		}
		//Closure作成時点では全てのローカル変数は捕捉できないので、ここでアドレスに置き換えるのは安全でない
		//したがって、Contextに存在したら名前とアドレスのどちらからも参照できるようにしておき、評価時に実際の参照先を決める
		const Address address = context.findAddress(node);
		if (address.isValid())
		{
			//Identifier RecordConstructor の形をしたレコード継承の head 部分
			//とりあえず参照先のレコードのメンバはローカル変数とおく
			if (isInnerRecord)
			{
				const Val& evaluated = context.expand(LRValue(address), node);
				if (auto opt = AsOpt<Record>(evaluated))
				{
					const Record& record = opt.get();
					
					for (const auto& keyval : record.values)
					{
						addLocalVariable(keyval.first);
					}
				}
			}

			return LRValue(EitherReference(node, address)).setLocation(node);
		}

		//それ以外の場合は実行してみないと分からないため、ローカル変数と仮定する（参照エラーはEvalで出す）
		//例：f = (s -> s{g()}) のgがsのメンバかどうかはfのクロージャ生成時点では分からない
		return node;
	}

	Expr ClosureMaker::operator()(const UnaryExpr& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const UnaryExpr& node)");

		if (node.op == UnaryOp::Dynamic && !isInnerClosure)
		{
			if (IsType<Identifier>(node.lhs))
			{
				if (isLocalVariable(As<Identifier>(node.lhs)))
				{
					return node;
				}
				return LRValue(context.bindReference(As<Identifier>(node.lhs))).setLocation(node);
			}
			else if (IsType<Accessor>(node.lhs))
			{
				if (IsType<Identifier>(As<Accessor>(node.lhs).head) && isLocalVariable(As<Identifier>(As<Accessor>(node.lhs).head)))
				{
					return node;
				}
				return LRValue(context.bindReference(As<Accessor>(node.lhs), node)).setLocation(node);
			}

			CGL_ErrorNode(node, "参照演算子\"@\"の右辺には識別子かアクセッサしか用いることができません。");
		}

		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op).setLocation(node);
	}

	Expr ClosureMaker::operator()(const BinaryExpr& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const BinaryExpr& node)");

		const Expr rhs = boost::apply_visitor(*this, node.rhs);

		if (node.op != BinaryOp::Assign)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);
			return BinaryExpr(lhs, rhs, node.op).setLocation(node);
		}

		//Assignの場合、lhs は Address or Identifier or Accessor に限られる
		//つまり現時点では、(if cond then x else y) = true のような式を許可していない
		//ここで左辺に直接アドレスが入っていることは有り得る？
		//a = b = 10　のような式でも、右結合であり左側は常に識別子が残っているはずなので、あり得ないと思う
		if (auto valOpt = AsOpt<Identifier>(node.lhs))
		{
			const Identifier identifier = valOpt.get();

			//ローカル変数にあれば、その場で解決できる識別子なので何もしない
			if (isLocalVariable(identifier))
			{
				return BinaryExpr(node.lhs, rhs, node.op).setLocation(node);
			}
			else
			{
				const Address address = context.findAddress(identifier);

				//ローカル変数に無く、スコープにあれば、アドレスに置き換える
				if (address.isValid())
				{
					//TODO: 制約式の場合は、ここではじく必要がある
					//return BinaryExpr(LRValue(address), rhs, node.op).setLocation(node);
					return LRValue(EitherReference(identifier, address)).setLocation(node);
				}
				//スコープにも無い場合は新たなローカル変数の宣言なので、ローカル変数に追加しておく
				else
				{
					addLocalVariable(identifier);
					return BinaryExpr(node.lhs, rhs, node.op).setLocation(node);
				}
			}
		}
		else if (auto valOpt = AsOpt<Accessor>(node.lhs))
		{
			//アクセッサの場合は少なくとも変数宣言ではない
			//ローカル変数 or スコープ
			/*～～～～～～～～～～～～～～～～～～～～～～～～～～～～～
			Accessorのheadだけ評価してアドレス値に変換したい
				headさえ分かればあとはそこから辿れるので
				今の実装ではheadは式になっているが、これだと良くない
				今は左辺にはそんなに複雑な式は許可していないので、これも識別子くらいの単純な形に制限してよいのではないか
			～～～～～～～～～～～～～～～～～～～～～～～～～～～～～*/

			//評価することにした
			const Expr lhs = boost::apply_visitor(*this, node.lhs);

			return BinaryExpr(lhs, rhs, node.op).setLocation(node);
		}
		//左辺に参照変数が来るケース：@a = x
		else if (auto valOpt = AsOpt<UnaryExpr>(node.lhs))
		{
			if (valOpt.get().op == UnaryOp::Dynamic)
			{
				const Expr lhs = boost::apply_visitor(*this, node.lhs);

				return BinaryExpr(lhs, rhs, node.op).setLocation(node);
			}
		}

		CGL_ErrorNode(node, "二項演算子\"=\"の左辺は単一の左辺値でなければなりません。");
		return LRValue(0);
	}

	Expr ClosureMaker::operator()(const Range& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const Range& node)");

		return Range(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs)).setLocation(node);
	}

	Expr ClosureMaker::operator()(const Lines& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const Lines& node)");

		Lines result;
		for (size_t i = 0; i < node.size(); ++i)
		{
			result.add(boost::apply_visitor(*this, node.exprs[i]));
		}
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const DefFunc& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const DefFunc& node)");

		//ClosureMaker child(*this);
		ClosureMaker child(context, localVariables, false, true);
		for (const auto& argument : node.arguments)
		{
			child.addLocalVariable(argument);
		}

		return DefFunc(node.arguments, boost::apply_visitor(child, node.expr)).setLocation(node);
	}

	Expr ClosureMaker::operator()(const If& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const If& node)");

		If result(boost::apply_visitor(*this, node.cond_expr));
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (node.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.get());
		}

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const For& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const For& node)");

		For result;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

		//ClosureMaker child(*this);
		ClosureMaker child(context, localVariables, false, isInnerClosure);
		child.addLocalVariable(node.loopCounter);
		result.doExpr = boost::apply_visitor(child, node.doExpr);
		result.loopCounter = node.loopCounter;
		result.asList = node.asList;

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const Return& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const Return& node)");

		return Return(boost::apply_visitor(*this, node.expr)).setLocation(node);
		//これだとダメかもしれない？
		//return a = 6, a + 2
	}

	Expr ClosureMaker::operator()(const ListConstractor& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const ListConstractor& node)");

		ListConstractor result;
		//ClosureMaker child(*this);
		ClosureMaker child(context, localVariables, false, isInnerClosure);
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(child, expr));
		}
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const KeyExpr& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const KeyExpr& node)");

		//変数宣言式 or 再代入式
		//どちらにしろこれ以降この識別子はローカル変数と扱ってよい
		addLocalVariable(node.name);

		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const RecordConstractor& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const RecordConstractor& node)");

		RecordConstractor result;

		if (isInnerRecord)
		{
			isInnerRecord = false;
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(*this, expr));
			}
			isInnerRecord = true;
		}
		else
		{
			ClosureMaker child(context, localVariables, false, isInnerClosure);
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(child, expr));
			}
		}

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const Accessor& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const Accessor& node)");

		Accessor result;

		result.head = boost::apply_visitor(*this, node.head);
		//DeclSatの評価後ではsat式中のアクセッサ（の内sat式のローカル変数でないもの）のheadはアドレス値に評価されている必要がある。
		//しかし、ここでは*thisを使っているので、任意の式がアドレス値に評価されるわけではない。
		//例えば、次の式 ([f1,f2] @ [f3])[0](x) だとhead部はリストの結合式であり、Evalを通さないとアドレス値にできない。
		//しかし、ここでEvalは使いたくない（ClosureMakerが副作用を起こすのは良くない）ため、現時点ではアクセッサのhead部は単一の識別子のみで構成されるものと仮定している。
		//こうすることにより、識別子がローカル変数ならそのまま残り、外部の変数ならアドレス値に変換されることが保証できる。

		for (const auto& access : node.accesses)
		{
			if (auto listAccess = AsOpt<ListAccess>(access))
			{
				if (listAccess.get().isArbitrary)
				{
					result.add(listAccess.get());
				}
				else
				{
					result.add(ListAccess(boost::apply_visitor(*this, listAccess.get().index)));
				}
			}
			else if (auto recordAccess = AsOpt<RecordAccess>(access))
			{
				result.add(recordAccess.get());
			}
			else if (auto functionAccess = AsOpt<FunctionAccess>(access))
			{
				FunctionAccess access;
				for (const auto& argument : functionAccess.get().actualArguments)
				{
					access.add(boost::apply_visitor(*this, argument));
				}
				result.add(access);
			}
			else if (auto inheritAccess = AsOpt<InheritAccess>(access))
			{
				ClosureMaker child(context, localVariables, true, isInnerClosure);

				Expr originalAdder = inheritAccess.get().adder;
				Expr replacedAdder = boost::apply_visitor(child, originalAdder);
				if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
				{
					InheritAccess access(opt.get());
					result.add(access);
				}
				else
				{
					CGL_ErrorNodeInternal(node, "node.adderの評価結果がRecordConstractorでありませんでした。");
				}
			}
			else
			{
				CGL_ErrorNodeInternal(node, "アクセッサの評価結果が不正です。");
			}
		}

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const DeclSat& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const DeclSat& node)");

		DeclSat result;
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const DeclFree& node)
	{
		auto scopeLog = ScopeLog("ClosureMaker::operator()(const DeclFree& node)");

		DeclFree result;
		for (const auto& freeVar : node.accessors)
		{
			const Expr expr = freeVar.accessor;
			const Expr closedAccessor = boost::apply_visitor(*this, expr);
			if (IsType<Accessor>(closedAccessor) || IsType<UnaryExpr>(closedAccessor))
			{
				result.addAccessor(closedAccessor);
			}
			else
			{
				CGL_ErrorNodeInternal(node, "アクセッサの評価結果が不正です。");
			}

			if (freeVar.range)
			{
				const Expr closedRange = boost::apply_visitor(*this, freeVar.range.get());
				result.addRange(closedRange);
			}
		}
		return result.setLocation(node);
	}

	Expr AsClosure(Context& context, const Expr& expr, const std::set<std::string>& functionArguments)
	{
		const bool original = TreeLogger::instance().isActive();
		TreeLogger::instance().setActive(false);

		ClosureMaker maker(context, functionArguments);
		auto result = boost::apply_visitor(maker, expr);

		TreeLogger::instance().setActive(original);

		return result;
	}
}
