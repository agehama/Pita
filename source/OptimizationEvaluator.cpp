#pragma warning(disable:4996)
#include <functional>

#include <Pita/Evaluator.hpp>
#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/Printer.hpp>

namespace cgl
{
	SatVariableBinder& SatVariableBinder::addLocalVariable(const std::string& name)
	{
		localVariables.insert(name);
		return *this;
	}

	bool SatVariableBinder::isLocalVariable(const std::string& name) const
	{
		return localVariables.find(name) != localVariables.end();
	}

	boost::optional<size_t> SatVariableBinder::freeVariableIndex(Address reference) const
	{
		for (size_t i = 0; i < freeVariables.size(); ++i)
		{
			if (freeVariables[i].first == reference)
			{
				return i;
			}
		}

		return boost::none;
	}

	//Address -> 参照ID
	boost::optional<int> SatVariableBinder::addSatRefImpl(Address reference)
	{
		//以前に出現して登録済みのfree変数はそのまま返す
		auto refID_It = invRefs.find(reference);
		if (refID_It != invRefs.end())
		{
			//CGL_DebugLog("addSatRef: 登録済み");
			return refID_It->second;
		}

		//初めて出現したfree変数は登録してから返す
		if (auto indexOpt = freeVariableIndex(reference))
		{
			//std::cout << "Register Free Variable" << std::endl;
			const int referenceID = refs.size();
			usedInSat[indexOpt.get()] = 1;
			invRefs[reference] = referenceID;
			refs.push_back(reference);
			refsSet.insert(reference);

			{
				pEnv->printContext(true);
			}

			return referenceID;
		}

		return boost::none;
	}

	bool SatVariableBinder::addSatRef(Address reference)
	{
		bool result = false;
		const auto addresses = pEnv->expandReferences(reference, LocationInfo());
		for (Address address : addresses)
		{
			result = static_cast<bool>(addSatRefImpl(address)) || result;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const LRValue& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		if (node.isLValue())
		{
			const Address address = node.deref(*pEnv).get();

			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//free変数にあった場合は制約用の参照値を追加する
			return addSatRef(address);
		}

		return false;
	}

	bool SatVariableBinder::operator()(const Identifier& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		if (isLocalVariable(node))
		{
			return false;
		}

		Address address = pEnv->findAddress(node);
		if (!address.isValid())
		{
			CGL_Error("識別子\"" + static_cast<std::string>(node) + "\"が定義されていません");
			return false;
		}

		//free変数にあった場合は制約用の参照値を追加する
		return addSatRef(address);
	}

	bool SatVariableBinder::operator()(const UnaryExpr& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		//++depth;
		const bool a = boost::apply_visitor(*this, node.lhs);
		//--depth;
		return a;
	}

	bool SatVariableBinder::operator()(const BinaryExpr& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		//ローカル変数の宣言となるのは、左辺が識別子の時のみ
		if (node.op == BinaryOp::Assign && IsType<Identifier>(node.lhs))
		{
			addLocalVariable(As<Identifier>(node.lhs));
		}

		bool a, b;

		//論理積は単位制約同士をまとめるのに使っているので、基本的にスコープは分かれているべき
		//内側の論理積はつながっている？
		if (node.op == BinaryOp::And)
		{
			{
				SatVariableBinder child(*this);
				a = boost::apply_visitor(child, node.lhs);
			}
			{
				SatVariableBinder child(*this);
				b = boost::apply_visitor(child, node.rhs);
			}
		}
		else
		{
			a = boost::apply_visitor(*this, node.lhs);
			b = boost::apply_visitor(*this, node.rhs);
		}

		return a || b;
	}
	
	bool SatVariableBinder::callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments, const LocationInfo& info)
	{
		//std::cout << getIndent() << typeid(funcVal).name() << std::endl;

		/*FuncVal funcVal;

		if (auto opt = AsOpt<FuncVal>(node.funcRef))
		{
			funcVal = opt.get();
		}
		else
		{
			const Address funcAddress = pEnv->findAddress(As<Identifier>(node.funcRef));
			if (funcAddress.isValid())
			{
				if (auto funcOpt = pEnv->expandOpt(funcAddress))
				{
					if (IsType<FuncVal>(funcOpt.get()))
					{
						funcVal = As<FuncVal>(funcOpt.get());
					}
					else
					{
						CGL_Error("指定された変数名に紐つけられた値が関数でない");
					}
				}
				else
				{
					CGL_Error("ここは通らないはず");
				}
			}
			else
			{
				CGL_Error("指定された変数名に値が紐つけられていない");
			}
		}*/

		//組み込み関数の場合は関数定義の中身と照らし合わせるという事ができないため、とりあえず引数から辿れる要素を全て指定する
		if (funcVal.builtinFuncAddress)
		{
			bool result = false;
			for (Address argument : expandedArguments)
			{
				const auto addresses = pEnv->expandReferences(argument, info);
				for (Address address : addresses)
				{
					result = addSatRef(address) || result;
				}
			}

			//もし組み込み関数の引数に変数が指定されていた場合は不連続関数かどうかを保存し、これによって最適化手法を切り替えられるようにしておく
			if (result)
			{
				hasPlateausFunction = pEnv->isPlateausBuiltInFunction(funcVal.builtinFuncAddress.get()) || hasPlateausFunction;
			}

			return result;
		}
		/*std::vector<Address> expandedArguments(node.actualArguments.size());
		for (size_t i = 0; i < expandedArguments.size(); ++i)
		{
			expandedArguments[i] = pEnv->makeTemporaryValue(node.actualArguments[i]);
		}*/

		if (funcVal.arguments.size() != expandedArguments.size())
		{
			CGL_Error("仮引数の数と実引数の数が合っていない");
		}

		pEnv->switchFrontScope();
		pEnv->enterScope();
		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
			//addLocalVariable(funcVal.arguments[i]);
		}
		bool result;
		{
			SatVariableBinder child(*this);
			//printExpr2(funcVal.expr, pEnv, std::cout);
			//++depth;
			result = boost::apply_visitor(child, funcVal.expr);
			//--depth;
		}
		pEnv->exitScope();
		pEnv->switchBackScope();

