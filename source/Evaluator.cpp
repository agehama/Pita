#pragma warning(disable:4996)
#include <cppoptlib/meta.h>
#include <cppoptlib/problem.h>
#include <cppoptlib/solver/bfgssolver.h>

#include <cmaes.h>

#include <Pita/Evaluator.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/Printer.hpp>
#include <Pita/Geometry.hpp>

extern int constraintViolationCount;

namespace cgl
{
	void ProgressStore::TryWrite(std::shared_ptr<Context> env, const Val& value)
	{
		ProgressStore& i = Instance();
		if (i.mtx.try_lock())
		{
			i.pEnv = env->clone();
			i.evaluated = value;

			i.mtx.unlock();
		}
	}

	bool ProgressStore::TryLock()
	{
		ProgressStore& i = Instance();
		return i.mtx.try_lock();
	}

	void ProgressStore::Unlock()
	{
		ProgressStore& i = Instance();
		i.mtx.unlock();
	}

	std::shared_ptr<Context> ProgressStore::GetContext()
	{
		ProgressStore& i = Instance();
		return i.pEnv;
	}

	boost::optional<Val>& ProgressStore::GetVal()
	{
		ProgressStore& i = Instance();
		return i.evaluated;
	}

	void ProgressStore::Reset()
	{
		ProgressStore& i = Instance();
		i.pEnv = nullptr;
		i.evaluated = boost::none;
	}

	bool ProgressStore::HasValue()
	{
		ProgressStore& i = Instance();
		return static_cast<bool>(i.evaluated);
	}

	ProgressStore& ProgressStore::Instance()
	{
		static ProgressStore i;
		return i;
	}

	boost::optional<Address> AddressReplacer::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Expr AddressReplacer::operator()(const LRValue& node)const
	{
		if (node.isRValue())
		{
			return node;
		}

		if (auto opt = getOpt(node.address(*pEnv)))
		{
			if (node.isReference())
			{
				return LRValue(pEnv->cloneReference(node.reference(), replaceMap));
			}
			else
			{
				return LRValue(opt.value());
			}
		}

		return node;
	}
	
