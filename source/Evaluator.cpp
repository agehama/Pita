#pragma warning(disable:4996)
#include <iostream>

#include <Pita/Evaluator.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/Printer.hpp>
#include <Pita/Geometry.hpp>

extern int constraintViolationCount;

namespace cgl
{
	class ExprLocationInfo : public boost::static_visitor<std::string>
	{
	public:
		ExprLocationInfo() = default;

		std::string operator()(const Lines& node)const
		{
			std::string result = static_cast<const LocationInfo&>(node).getInfo();
			for (const auto& expr : node.exprs)
			{
				result += boost::apply_visitor(*this, expr);
			}
			return result;
		}

		std::string operator()(const UnaryExpr& node)const
		{
			std::string result = static_cast<const LocationInfo&>(node).getInfo();
			result += boost::apply_visitor(*this, node.lhs);
			return result;
		}

		std::string operator()(const BinaryExpr& node)const
		{
			std::string result = static_cast<const LocationInfo&>(node).getInfo();
			result += boost::apply_visitor(*this, node.lhs);
			result += boost::apply_visitor(*this, node.rhs);
			return result;
		}

		std::string operator()(const LRValue& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Identifier& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Import& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Range& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const DefFunc& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const If& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const For& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Return& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const ListConstractor& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const KeyExpr& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const RecordConstractor& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const RecordInheritor& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Accessor& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const DeclSat& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const DeclFree& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
	};