		return result;
	}

	bool SatVariableBinder::operator()(const Lines& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		bool result = false;
		for (const auto& expr : node.exprs)
		{
			//++depth;
			result = boost::apply_visitor(*this, expr) || result;
			//--depth;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const If& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		bool result = false;
		//++depth;
		result = boost::apply_visitor(*this, node.cond_expr) || result;
		result = boost::apply_visitor(*this, node.then_expr) || result;
		//--depth;
		if (node.else_expr)
		{
			//++depth;
			result = boost::apply_visitor(*this, node.else_expr.get()) || result;
			//--depth;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const For& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		bool result = false;

		addLocalVariable(node.loopCounter);

		//for式の範囲を制約で制御できるようにする意味はあるか？
		//result = boost::apply_visitor(*this, node.rangeEnd) || result;
		//result = boost::apply_visitor(*this, node.rangeStart) || result;

		//++depth;
		result = boost::apply_visitor(*this, node.doExpr) || result;
		//--depth;
		return result;
	}

	bool SatVariableBinder::operator()(const ListConstractor& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		bool result = false;
		for (const auto& expr : node.data)
		{
			//++depth;
			result = boost::apply_visitor(*this, expr) || result;
			//--depth;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const KeyExpr& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;
		
		bool result = boost::apply_visitor(*this, node.expr);
		addLocalVariable(node.name);

		return result;
	}

	bool SatVariableBinder::operator()(const RecordConstractor& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		bool result = false;
		//for (const auto& expr : node.exprs)
		for (size_t i = 0; i < node.exprs.size(); ++i)
		{
			const auto& expr = node.exprs[i];
			//CGL_DebugLog(std::string("BindRecordExpr(") + ToS(i) + ")");
			//printExpr(expr);
			//++depth;
			result = boost::apply_visitor(*this, expr) || result;
			//--depth;
		}
		return result;
	}

	bool SatVariableBinder::operator()(const Accessor& node)
	{
		//std::cout << getIndent() << typeid(node).name() << std::endl;

		/*CGL_DebugLog("SatVariableBinder::operator()(const Accessor& node)");
		{
			Expr expr = node;
			printExpr(expr);
		}*/

		Address headAddress;
		const Expr& head = node.head;

		//isDeterministicがtrueであれば、Evalしないとわからないところまで見に行く
		//isDeterministicがfalseの時は、評価するまでアドレスが不定なので関数の中身までは見に行かない
		bool isDeterministic = true;

		//headがsat式中のローカル変数
		if (auto headOpt = AsOpt<Identifier>(head))
		{
			if (isLocalVariable(headOpt.get()))
			{
				return false;
			}

			Address address = pEnv->findAddress(headOpt.get());
			if (!address.isValid())
			{
				CGL_Error(std::string("識別子\"") + static_cast<std::string>(headOpt.get()) + "\"が定義されていません");
			}

			//headは必ず Record/List/FuncVal のどれかであり、double型であることはあり得ない。
			//したがって、free変数にあるかどうかは考慮せず（free変数は冗長に指定できるのであったとしても別にエラーにはしない）、
			//直接Addressとして展開する
			headAddress = address;
		}
		//headがアドレス値
		else if (IsType<LRValue>(head))
		{
			const LRValue& headAddressValue = As<LRValue>(head);
			if (headAddressValue.isRValue())
			{
				CGL_Error("sat式中のアクセッサの先頭部が不正な値です");
			}

			headAddress = headAddressValue.deref(*pEnv).get();
		}
		//それ以外であれば、headはその場で作られるローカル変数とみなす
		else
		{
			//CGL_Error("sat中のアクセッサの先頭部に単一の識別子以外の式を用いることはできません");
			//isDeterministic = !boost::apply_visitor(*this, head);
			isDeterministic = false;
		}

		Eval evaluator(pEnv);

		Accessor result;

		for (const auto& access : node.accesses)
		{
			boost::optional<const Val&> objOpt;
			if (isDeterministic)
			{
				objOpt = pEnv->expandOpt(headAddress);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}
			}

			if (IsType<ListAccess>(access))
			{
				const ListAccess& listAccess = As<ListAccess>(access);
				
				//++depth;
				isDeterministic = !boost::apply_visitor(*this, listAccess.index) && isDeterministic;
				//--depth;

				if (isDeterministic)
				{
					const Val& objRef = objOpt.get();
					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					const List& list = As<List>(objRef);

					Val indexValue = pEnv->expand(boost::apply_visitor(evaluator, listAccess.index), node);

					if (auto indexOpt = AsOpt<int>(indexValue))
					{
						headAddress = list.get(indexOpt.get());
					}
					else
					{
						CGL_Error("list[index] の index が int 型でない");
					}
				}
			}
			else if (IsType<RecordAccess>(access))
			{
				const RecordAccess& recordAccess = As<RecordAccess>(access);

				if (isDeterministic)
				{
					const Val& objRef = objOpt.get();
					if (!IsType<Record>(objRef))
					{
						CGL_Error("オブジェクトがレコードでない");
					}
					
					const Record& record = As<Record>(objRef);
					auto it = record.values.find(recordAccess.name);
					if (it == record.values.end())
					{
						CGL_Error("指定された識別子がレコード中に存在しない");
					}

					headAddress = it->second;
				}
			}
			else if (IsType<FunctionAccess>(access))
			{
				const FunctionAccess& funcAccess = As<FunctionAccess>(access);

				if (isDeterministic)
				{
					//Case2(関数引数がfree)への対応
					for (const auto& argument : funcAccess.actualArguments)
					{
						//++depth;
						isDeterministic = !boost::apply_visitor(*this, argument) && isDeterministic;
						//--depth;
					}

					//呼ばれる関数の実体はその引数には依存しないため、ここでisDeterministicがfalseになっても問題ない

					//std::cout << getIndent() << typeid(node).name() << " -> objOpt.get()" << std::endl;
					//Case4以降への対応は関数の中身を見に行く必要がある
					const Val& objRef = objOpt.get();
					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("オブジェクトが関数でない");
					}
					const FuncVal& function = As<FuncVal>(objRef);

					//Case4,6への対応
					/*
					std::vector<Val> arguments;
					for (const auto& expr : funcAccess.actualArguments)
					{
					//ここでexpandして大丈夫なのか？
					arguments.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
					}
					Expr caller = FunctionCaller(function, arguments);
					isDeterministic = !boost::apply_visitor(*this, caller) && isDeterministic;
					*/
					//std::cout << getIndent() << typeid(node).name() << " -> actualArguments" << std::endl;
					std::vector<Address> arguments;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue lrvalue = boost::apply_visitor(evaluator, expr);
						lrvalue.push_back(arguments, *pEnv);
					}
					isDeterministic = !callFunction(function, arguments, node) && isDeterministic;

					//ここまでで一つもfree変数が出てこなければこの先の中身も見に行く
					if (isDeterministic)
					{
						//std::cout << getIndent() << typeid(node).name() << " -> isDeterministic" << std::endl;
						//const Val returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
						const Val returnedValue = pEnv->expand(evaluator.callFunction(node, function, arguments), node);
						headAddress = pEnv->makeTemporaryValue(returnedValue);
					}
				}
			}
			else
			{
				const InheritAccess& inheritAccess = As<InheritAccess>(access);
				{
					bool searchResult = false;
					for (size_t i = 0; i < inheritAccess.adder.exprs.size(); ++i)
					{
						const auto& expr = inheritAccess.adder.exprs[i];
						//CGL_DebugLog(std::string("BindRecordExpr(") + ToS(i) + ")");
						//printExpr(expr);
						//++depth;
						searchResult = boost::apply_visitor(*this, expr) || searchResult;
						//--depth;
					}
					return searchResult;
				}
			}
		}

		if (isDeterministic)
		{
			return static_cast<bool>(addSatRef(headAddress));
		}

		//std::cout << getIndent() << "End " << typeid(node).name() << std::endl;
		return true;
	}

	boost::optional<double> EvalSatExpr::expandFreeOpt(Address address)const
	{
		auto it = invRefs.find(address);
		if (it != invRefs.end())
		{
			return data[it->second];
		}
		return boost::none;
	}

	LRValue EvalSatExpr::operator()(const UnaryExpr& node)
	{
		if (node.op == UnaryOp::Not)
		{
			CGL_ErrorNode(node, "TODO: sat宣言中のnot演算子は未対応です。");
		}

		Val lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs), node);
		switch (node.op)
		{
		case UnaryOp::Plus: return lhs;
		case UnaryOp::Minus:  return MinusFunc(lhs, *pEnv);
		}

		CGL_ErrorNodeInternal(node, "不明な単項演算子です。");
		return LRValue(0);
	}

	LRValue EvalSatExpr::operator()(const BinaryExpr& node)
	{
		const double true_cost = 0.0;
		const double false_cost = 10000.0;

		Val rhs = pEnv->expand(boost::apply_visitor(*this, node.rhs), node);
		if (node.op != BinaryOp::Assign)
		{
			Val lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs), node);

			if (IsType<bool>(rhs))
			{
				rhs = As<bool>(rhs) ? true_cost : false_cost;
			}
			if (IsType<bool>(lhs))
			{
				lhs = As<bool>(lhs) ? true_cost : false_cost;
			}

			switch (node.op)
			{
			case BinaryOp::And: return AddFunc(lhs, rhs, *pEnv);
			case BinaryOp::Or:  return MinFunc(lhs, rhs, *pEnv);

			case BinaryOp::Equal:        return AbsFunc(SubFunc(lhs, rhs, *pEnv), *pEnv);
			case BinaryOp::NotEqual:     return EqualFunc(lhs, rhs, *pEnv) ? LRValue(true_cost) : LRValue(false_cost);
			case BinaryOp::LessThan:     return MaxFunc(SubFunc(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::LessEqual:    return MaxFunc(SubFunc(lhs, rhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterThan:  return MaxFunc(SubFunc(rhs, lhs, *pEnv), 0.0, *pEnv);
			case BinaryOp::GreaterEqual: return MaxFunc(SubFunc(rhs, lhs, *pEnv), 0.0, *pEnv);

			case BinaryOp::Add: return AddFunc(lhs, rhs, *pEnv);
			case BinaryOp::Sub: return SubFunc(lhs, rhs, *pEnv);
			case BinaryOp::Mul: return MulFunc(lhs, rhs, *pEnv);
			case BinaryOp::Div: return DivFunc(lhs, rhs, *pEnv);

			case BinaryOp::Pow:    return PowFunc(lhs, rhs, *pEnv);
			case BinaryOp::Concat: return ConcatFunc(lhs, rhs, *pEnv);

			case BinaryOp::SetDiff: return SetDiffFunc(lhs, rhs, *pEnv);
			default:;
			}
		}
		else if (auto valOpt = AsOpt<LRValue>(node.lhs))
		{
			CGL_ErrorNode(node, "一時オブジェクトへの代入はできません。");
		}
		else if (auto valOpt = AsOpt<Identifier>(node.lhs))
		{
			const Identifier& identifier = valOpt.get();

			const Address address = pEnv->findAddress(identifier);
			//変数が存在する：代入式
			if (address.isValid())
			{
				//pEnv->assignToObject(address, rhs);
				pEnv->bindValueID(identifier, pEnv->makeTemporaryValue(rhs));
				//CGL_ErrorNode(node, "制約式中では変数への再代入は行えません。");
			}
			//変数が存在しない：変数宣言式
			else
			{
				pEnv->bindNewValue(identifier, rhs);
			}

			return rhs;
		}
		else if (auto accessorOpt = AsOpt<Accessor>(node.lhs))
		{
			Eval evaluator(pEnv);
			const LRValue lhs = boost::apply_visitor(evaluator, node.lhs);
			if (lhs.isLValue())
			{
				if (lhs.isValid())
				{
					//pEnv->assignToObject(address, rhs);
					pEnv->assignToAccessor(accessorOpt.get(), LRValue(rhs), node);
					return rhs;
				}
				else
				{
					CGL_ErrorNode(node, "アクセッサの評価結果が無効なアドレス値です。");
				}
			}
			else
			{
				CGL_Error("アクセッサの評価結果が無効な値です。");
			}
		}

		CGL_ErrorNodeInternal(node, "不明な二項演算子です。");
		return LRValue(0);
	}

	LRValue EvalSatExpr::operator()(const KeyExpr& node)
	{
		const LRValue rhs_ = boost::apply_visitor(*this, node.expr);
		Val rhs = pEnv->expand(rhs_, node);
		if (pEnv->existsInCurrentScope(node.name))
		{
			CGL_ErrorNode(node, "制約式中では変数への再代入は行えません。");
		}
		else
		{
			pEnv->bindNewValue(node.name, rhs);
		}

		return RValue(rhs);
	}
}