	Expr AddressReplacer::operator()(const UnaryExpr& node)const
	{
		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op);
	}

	Expr AddressReplacer::operator()(const BinaryExpr& node)const
	{
		return BinaryExpr(
			boost::apply_visitor(*this, node.lhs), 
			boost::apply_visitor(*this, node.rhs), 
			node.op);
	}

	Expr AddressReplacer::operator()(const Range& node)const
	{
		return Range(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs));
	}

	Expr AddressReplacer::operator()(const Lines& node)const
	{
		Lines result;
		for (size_t i = 0; i < node.size(); ++i)
		{
			result.add(boost::apply_visitor(*this, node.exprs[i]));
		}
		return result;
	}

	Expr AddressReplacer::operator()(const DefFunc& node)const
	{
		return DefFunc(node.arguments, boost::apply_visitor(*this, node.expr));
	}

	Expr AddressReplacer::operator()(const If& node)const
	{
		If result(boost::apply_visitor(*this, node.cond_expr));
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (node.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
		}

		return result;
	}

	Expr AddressReplacer::operator()(const For& node)const
	{
		For result;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
		result.loopCounter = node.loopCounter;
		result.doExpr = boost::apply_visitor(*this, node.doExpr);
		return result;
	}

	Expr AddressReplacer::operator()(const Return& node)const
	{
		return Return(boost::apply_visitor(*this, node.expr));
	}

	Expr AddressReplacer::operator()(const ListConstractor& node)const
	{
		ListConstractor result;
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(*this, expr));
		}
		return result;
	}

	Expr AddressReplacer::operator()(const KeyExpr& node)const
	{
		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result;
	}

	Expr AddressReplacer::operator()(const RecordConstractor& node)const
	{
		RecordConstractor result;
		for (const auto& expr : node.exprs)
		{
			result.exprs.push_back(boost::apply_visitor(*this, expr));
		}
		return result;
	}
		
	Expr AddressReplacer::operator()(const RecordInheritor& node)const
	{
		RecordInheritor result;
		result.original = boost::apply_visitor(*this, node.original);

		Expr originalAdder = node.adder;
		Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
		if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
		{
			result.adder = opt.value();
			return result;
		}

		CGL_Error("node.adderの置き換え結果がRecordConstractorでない");
		return LRValue(0);
	}

	Expr AddressReplacer::operator()(const Accessor& node)const
	{
		Accessor result;
		result.head = boost::apply_visitor(*this, node.head);
			
		for (const auto& access : node.accesses)
		{
			if (auto listAccess = AsOpt<ListAccess>(access))
			{
				result.add(ListAccess(boost::apply_visitor(*this, listAccess.value().index)));
			}
			else if (auto recordAccess = AsOpt<RecordAccess>(access))
			{
				result.add(recordAccess.value());
			}
			else if (auto functionAccess = AsOpt<FunctionAccess>(access))
			{
				FunctionAccess access;
				for (const auto& argument : functionAccess.value().actualArguments)
				{
					access.add(boost::apply_visitor(*this, argument));
				}
				result.add(access);
			}
			else
			{
				CGL_Error("aaa");
			}
		}

		return result;
	}

	Expr AddressReplacer::operator()(const DeclSat& node)const
	{
		return DeclSat(boost::apply_visitor(*this, node.expr));
	}

	Expr AddressReplacer::operator()(const DeclFree& node)const
	{
		DeclFree result;
		for (const auto& accessor : node.accessors)
		{
			Expr expr = accessor;
			result.addAccessor(boost::apply_visitor(*this, expr));
		}
		for (const auto& range : node.ranges)
		{
			result.addRange(boost::apply_visitor(*this, range));
		}
		return result;
	}

	boost::optional<Address> ValueCloner::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Val ValueCloner::operator()(const List& node)
	{
		List result;
		const auto& data = node.data;
		
		for (size_t i = 0; i < data.size(); ++i)
		{
			if (auto opt = getOpt(data[i]))
			{
				result.append(opt.value());
			}
			else
			{
				const Val& substance = pEnv->expand(data[i]);
				const Val clone = boost::apply_visitor(*this, substance);
				const Address newAddress = pEnv->makeTemporaryValue(clone);
				result.append(newAddress);
				replaceMap[data[i]] = newAddress;
			}
		}

		return result;
	}

	Val ValueCloner::operator()(const Record& node)
	{
		Record result;

		for (const auto& value : node.values)
		{
			if (auto opt = getOpt(value.second))
			{
				result.append(value.first, opt.value());
			}
			else
			{
				const Val& substance = pEnv->expand(value.second);
				const Val clone = boost::apply_visitor(*this, substance);
				const Address newAddress = pEnv->makeTemporaryValue(clone);
				result.append(value.first, newAddress);
				replaceMap[value.second] = newAddress;
			}
		}

		result.problems = node.problems;
		result.freeVariables = node.freeVariables;
		//result.freeVariableRefs = node.freeVariableRefs;
		result.type = node.type;
		result.isSatisfied = node.isSatisfied;

		return result;
	}

	boost::optional<Address> ValueCloner2::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Val ValueCloner2::operator()(const List& node)
	{
		const auto& data = node.data;

		for (size_t i = 0; i < data.size(); ++i)
		{
			//ValueCloner1でクローンは既に作ったので、そのクローンを直接書き換える
			const Val& substance = pEnv->expand(data[i]);
			pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(data[i], boost::apply_visitor(*this, substance));
		}

		return node;
	}
		
	Val ValueCloner2::operator()(const Record& node)
	{
		for (const auto& value : node.values)
		{
			//ValueCloner1でクローンは既に作ったので、そのクローンを直接書き換える
			const Val& substance = pEnv->expand(value.second);
			pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(value.second, boost::apply_visitor(*this, substance));
		}

		return node;
	}

	Val ValueCloner2::operator()(const FuncVal& node)const
	{
		if (node.builtinFuncAddress)
		{
			return node;
		}

		FuncVal result;
		result.arguments = node.arguments;
		result.builtinFuncAddress = node.builtinFuncAddress;

		AddressReplacer replacer(replaceMap, pEnv);
		result.expr = boost::apply_visitor(replacer, node.expr);

		return result;
	}
	
	boost::optional<Address> ValueCloner3::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Address ValueCloner3::replaced(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return address;
	}

	void ValueCloner3::operator()(Record& node)
	{
		AddressReplacer replacer(replaceMap, pEnv);

		if (node.constraint)
		{
			node.constraint = boost::apply_visitor(replacer, node.constraint.value());
		}

		for (auto& problem : node.problems)
		{
			if (problem.expr)
			{
				problem.expr = boost::apply_visitor(replacer, problem.expr.value());
			}

			for (size_t i = 0; i < problem.refs.size(); ++i)
			{
				const Address oldAddress = problem.refs[i];
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.value();
					problem.refs[i] = newAddress;
					problem.invRefs[newAddress] = problem.invRefs[oldAddress];
					problem.invRefs.erase(oldAddress);
				}
			}	
		}

		for (auto& freeVariable : node.freeVariables)
		{
			Expr expr = freeVariable;
			freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
		}
		
		/*
		auto& problem = node.problem;
		if (problem.candidateExpr)
		{
			problem.candidateExpr = boost::apply_visitor(replacer, problem.candidateExpr.value());
		}

		for (size_t i = 0; i < problem.refs.size(); ++i)
		{
			const Address oldAddress = problem.refs[i];
			if (auto newAddressOpt = getOpt(oldAddress))
			{
				const Address newAddress = newAddressOpt.value();
				problem.refs[i] = newAddress;
				problem.invRefs[newAddress] = problem.invRefs[oldAddress];
				problem.invRefs.erase(oldAddress);
			}
		}

		for (auto& freeVariable : node.freeVariables)
		{
			Expr expr = freeVariable;
			freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
		}
		*/
	}

	Val Clone(std::shared_ptr<Context> pEnv, const Val& value)
	{
		/*
		関数値がアドレスを内部に持っている時、クローン作成の前後でその依存関係を保存する必要があるので、クローン作成は2ステップに分けて行う。
		1. リスト・レコードの再帰的なコピー
		2. 関数の持つアドレスを新しい方に付け替える		
		*/
		ValueCloner cloner(pEnv);
		const Val& evaluated = boost::apply_visitor(cloner, value);
		ValueCloner2 cloner2(pEnv, cloner.replaceMap);
		ValueCloner3 cloner3(pEnv, cloner.replaceMap);
		Val evaluated2 = boost::apply_visitor(cloner2, evaluated);
		boost::apply_visitor(cloner3, evaluated2);
		return evaluated2;
	}


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
		//その関数のローカル変数であれば関数の実行時に評価すればよいので、名前を残す
		if (isLocalVariable(node))
		{
			return node;
		}
		//ローカル変数に無ければアドレスに置き換える
		const Address address = pEnv->findAddress(node);
		if (address.isValid())
		{
			//Identifier RecordConstructor の形をしたレコード継承の head 部分
			//とりあえず参照先のレコードのメンバはローカル変数とおく
			if (isInnerRecord)
			{
				const Val& evaluated = pEnv->expand(address);
				if (auto opt = AsOpt<Record>(evaluated))
				{
					const Record& record = opt.value();
					
					for (const auto& keyval : record.values)
					{
						addLocalVariable(keyval.first);
					}
				}
			}

			return LRValue(address);
		}

		CGL_Error("識別子が定義されていません");
		return LRValue(0);
	}

	Expr ClosureMaker::operator()(const UnaryExpr& node)
	{
		if (node.op == UnaryOp::Dynamic && !isInnerClosure)
		{
			/*Eval evaluator(pEnv);
			
			const LRValue lrvalue = boost::apply_visitor(evaluator, node.lhs);
			if (lrvalue.isRValue())
			{
				CGL_Error("単項@演算子の右辺には識別子かアクセッサしか用いることができません");
			}

			return LRValue(pEnv->bindReference(lrvalue.address(*pEnv)));*/

			if (IsType<Identifier>(node.lhs))
			{
				return LRValue(pEnv->bindReference(As<Identifier>(node.lhs)));
			}
			else if (IsType<Accessor>(node.lhs))
			{
				return LRValue(pEnv->bindReference(As<Accessor>(node.lhs)));
			}

			CGL_Error("参照演算子@の右辺には識別子かアクセッサしか用いることができません");
		}
		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op);
	}

	Expr ClosureMaker::operator()(const BinaryExpr& node)
	{
		const Expr rhs = boost::apply_visitor(*this, node.rhs);

		if (node.op != BinaryOp::Assign)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);
			return BinaryExpr(lhs, rhs, node.op);
		}

		//Assignの場合、lhs は Address or Identifier or Accessor に限られる
		//つまり現時点では、(if cond then x else y) = true のような式を許可していない
		//ここで左辺に直接アドレスが入っていることは有り得る？
		//a = b = 10　のような式でも、右結合であり左側は常に識別子が残っているはずなので、あり得ないと思う
		if (auto valOpt = AsOpt<Identifier>(node.lhs))
		{
			const Identifier identifier = valOpt.value();

			//ローカル変数にあれば、その場で解決できる識別子なので何もしない
			if (isLocalVariable(identifier))
			{
				return BinaryExpr(node.lhs, rhs, node.op);
			}
			else
			{
				const Address address = pEnv->findAddress(identifier);

				//ローカル変数に無く、スコープにあれば、アドレスに置き換える
				if (address.isValid())
				{
					//TODO: 制約式の場合は、ここではじく必要がある
					return BinaryExpr(LRValue(address), rhs, node.op);
				}
				//スコープにも無い場合は新たなローカル変数の宣言なので、ローカル変数に追加しておく
				else
				{
					addLocalVariable(identifier);
					return BinaryExpr(node.lhs, rhs, node.op);
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

			return BinaryExpr(lhs, rhs, node.op);
		}

		CGL_Error("二項演算子\"=\"の左辺は単一の左辺値でなければなりません");
		return LRValue(0);
	}

	Expr ClosureMaker::operator()(const Range& node)
	{
		return Range(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs));
	}

	Expr ClosureMaker::operator()(const Lines& node)
	{
		Lines result;
		for (size_t i = 0; i < node.size(); ++i)
		{
			result.add(boost::apply_visitor(*this, node.exprs[i]));
		}
		return result;
	}

	Expr ClosureMaker::operator()(const DefFunc& node)
	{
		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, true);
		for (const auto& argument : node.arguments)
		{
			child.addLocalVariable(argument);
		}

		return DefFunc(node.arguments, boost::apply_visitor(child, node.expr));
	}

	Expr ClosureMaker::operator()(const If& node)
	{
		If result(boost::apply_visitor(*this, node.cond_expr));
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (node.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
		}

		return result;
	}

	Expr ClosureMaker::operator()(const For& node)
	{
		For result;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, isInnerClosure);
		child.addLocalVariable(node.loopCounter);
		result.doExpr = boost::apply_visitor(child, node.doExpr);
		result.loopCounter = node.loopCounter;

		return result;
	}

	Expr ClosureMaker::operator()(const Return& node)
	{
		return Return(boost::apply_visitor(*this, node.expr));
		//これだとダメかもしれない？
		//return a = 6, a + 2
	}

	Expr ClosureMaker::operator()(const ListConstractor& node)
	{
		ListConstractor result;
		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, isInnerClosure);
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(child, expr));
		}
		return result;
	}

	Expr ClosureMaker::operator()(const KeyExpr& node)
	{
		//変数宣言式
		//再代入の可能性もあるがどっちにしろこれ以降この識別子はローカル変数と扱ってよい
		addLocalVariable(node.name);

		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result;
	}

	Expr ClosureMaker::operator()(const RecordConstractor& node)
	{
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
			ClosureMaker child(pEnv, localVariables, false, isInnerClosure);
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(child, expr));
			}
		}			
			
		return result;
	}

	//レコード継承構文については特殊で、adderを評価する時のスコープはheadと同じである必要がある。
	//つまり、headを評価する時にはその中身を、一段階だけ（波括弧一つ分だけ）展開するようにして評価しなければならない。
	Expr ClosureMaker::operator()(const RecordInheritor& node)
	{
		RecordInheritor result;

		//新たに追加
		ClosureMaker child(pEnv, localVariables, true, isInnerClosure);

		//result.original = boost::apply_visitor(*this, node.original);
		result.original = boost::apply_visitor(child, node.original);
			
		Expr originalAdder = node.adder;
		//Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
		Expr replacedAdder = boost::apply_visitor(child, originalAdder);
		if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
		{
			result.adder = opt.value();
			return result;
		}

		CGL_Error("node.adderの置き換え結果がRecordConstractorでない");
		return LRValue(0);
	}

	Expr ClosureMaker::operator()(const Accessor& node)
	{
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
				if (listAccess.value().isArbitrary)
				{
					result.add(listAccess.value());
				}
				else
				{
					result.add(ListAccess(boost::apply_visitor(*this, listAccess.value().index)));
				}
			}
			else if (auto recordAccess = AsOpt<RecordAccess>(access))
			{
				result.add(recordAccess.value());
			}
			else if (auto functionAccess = AsOpt<FunctionAccess>(access))
			{
				FunctionAccess access;
				for (const auto& argument : functionAccess.value().actualArguments)
				{
					access.add(boost::apply_visitor(*this, argument));
				}
				result.add(access);
			}
			else
			{
				CGL_Error("aaa");
			}
		}

		return result;
	}
		
	Expr ClosureMaker::operator()(const DeclSat& node)
	{
		DeclSat result;
		result.expr = boost::apply_visitor(*this, node.expr);
		return result;
	}

	Expr ClosureMaker::operator()(const DeclFree& node)
	{
		DeclFree result;
		for (const auto& accessor : node.accessors)
		{
			const Expr expr = accessor;
			const Expr closedAccessor = boost::apply_visitor(*this, expr);
			if (!IsType<Accessor>(closedAccessor))
			{
				CGL_Error("不正な式です");
			}
			result.addAccessor(As<Accessor>(closedAccessor));
		}
		for (const auto& range : node.ranges)
		{
			const Expr closedRange = boost::apply_visitor(*this, range);
			result.addRange(closedRange);
		}
		return result;
	}

	bool ConstraintProblem::callback(const cppoptlib::Criteria<cppoptlib::Problem<double>::Scalar> &state, const TVector &x)
	{
		/*
		Record tempRecord = originalRecord;

		for (size_t i = 0; i < x.size(); ++i)
		{
			Address address = originalRecord.freeVariableRefs[i];
			pEnv->assignToObject(address, x[i]);
		}

		for (const auto& key : keyList)
		{
			Address address = pEnv->findAddress(key);
			tempRecord.append(key, address);
		}

		ProgressStore::TryWrite(pEnv, tempRecord);
		*/
		return true;
	}
	
	LRValue Eval::operator()(const UnaryExpr& node)
	{
		const Val lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs));

		switch (node.op)
		{
		case UnaryOp::Not:     return RValue(Not(lhs, *pEnv));
		case UnaryOp::Minus:   return RValue(Minus(lhs, *pEnv));
		case UnaryOp::Plus:    return RValue(Plus(lhs, *pEnv));
		case UnaryOp::Dynamic: return RValue(lhs);
		}

		CGL_Error("Invalid UnaryExpr");
		return RValue(0);
	}

	LRValue Eval::operator()(const BinaryExpr& node)
	{
		const LRValue lhs_ = boost::apply_visitor(*this, node.lhs);
		const LRValue rhs_ = boost::apply_visitor(*this, node.rhs);

		Val lhs;
		if (node.op != BinaryOp::Assign)
		{
			lhs = pEnv->expand(lhs_);
			
		}

		Val rhs = pEnv->expand(rhs_);

		//const Val rhs = pEnv->expand(boost::apply_visitor(*this, node.rhs));

		switch (node.op)
		{
		case BinaryOp::And: return RValue(And(lhs, rhs, *pEnv));
		case BinaryOp::Or:  return RValue(Or(lhs, rhs, *pEnv));

		case BinaryOp::Equal:        return RValue(Equal(lhs, rhs, *pEnv));
		case BinaryOp::NotEqual:     return RValue(NotEqual(lhs, rhs, *pEnv));
		case BinaryOp::LessThan:     return RValue(LessThan(lhs, rhs, *pEnv));
		case BinaryOp::LessEqual:    return RValue(LessEqual(lhs, rhs, *pEnv));
		case BinaryOp::GreaterThan:  return RValue(GreaterThan(lhs, rhs, *pEnv));
		case BinaryOp::GreaterEqual: return RValue(GreaterEqual(lhs, rhs, *pEnv));

		case BinaryOp::Add: return RValue(Add(lhs, rhs, *pEnv));
		case BinaryOp::Sub: return RValue(Sub(lhs, rhs, *pEnv));
		case BinaryOp::Mul: return RValue(Mul(lhs, rhs, *pEnv));
		case BinaryOp::Div: return RValue(Div(lhs, rhs, *pEnv));

		case BinaryOp::Pow:    return RValue(Pow(lhs, rhs, *pEnv));
		case BinaryOp::Concat: return RValue(Concat(lhs, rhs, *pEnv));
		case BinaryOp::Assign:
		{
			//return Assign(lhs, rhs, *pEnv);

			//Assignの場合、lhs は Address or Identifier or Accessor に限られる
			//つまり現時点では、(if cond then x else y) = true のような式を許可していない
			//ここで左辺に直接アドレスが入っていることは有り得る？
			//a = b = 10　のような式でも、右結合であり左側は常に識別子が残っているはずなので、あり得ないと思う
			if (auto valOpt = AsOpt<LRValue>(node.lhs))
			{
				CGL_Error("一時オブジェクトへの代入はできません");
				/*const LRValue& val = valOpt.value();
				if (val.isLValue())
				{
					if (val.address().isValid())
					{
						pEnv->assignToObject(val.address(), rhs);
					}
					else
					{
						CGL_Error("reference error");
					}
				}
				else
				{
					CGL_Error("");
				}*/
			}
			else if (auto valOpt = AsOpt<Identifier>(node.lhs))
			{
				const Identifier& identifier = valOpt.value();

				const Address address = pEnv->findAddress(identifier);
				//変数が存在する：代入式
				if (address.isValid())
				{
					CGL_DebugLog("代入式");
					//pEnv->assignToObject(address, rhs);
					pEnv->bindValueID(identifier, pEnv->makeTemporaryValue(rhs));
				}
				//変数が存在しない：変数宣言式
				else
				{
					CGL_DebugLog("変数宣言式");
					pEnv->bindNewValue(identifier, rhs);
					CGL_DebugLog("");
				}

				return RValue(rhs);
			}
			else if (auto accessorOpt = AsOpt<Accessor>(node.lhs))
			{
				if (lhs_.isLValue())
				{
					if (lhs_.isValid())
					{
						pEnv->assignToAccessor(accessorOpt.value(), rhs_);
						return RValue(rhs);
					}
					else
					{
						CGL_Error("参照エラー");
					}
				}
				else
				{
					CGL_Error("アクセッサの評価結果がアドレスでない");
				}
			}
		}
		}

		CGL_Error("Invalid BinaryExpr");
		return RValue(0);
	}

	LRValue Eval::operator()(const DefFunc& defFunc)
	{
		return pEnv->makeFuncVal(pEnv, defFunc.arguments, defFunc.expr);
	}

	LRValue Eval::callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments)
	{
		CGL_DebugLog("Function Context:");
		pEnv->printContext();

		/*
		まだ参照をスコープ間で共有できるようにしていないため、引数に与えられたオブジェクトは全て展開して渡す。
		そして、引数の評価時点ではまだ関数の中に入っていないので、スコープを変える前に展開を行う。
		*/

		/*
		12/14
		全ての値はIDで管理するようにする。
		そしてスコープが変わると、変数のマッピングは変わるが、値は共通なのでどちらからも参照できる。
		*/
		/*
		std::vector<Address> expandedArguments(callFunc.actualArguments.size());
		for (size_t i = 0; i < expandedArguments.size(); ++i)
		{
			expandedArguments[i] = pEnv->makeTemporaryValue(callFunc.actualArguments[i]);
		}

		CGL_DebugLog("");

		FuncVal funcVal;

		if (auto opt = AsOpt<FuncVal>(callFunc.funcRef))
		{
			funcVal = opt.value();
		}
		else
		{
			const Address funcAddress = pEnv->findAddress(As<Identifier>(callFunc.funcRef));
			if (funcAddress.isValid())
			{
				if (auto funcOpt = pEnv->expandOpt(funcAddress))
				{
					if (IsType<FuncVal>(funcOpt.value()))
					{
						funcVal = As<FuncVal>(funcOpt.value());
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
		}
		*/

		/*if (funcVal.builtinFuncAddress)
		{
			return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments));
		}*/
		if (funcVal.builtinFuncAddress)
		{
			return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments));
		}

		CGL_DebugLog("");

		//if (funcVal.arguments.size() != callFunc.actualArguments.size())
		if (funcVal.arguments.size() != expandedArguments.size())
		{
			CGL_Error("仮引数の数と実引数の数が合っていない");
		}

		//関数の評価
		//(1)ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える

		pEnv->switchFrontScope();
		//今の意味論ではスコープへの参照は全てアドレス値に変換している
		//ここで、関数内からグローバル変数の書き換えを行おうとすると、アドレスに紐つけられた値を直接変更することになる
		//TODO: これは、意味論的に正しいのか一度考える必要がある
		//とりあえず関数がスコープに依存することはなくなったので、単純に別のスコープに切り替えるだけで良い

		CGL_DebugLog("");

		//(2)関数の引数用にスコープを一つ追加する
		pEnv->enterScope();

		CGL_DebugLog("");

		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			/*
			12/14
			引数はスコープをまたぐ時に参照先が変わらないように全てIDで渡すことにする。
			*/
			pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
		}

		CGL_DebugLog("Function Definition:");
		printExpr(funcVal.expr);

		//(3)関数の戻り値を元のスコープに戻す時も、引数と同じ理由で全て展開して渡す。
		//Val result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));
		Val result;
		{
			//const Val resultValue = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
			//result = pEnv->expandRef(pEnv->makeTemporaryValue(resultValue));
			/*
			EliminateScopeDependency elim(pEnv);
			result = pEnv->makeTemporaryValue(boost::apply_visitor(elim, resultValue));
			*/
			result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
			CGL_DebugLog("Function Val:");
			printVal(result, nullptr);
		}
		//Val result = pEnv->expandObject();

		CGL_DebugLog("");

		//(4)関数を抜ける時に、仮引数は全て解放される
		pEnv->exitScope();

		CGL_DebugLog("");

		//(5)最後にローカル変数の環境を関数の実行前のものに戻す。
		pEnv->switchBackScope();

		CGL_DebugLog("");
			
		//評価結果がreturn式だった場合はreturnを外して中身を返す
		//return以外のジャンプ命令は関数では効果を持たないのでそのまま上に返す
		if (IsType<Jump>(result))
		{
			auto& jump = As<Jump>(result);
			if (jump.isReturn())
			{
				if (jump.lhs)
				{
					return RValue(jump.lhs.value());
				}
				else
				{
					CGL_Error("return式の中身が入って無い");
				}
			}
		}

		return RValue(result);
	}

	LRValue Eval::operator()(const Lines& statement)
	{
		pEnv->enterScope();

		Val result;
		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			CGL_DebugLog("Evaluate expression(" + std::to_string(i) + ")");
			pEnv->printContext();

			//printExpr(expr);
			CGL_DebugLog("");
			result = pEnv->expand(boost::apply_visitor(*this, expr));
			printVal(result, pEnv);

			CGL_DebugLog("");

			//TODO: 後で考える
			//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
			/*
			if (IsLValue(result))
			{
				//const Val& resultValue = pEnv->dereference(result);
				const Val resultValue = pEnv->expandRef(result);
				const bool isMacro = IsType<Jump>(resultValue);
				if (isMacro)
				{
					result = As<Jump>(resultValue);
				}
			}
			*/

			CGL_DebugLog("");
			//途中でジャンプ命令を読んだら即座に評価を終了する
			if (IsType<Jump>(result))
			{
				CGL_DebugLog("");
				break;
			}

			CGL_DebugLog("");

			++i;
		}

		CGL_DebugLog("");

		//この後すぐ解放されるので dereference しておく
		bool deref = true;
		/*
		if (auto refOpt = AsOpt<ObjectReference>(result))
		{
			if (IsType<unsigned>(refOpt.value().headValue))
			{
				deref = false;
			}
		}
		
		if (deref)
		{
			result = pEnv->dereference(result);
		}
		*/

		/*
		if (auto refOpt = AsOpt<Address>(result))
		{
			//result = pEnv->dereference(refOpt.value());
			//result = pEnv->expandRef(refOpt.value());
			result = pEnv->expand(refOpt.value());
		}
		*/

		CGL_DebugLog("");

		pEnv->exitScope();

		CGL_DebugLog("");
		return LRValue(result);
	}

	LRValue Eval::operator()(const If& if_statement)
	{
		const Val cond = pEnv->expand(boost::apply_visitor(*this, if_statement.cond_expr));
		if (!IsType<bool>(cond))
		{
			//条件は必ずブール値である必要がある
			//std::cerr << "Error(" << __LINE__ << ")\n";
			CGL_Error("条件は必ずブール値である必要がある");
		}

		if (As<bool>(cond))
		{
			const Val result = pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr));
			return RValue(result);

		}
		else if (if_statement.else_expr)
		{
			const Val result = pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.value()));
			return RValue(result);
		}

		//else式が無いケースで cond = False であったら一応警告を出す
		//std::cerr << "Warning(" << __LINE__ << ")\n";
		CGL_WarnLog("else式が無いケースで cond = False であった");
		return RValue(0);
	}

	LRValue Eval::operator()(const For& forExpression)
	{
		const Val startVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeStart));
		const Val endVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeEnd));

		//startVal <= endVal なら 1
		//startVal > endVal なら -1
		//を適切な型に変換して返す
		const auto calcStepValue = [&](const Val& a, const Val& b)->boost::optional<std::pair<Val, bool>>
		{
			const bool a_IsInt = IsType<int>(a);
			const bool a_IsDouble = IsType<double>(a);

			const bool b_IsInt = IsType<int>(b);
			const bool b_IsDouble = IsType<double>(b);

			if (!((a_IsInt || a_IsDouble) && (b_IsInt || b_IsDouble)))
			{
				//エラー：ループのレンジが不正な型（整数か実数に評価できる必要がある）
				//std::cerr << "Error(" << __LINE__ << ")\n";
				//return boost::none;
				CGL_Error("ループのレンジが不正な型（整数か実数に評価できる必要がある）");
			}

			const bool result_IsDouble = a_IsDouble || b_IsDouble;
			//const auto lessEq = LessEqual(a, b, *pEnv);
			//if (!IsType<bool>(lessEq))
			//{
			//	//エラー：aとbの比較に失敗した
			//	//一応確かめているだけでここを通ることはないはず
			//	//LessEqualの実装ミス？
			//	//std::cerr << "Error(" << __LINE__ << ")\n";
			//	//return boost::none;
			//	CGL_Error("LessEqualの実装ミス？");
			//}

			//const bool isInOrder = As<bool>(lessEq);
			const bool isInOrder = LessEqual(a, b, *pEnv);

			const int sign = isInOrder ? 1 : -1;

			if (result_IsDouble)
			{
				/*const Val xx = Mul(1.0, sign, *pEnv);
				return std::pair<Val, bool>(xx, isInOrder);*/

				//std::pair<Val, bool> xa(Mul(1.0, sign, *pEnv), isInOrder);

				/*const Val xx = Mul(1.0, sign, *pEnv);
				std::pair<Val, bool> xa(xx, isInOrder);
				return boost::optional<std::pair<Val, bool>>(xa);*/

				return std::pair<Val, bool>(Mul(1.0, sign, *pEnv), isInOrder);
			}

			return std::pair<Val, bool>(Mul(1, sign, *pEnv), isInOrder);
		};

		const auto loopContinues = [&](const Val& loopCount, bool isInOrder)->boost::optional<bool>
		{
			//isInOrder == true
			//loopCountValue <= endVal

			//isInOrder == false
			//loopCountValue > endVal

			const Val result = LessEqual(loopCount, endVal, *pEnv);
			if (!IsType<bool>(result))
			{
				CGL_Error("ここを通ることはないはず");
			}

			return As<bool>(result) == isInOrder;
		};

		const auto stepOrder = calcStepValue(startVal, endVal);
		if (!stepOrder)
		{
			CGL_Error("ループのレンジが不正");
		}

		const Val step = stepOrder.value().first;
		const bool isInOrder = stepOrder.value().second;

		Val loopCountValue = startVal;

		Val loopResult;

		pEnv->enterScope();

		while (true)
		{
			const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
			if (!isLoopContinuesOpt)
			{
				CGL_Error("ここを通ることはないはず");
			}

			//ループの継続条件を満たさなかったので抜ける
			if (!isLoopContinuesOpt.value())
			{
				break;
			}

			pEnv->bindNewValue(forExpression.loopCounter, loopCountValue);

			loopResult = pEnv->expand(boost::apply_visitor(*this, forExpression.doExpr));

			//ループカウンタの更新
			loopCountValue = Add(loopCountValue, step, *pEnv);
		}

		pEnv->exitScope();

		return RValue(loopResult);
	}

	LRValue Eval::operator()(const Return& return_statement)
	{
		const Val lhs = pEnv->expand(boost::apply_visitor(*this, return_statement.expr));

		return RValue(Jump::MakeReturn(lhs));
	}

	LRValue Eval::operator()(const ListConstractor& listConstractor)
	{
		List list;
		for (const auto& expr : listConstractor.data)
		{
			LRValue lrvalue = boost::apply_visitor(*this, expr);
			if (lrvalue.isLValue())
			{
				list.append(lrvalue.address(*pEnv));
			}
			else
			{
				list.append(pEnv->makeTemporaryValue(lrvalue.evaluated()));
			}
		}

		return RValue(list);
	}

	LRValue Eval::operator()(const KeyExpr& keyExpr)
	{
		const Val value = pEnv->expand(boost::apply_visitor(*this, keyExpr.expr));

		return RValue(KeyValue(keyExpr.name, value));
		//return LRValue(pEnv->makeTemporaryValue(KeyValue(keyExpr.name, value)));
	}

	LRValue Eval::operator()(const RecordConstractor& recordConsractor)
	{
		CGL_DebugLog("");
		pEnv->enterScope();
		CGL_DebugLog("");

		std::vector<Identifier> keyList;
		/*
		レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
		レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアスとして定義（スコープを抜けたら元に戻る≒遮蔽）
		*/

		for (size_t i = 0; i < recordConsractor.exprs.size(); ++i)
		{
			CGL_DebugLog(std::string("RecordExpr(") + ToS(i) + "): ");
			printExpr(recordConsractor.exprs[i]);
		}

		Record newRecord;

		if (pEnv->temporaryRecord)
		{
			//pEnv->currentRecords.push(pEnv->temporaryRecord.value());
			pEnv->currentRecords.push_back(pEnv->temporaryRecord.value());
			pEnv->temporaryRecord = boost::none;
		}
		else
		{
			pEnv->currentRecords.push_back(std::ref(newRecord));
		}

		//Record& record = pEnv->currentRecords.top();
		Record& record = pEnv->currentRecords.back();

		int i = 0;

		for (const auto& expr : recordConsractor.exprs)
		{
			CGL_DebugLog("");
			CGL_DebugLog("Evaluate: ");
			printExpr(expr/*, std::cout*/);
			Val value = pEnv->expand(boost::apply_visitor(*this, expr));
			CGL_DebugLog("Result: ");
			printVal(value, pEnv);

			//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
			if (auto keyValOpt = AsOpt<KeyValue>(value))
			{
				const auto keyVal = keyValOpt.value();
				keyList.push_back(keyVal.name);

				CGL_DebugLog(std::string("assign to ") + static_cast<std::string>(keyVal.name));

				Expr exprVal = LRValue(keyVal.value);
				Expr expr = BinaryExpr(keyVal.name, exprVal, BinaryOp::Assign);
				boost::apply_visitor(*this, expr);

				CGL_DebugLog("");
			}

			//valueは今は右辺値のみになっている
			//TODO: もう一度考察する
			/*
			//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
			if (IsLValue(value))
			{
				const Val resultValue = pEnv->expandRef(value);
				const bool isMacro = IsType<Jump>(resultValue);
				if (isMacro)
				{
					value = As<Jump>(resultValue);
				}
			}

			//途中でジャンプ命令を読んだら即座に評価を終了する
			if (IsType<Jump>(value))
			{
				break;
			}
			*/

			++i;
		}

		pEnv->printContext();
		CGL_DebugLog("");

		/*for (const auto& satExpr : innerSatClosures)
		{
			record.problem.addConstraint(satExpr);
		}
		innerSatClosures.clear();*/

		CGL_DebugLog("");

		if (record.constraint)
		{
			record.problems.clear();

			///////////////////////////////////
			//1. free変数に指定されたアドレスの展開

			const auto& ranges = record.freeRanges;
			std::vector<PackedVal> packedRanges;
			bool hasRange = !ranges.empty();
			for (const auto& rangeExpr : ranges)
			{
				packedRanges.push_back(Packed(pEnv->expand(boost::apply_visitor(*this, rangeExpr)), *pEnv));
			}

			//変数ID->アドレス
			std::vector<std::pair<Address, VariableRange>> freeVariableAddresses;
			for (size_t i = 0; i < record.freeVariables.size(); ++i)
			{
				const auto& accessor = record.freeVariables[i];
				boost::optional<const PackedVal&> range = hasRange ? boost::optional<const PackedVal&>(packedRanges[i]) : boost::none;
				const auto addresses = pEnv->expandReferences2(accessor, range);

				freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
			}


			//TODO:freeVariableAddressesの重複を削除する


			///////////////////////////////////
			//2. 変数の依存関係を見て独立した制約を分解

			ConjunctionSeparater separater;
			boost::apply_visitor(separater, record.constraint.value());
			const auto& unitConstraints = separater.conjunctions;
			
			using ConstraintAppearance = std::unordered_set<Address>;//unitConstraintに出現するfree変数のAddress
			std::vector<ConstraintAppearance> variableAppearances;//制約ID -> ConstraintAppearance
			for (const auto& constraint : unitConstraints)
			{
				ConstraintAppearance appearingList;

				std::vector<char> usedInSat(freeVariableAddresses.size(), 0);
				std::vector<Address> refs;
				std::unordered_map<Address, int> invRefs;
				bool hasPlateausFunction = false;

				SatVariableBinder binder(pEnv, freeVariableAddresses, usedInSat, refs, appearingList, invRefs, hasPlateausFunction);
				boost::apply_visitor(binder, constraint);

				variableAppearances.push_back(appearingList);
			}

			using ConstraintGroup = std::unordered_set<size_t>;//同じfree変数への依存性を持つ制約IDの組

			const auto intersects = [](const ConstraintAppearance& addressesA, const ConstraintAppearance& addressesB)
			{
				for (const Address address : addressesA)
				{
					if (addressesB.find(address) != addressesB.end())
					{
						return true;
					}
				}
				return false;
			};

			const auto intersectsToConstraintGroup = [&](const ConstraintAppearance& addressesA, const ConstraintGroup& targetGroup)
			{
				for (const size_t targetConstraintID : targetGroup)
				{
					const auto& targetAppearingAddresses = variableAppearances[targetConstraintID];
					if (intersects(addressesA, targetAppearingAddresses))
					{
						return true;
					}
				}
				return false;
			};

			std::vector<ConstraintGroup> constraintGroups;
			for (size_t constraintID = 0; constraintID < unitConstraints.size(); ++constraintID)
			{
				const auto& currentAppearingAddresses = variableAppearances[constraintID];

				std::set<size_t> currentIntersectsGroupIDs;
				for (size_t constraintGroupID = 0; constraintGroupID < constraintGroups.size(); ++constraintGroupID)
				{
					if (intersectsToConstraintGroup(currentAppearingAddresses, constraintGroups[constraintGroupID]))
					{
						currentIntersectsGroupIDs.insert(constraintGroupID);
					}
				}

				if (currentIntersectsGroupIDs.empty())
				{
					ConstraintGroup newGroup;
					newGroup.insert(constraintID);
					constraintGroups.push_back(newGroup);
				}
				else
				{
					ConstraintGroup newGroup;
					newGroup.insert(constraintID);

					for (const size_t groupID : currentIntersectsGroupIDs)
					{
						const auto& targetGroup = constraintGroups[groupID];
						for (const size_t targetConstraintID : targetGroup)
						{
							newGroup.insert(targetConstraintID);
						}
					}

					for (auto it = currentIntersectsGroupIDs.rbegin(); it != currentIntersectsGroupIDs.rend(); ++it)
					{
						const size_t existingGroupIndex = *it;
						constraintGroups.erase(constraintGroups.begin() + existingGroupIndex);
					}

					constraintGroups.push_back(newGroup);
				}
			}
			
			std::cout << "Record constraint separated to " << std::to_string(constraintGroups.size()) << " independent constraints" << std::endl;


			///////////////////////////////////
			//3. 独立した制約ごとに解く

			record.problems.resize(constraintGroups.size());
			for (size_t constraintGroupID = 0; constraintGroupID < constraintGroups.size(); ++constraintGroupID)
			{
				auto& currentProblem = record.problems[constraintGroupID];
				const auto& currentConstraintIDs = constraintGroups[constraintGroupID];
				
				for (size_t constraintID : currentConstraintIDs)
				{
					currentProblem.addUnitConstraint(unitConstraints[constraintID]);
				}

				const auto& problemRefs = currentProblem.refs;

				currentProblem.freeVariableRefs = freeVariableAddresses;

				//展開された式をSatExprに変換する
				//＋satの式に含まれる変数の内、freeVariableRefsに入っていないものは即時に評価して畳み込む
				//freeVariableRefsに入っているものは最適化対象の変数としてdataに追加していく
				//＋freeVariableRefsの変数についてはsat式に出現したかどうかを記録し削除する
				currentProblem.constructConstraint(pEnv);

				//設定されたdataが参照している値を見て初期値を設定する
				if (!currentProblem.initializeData(pEnv))
				{
					return LRValue(0);
				}

				std::cout << "Current constraint freeVariablesSize: " << std::to_string(currentProblem.freeVariableRefs.size()) << std::endl;

				std::vector<double> resultxs;

				if (!currentProblem.freeVariableRefs.empty())
				{
					//varのアドレス(の内実際にsatに現れるもののリスト)から、OptimizationProblemSat中の変数リストへの対応付けを行うマップを作成
					std::unordered_map<int, int> variable2Data;
					for (size_t freeIndex = 0; freeIndex < currentProblem.freeVariableRefs.size(); ++freeIndex)
					{
						CGL_DebugLog(ToS(freeIndex));
						CGL_DebugLog(std::string("Address(") + currentProblem.freeVariableRefs[freeIndex].toString() + ")");
						const auto& ref1 = currentProblem.freeVariableRefs[freeIndex];

						bool found = false;
						for (size_t dataIndex = 0; dataIndex < problemRefs.size(); ++dataIndex)
						{
							CGL_DebugLog(ToS(dataIndex));
							CGL_DebugLog(std::string("Address(") + problemRefs[dataIndex].toString() + ")");

							const auto& ref2 = problemRefs[dataIndex];

							if (ref1.first == ref2)
							{
								//std::cout << "    " << freeIndex << " -> " << dataIndex << std::endl;

								found = true;
								variable2Data[freeIndex] = dataIndex;
								break;
							}
						}

						//DeclFreeにあってDeclSatに無い変数は意味がない。
						//単に無視しても良いが、恐らく入力のミスと思われるので警告を出す
						if (!found)
						{
							CGL_WarnLog("freeに指定された変数が無効です");
						}
					}
					CGL_DebugLog("End Record MakeMap");

					if (currentProblem.hasPlateausFunction)
					{
						std::cout << "Solve constraint by CMA-ES...\n";

						libcmaes::FitFunc func = [&](const double *x, const int N)->double
						{
							for (int i = 0; i < N; ++i)
							{
								currentProblem.update(variable2Data[i], x[i]);
							}

							{
								for (const auto& keyval : currentProblem.invRefs)
								{
									pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(keyval.first, currentProblem.data[keyval.second]);
								}
							}

							pEnv->switchFrontScope();
							double result = currentProblem.eval(pEnv);
							pEnv->switchBackScope();

							CGL_DebugLog(std::string("cost: ") + ToS(result, 17));

							return result;
						};

						CGL_DebugLog("");

						std::vector<double> x0(currentProblem.freeVariableRefs.size());
						for (int i = 0; i < x0.size(); ++i)
						{
							x0[i] = currentProblem.data[variable2Data[i]];
							CGL_DebugLog(ToS(i) + " : " + ToS(x0[i]));
						}

						CGL_DebugLog("");

						const double sigma = 0.1;

						const int lambda = 100;

						libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda, 1);
						CGL_DebugLog("");
						libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
						CGL_DebugLog("");
						resultxs = cmasols.best_candidate().get_x();
						CGL_DebugLog("");

						std::cout << "solved\n";
					}
					else
					{
						std::cout << "Solve constraint by BFGS...\n";

						ConstraintProblem constraintProblem;
						constraintProblem.evaluator = [&](const ConstraintProblem::TVector& v)->double
						{
							//-1000 -> 1000
							for (int i = 0; i < v.size(); ++i)
							{
								currentProblem.update(variable2Data[i], v[i]);
								//problem.update(variable2Data[i], (v[i] - 0.5)*2000.0);
							}

							{
								for (const auto& keyval : currentProblem.invRefs)
								{
									pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(keyval.first, currentProblem.data[keyval.second]);
								}
							}

							pEnv->switchFrontScope();
							double result = currentProblem.eval(pEnv);
							pEnv->switchBackScope();

							CGL_DebugLog(std::string("cost: ") + ToS(result, 17));
							//std::cout << std::string("cost: ") << ToS(result, 17) << "\n";
							return result;
						};
						constraintProblem.originalRecord = record;
						constraintProblem.keyList = keyList;
						constraintProblem.pEnv = pEnv;

						Eigen::VectorXd x0s(currentProblem.freeVariableRefs.size());
						for (int i = 0; i < x0s.size(); ++i)
						{
							x0s[i] = currentProblem.data[variable2Data[i]];
							//x0s[i] = (problem.data[variable2Data[i]] / 2000.0) + 0.5;
							CGL_DebugLog(ToS(i) + " : " + ToS(x0s[i]));
						}

						cppoptlib::BfgsSolver<ConstraintProblem> solver;
						solver.minimize(constraintProblem, x0s);

						resultxs.resize(x0s.size());
						for (int i = 0; i < x0s.size(); ++i)
						{
							resultxs[i] = x0s[i];
						}

						//std::cout << "solved\n";
					}
				}


				CGL_DebugLog("");
				for (size_t i = 0; i < resultxs.size(); ++i)
				{
					Address address = currentProblem.freeVariableRefs[i].first;
					const auto range = currentProblem.freeVariableRefs[i].second;
					std::cout << "Address(" << address.toString() << "): [" << range.minimum << ", " << range.maximum << "]\n";
					pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, resultxs[i]);
					//pEnv->assignToObject(address, (resultxs[i] - 0.5)*2000.0);
				}

				if (currentProblem.expr)
				{
					const Val result = pEnv->expand(boost::apply_visitor(*this, currentProblem.expr.value()));
					if (!IsType<bool>(result))
					{
						CGL_Error("制約式の評価結果がブール値でない");
					}

					const bool currentConstraintIsSatisfied = As<bool>(result);;
					record.isSatisfied = record.isSatisfied && currentConstraintIsSatisfied;
					//std::cout << "Constraint is " << (record.isSatisfied ? "satisfied\n" : "not satisfied\n");
					if (!currentConstraintIsSatisfied)
					{
						++constraintViolationCount;
					}
				}
			}