	inline std::string GetLocationInfo(const Expr& expr)
	{
		return boost::apply_visitor(ExprLocationInfo(), expr);
	}

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
				return LRValue(pEnv->cloneReference(node.reference(), replaceMap)).setLocation(node);
			}
			else
			{
				return LRValue(opt.get()).setLocation(node);
			}
		}

		return node;
	}
	
	Expr AddressReplacer::operator()(const UnaryExpr& node)const
	{
		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op).setLocation(node);
	}

	Expr AddressReplacer::operator()(const BinaryExpr& node)const
	{
		return BinaryExpr(
			boost::apply_visitor(*this, node.lhs), 
			boost::apply_visitor(*this, node.rhs), 
			node.op).setLocation(node);
	}

	Expr AddressReplacer::operator()(const Range& node)const
	{
		return Range(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs)).setLocation(node);
	}

	Expr AddressReplacer::operator()(const Lines& node)const
	{
		Lines result;
		for (size_t i = 0; i < node.size(); ++i)
		{
			result.add(boost::apply_visitor(*this, node.exprs[i]));
		}
		return result.setLocation(node);
	}

	Expr AddressReplacer::operator()(const DefFunc& node)const
	{
		return DefFunc(node.arguments, boost::apply_visitor(*this, node.expr)).setLocation(node);
	}

	Expr AddressReplacer::operator()(const If& node)const
	{
		If result(boost::apply_visitor(*this, node.cond_expr));
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (node.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.get());
		}

		return result.setLocation(node);
	}

	Expr AddressReplacer::operator()(const For& node)const
	{
		For result;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
		result.loopCounter = node.loopCounter;
		result.doExpr = boost::apply_visitor(*this, node.doExpr);
		result.asList = node.asList;
		return result.setLocation(node);
	}

	Expr AddressReplacer::operator()(const Return& node)const
	{
		return Return(boost::apply_visitor(*this, node.expr)).setLocation(node);
	}

	Expr AddressReplacer::operator()(const ListConstractor& node)const
	{
		ListConstractor result;
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(*this, expr));
		}
		return result.setLocation(node);
	}

	Expr AddressReplacer::operator()(const KeyExpr& node)const
	{
		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
	}

	Expr AddressReplacer::operator()(const RecordConstractor& node)const
	{
		RecordConstractor result;
		for (const auto& expr : node.exprs)
		{
			result.exprs.push_back(boost::apply_visitor(*this, expr));
		}
		return result.setLocation(node);
	}
		
	Expr AddressReplacer::operator()(const RecordInheritor& node)const
	{
		RecordInheritor result;
		result.original = boost::apply_visitor(*this, node.original);

		Expr originalAdder = node.adder;
		Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
		if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
		{
			result.adder = opt.get();
			return result.setLocation(node);
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
				result.add(ListAccess(boost::apply_visitor(*this, listAccess.get().index)));
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
			else
			{
				CGL_Error("aaa");
			}
		}

		return result.setLocation(node);
	}

	Expr AddressReplacer::operator()(const DeclSat& node)const
	{
		return DeclSat(boost::apply_visitor(*this, node.expr)).setLocation(node);
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
		return result.setLocation(node);
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
				result.append(opt.get());
			}
			else
			{
				const Val& substance = pEnv->expand(data[i], info);
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
				result.append(value.first, opt.get());
			}
			else
			{
				const Val& substance = pEnv->expand(value.second, info);
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
		result.constraint = node.constraint;
		result.isSatisfied = node.isSatisfied;

		//result.unitConstraints = node.unitConstraints;
		//result.variableAppearances = node.variableAppearances;
		////result.constraintGroups = node.constraintGroups;
		//result.groupConstraints = node.groupConstraints;
		result.original = node.original;

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
			const Val& substance = pEnv->expand(data[i], info);
			pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(data[i], boost::apply_visitor(*this, substance));
		}

		return node;
	}
		
	Val ValueCloner2::operator()(const Record& node)
	{
		for (const auto& value : node.values)
		{
			//ValueCloner1でクローンは既に作ったので、そのクローンを直接書き換える
			const Val& substance = pEnv->expand(value.second, info);
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
			node.constraint = boost::apply_visitor(replacer, node.constraint.get());
		}

		for (auto& problem : node.problems)
		{
			if (problem.expr)
			{
				problem.expr = boost::apply_visitor(replacer, problem.expr.get());
			}

			for (size_t i = 0; i < problem.refs.size(); ++i)
			{
				const Address oldAddress = problem.refs[i];
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					problem.refs[i] = newAddress;
					problem.invRefs[newAddress] = problem.invRefs[oldAddress];
					problem.invRefs.erase(oldAddress);
				}
			}

			for (size_t i = 0; i < problem.freeVariableRefs.size(); ++i)
			{
				auto& currentElem = problem.freeVariableRefs[i];
				const Address oldAddress = currentElem.first;
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					currentElem.first = newAddress;
				}
			}
		}

		auto& original = node.original;

		for (auto& currentElem : original.freeVariableAddresses)
		{
			const Address oldAddress = currentElem.first;
			if (auto newAddressOpt = getOpt(oldAddress))
			{
				const Address newAddress = newAddressOpt.get();
				currentElem.first = newAddress;
			}
		}

		for (auto& unitConstraint : original.unitConstraints)
		{
			unitConstraint = boost::apply_visitor(replacer, unitConstraint);
		}

		if (!original.variableAppearances.empty())
		{
			std::vector<ConstraintAppearance> newAppearance;

			for (auto& appearance : original.variableAppearances)
			{
				ConstraintAppearance currentAppearance;
				for (const Address address : appearance)
				{
					if (auto newAddressOpt = getOpt(address))
					{
						currentAppearance.insert(newAddressOpt.get());
					}
					else
					{
						currentAppearance.insert(address);
					}
				}
				newAppearance.push_back(currentAppearance);
			}

			original.variableAppearances = newAppearance;
		}

		for (auto& problem : original.groupConstraints)
		{
			if (problem.expr)
			{
				problem.expr = boost::apply_visitor(replacer, problem.expr.get());
			}

			for (size_t i = 0; i < problem.refs.size(); ++i)
			{
				const Address oldAddress = problem.refs[i];
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					problem.refs[i] = newAddress;
					problem.invRefs[newAddress] = problem.invRefs[oldAddress];
					problem.invRefs.erase(oldAddress);
				}
			}

			for (size_t i = 0; i < problem.freeVariableRefs.size(); ++i)
			{
				auto& currentElem = problem.freeVariableRefs[i];
				const Address oldAddress = currentElem.first;
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					currentElem.first = newAddress;
				}
			}
		}

		//constraintGroupsはただの数字だからぞのままでよい

		for (auto& freeVariable : node.freeVariables)
		{
			Expr expr = freeVariable;
			freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
		}
		
		/*
		auto& problem = node.problem;
		if (problem.candidateExpr)
		{
			problem.candidateExpr = boost::apply_visitor(replacer, problem.candidateExpr.get());
		}

		for (size_t i = 0; i < problem.refs.size(); ++i)
		{
			const Address oldAddress = problem.refs[i];
			if (auto newAddressOpt = getOpt(oldAddress))
			{
				const Address newAddress = newAddressOpt.get();
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

	Val Clone(std::shared_ptr<Context> pEnv, const Val& value, const LocationInfo& info)
	{
		/*
		関数値がアドレスを内部に持っている時、クローン作成の前後でその依存関係を保存する必要があるので、クローン作成は2ステップに分けて行う。
		1. リスト・レコードの再帰的なコピー
		2. 関数の持つアドレスを新しい方に付け替える		
		*/
		ValueCloner cloner(pEnv, info);
		const Val& evaluated = boost::apply_visitor(cloner, value);
		ValueCloner2 cloner2(pEnv, cloner.replaceMap, info);
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
				const Val& evaluated = pEnv->expand(address, node);
				if (auto opt = AsOpt<Record>(evaluated))
				{
					const Record& record = opt.get();
					
					for (const auto& keyval : record.values)
					{
						addLocalVariable(keyval.first);
					}
				}
			}

			return LRValue(address).setLocation(node);
		}

		CGL_ErrorNode(node, msgs::Undefined(node));
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
				return LRValue(pEnv->bindReference(As<Identifier>(node.lhs))).setLocation(node);
			}
			else if (IsType<Accessor>(node.lhs))
			{
				return LRValue(pEnv->bindReference(As<Accessor>(node.lhs), node)).setLocation(node);
			}

			CGL_ErrorNode(node, "参照演算子\"@\"の右辺には識別子かアクセッサしか用いることができません。");
		}
		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op).setLocation(node);
	}

	Expr ClosureMaker::operator()(const BinaryExpr& node)
	{
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
				const Address address = pEnv->findAddress(identifier);

				//ローカル変数に無く、スコープにあれば、アドレスに置き換える
				if (address.isValid())
				{
					//TODO: 制約式の場合は、ここではじく必要がある
					return BinaryExpr(LRValue(address), rhs, node.op).setLocation(node);
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

		CGL_ErrorNode(node, "二項演算子\"=\"の左辺は単一の左辺値でなければなりません。");
		return LRValue(0);
	}

	Expr ClosureMaker::operator()(const Range& node)
	{
		return Range(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs)).setLocation(node);
	}

	Expr ClosureMaker::operator()(const Lines& node)
	{
		Lines result;
		for (size_t i = 0; i < node.size(); ++i)
		{
			result.add(boost::apply_visitor(*this, node.exprs[i]));
		}
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const DefFunc& node)
	{
		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, true);
		for (const auto& argument : node.arguments)
		{
			child.addLocalVariable(argument);
		}

		return DefFunc(node.arguments, boost::apply_visitor(child, node.expr)).setLocation(node);
	}

	Expr ClosureMaker::operator()(const If& node)
	{
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
		For result;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, isInnerClosure);
		child.addLocalVariable(node.loopCounter);
		result.doExpr = boost::apply_visitor(child, node.doExpr);
		result.loopCounter = node.loopCounter;
		result.asList = node.asList;

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const Return& node)
	{
		return Return(boost::apply_visitor(*this, node.expr)).setLocation(node);
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
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const KeyExpr& node)
	{
		//変数宣言式
		//再代入の可能性もあるがどっちにしろこれ以降この識別子はローカル変数と扱ってよい
		addLocalVariable(node.name);

		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
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

		return result.setLocation(node);
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
			result.adder = opt.get();
			return result.setLocation(node);
		}

		CGL_ErrorNodeInternal(node, "node.adderの評価結果がRecordConstractorでありませんでした。");
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
			else
			{
				CGL_ErrorNodeInternal(node, "アクセッサの評価結果が不正です。");
			}
		}

		return result.setLocation(node);
	}
		
	Expr ClosureMaker::operator()(const DeclSat& node)
	{
		DeclSat result;
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
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
				CGL_ErrorNodeInternal(node, "アクセッサの評価結果が不正です。");
			}
			result.addAccessor(As<Accessor>(closedAccessor));
		}
		for (const auto& range : node.ranges)
		{
			const Expr closedRange = boost::apply_visitor(*this, range);
			result.addRange(closedRange);
		}
		return result.setLocation(node);
	}

	LRValue Eval::operator()(const Identifier& node)
	{
		const Address address = pEnv->findAddress(node);
		if (address.isValid())
		{
			return LRValue(address);
		}

		CGL_ErrorNode(node, msgs::Undefined(node));
	}

	LRValue Eval::operator()(const Import& node)
	{
		return node.eval(pEnv);
	}

	LRValue Eval::operator()(const UnaryExpr& node)
	{
		const Val lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs), node);

		switch (node.op)
		{
		case UnaryOp::Not:     return RValue(Not(lhs, *pEnv));
		case UnaryOp::Minus:   return RValue(Minus(lhs, *pEnv));
		case UnaryOp::Plus:    return RValue(Plus(lhs, *pEnv));
		case UnaryOp::Dynamic: return RValue(lhs);
		}

		CGL_ErrorNodeInternal(node, "不明な単項演算子です。");
		return RValue(0);
	}

	LRValue Eval::operator()(const BinaryExpr& node)
	{
		const LRValue rhs_ = boost::apply_visitor(*this, node.rhs);

		Val lhs;
		if (node.op != BinaryOp::Assign)
		{
			const LRValue lhs_ = boost::apply_visitor(*this, node.lhs);
			lhs = pEnv->expand(lhs_, node);
		}

		Val rhs = pEnv->expand(rhs_, node);

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
				CGL_ErrorNodeInternal(node, "一時オブジェクトへの代入はできません。");
			}
			else if (auto valOpt = AsOpt<Identifier>(node.lhs))
			{
				const Identifier& identifier = valOpt.get();

				const Address address = pEnv->findAddress(identifier);
				//変数が存在する：代入式
				if (address.isValid())
				{
					//CGL_DebugLog("代入式");
					//pEnv->assignToObject(address, rhs);
					pEnv->bindValueID(identifier, pEnv->makeTemporaryValue(rhs));
				}
				//変数が存在しない：変数宣言式
				else
				{
					//CGL_DebugLog("変数宣言式");
					pEnv->bindNewValue(identifier, rhs);
					//CGL_DebugLog("");
				}

				return RValue(rhs);
			}
			else if (auto accessorOpt = AsOpt<Accessor>(node.lhs))
			{
				const LRValue lhs_ = boost::apply_visitor(*this, node.lhs);
				if (lhs_.isLValue())
				{
					if (lhs_.isValid())
					{
						pEnv->assignToAccessor(accessorOpt.get(), rhs_, node);
						return RValue(rhs);
					}
					else
					{
						CGL_ErrorNodeInternal(node, "アクセッサの評価結果が無効なアドレス値です。");
					}
				}
				else
				{
					CGL_ErrorNodeInternal(node, "アクセッサの評価結果が無効な値です。");
				}
			}
		}
		}

		CGL_ErrorNodeInternal(node, "不明な二項演算子です。");
		return RValue(0);
	}

	LRValue Eval::operator()(const DefFunc& defFunc)
	{
		return pEnv->makeFuncVal(pEnv, defFunc.arguments, defFunc.expr);
	}

	LRValue Eval::callFunction(const LocationInfo& info, const FuncVal& funcVal, const std::vector<Address>& expandedArguments)
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
			funcVal = opt.get();
		}
		else
		{
			const Address funcAddress = pEnv->findAddress(As<Identifier>(callFunc.funcRef));
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
		}
		*/

		/*if (funcVal.builtinFuncAddress)
		{
			return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.get(), expandedArguments));
		}*/
		if (funcVal.builtinFuncAddress)
		{
			return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.get(), expandedArguments, info));
		}

		CGL_DebugLog("");

		//if (funcVal.arguments.size() != callFunc.actualArguments.size())
		if (funcVal.arguments.size() != expandedArguments.size())
		{
			CGL_ErrorNode(info, "仮引数の数と実引数の数が合っていません。");
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
			result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr), info);
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
					return RValue(jump.lhs.get());
				}
				else
				{
					CGL_ErrorNode(info, "return式の右辺が無効な値です。");
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
			result = pEnv->expand(boost::apply_visitor(*this, expr), statement);
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
			if (IsType<unsigned>(refOpt.get().headValue))
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
			//result = pEnv->dereference(refOpt.get());
			//result = pEnv->expandRef(refOpt.get());
			result = pEnv->expand(refOpt.get());
		}
		*/

		CGL_DebugLog("");

		pEnv->exitScope();

		CGL_DebugLog("");
		return LRValue(result);
	}

	LRValue Eval::operator()(const If& if_statement)
	{
		const Val cond = pEnv->expand(boost::apply_visitor(*this, if_statement.cond_expr), if_statement);
		if (!IsType<bool>(cond))
		{
			CGL_ErrorNode(if_statement, "条件式の評価結果がブール値ではありませんでした。");
		}

		if (As<bool>(cond))
		{
			const Val result = pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr), if_statement);
			return RValue(result);

		}
		else if (if_statement.else_expr)
		{
			const Val result = pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.get()), if_statement);
			return RValue(result);
		}

		//else式が無いケースで cond = False であったら一応警告を出す
		//std::cerr << "Warning(" << __LINE__ << ")\n";
		CGL_WarnLog("else式が無いケースで cond = False であった");
		return RValue(0);
	}

	LRValue Eval::operator()(const For& forExpression)
	{
		const Val startVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeStart), forExpression);
		const Val endVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeEnd), forExpression);

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
				CGL_ErrorNode(forExpression, "ループ範囲式の評価結果が数値ではありませんでした。");
			}

			const bool result_IsDouble = a_IsDouble || b_IsDouble;

			const bool isInOrder = LessEqual(a, b, *pEnv);

			const int sign = isInOrder ? 1 : -1;

			if (result_IsDouble)
			{
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
				CGL_ErrorNodeInternal(forExpression, "loopCountの評価結果がブール値ではありませんでした。");
			}

			return As<bool>(result) == isInOrder;
		};

		const auto stepOrder = calcStepValue(startVal, endVal);
		if (!stepOrder)
		{
			CGL_ErrorNodeInternal(forExpression, "ループ範囲式の評価結果が不正な値です。");
		}

		const Val step = stepOrder.get().first;
		const bool isInOrder = stepOrder.get().second;

		Val loopCountValue = startVal;

		Val loopResult;

		pEnv->enterScope();

		PackedList loopResults;
		while (true)
		{
			const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
			if (!isLoopContinuesOpt)
			{
				CGL_ErrorNodeInternal(forExpression, "loopContinuesの評価結果が不正な値です。");
			}

			//ループの継続条件を満たさなかったので抜ける
			if (!isLoopContinuesOpt.get())
			{
				break;
			}

			pEnv->bindNewValue(forExpression.loopCounter, loopCountValue);

			loopResult = pEnv->expand(boost::apply_visitor(*this, forExpression.doExpr), forExpression);

			if (forExpression.asList)
			{
				loopResults.add(Packed(loopResult, *pEnv));
			}

			//ループカウンタの更新
			loopCountValue = Add(loopCountValue, step, *pEnv);
		}

		pEnv->exitScope();

		if (forExpression.asList)
		{
			return RValue(Unpacked(loopResults, *pEnv));
		}

		return RValue(loopResult);
	}

	LRValue Eval::operator()(const Return& return_statement)
	{
		const Val lhs = pEnv->expand(boost::apply_visitor(*this, return_statement.expr), return_statement);

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

	LRValue Eval::operator()(const KeyExpr& node)
	{
		/*
		const LRValue rhs_ = boost::apply_visitor(*this, node.expr);
		Val rhs = pEnv->expand(rhs_, node);
		if (pEnv->existsInCurrentScope(node.name))
		{
			CGL_ErrorNode(node, "宣言演算子\":\"による変数への値の再代入は行えません。代わりに代入演算子\"=\"を使用してください。");
		}
		else
		{
			pEnv->bindNewValue(node.name, rhs);
		}

		return RValue(rhs);
		*/

		const LRValue rhs_ = boost::apply_visitor(*this, node.expr);
		Val rhs = pEnv->expand(rhs_, node);
		if (pEnv->existsInCurrentScope(node.name))
		{
			pEnv->bindValueID(node.name, pEnv->makeTemporaryValue(rhs));
		}
		else
		{
			pEnv->bindNewValue(node.name, rhs);
		}

		return RValue(rhs);
	}

	LRValue Eval::operator()(const RecordConstractor& recordConsractor)
	{
		//CGL_DebugLog("");
		//pEnv->enterScope();
		//CGL_DebugLog("");

		std::vector<Identifier> keyList;
		/*
		レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
		レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアスとして定義（スコープを抜けたら元に戻る≒遮蔽）
		*/

		/*for (size_t i = 0; i < recordConsractor.exprs.size(); ++i)
		{
			CGL_DebugLog(std::string("RecordExpr(") + ToS(i) + "): ");
			printExpr(recordConsractor.exprs[i]);
		}*/

		Record newRecord;

		const bool isNewScope = !static_cast<bool>(pEnv->temporaryRecord);

		if (pEnv->temporaryRecord)
		{
			//pEnv->currentRecords.push(pEnv->temporaryRecord.get());
			pEnv->currentRecords.push_back(pEnv->temporaryRecord.get());
			pEnv->temporaryRecord = boost::none;
		}
		else
		{
			//現在のレコードが継承でない場合のみスコープを作る
			pEnv->enterScope();
			pEnv->currentRecords.push_back(std::ref(newRecord));
		}

		//Record& record = pEnv->currentRecords.top();
		Record& record = pEnv->currentRecords.back();

		int i = 0;

		for (const auto& expr : recordConsractor.exprs)
		{
			/*
			CGL_DebugLog("");
			CGL_DebugLog("Evaluate: ");
			Val value = pEnv->expand(boost::apply_visitor(*this, expr));
			CGL_DebugLog("Result: ");
			printVal(value, pEnv);

			//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
			if (auto keyValOpt = AsOpt<KeyValue>(value))
			{
				const auto keyVal = keyValOpt.get();
				keyList.push_back(keyVal.name);

				CGL_DebugLog(std::string("assign to ") + static_cast<std::string>(keyVal.name));

				pEnv->bindNewValue(keyVal.name, keyVal.value);

				CGL_DebugLog("");
			}
			*/

			if (IsType<KeyExpr>(expr))
			{
				keyList.push_back(As<KeyExpr>(expr).name);
			}

			Val value = pEnv->expand(boost::apply_visitor(*this, expr), recordConsractor);

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

		//各free変数の範囲をまとめたレコードを作成する
		const auto makePackedRanges = [&](std::shared_ptr<Context> pContext, const std::vector<Expr>& ranges)->std::vector<PackedVal>
		{
			Eval evaluator(pContext);
			std::vector<PackedVal> packedRanges;
			for (const auto& rangeExpr : ranges)
			{
				packedRanges.push_back(Packed(pContext->expand(boost::apply_visitor(evaluator, rangeExpr), recordConsractor), *pContext));
			}
			return packedRanges;
		};

		//declfreeで指定されたアクセッサを全て展開して辿れるアドレスを返す（範囲指定なし）
		const auto makeFreeVariableAddresses = [&](std::shared_ptr<Context> pContext, const Record& record)
			->std::vector<std::pair<Address, VariableRange>>
		{
			std::vector<std::pair<Address, VariableRange>> freeVariableAddresses;
			for (size_t i = 0; i < record.freeVariables.size(); ++i)
			{
				const auto& accessor = record.freeVariables[i];
				const auto addresses = pContext->expandReferences2(accessor, boost::none, recordConsractor);

				freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
			}
			return freeVariableAddresses;
		};

		//declfreeで指定されたアクセッサを全て展開して辿れるアドレスを返す（範囲指定あり）
		const auto makeFreeVariableAddressesRange = [&](std::shared_ptr<Context> pContext, const Record& record, const std::vector<PackedVal>& packedRanges)
			->std::vector<std::pair<Address, VariableRange>>
		{
			std::vector<std::pair<Address, VariableRange>> freeVariableAddresses;
			for (size_t i = 0; i < record.freeVariables.size(); ++i)
			{
				const auto& accessor = record.freeVariables[i];
				const auto addresses = pContext->expandReferences2(accessor, packedRanges[i], recordConsractor);

				freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
			}
			return freeVariableAddresses;
		};

		//論理積で繋がれた制約をリストに分解して返す
		const auto separateUnitConstraints = [](const Expr& constraint)->std::vector<Expr>
		{
			ConjunctionSeparater separater;
			boost::apply_visitor(separater, constraint);
			return separater.conjunctions;
		};

		//それぞれのunitConstraintについて、出現するアドレスの集合を求めたものを返す
		const auto searchFreeVariablesOfConstraint = [](std::shared_ptr<Context> pContext, const Expr& constraint,
			const std::vector<std::pair<Address, VariableRange>>& freeVariableAddresses)->ConstraintAppearance
		{
			//制約ID -> ConstraintAppearance
			ConstraintAppearance appearingList;

			std::vector<char> usedInSat(freeVariableAddresses.size(), 0);
			std::vector<Address> refs;
			std::unordered_map<Address, int> invRefs;
			bool hasPlateausFunction = false;

			SatVariableBinder binder(pContext, freeVariableAddresses, usedInSat, refs, appearingList, invRefs, hasPlateausFunction);
			boost::apply_visitor(binder, constraint);

			return appearingList;
		};

		//二つのAddress集合が共通する要素を持っているかどうかを返す
		const auto intersects = [](const ConstraintAppearance& addressesA, const ConstraintAppearance& addressesB)
		{
			if (addressesA.empty() || addressesB.empty())
			{
				return false;
			}

			for (const Address address : addressesA)
			{
				if (addressesB.find(address) != addressesB.end())
				{
					return true;
				}
			}
			return false;
		};

		//ある制約の変数であるAddress集合が、ある制約グループの変数であるAddress集合と共通するAddressを持っているかどうかを返す
		const auto intersectsToConstraintGroup = [&intersects](const ConstraintAppearance& addressesA, const ConstraintGroup& targetGroup, 
			const std::vector<ConstraintAppearance>& variableAppearances)
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

		//各制約に出現するAddressの集合をまとめ、独立な制約の組にグループ分けする
		//variableAppearances = [{a, b, c}, {a, d}, {e, f}, {g, h}]
		//constraintsGroups(variableAppearances) = [{0, 1}, {2}, {3}]
		const auto addConstraintsGroups = [&intersectsToConstraintGroup](std::vector<ConstraintGroup>& constraintsGroups,
			size_t unitConstraintID, const ConstraintAppearance& unitConstraintAppearance, const std::vector<ConstraintAppearance>& variableAppearances)
		{
			std::set<size_t> currentIntersectsGroupIDs;
			for (size_t constraintGroupID = 0; constraintGroupID < constraintsGroups.size(); ++constraintGroupID)
			{
				if (intersectsToConstraintGroup(unitConstraintAppearance, constraintsGroups[constraintGroupID], variableAppearances))
				{
					currentIntersectsGroupIDs.insert(constraintGroupID);
				}
			}

			if (currentIntersectsGroupIDs.empty())
			{
				ConstraintGroup newGroup;
				newGroup.insert(unitConstraintID);
				constraintsGroups.push_back(newGroup);
			}
			else
			{
				ConstraintGroup newGroup;
				newGroup.insert(unitConstraintID);

				for (const size_t groupID : currentIntersectsGroupIDs)
				{
					const auto& targetGroup = constraintsGroups[groupID];
					for (const size_t targetConstraintID : targetGroup)
					{
						newGroup.insert(targetConstraintID);
					}
				}

				for (auto it = currentIntersectsGroupIDs.rbegin(); it != currentIntersectsGroupIDs.rend(); ++it)
				{
					const size_t existingGroupIndex = *it;
					constraintsGroups.erase(constraintsGroups.begin() + existingGroupIndex);
				}

				constraintsGroups.push_back(newGroup);
			}
		};

		const auto mergeConstraintAppearance = [](ConstraintAppearance& a, const ConstraintAppearance& b)
		{
			for (const Address address : b)
			{
				a.insert(address);
			}
		};

		const auto readResult = [](std::shared_ptr<Context> pContext, const std::vector<double>& resultxs, const OptimizationProblemSat& problem)
		{
			for (size_t i = 0; i < resultxs.size(); ++i)
			{
				Address address = problem.freeVariableRefs[i].first;
				const auto range = problem.freeVariableRefs[i].second;
				//std::cout << "Address(" << address.toString() << "): [" << range.minimum << ", " << range.maximum << "]\n";
				std::cout << "Address(" << address.toString() << "): " << resultxs[i] << "\n";
				pContext->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, resultxs[i]);
				//pEnv->assignToObject(address, (resultxs[i] - 0.5)*2000.0);
			}
		};

		const auto checkSatisfied = [&](std::shared_ptr<Context> pContext, const OptimizationProblemSat& problem)
		{
			if (problem.expr)
			{
				Eval evaluator(pContext);
				const Val result = pContext->expand(boost::apply_visitor(evaluator, problem.expr.get()), recordConsractor);
				if (IsType<bool>(result))
				{
					return As<bool>(result);
				}
				else if (IsNum(result))
				{
					return Equal(AsDouble(result), 0.0, *pEnv);
				}
				else
				{
					CGL_ErrorNode(recordConsractor, "制約式の評価結果が不正な値です。");
				}

				const bool currentConstraintIsSatisfied = As<bool>(result);
				return currentConstraintIsSatisfied;
			}

			return true;
		};

		auto& original = record.original;

		//TODO: record.constraintには継承時のoriginalが持つ制約は含まれていないものとする
		if (record.constraint || !original.unitConstraints.empty())
		{
			//std::cout << "Solve Record Constraints:" << std::endl;
			record.problems.clear();

			//std::cout << "  1. Address expansion" << std::endl;
			///////////////////////////////////
			//1. free変数に指定されたアドレスの展開

			const auto& ranges = record.freeRanges;
			const bool hasRange = !ranges.empty();

			const std::vector<PackedVal> adderPackedRanges = makePackedRanges(pEnv, ranges);

			//変数ID->アドレス
			std::vector<FreeVariable> adderFreeVariableAddresses_ = (hasRange
				? makeFreeVariableAddressesRange(pEnv, record, adderPackedRanges)
				: makeFreeVariableAddresses(pEnv, record));

			//TODO:freeVariableAddressesの重複を削除するべき？
			/*
			std::cout << "freeVariableAddresses before unique: " << freeVariableAddresses.size() << std::endl;

			freeVariableAddresses.erase(std::unique(freeVariableAddresses.begin(), freeVariableAddresses.end(), 
				[](const FreeVariable& a, const FreeVariable& b) {return a.first == b.first; }), freeVariableAddresses.end());

			std::cout << "freeVariableAddresses after  unique: " << freeVariableAddresses.size() << std::endl;
			*/
			
			std::vector<FreeVariable> mergedFreeVariableAddresses = original.freeVariableAddresses;
			mergedFreeVariableAddresses.insert(mergedFreeVariableAddresses.end(), adderFreeVariableAddresses_.begin(), adderFreeVariableAddresses_.end());

			//std::cout << "  2. Constraints separation" << std::endl;
			///////////////////////////////////
			//2. 変数の依存関係を見て独立した制約を分解

			//分解された単位制約
			const std::vector<Expr> adderUnitConstraints = record.constraint ? separateUnitConstraints(record.constraint.get()) : std::vector<Expr>();

			//単位制約ごとの依存するfree変数の集合
			std::vector<ConstraintAppearance> adderVariableAppearances;
			for (const auto& constraint : adderUnitConstraints)
			{
				adderVariableAppearances.push_back(searchFreeVariablesOfConstraint(pEnv, constraint, mergedFreeVariableAddresses));

				/*std::cout << "constraint:\n";
				printExpr(constraint, pEnv, std::cout);
				std::stringstream ss;
				for (const Address address: adderVariableAppearances.back())
				{
					ss << "Address(" << address.toString() << "), ";
				}
				std::cout << ss.str() << "\n";*/
			}

			std::cout << "1 mergedFreeVariableAddresses.size(): " << mergedFreeVariableAddresses.size() << "\n";

			//現在のレコードが継承前の制約を持っているならば、制約が独立かどうかを判定して必要ならば合成を行う
			{
				//std::cout << "  3. Dependency analysis" << std::endl;

				//std::vector<Address> addressesOriginal;
				//std::vector<Address> addressesAdder;

				ConstraintAppearance wholeAddressesOriginal, wholeAddressesAdder;
				for (const auto& appearance : original.variableAppearances)
				{
					mergeConstraintAppearance(wholeAddressesOriginal, appearance);
				}
				for (const auto& appearance : adderVariableAppearances)
				{
					mergeConstraintAppearance(wholeAddressesAdder, appearance);
				}

				const size_t adderOffset = original.unitConstraints.size();

				std::vector<Expr> mergedUnitConstraints = original.unitConstraints;
				mergedUnitConstraints.insert(mergedUnitConstraints.end(), adderUnitConstraints.begin(), adderUnitConstraints.end());

				std::vector<ConstraintAppearance> mergedVariableAppearances = original.variableAppearances;
				mergedVariableAppearances.insert(mergedVariableAppearances.end(), adderVariableAppearances.begin(), adderVariableAppearances.end());

				/*{
					for (const Address address : wholeAddressesOriginal)
					{
						addressesOriginal.push_back(address);
					}
					for (const Address address : wholeAddressesAdder)
					{
						addressesAdder.push_back(address);
					}
					std::sort(addressesOriginal.begin(), addressesOriginal.end());
					std::sort(addressesAdder.begin(), addressesAdder.end());
				}

				{
					std::stringstream ss;
					for (const Address address : addressesOriginal)
					{
						ss << address.toString() << ", ";
					}
					std::cout << "original: ";
					std::cout << ss.str() << "\n";
				}
				{
					std::stringstream ss;
					for (const Address address : addressesAdder)
					{
						ss << address.toString() << ", ";
					}
					std::cout << "adder   : ";
					std::cout << ss.str() << "\n";
				}*/

				//ケースB: 独立でない
				if (intersects(wholeAddressesAdder, wholeAddressesOriginal))
				{
					std::cout << "Case B" << std::endl;

					std::vector<ConstraintGroup> mergedConstraintGroups = original.constraintGroups;
					for (size_t adderConstraintID = 0; adderConstraintID < adderUnitConstraints.size(); ++adderConstraintID)
					{
						addConstraintsGroups(mergedConstraintGroups, adderOffset + adderConstraintID, adderVariableAppearances[adderConstraintID], mergedVariableAppearances);
					}

					std::cout << "Record constraint separated to " << std::to_string(mergedConstraintGroups.size()) << " independent constraints" << std::endl;

					/*if (mergedConstraintGroups.size() == 1)
					{
						std::cout << "group   : ";
						const auto& group = mergedConstraintGroups.front();
						for (const auto val : group)
						{
							std::cout << val << ", ";
						}
						std::cout << std::endl;
					}*/

					original.freeVariableAddresses = mergedFreeVariableAddresses;
					original.unitConstraints = mergedUnitConstraints;
					original.variableAppearances = mergedVariableAppearances;
					original.constraintGroups = mergedConstraintGroups;
					
					original.groupConstraints.clear();

					record.problems.resize(mergedConstraintGroups.size());
					for (size_t constraintGroupID = 0; constraintGroupID < mergedConstraintGroups.size(); ++constraintGroupID)
					{
						auto& currentProblem = record.problems[constraintGroupID];
						const auto& currentConstraintIDs = mergedConstraintGroups[constraintGroupID];

						for (size_t constraintID : currentConstraintIDs)
						{
							//currentProblem.addUnitConstraint(adderUnitConstraints[constraintID]);
							currentProblem.addUnitConstraint(mergedUnitConstraints[constraintID]);
						}

						currentProblem.freeVariableRefs = mergedFreeVariableAddresses;

						//std::cout << "Current constraint freeVariablesSize: " << std::to_string(currentProblem.freeVariableRefs.size()) << std::endl;
						//std::cout << "2 mergedFreeVariableAddresses.size(): " << mergedFreeVariableAddresses.size() << "\n";
						std::vector<double> resultxs = currentProblem.solve(pEnv, recordConsractor, record, keyList);

						readResult(pEnv, resultxs, currentProblem);

						const bool currentConstraintIsSatisfied = checkSatisfied(pEnv, currentProblem);
						record.isSatisfied = record.isSatisfied && currentConstraintIsSatisfied;
						if (!currentConstraintIsSatisfied)
						{
							++constraintViolationCount;
						}

						original.groupConstraints.push_back(currentProblem);
					}
				}
				//ケースA: 独立 or originalのみ or adderのみ
				else
				{
					std::cout << "Case A" << std::endl;
					//独立な場合は、まずoriginalの制約がadderの評価によって満たされなくなっていないかを確認する
					//満たされなくなっていた制約は解きなおす
					for (auto& oldConstraint : original.groupConstraints)
					{
						const Val result = pEnv->expand(boost::apply_visitor(*this, oldConstraint.expr.get()), recordConsractor);
						/*if (!IsType<bool>(result))
						{
							CGL_ErrorNode(recordConsractor, "制約式の評価結果がブール値ではありませんでした。");
						}*/

						//const bool currentConstraintIsSatisfied = As<bool>(result);

						bool currentConstraintIsSatisfied = false;
						if (IsType<bool>(result))
						{
							currentConstraintIsSatisfied = As<bool>(result);
						}
						else if (IsNum(result))
						{
							currentConstraintIsSatisfied = Equal(AsDouble(result), 0.0, *pEnv);
						}
						else
						{
							CGL_ErrorNode(recordConsractor, "制約式の評価結果が不正な値です。");
						}

						if (!currentConstraintIsSatisfied)
						{
							//const auto& problemRefs = currentProblem.refs;

							//currentProblem.freeVariableRefs = freeVariableAddresses;

							oldConstraint.freeVariableRefs = original.freeVariableAddresses;

							//TODO: SatVariableBinderをやり直す必要まではない？
							//クローンでずれたアドレスを張り替えるだけで十分かもしれない？
							//oldConstraint.constructConstraint(pEnv);

							//std::cout << "Current constraint freeVariablesSize: " << std::to_string(oldConstraint.freeVariableRefs.size()) << std::endl;

							std::vector<double> resultxs = oldConstraint.solve(pEnv, recordConsractor, record, keyList);

							readResult(pEnv, resultxs, oldConstraint);

							const bool currentConstraintIsSatisfied = checkSatisfied(pEnv, oldConstraint);
							record.isSatisfied = record.isSatisfied && currentConstraintIsSatisfied;
							if (!currentConstraintIsSatisfied)
							{
								++constraintViolationCount;
							}
						}
					}

					//次に、新たに追加される制約について解く

					//同じfree変数への依存性を持つ単位制約の組
					std::vector<ConstraintGroup> constraintGroups;
					for (size_t constraintID = 0; constraintID < adderUnitConstraints.size(); ++constraintID)
					{
						addConstraintsGroups(constraintGroups, constraintID, adderVariableAppearances[constraintID], adderVariableAppearances);
					}

					std::vector<ConstraintGroup> mergedConstraintGroups = original.constraintGroups;
					for (const auto& adderGroup : constraintGroups)
					{
						ConstraintGroup newGroup;
						for (size_t adderConstraintID : adderGroup)
						{
							newGroup.insert(adderOffset + adderConstraintID);
						}
						mergedConstraintGroups.push_back(newGroup);
					}

					if (!constraintGroups.empty())
					{
						std::cout << "Record constraint separated to " << std::to_string(constraintGroups.size()) << " independent constraints" << std::endl;
					}

					original.freeVariableAddresses = mergedFreeVariableAddresses;
					original.unitConstraints = mergedUnitConstraints;
					original.variableAppearances = mergedVariableAppearances;
					original.constraintGroups = mergedConstraintGroups;

					original.groupConstraints.clear();

					record.problems.resize(constraintGroups.size());
					for (size_t constraintGroupID = 0; constraintGroupID < constraintGroups.size(); ++constraintGroupID)
					{
						auto& currentProblem = record.problems[constraintGroupID];
						const auto& currentConstraintIDs = constraintGroups[constraintGroupID];

						for (size_t constraintID : currentConstraintIDs)
						{
							//CGL_DBG1("Constraint:");
							//printExpr(adderUnitConstraints[constraintID], pEnv, std::cout);
							currentProblem.addUnitConstraint(adderUnitConstraints[constraintID]);
						}

						currentProblem.freeVariableRefs = mergedFreeVariableAddresses;

						std::vector<double> resultxs = currentProblem.solve(pEnv, recordConsractor, record, keyList);

						readResult(pEnv, resultxs, currentProblem);

						const bool currentConstraintIsSatisfied = checkSatisfied(pEnv, currentProblem);
						record.isSatisfied = record.isSatisfied && currentConstraintIsSatisfied;
						if (!currentConstraintIsSatisfied)
						{
							++constraintViolationCount;
						}

						original.groupConstraints.push_back(currentProblem);
					}
				}
			}

			record.problems.clear();
			record.constraint = boost::none;
			record.freeVariables.clear();
			record.freeRanges.clear();
		}

		for (const auto& key : keyList)
		{
			//record.append(key.name, pEnv->dereference(key));

			//record.append(key.name, pEnv->makeTemporaryValue(pEnv->dereference(ObjectReference(key))));

			/*Address address = pEnv->findAddress(key);
			boost::optional<const Val&> opt = pEnv->expandOpt(address);
			record.append(key, pEnv->makeTemporaryValue(opt.get()));*/

			Address address = pEnv->findAddress(key);
			record.append(key, address);
		}

		/*if (record.type == RecordType::Path)
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
		}*/

		pEnv->printContext();

		CGL_DebugLog("");

		//pEnv->currentRecords.pop();
		pEnv->currentRecords.pop_back();

		//pEnv->pop();
		if (isNewScope)
		{
			pEnv->exitScope();
		}

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
			//auto originalOpt = pEnv->find(opt.get().name);
			boost::optional<const Val&> originalOpt = pEnv->dereference(pEnv->findAddress(opt.get()));
			if (!originalOpt)
			{
				//エラー：未定義のレコードを参照しようとした
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}

			recordOpt = AsOpt<Record>(originalOpt.get());
			if (!recordOpt)
			{
				//エラー：識別子の指すオブジェクトがレコード型ではない
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
		}
		else if (auto opt = AsOpt<Record>(record.original))
		{
			recordOpt = opt.get();
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

		if (IsType<Identifier>(record.original) && As<Identifier>(record.original).toString() == std::string("path"))
		{
			Record pathRecord;
			pathRecord.type = RecordType::Path;
			recordOpt = pathRecord;
		}
		else if (IsType<Identifier>(record.original) && As<Identifier>(record.original).toString() == std::string("text"))
		{
			Record pathRecord;
			pathRecord.type = RecordType::Text;
			recordOpt = pathRecord;
		}
		else if (IsType<Identifier>(record.original) && As<Identifier>(record.original).toString() == std::string("shapepath"))
		{
			Record pathRecord;
			pathRecord.type = RecordType::ShapePath;
			recordOpt = pathRecord;
		}
		else
		{
			Val originalRecordVal = pEnv->expand(boost::apply_visitor(*this, record.original), record);
			if (auto opt = AsOpt<Record>(originalRecordVal))
			{
				recordOpt = opt.get();
			}
			else
			{
				CGL_ErrorNode(record, "レコード値ではない値に対してレコード継承式を適用しようとしました。");
			}

			pEnv->printContext(true);
			CGL_DebugLog("Original:");
			printVal(originalRecordVal, pEnv);
		}

		//(1)オリジナルのレコードaのクローン(a')を作る
		Record clone = As<Record>(Clone(pEnv, recordOpt.get(), record));

		CGL_DebugLog("Clone:");
		printVal(clone, pEnv);

		if (pEnv->temporaryRecord)
		{
			CGL_ErrorNodeInternal(record, "二重にレコード継承式を適用しようとしました。temporaryRecordは既に存在しています。");
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
		Val recordValue = pEnv->expand(boost::apply_visitor(*this, expr), record);

		//(4) ローカルスコープの参照値を読みレコードに上書きする
		//レコード中のコロン式はレコードの最後でkeylistを見て値が紐づけられるので問題ないが
		//レコード中の代入式については、そのローカル環境の更新を手動でレコードに反映する必要がある
		if (auto opt = AsOpt<Record>(recordValue))
		{
			Record& newRecord = opt.get();
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
			for (auto& keyval : recordOpt.get().values)
			{
				clone.values[keyval.first] = pEnv->findAddress(keyval.first);
			}

			CGL_DebugLog("");

			//(5)
			MargeRecordInplace(clone, opt.get());

			CGL_DebugLog("");

			//TODO:ここで制約処理を行う

			//pEnv->pop();
			pEnv->exitScope();

			//return RValue(clone);
			return pEnv->makeTemporaryValue(clone);

			//MargeRecord(recordOpt.get(), opt.get());
		}

		CGL_DebugLog("");

		//pEnv->pop();
		pEnv->exitScope();
		*/
	}

	LRValue Eval::operator()(const DeclSat& node)
	{
		//ここでクロージャを作る必要がある
		ClosureMaker closureMaker(pEnv, std::set<std::string>());
		const Expr closedSatExpr = boost::apply_visitor(closureMaker, node.expr);
		//innerSatClosures.push_back(closedSatExpr);

		pEnv->enterScope();
		//DeclSat自体は現在制約が満たされているかどうかを評価結果として返す
		const Val result = pEnv->expand(boost::apply_visitor(*this, closedSatExpr), node);
		pEnv->exitScope();

		if (pEnv->currentRecords.empty())
		{
			CGL_ErrorNode(node, "制約宣言はレコード式の中にしか書くことができません。");
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
				CGL_ErrorNode(node, "var宣言はレコード式の中にしか書くことができません。");
			}

			ClosureMaker closureMaker(pEnv, std::set<std::string>());
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
			else if (IsType<Reference>(closedVarExpr))
			{
				ここで返ってくるReferenceはaccessorの最後の要素への参照になっている？
				closedVarExpr;
				pEnv->currentRecords.back().get().freeVariables.push_back(As<Accessor>(closedVarExpr));
			}
			else
			{
				CGL_ErrorNode(node, "var宣言には識別子かアクセッサしか用いることができません。");
			}
		}

		for (const auto& range : node.ranges)
		{
			ClosureMaker closureMaker(pEnv, std::set<std::string>());
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
			result.headValue = opt.get();
		}
		else if (auto opt = AsOpt<Record>(headValue))
		{
			result.headValue = opt.get();
		}
		else if (auto opt = AsOpt<List>(headValue))
		{
			result.headValue = opt.get();
		}
		else if (auto opt = AsOpt<FuncVal>(headValue))
		{
			result.headValue = opt.get();
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
			address = opt.get();
		}
		else if (auto opt = AsOpt<Record>(headValue))
		{
			address = pEnv->makeTemporaryValue(opt.get());
		}
		else if (auto opt = AsOpt<List>(headValue))
		{
			address = pEnv->makeTemporaryValue(opt.get());
		}
		else if (auto opt = AsOpt<FuncVal>(headValue))
		{
			address = pEnv->makeTemporaryValue(opt.get());
		}
		else
		{
			//エラー：識別子かリテラル以外（評価結果としてオブジェクトを返すような式）へのアクセスには未対応
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0;
		}
		*/


		if (IsType<Identifier>(accessor.head))
		{
			const auto head = As<Identifier>(accessor.head);
			if (!pEnv->findAddress(head).isValid())
			{
				CGL_ErrorNode(accessor, msgs::Undefined(head));
			}
		}

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
				address = pEnv->makeTemporaryValue(opt.get());
			}
			else if (auto opt = AsOpt<List>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.get());
			}
			else if (auto opt = AsOpt<FuncVal>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.get());
			}
			else
			{
				CGL_ErrorNodeInternal(accessor, "アクセッサの先頭の評価結果がレコード、リスト、関数のどの値でもありませんでした。");
			}
		}

		for (const auto& access : accessor.accesses)
		{
			//boost::optional<Val&> objOpt = pEnv->expandOpt(address);
			LRValue lrAddress = LRValue(address);
			boost::optional<Val&> objOpt = pEnv->mutableExpandOpt(lrAddress);
			if (!objOpt)
			{
				CGL_ErrorNode(accessor, "アクセッサによる参照先が存在しませんでした。");
			}

			Val& objRef = objOpt.get();

			if (auto listAccessOpt = AsOpt<ListAccess>(access))
			{
				Val value = pEnv->expand(boost::apply_visitor(*this, listAccessOpt.get().index), accessor);

				if (!IsType<List>(objRef))
				{
					CGL_ErrorNode(accessor, "リストでない値に対してリストアクセスを行おうとしました。");
				}

				List& list = As<List>(objRef);

				if (auto indexOpt = AsOpt<int>(value))
				{
					const int indexValue = indexOpt.get();
					const int listSize = static_cast<int>(list.data.size());
					const int maxIndex = listSize - 1;

					if (0 <= indexValue && indexValue <= maxIndex)
					{
						address = list.get(indexValue);
					}
					else if (indexValue < 0 || !pEnv->isAutomaticExtendMode())
					{
						CGL_ErrorNode(accessor, "リストの範囲外アクセスが発生しました。");
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
					CGL_ErrorNode(accessor, "リストアクセスのインデックスが整数値ではありませんでした。");
				}
			}
			else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
			{
				if (!IsType<Record>(objRef))
				{
					CGL_ErrorNode(accessor, "レコードでない値に対してレコードアクセスを行おうとしました。");
				}

				const Record& record = As<Record>(objRef);
				auto it = record.values.find(recordAccessOpt.get().name);
				if (it == record.values.end())
				{
					CGL_ErrorNode(accessor, "指定された識別子がレコード中に存在しませんでした。");
				}

				address = it->second;
			}
			else
			{
				auto funcAccess = As<FunctionAccess>(access);

				if (!IsType<FuncVal>(objRef))
				{
					CGL_ErrorNode(accessor, "関数でない値に対して関数呼び出しを行おうとしました。");
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
					const LRValue currentArgument = pEnv->expand(boost::apply_visitor(*this, expr), accessor);
					if (currentArgument.isLValue())
					{
						args.push_back(currentArgument.address(*pEnv));
					}
					else
					{
						args.push_back(pEnv->makeTemporaryValue(currentArgument.evaluated()));
					}
				}

				const Val returnedValue = pEnv->expand(callFunction(accessor, function, args), accessor);
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

			CGL_ErrorNodeInternal(node, "制約式の評価結果が不正な値でした。");
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

		CGL_ErrorNodeInternal(node, "制約式の評価結果が不正な値でした。");
		return false;
	}

	boost::optional<const Val&> evalExpr(const Expr& expr)
	{
		auto env = Context::Make();

		Eval evaluator(env);

		boost::optional<const Val&> result = env->expandOpt(boost::apply_visitor(evaluator, expr));

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
			CGL_Error("TODO:未対応");
			return false;
		}
		else if (IsType<KeyValue>(value1))
		{
			return As<KeyValue>(value1) == As<KeyValue>(value2);
		}
		else if (IsType<Record>(value1))
		{
			//return As<Record>(value1) == As<Record>(value2);
			CGL_Error("TODO:未対応");
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
		else if (IsType<Import>(value1))
		{
			//return As<SatReference>(value1) == As<SatReference>(value2);
			return false;
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