#ifdef commentout

			auto& problem = record.problem;
			auto& freeVariableRefs = record.freeVariableRefs;

			const auto& problemRefs = problem.refs;
			const auto& freeVariables = record.freeVariables;

			const auto& ranges = record.freeRanges;
			std::vector<PackedVal> packedRanges;

			bool hasRange = !ranges.empty();

			for (const auto& rangeExpr : ranges)
			{
				packedRanges.push_back(Packed(pEnv->expand(boost::apply_visitor(*this, rangeExpr)), *pEnv));
			}

			{
				//record.freeVariablesをもとにrecord.freeVariableRefsを構築
				//全てのアクセッサを展開し、各変数の参照リストを作成する
				freeVariableRefs.clear();
				//for (const auto& accessor : record.freeVariables)
				for (size_t i = 0; i < record.freeVariables.size(); ++i)
				{
					/*
					const Address refAddress = pEnv->evalReference(accessor);
					//単一の値 or List or Record
					if (refAddress.isValid())
					{
						const auto addresses = pEnv->expandReferences(refAddress);
						freeVariableRefs.insert(freeVariableRefs.end(), addresses.begin(), addresses.end());
					}
					else
					{
						CGL_Error("accessor refers null address");
					}
					*/
					const auto& accessor = record.freeVariables[i];
					boost::optional<const PackedVal&> range = hasRange ? boost::optional<const PackedVal&>(packedRanges[i]) : boost::none;
					const auto addresses = pEnv->expandReferences2(accessor, range);

					freeVariableRefs.insert(freeVariableRefs.end(), addresses.begin(), addresses.end());
				}

				CGL_DebugLog(std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size()));

				//一度sat式の中身を展開し、
				//Accessorを展開するvisitor（Expr -> Expr）を作り、実行する
				//その後Sat2Exprに掛ける
				//Expr2SatExprでは単項演算子・二項演算子の評価の際、中身をみて定数であったら畳み込む処理を行う

				//satの関数呼び出しを全て式に展開する
				//{
				//	//problem.candidateExpr = pEnv->expandFunction(problem.candidateExpr.value());
				//	Expr ee = pEnv->expandFunction(problem.candidateExpr.value());
				//	problem.candidateExpr = ee;
				//}

				//展開された式をSatExprに変換する
				//＋satの式に含まれる変数の内、freeVariableRefsに入っていないものは即時に評価して畳み込む
				//freeVariableRefsに入っているものは最適化対象の変数としてdataに追加していく
				//＋freeVariableRefsの変数についてはsat式に出現したかどうかを記録し削除する
				problem.constructConstraint(pEnv, record.freeVariableRefs);

				//設定されたdataが参照している値を見て初期値を設定する
				if (!problem.initializeData(pEnv))
				{
					return LRValue(0);
				}

				CGL_DebugLog(std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size()));
				CGL_DebugLog(std::string("Record SatExpr: "));
				std::cout << (std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size())) << std::endl;
			}

			if (!record.freeVariableRefs.empty())
			{
				//varのアドレス(の内実際にsatに現れるもののリスト)から、OptimizationProblemSat中の変数リストへの対応付けを行うマップを作成
				std::unordered_map<int, int> variable2Data;
				for (size_t freeIndex = 0; freeIndex < record.freeVariableRefs.size(); ++freeIndex)
				{
					CGL_DebugLog(ToS(freeIndex));
					CGL_DebugLog(std::string("Address(") + record.freeVariableRefs[freeIndex].toString() + ")");
					const auto& ref1 = record.freeVariableRefs[freeIndex];

					bool found = false;
					for (size_t dataIndex = 0; dataIndex < problemRefs.size(); ++dataIndex)
					{
						CGL_DebugLog(ToS(dataIndex));
						CGL_DebugLog(std::string("Address(") + problemRefs[dataIndex].toString() + ")");

						const auto& ref2 = problemRefs[dataIndex];

						if (ref1.first == ref2)
						{
							//std::cout << "    " << freeIndex << " -> " << dataIndex << std::endl;

							found = true;
							variable2Data[freeIndex] = dataIndex;
							break;
						}
					}

					//DeclFreeにあってDeclSatに無い変数は意味がない。
					//単に無視しても良いが、恐らく入力のミスと思われるので警告を出す
					if (!found)
					{
						//std::cerr << "Error(" << __LINE__ << "):freeに指定された変数が無効です。\n";
						//return 0;
						CGL_WarnLog("freeに指定された変数が無効です");
					}
				}
				CGL_DebugLog("End Record MakeMap");

				if (problem.hasPlateausFunction)
				{
					std::cout << "Solve constraint by CMA-ES...\n";

					libcmaes::FitFunc func = [&](const double *x, const int N)->double
					{
						for (int i = 0; i < N; ++i)
						{
							problem.update(variable2Data[i], x[i]);
						}

						{
							for (const auto& keyval : problem.invRefs)
							{
								pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(keyval.first, problem.data[keyval.second]);
							}
						}

						pEnv->switchFrontScope();
						double result = problem.eval(pEnv);
						pEnv->switchBackScope();

						CGL_DebugLog(std::string("cost: ") + ToS(result, 17));

						return result;
					};

					CGL_DebugLog("");

					std::vector<double> x0(record.freeVariableRefs.size());
					for (int i = 0; i < x0.size(); ++i)
					{
						x0[i] = problem.data[variable2Data[i]];
						CGL_DebugLog(ToS(i) + " : " + ToS(x0[i]));
					}

					CGL_DebugLog("");

					const double sigma = 0.1;

					const int lambda = 100;

					libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda, 1);
					CGL_DebugLog("");
					libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
					CGL_DebugLog("");
					resultxs = cmasols.best_candidate().get_x();
					CGL_DebugLog("");

					std::cout << "solved\n";
				}
				else
				{
					std::cout << "Solve constraint by BFGS...\n";

					ConstraintProblem constraintProblem;
					constraintProblem.evaluator = [&](const ConstraintProblem::TVector& v)->double
					{
						//-1000 -> 1000
						for (int i = 0; i < v.size(); ++i)
						{
							problem.update(variable2Data[i], v[i]);
							//problem.update(variable2Data[i], (v[i] - 0.5)*2000.0);
						}

						{
							for (const auto& keyval : problem.invRefs)
							{
								pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(keyval.first, problem.data[keyval.second]);
							}
						}

						pEnv->switchFrontScope();
						double result = problem.eval(pEnv);
						pEnv->switchBackScope();

						CGL_DebugLog(std::string("cost: ") + ToS(result, 17));
						//std::cout << std::string("cost: ") << ToS(result, 17) << "\n";
						return result;
					};
					constraintProblem.originalRecord = record;
					constraintProblem.keyList = keyList;
					constraintProblem.pEnv = pEnv;

					Eigen::VectorXd x0s(record.freeVariableRefs.size());
					for (int i = 0; i < x0s.size(); ++i)
					{
						x0s[i] = problem.data[variable2Data[i]];
						//x0s[i] = (problem.data[variable2Data[i]] / 2000.0) + 0.5;
						CGL_DebugLog(ToS(i) + " : " + ToS(x0s[i]));
					}

					cppoptlib::BfgsSolver<ConstraintProblem> solver;
					solver.minimize(constraintProblem, x0s);

					resultxs.resize(x0s.size());
					for (int i = 0; i < x0s.size(); ++i)
					{
						resultxs[i] = x0s[i];
					}

					//std::cout << "solved\n";
				}
			}
#endif
		}


#ifdef commentout
		CGL_DebugLog("");
		for (size_t i = 0; i < resultxs.size(); ++i)
		{
			Address address = record.freeVariableRefs[i].first;
			const auto range = record.freeVariableRefs[i].second;
			std::cout << "Address(" << address.toString() << "): [" << range.minimum << ", " << range.maximum << "]\n";
			pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, resultxs[i]);
			//pEnv->assignToObject(address, (resultxs[i] - 0.5)*2000.0);
		}

		if (record.problem.candidateExpr)
		{
			const Val result = pEnv->expand(boost::apply_visitor(*this, record.problem.candidateExpr.value()));
			if (!IsType<bool>(result))
			{
				CGL_Error("制約式の評価結果がブール値でない");
			}

			record.isSatisfied = As<bool>(result);
			//std::cout << "Constraint is " << (record.isSatisfied ? "satisfied\n" : "not satisfied\n");
			if (!record.isSatisfied)
			{
				++constraintViolationCount;
			}
		}
#endif

		for (const auto& key : keyList)
		{
			//record.append(key.name, pEnv->dereference(key));

			//record.append(key.name, pEnv->makeTemporaryValue(pEnv->dereference(ObjectReference(key))));

			/*Address address = pEnv->findAddress(key);
			boost::optional<const Val&> opt = pEnv->expandOpt(address);
			record.append(key, pEnv->makeTemporaryValue(opt.value()));*/

			Address address = pEnv->findAddress(key);
			record.append(key, address);
		}

		if (record.type == RecordType::Path)
		{
			GetPath(record, pEnv);
		}
		else if (record.type == RecordType::Text)
		{
			GetText(record, pEnv);
		}
		else if (record.type == RecordType::ShapePath)
		{
			GetText(record, pEnv);
		}

		pEnv->printContext();

		CGL_DebugLog("");

		//pEnv->currentRecords.pop();
		pEnv->currentRecords.pop_back();

		//pEnv->pop();
		pEnv->exitScope();

		const Address address = pEnv->makeTemporaryValue(record);

		//pEnv->garbageCollect();

		CGL_DebugLog("--------------------------- Print Context ---------------------------");
		pEnv->printContext();
		CGL_DebugLog("-------------------------------------------------------------------------");

		//return RValue(record);
		return LRValue(address);
	}

	LRValue Eval::operator()(const RecordInheritor& record)
	{
		boost::optional<Record> recordOpt;

		/*
		if (auto opt = AsOpt<Identifier>(record.original))
		{
			//auto originalOpt = pEnv->find(opt.value().name);
			boost::optional<const Val&> originalOpt = pEnv->dereference(pEnv->findAddress(opt.value()));
			if (!originalOpt)
			{
				//エラー：未定義のレコードを参照しようとした
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}

			recordOpt = AsOpt<Record>(originalOpt.value());
			if (!recordOpt)
			{
				//エラー：識別子の指すオブジェクトがレコード型ではない
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
		}
		else if (auto opt = AsOpt<Record>(record.original))
		{
			recordOpt = opt.value();
		}
		*/

		//Val originalRecordRef = boost::apply_visitor(*this, record.original);
		//Val originalRecordVal = pEnv->expandRef(originalRecordRef);

		/*
		a{}を評価する手順
		(1) オリジナルのレコードaのクローン(a')を作る
		(2) a'の各キーと値に対する参照をローカルスコープに追加する
		(3) 追加するレコードの中身を評価する
		(4) ローカルスコープの参照値を読みレコードに上書きする //リストアクセスなどの変更処理
		(5) レコードをマージする //ローカル変数などの変更処理
		*/

		if (IsType<Identifier>(record.original) && As<Identifier>(record.original) == std::string("path"))
		{
			Record pathRecord;
			pathRecord.type = RecordType::Path;
			recordOpt = pathRecord;
		}
		else if (IsType<Identifier>(record.original) && As<Identifier>(record.original) == std::string("text"))
		{
			Record pathRecord;
			pathRecord.type = RecordType::Text;
			recordOpt = pathRecord;
		}
		else if (IsType<Identifier>(record.original) && As<Identifier>(record.original) == std::string("shapepath"))
		{
			Record pathRecord;
			pathRecord.type = RecordType::ShapePath;
			recordOpt = pathRecord;
		}
		else
		{
			Val originalRecordVal = pEnv->expand(boost::apply_visitor(*this, record.original));
			if (auto opt = AsOpt<Record>(originalRecordVal))
			{
				recordOpt = opt.value();
			}
			else
			{
				CGL_Error("not record");
			}

			pEnv->printContext(true);
			CGL_DebugLog("Original:");
			printVal(originalRecordVal, pEnv);
		}

		//(1)オリジナルのレコードaのクローン(a')を作る
		Record clone = As<Record>(Clone(pEnv, recordOpt.value()));

		CGL_DebugLog("Clone:");
		printVal(clone, pEnv);

		if (pEnv->temporaryRecord)
		{
			CGL_Error("レコード拡張に失敗");
		}
		pEnv->temporaryRecord = clone;

		//(2) a'の各キーと値に対する参照をローカルスコープに追加する
		pEnv->enterScope();
		for (auto& keyval : clone.values)
		{
			pEnv->makeVariable(keyval.first, keyval.second);
			
			CGL_DebugLog(std::string("Bind ") + keyval.first + " -> " + "Address(" + keyval.second.toString() + ")");
		}

		//(3) 追加するレコードの中身を評価する
		Expr expr = record.adder;
		Val recordValue = pEnv->expand(boost::apply_visitor(*this, expr));

		//(4) ローカルスコープの参照値を読みレコードに上書きする
		if (auto opt = AsOpt<Record>(recordValue))
		{
			Record& newRecord = opt.value();
			for (const auto& keyval : clone.values)
			{
				const Address newAddress = pEnv->findAddress(keyval.first);
				if (newAddress.isValid() && newAddress != keyval.second)
				{
					CGL_DebugLog(std::string("Updated ") + keyval.first + ": " + "Address(" + keyval.second.toString() + ") -> Address(" + newAddress.toString() + ")");
					newRecord.values[keyval.first] = newAddress;
				}
			}
		}

		pEnv->exitScope();

		return pEnv->makeTemporaryValue(recordValue);

		/*
		//(2)
		pEnv->enterScope();
		for (auto& keyval : clone.values)
		{
			//pEnv->bindObjectRef(keyval.first, keyval.second);
			pEnv->makeVariable(keyval.first, keyval.second);

			CGL_DebugLog(std::string("Bind ") + keyval.first + " -> " + "Address(" + keyval.second.toString() + ")");
		}

		CGL_DebugLog("");

		//(3)
		Expr expr = record.adder;
		//Val recordValue = boost::apply_visitor(*this, expr);
		Val recordValue = pEnv->expand(boost::apply_visitor(*this, expr));
		if (auto opt = AsOpt<Record>(recordValue))
		{
			CGL_DebugLog("");

			//(4)
			for (auto& keyval : recordOpt.value().values)
			{
				clone.values[keyval.first] = pEnv->findAddress(keyval.first);
			}

			CGL_DebugLog("");

			//(5)
			MargeRecordInplace(clone, opt.value());

			CGL_DebugLog("");

			//TODO:ここで制約処理を行う

			//pEnv->pop();
			pEnv->exitScope();

			//return RValue(clone);
			return pEnv->makeTemporaryValue(clone);

			//MargeRecord(recordOpt.value(), opt.value());
		}

		CGL_DebugLog("");

		//pEnv->pop();
		pEnv->exitScope();
		*/

		//ここは通らないはず。{}で囲まれた式を評価した結果がレコードでなかった。
		//std::cerr << "Error(" << __LINE__ << ")\n";
		CGL_Error("ここは通らないはず");
		return RValue(0);
	}

	LRValue Eval::operator()(const DeclSat& node)
	{
		//ここでクロージャを作る必要がある
		ClosureMaker closureMaker(pEnv, {});
		const Expr closedSatExpr = boost::apply_visitor(closureMaker, node.expr);
		//innerSatClosures.push_back(closedSatExpr);

		pEnv->enterScope();
		//DeclSat自体は現在制約が満たされているかどうかを評価結果として返す
		const Val result = pEnv->expand(boost::apply_visitor(*this, closedSatExpr));
		pEnv->exitScope();

		if (pEnv->currentRecords.empty())
		{
			CGL_Error("sat宣言はレコードの中にしか書くことができません");
		}

		//pEnv->currentRecords.back().get().problem.addConstraint(closedSatExpr);
		pEnv->currentRecords.back().get().addConstraint(closedSatExpr);

		return RValue(result);
		//return RValue(false);
	}

	LRValue Eval::operator()(const DeclFree& node)
	{
		for (const auto& accessor : node.accessors)
		{
			//std::cout << "  accessor:" << std::endl;
			if (pEnv->currentRecords.empty())
			{
				CGL_Error("var宣言はレコードの中にしか書くことができません");
			}

			ClosureMaker closureMaker(pEnv, {});
			const Expr varExpr = accessor;
			const Expr closedVarExpr = boost::apply_visitor(closureMaker, varExpr);

			if (IsType<Accessor>(closedVarExpr))
			{
				//std::cout << "    Free Expr:" << std::endl;
				//printExpr(closedVarExpr, std::cout);
				pEnv->currentRecords.back().get().freeVariables.push_back(As<Accessor>(closedVarExpr));
			}
			else if (IsType<Identifier>(closedVarExpr))
			{
				Accessor result(closedVarExpr);
				pEnv->currentRecords.back().get().freeVariables.push_back(result);
			}
			else
			{
				CGL_Error("var宣言に指定された変数が無効です");
			}
			/*const LRValue result = boost::apply_visitor(*this, expr);
			if (!result.isLValue())
			{
				CGL_Error("var宣言に指定された変数は無効です");
			}*/

			//currentRecords.top().get().freeVariables.push_back(result.address());
		}

		for (const auto& range : node.ranges)
		{
			ClosureMaker closureMaker(pEnv, {});
			const Expr closedRangeExpr = boost::apply_visitor(closureMaker, range);
			pEnv->currentRecords.back().get().freeRanges.push_back(closedRangeExpr);
		}

		return RValue(0);
	}

	LRValue Eval::operator()(const Accessor& accessor)
	{
		/*
		ObjectReference result;

		Val headValue = boost::apply_visitor(*this, accessor.head);
		if (auto opt = AsOpt<Identifier>(headValue))
		{
			result.headValue = opt.value();
		}
		else if (auto opt = AsOpt<Record>(headValue))
		{
			result.headValue = opt.value();
		}
		else if (auto opt = AsOpt<List>(headValue))
		{
			result.headValue = opt.value();
		}
		else if (auto opt = AsOpt<FuncVal>(headValue))
		{
			result.headValue = opt.value();
		}
		else
		{
			//エラー：識別子かリテラル以外（評価結果としてオブジェクトを返すような式）へのアクセスには未対応
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
		}
		*/

		Address address;

		/*
		Val headValue = boost::apply_visitor(*this, accessor.head);
		if (auto opt = AsOpt<Address>(headValue))
		{
			address = opt.value();
		}
		else if (auto opt = AsOpt<Record>(headValue))
		{
			address = pEnv->makeTemporaryValue(opt.value());
		}
		else if (auto opt = AsOpt<List>(headValue))
		{
			address = pEnv->makeTemporaryValue(opt.value());
		}
		else if (auto opt = AsOpt<FuncVal>(headValue))
		{
			address = pEnv->makeTemporaryValue(opt.value());
		}
		else
		{
			//エラー：識別子かリテラル以外（評価結果としてオブジェクトを返すような式）へのアクセスには未対応
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
		}
		*/

		LRValue headValue = boost::apply_visitor(*this, accessor.head);
		if (headValue.isLValue())
		{
			address = headValue.address(*pEnv);
		}
		else
		{
			Val evaluated = headValue.evaluated();
			if (auto opt = AsOpt<Record>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<List>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<FuncVal>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else
			{
				CGL_Error("アクセッサのヘッドの評価結果が不正");
			}
		}

		for (const auto& access : accessor.accesses)
		{
			//boost::optional<Val&> objOpt = pEnv->expandOpt(address);
			LRValue lrAddress = LRValue(address);
			boost::optional<Val&> objOpt = pEnv->mutableExpandOpt(lrAddress);
			if (!objOpt)
			{
				CGL_Error("参照エラー");
			}

			Val& objRef = objOpt.value();

			if (auto listAccessOpt = AsOpt<ListAccess>(access))
			{
				Val value = pEnv->expand(boost::apply_visitor(*this, listAccessOpt.value().index));

				if (!IsType<List>(objRef))
				{
					CGL_Error("オブジェクトがリストでない");
				}

				List& list = As<List>(objRef);

				if (auto indexOpt = AsOpt<int>(value))
				{
					const int indexValue = indexOpt.value();
					const int listSize = static_cast<int>(list.data.size());
					const int maxIndex = listSize - 1;

					if (0 <= indexValue && indexValue <= maxIndex)
					{
						address = list.get(indexValue);
					}
					else if (indexValue < 0 || !pEnv->isAutomaticExtendMode())
					{
						CGL_Error("listの範囲外アクセスが発生しました");
					}
					else
					{
						while (static_cast<int>(list.data.size()) - 1 < indexValue)
						{
							list.data.push_back(pEnv->makeTemporaryValue(0));
						}
						address = list.get(indexValue);
					}
				}
				else
				{
					CGL_Error("list[index] の index が int 型でない");
				}
			}
			else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
			{
				if (!IsType<Record>(objRef))
				{
					CGL_Error("オブジェクトがレコードでない");
				}

				const Record& record = As<Record>(objRef);
				auto it = record.values.find(recordAccessOpt.value().name);
				if (it == record.values.end())
				{
					CGL_Error("指定された識別子がレコード中に存在しない");
				}

				address = it->second;
			}
			else
			{
				auto funcAccess = As<FunctionAccess>(access);

				if (!IsType<FuncVal>(objRef))
				{
					CGL_Error("オブジェクトが関数でない");
				}

				const FuncVal& function = As<FuncVal>(objRef);

				/*
				std::vector<Val> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
				}
					
				Expr caller = FunctionCaller(function, args);
				//const Val returnedValue = boost::apply_visitor(*this, caller);
				const Val returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
				address = pEnv->makeTemporaryValue(returnedValue);
				*/

				std::vector<Address> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					const LRValue currentArgument = pEnv->expand(boost::apply_visitor(*this, expr));
					if (currentArgument.isLValue())
					{
						args.push_back(currentArgument.address(*pEnv));
					}
					else
					{
						args.push_back(pEnv->makeTemporaryValue(currentArgument.evaluated()));
					}
				}

				const Val returnedValue = pEnv->expand(callFunction(function, args));
				address = pEnv->makeTemporaryValue(returnedValue);
			}
		}

		return LRValue(address);
	}


	bool HasFreeVariables::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			const Val& val = node.evaluated();
			if (IsType<double>(val) || IsType<int>(val) || IsType<bool>(val))
			{
				return false;
			}

			CGL_Error("不正な値");
		}

		Address address = node.address(*pEnv);
		for (const auto& freeVal : freeVariables)
		{
			if (address == freeVal)
			{
				return true;
			}
		}

		return false;
	}

	bool HasFreeVariables::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		for (const auto& freeVal : freeVariables)
		{
			if (address == freeVal)
			{
				return true;
			}
		}

		return false;
	}

	bool HasFreeVariables::operator()(const UnaryExpr& node)
	{
		return boost::apply_visitor(*this, node.lhs);
	}

	bool HasFreeVariables::operator()(const BinaryExpr& node)
	{
		const bool lhs = boost::apply_visitor(*this, node.lhs);
		if (lhs)
		{
			return true;
		}

		const bool rhs = boost::apply_visitor(*this, node.rhs);
		return rhs;
	}

	bool HasFreeVariables::operator()(const Accessor& node)
	{
		Expr expr = node;
		Eval evaluator(pEnv);
		const LRValue value = boost::apply_visitor(evaluator, expr);

		if (value.isLValue())
		{
			const Address address = value.address(*pEnv);

			for (const auto& freeVal : freeVariables)
			{
				if (address == freeVal)
				{
					return true;
				}
			}

			return false;
		}

		CGL_Error("invalid expression");
		return false;
	}


	boost::optional<size_t> Expr2SatExpr::freeVariableIndex(Address reference)
	{
		for (size_t i = 0; i < freeVariables.size(); ++i)
		{
			if (freeVariables[i] == reference)
			{
				return i;
			}
		}

		return boost::none;
	}

	boost::optional<SatReference> Expr2SatExpr::getSatRef(Address reference)
	{
		for (size_t i = 0; i < refs.size(); ++i)
		{
			if (refs[i] == reference)
			{
				return SatReference(refID_Offset + i);
				//return refs[i];
			}
		}

		return boost::none;
	}

	boost::optional<SatReference> Expr2SatExpr::addSatRef(Address reference)
	{
		if (auto indexOpt = freeVariableIndex(reference))
		{
			//以前に出現して登録済みのfree変数はそのまま返す
			if (auto satRefOpt = getSatRef(reference))
			{
				return satRefOpt;
			}

			//初めて出現したfree変数は登録してから返す
			usedInSat[indexOpt.value()] = 1;
			SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
			refs.push_back(reference);
			return satRef;
		}

		return boost::none;
	}
				
	Expr Expr2SatExpr::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			return node;
		}
		else
		{
			const Address address = node.address(*pEnv);

			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//free変数にあった場合は制約用の参照値を返す
			if (auto satRefOpt = addSatRef(address))
			{
				return satRefOpt.value();
			}
			//free変数になかった場合は評価した結果を返す
			else
			{
				const Val evaluated = pEnv->expand(address);
				return LRValue(evaluated);
			}
		}

		CGL_Error("ここは通らないはず");
		return LRValue(0);
	}

	//ここにIdentifierが残っている時点でClosureMakerにローカル変数だと判定された変数のはず
	Expr Expr2SatExpr::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		if (!address.isValid())
		{
			CGL_Error("識別子が定義されていません");
		}

		//free変数にあった場合は制約用の参照値を返す
		if (auto satRefOpt = addSatRef(address))
		{
			return satRefOpt.value();
		}
		//free変数になかった場合は評価した結果を返す
		else
		{
			const Val evaluated = pEnv->expand(address);
			return LRValue(evaluated);
		}

		CGL_Error("ここは通らないはず");
		return LRValue(0);
	}

	Expr Expr2SatExpr::operator()(const UnaryExpr& node)
	{
		const Expr lhs = boost::apply_visitor(*this, node.lhs);

		switch (node.op)
		{
		case UnaryOp::Not:     return UnaryExpr(lhs, UnaryOp::Not);
		case UnaryOp::Minus:   return UnaryExpr(lhs, UnaryOp::Minus);
		case UnaryOp::Plus:    return lhs;
		case UnaryOp::Dynamic: return lhs;
		}

		CGL_Error("invalid expression");
		return LRValue(0);
	}

	Expr Expr2SatExpr::operator()(const BinaryExpr& node)
	{
		const Expr lhs = boost::apply_visitor(*this, node.lhs);
		const Expr rhs = boost::apply_visitor(*this, node.rhs);

		if (node.op != BinaryOp::Assign)
		{
			return BinaryExpr(lhs, rhs, node.op);
		}

		CGL_Error("invalid expression");
		return LRValue(0);
	}

	Expr Expr2SatExpr::operator()(const Lines& node)
	{
		Lines result;
		for (const auto& expr : node.exprs)
		{
			result.add(boost::apply_visitor(*this, expr));
		}
		return result;
	}

	Expr Expr2SatExpr::operator()(const If& node)
	{
		If result;
		result.cond_expr = boost::apply_visitor(*this, node.cond_expr);
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (node.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
		}

		return result;
	}

	Expr Expr2SatExpr::operator()(const For& node)
	{
		For result;
		result.loopCounter = node.loopCounter;
		result.rangeEnd = node.rangeEnd;
		result.rangeStart = node.rangeStart;
		result.doExpr = boost::apply_visitor(*this, node.doExpr);
		return result;
	}

	Expr Expr2SatExpr::operator()(const ListConstractor& node)
	{
		ListConstractor result;
		for (const auto& expr : node.data)
		{
			result.add(boost::apply_visitor(*this, expr));
		}
		return result;
	}

	Expr Expr2SatExpr::operator()(const RecordConstractor& node)
	{
		RecordConstractor result;
		for (const auto& expr : node.exprs)
		{
			result.add(boost::apply_visitor(*this, expr));
		}
		return result;
	}

	Expr Expr2SatExpr::operator()(const Accessor& node)
	{
		Address headAddress;
		const Expr& head = node.head;

		//headがsat式中のローカル変数
		if (IsType<Identifier>(head))
		{
			Address address = pEnv->findAddress(As<Identifier>(head));
			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//headは必ず Record/List/FuncVal のどれかであり、double型であることはあり得ない。
			//したがって、free変数にあるかどうかは考慮せず（free変数は冗長に指定できるのであったとしても別にエラーではない）、
			//直接Valとして展開する
			//result.head = LRValue(address);
			headAddress = address;
		}
		//headがアドレス値
		else if (IsType<LRValue>(head))
		{
			const LRValue& headAddressValue = As<LRValue>(head);
			if (!headAddressValue.isLValue())
			{
				CGL_Error("sat式中のアクセッサの先頭部が不正な値です");
			}

			const Address address = headAddressValue.address(*pEnv);

			//↑のIdentifierと同様に直接展開する
			//result.head = LRValue(address);
			headAddress = address;
		}
		else
		{
			CGL_Error("sat中のアクセッサの先頭部に単一の識別子以外の式を用いることはできません");
		}

		Eval evaluator(pEnv);

		Accessor result;

		//TODO: アクセッサはfree変数を持たない間、それ自身がfree変数指定されるまでのアドレスを畳み込む
		bool selfDependsOnFreeVariables = false;
		bool childDependsOnFreeVariables = false;
		{
			HasFreeVariables searcher(pEnv, freeVariables);
			const Expr headExpr = LRValue(headAddress);
			selfDependsOnFreeVariables = boost::apply_visitor(searcher, headExpr);
		}

		for (const auto& access : node.accesses)
		{
			const Address lastHeadAddress = headAddress;

			boost::optional<const Val&> objOpt = pEnv->expandOpt(headAddress);
			if (!objOpt)
			{
				CGL_Error("参照エラー");
			}

			const Val& objRef = objOpt.value();

			if (IsType<ListAccess>(access))
			{
				const ListAccess& listAccess = As<ListAccess>(access);

				{
					HasFreeVariables searcher(pEnv, freeVariables);
					childDependsOnFreeVariables = childDependsOnFreeVariables || boost::apply_visitor(searcher, listAccess.index);
				}

				if (childDependsOnFreeVariables)
				{
					Expr accessIndex = boost::apply_visitor(*this, listAccess.index);
					result.add(ListAccess(accessIndex));
				}
				else
				{
					Val value = pEnv->expand(boost::apply_visitor(evaluator, listAccess.index));

					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					const List& list = As<List>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						headAddress = list.get(indexOpt.value());
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

				if (childDependsOnFreeVariables)
				{
					result.add(access);
				}
				else
				{
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
			else
			{
				const FunctionAccess& funcAccess = As<FunctionAccess>(access);

				{
					//TODO: HasFreeVariablesの実装は不完全で、アクセッサが関数呼び出しでさらにその引数がfree変数の場合に対応していない
					//      一度式を評価して、その過程でfree変数で指定したアドレスへのアクセスが発生するかどうかで見るべき
					//      AddressAppearanceCheckerのようなものを作る(Evalの簡易版)
					//inline bool HasFreeVariables::operator()(const Accessor& node)

					HasFreeVariables searcher(pEnv, freeVariables);
					for (const auto& arg : funcAccess.actualArguments)
					{
						childDependsOnFreeVariables = childDependsOnFreeVariables || boost::apply_visitor(searcher, arg);
					}
				}

				if (childDependsOnFreeVariables)
				{
					FunctionAccess resultAccess;
					for (const auto& arg : funcAccess.actualArguments)
					{
						resultAccess.add(boost::apply_visitor(*this, arg));
					}
					result.add(resultAccess);
				}
				else
				{
					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("オブジェクトが関数でない");
					}

					const FuncVal& function = As<FuncVal>(objRef);

					/*
					std::vector<Val> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
					args.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
					}

					Expr caller = FunctionCaller(function, args);
					const Val returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
					headAddress = pEnv->makeTemporaryValue(returnedValue);
					*/
					std::vector<Address> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue lrvalue = boost::apply_visitor(evaluator, expr);
						if (lrvalue.isLValue())
						{
							args.push_back(lrvalue.address(*pEnv));
						}
						else
						{
							args.push_back(pEnv->makeTemporaryValue(lrvalue.evaluated()));
						}
					}

					const Val returnedValue = pEnv->expand(evaluator.callFunction(function, args));
					headAddress = pEnv->makeTemporaryValue(returnedValue);
				}
			}

			if (lastHeadAddress != headAddress && !selfDependsOnFreeVariables)
			{
				HasFreeVariables searcher(pEnv, freeVariables);
				const Expr headExpr = LRValue(headAddress);
				selfDependsOnFreeVariables = boost::apply_visitor(searcher, headExpr);
			}
		}

		/*
		selfDependsOnFreeVariablesとchildDependsOnFreeVariablesが両方true : エラー
		selfDependsOnFreeVariablesがtrue  : アクセッサ本体がfree（アクセッサを評価すると必ず単一のdouble型になる）
		childDependsOnFreeVariablesがtrue : アクセッサの引数がfree（リストインデックスか関数引数）
		*/
		if (selfDependsOnFreeVariables && childDependsOnFreeVariables)
		{
			CGL_Error("sat式中のアクセッサについて、本体と引数の両方をfree変数に指定することはできません");
		}
		else if (selfDependsOnFreeVariables)
		{
			if (!result.accesses.empty())
			{
				CGL_Error("ここは通らないはず");
			}

			if (auto satRefOpt = addSatRef(headAddress))
			{
				return satRefOpt.value();
			}
			else
			{
				CGL_Error("ここは通らないはず");
			}
		}
		/*else if (childDependsOnFreeVariables)
		{
		CGL_Error("TODO: アクセッサの引数のfree変数指定は未対応");
		}*/

		result.head = LRValue(headAddress);

		return result;
	}
	
	Val evalExpr(const Expr& expr)
	{
		auto env = Context::Make();

		Eval evaluator(env);

		const Val result = env->expand(boost::apply_visitor(evaluator, expr));

		//std::cout << "Context:\n";
		//env->printContext();

		//std::cout << "Result Evaluation:\n";
		//printVal(result, env);

		return result;
	}

	bool IsEqualVal(const Val& value1, const Val& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
			if ((IsType<int>(value1) || IsType<double>(value1)) && (IsType<int>(value2) || IsType<double>(value2)))
			{
				const Val d1 = IsType<int>(value1) ? static_cast<double>(As<int>(value1)) : As<double>(value1);
				const Val d2 = IsType<int>(value2) ? static_cast<double>(As<int>(value2)) : As<double>(value2);
				return IsEqualVal(d1, d2);
			}

			std::cerr << "Values are not same type." << std::endl;
			return false;
		}

		if (IsType<bool>(value1))
		{
			return As<bool>(value1) == As<bool>(value2);
		}
		else if (IsType<int>(value1))
		{
			return As<int>(value1) == As<int>(value2);
		}
		else if (IsType<double>(value1))
		{
			const double d1 = As<double>(value1);
			const double d2 = As<double>(value2);
			if (d1 == d2)
			{
				return true;
			}
			else
			{
				std::cerr << "Warning: Comparison of floating point numbers might be incorrect." << std::endl;
				const double eps = 0.001;
				const bool result = std::abs(d1 - d2) < eps;
				std::cerr << std::setprecision(17);
				std::cerr << "    " << d1 << " == " << d2 << " => " << (result ? "Maybe true" : "Maybe false") << std::endl;
				std::cerr << std::setprecision(6);
				return result;
			}
			//return As<double>(value1) == As<double>(value2);
		}
		else if (IsType<CharString>(value1))
		{
			return As<CharString>(value1) == As<CharString>(value2);
		}
		else if (IsType<List>(value1))
		{
			//return As<List>(value1) == As<List>(value2);
			CGL_Error("未対応");
			return false;
		}
		else if (IsType<KeyValue>(value1))
		{
			return As<KeyValue>(value1) == As<KeyValue>(value2);
		}
		else if (IsType<Record>(value1))
		{
			//return As<Record>(value1) == As<Record>(value2);
			CGL_Error("未対応");
			return false;
		}
		else if (IsType<FuncVal>(value1))
		{
			return As<FuncVal>(value1) == As<FuncVal>(value2);
		}
		else if (IsType<Jump>(value1))
		{
			return As<Jump>(value1) == As<Jump>(value2);
		};
		/*else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		};*/

		std::cerr << "IsEqualVal: Type Error" << std::endl;
		return false;
	}

	bool IsEqual(const Expr& value1, const Expr& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
			std::cerr << "Values are not same type." << std::endl;
			return false;
		}
		/*
		if (IsType<bool>(value1))
		{
			return As<bool>(value1) == As<bool>(value2);
		}
		else if (IsType<int>(value1))
		{
			return As<int>(value1) == As<int>(value2);
		}
		else if (IsType<double>(value1))
		{
			const double d1 = As<double>(value1);
			const double d2 = As<double>(value2);
			if (d1 == d2)
			{
				return true;
			}
			else
			{
				std::cerr << "Warning: Comparison of floating point numbers might be incorrect." << std::endl;
				const double eps = 0.001;
				const bool result = abs(d1 - d2) < eps;
				std::cerr << std::setprecision(17);
				std::cerr << "    " << d1 << " == " << d2 << " => " << (result ? "Maybe true" : "Maybe false") << std::endl;
				std::cerr << std::setprecision(6);
				return result;
			}
			//return As<double>(value1) == As<double>(value2);
		}
		*/
		/*if (IsType<RValue>(value1))
		{
			return IsEqualVal(As<RValue>(value1).value, As<RValue>(value2).value);
		}*/
		if (IsType<LRValue>(value1))
		{
			const LRValue& lrvalue1 = As<LRValue>(value1);
			const LRValue& lrvalue2 = As<LRValue>(value2);
			if (lrvalue1.isLValue() && lrvalue2.isLValue())
			{
				return lrvalue1 == lrvalue2;
			}
			else if (lrvalue1.isRValue() && lrvalue2.isRValue())
			{
				return IsEqualVal(lrvalue1.evaluated(), lrvalue2.evaluated());
			}
			std::cerr << "Values are not same type." << std::endl;
			return false;
		}
		else if (IsType<Identifier>(value1))
		{
			return As<Identifier>(value1) == As<Identifier>(value2);
		}
		else if (IsType<SatReference>(value1))
		{
			return As<SatReference>(value1) == As<SatReference>(value2);
		}
		else if (IsType<UnaryExpr>(value1))
		{
			return As<UnaryExpr>(value1) == As<UnaryExpr>(value2);
		}
		else if (IsType<BinaryExpr>(value1))
		{
			return As<BinaryExpr>(value1) == As<BinaryExpr>(value2);
		}
		else if (IsType<DefFunc>(value1))
		{
			return As<DefFunc>(value1) == As<DefFunc>(value2);
		}
		else if (IsType<Range>(value1))
		{
			return As<Range>(value1) == As<Range>(value2);
		}
		else if (IsType<Lines>(value1))
		{
			return As<Lines>(value1) == As<Lines>(value2);
		}
		else if (IsType<If>(value1))
		{
			return As<If>(value1) == As<If>(value2);
		}
		else if (IsType<For>(value1))
		{
			return As<For>(value1) == As<For>(value2);
		}
		else if (IsType<Return>(value1))
		{
			return As<Return>(value1) == As<Return>(value2);
		}
		else if (IsType<ListConstractor>(value1))
		{
			return As<ListConstractor>(value1) == As<ListConstractor>(value2);
		}
		else if (IsType<KeyExpr>(value1))
		{
			return As<KeyExpr>(value1) == As<KeyExpr>(value2);
		}
		else if (IsType<RecordConstractor>(value1))
		{
			return As<RecordConstractor>(value1) == As<RecordConstractor>(value2);
		}
		else if (IsType<RecordInheritor>(value1))
		{
			return As<RecordInheritor>(value1) == As<RecordInheritor>(value2);
		}
		else if (IsType<DeclSat>(value1))
		{
			return As<DeclSat>(value1) == As<DeclSat>(value2);
		}
		else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		}
		else if (IsType<Accessor>(value1))
		{
			return As<Accessor>(value1) == As<Accessor>(value2);
		};

		std::cerr << "IsEqual: Type Error" << std::endl;
		return false;
	}
}
