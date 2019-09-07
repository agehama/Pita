#include <stack>
#include <unordered_set>

#include <Unicode.hpp>

#include <Pita/Context.hpp>
#include <Pita/Evaluator.hpp>
#include <Pita/IntrinsicGeometricFunctions.hpp>
#include <Pita/Printer.hpp>
#include <Pita/BinaryEvaluator.hpp>

namespace cgl
{
	class ExprAddressCheker : public boost::static_visitor<void>
	{
	public:
		ExprAddressCheker(const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet,
			RegionVariable::Attribute currentAttribute) :
			context(context),
			reachableAddressSet(reachableAddressSet),
			newAddressSet(newAddressSet),
			currentAttribute(currentAttribute)
		{}

		ExprAddressCheker(const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet,
			RegionVariable::Attribute currentAttribute, OutputAddresses& outputAddresses) :
			context(context),
			reachableAddressSet(reachableAddressSet),
			newAddressSet(newAddressSet),
			outputAddresses(outputAddresses),
			currentAttribute(currentAttribute)
		{}

		const Context& context;
		std::unordered_set<Address>& reachableAddressSet;
		std::unordered_set<Address>& newAddressSet;
		boost::optional<OutputAddresses> outputAddresses;
		RegionVariable::Attribute currentAttribute;

		bool isMarked(Address address)const
		{
			return reachableAddressSet.find(address) != reachableAddressSet.end();
		}

		void checkValue(const Val& val)
		{
			if (outputAddresses)
			{
				CheckValue(val, context, reachableAddressSet, newAddressSet, currentAttribute, outputAddresses.get());
			}
			else
			{
				CheckValue(val, context, reachableAddressSet, newAddressSet, currentAttribute);
			}
		}

		void update(Address address)
		{
			if (!isMarked(address))
			{
				reachableAddressSet.emplace(address);
				newAddressSet.emplace(address);

				if (outputAddresses && outputAddresses.get().pred(address))
				{
					outputAddresses.get().outputs.emplace_back(address, currentAttribute);
				}

				if (auto opt = context.expandOpt(LRValue(address)))
				{
					checkValue(opt.get());
				}
				else
				{
					CGL_ErrorInternal("不正なアドレスを参照しました。");
				}
			}
		}

		void operator()(const LRValue& node)
		{
			if (node.isRValue())
			{
				return;
			}

			update(node.address(context));
		}

		void operator()(const Identifier& node) {}

		void operator()(const Import& node) {}

		void operator()(const UnaryExpr& node)
		{
			boost::apply_visitor(*this, node.lhs);
		}

		void operator()(const BinaryExpr& node)
		{
			boost::apply_visitor(*this, node.lhs);
			boost::apply_visitor(*this, node.rhs);
		}

		void operator()(const Range& node)
		{
			boost::apply_visitor(*this, node.lhs);
			boost::apply_visitor(*this, node.rhs);
		}

		void operator()(const Lines& node)
		{
			for (const auto& expr : node.exprs)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const DefFunc& node)
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const If& node)
		{
			boost::apply_visitor(*this, node.cond_expr);
			boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				boost::apply_visitor(*this, node.else_expr.get());
			}
		}

		void operator()(const For& node)
		{
			boost::apply_visitor(*this, node.rangeStart);
			boost::apply_visitor(*this, node.rangeEnd);
			boost::apply_visitor(*this, node.doExpr);
		}

		void operator()(const Return& node)
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const ListConstractor& node)
		{
			for (const auto& expr : node.data)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const KeyExpr& node)
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const RecordConstractor& node)
		{
			for (const auto& expr : node.exprs)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const Accessor& node)
		{
			boost::apply_visitor(*this, node.head);

			for (const auto& access : node.accesses)
			{
				if (auto listAccess = AsOpt<ListAccess>(access))
				{
					boost::apply_visitor(*this, listAccess.get().index);
				}
				else if (auto functionAccess = AsOpt<FunctionAccess>(access))
				{
					for (const auto& argument : functionAccess.get().actualArguments)
					{
						boost::apply_visitor(*this, argument);
					}
				}
				else if (auto inheritAccess = AsOpt<InheritAccess>(access))
				{
					for (const auto& expr : inheritAccess.get().adder.exprs)
					{
						boost::apply_visitor(*this, expr);
					}
				}
			}
		}

		void operator()(const DeclSat& node)
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const DeclFree& node)
		{
			for (const auto& varRange : node.accessors)
			{
				Expr expr = varRange.accessor;
				boost::apply_visitor(*this, expr);

				if (varRange.range)
				{
					boost::apply_visitor(*this, varRange.range.get());
				}
			}
		}
	};

	class ValueAddressChecker : public boost::static_visitor<void>
	{
	public:
		ValueAddressChecker(const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet,
			RegionVariable::Attribute currentAttribute) :
			context(context),
			reachableAddressSet(reachableAddressSet),
			newAddressSet(newAddressSet),
			currentAttribute(currentAttribute)
		{}

		ValueAddressChecker(const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet,
			RegionVariable::Attribute currentAttribute, OutputAddresses& outputAddresses) :
			context(context),
			reachableAddressSet(reachableAddressSet),
			newAddressSet(newAddressSet),
			outputAddresses(outputAddresses),
			currentAttribute(currentAttribute)
		{}

		const Context& context;
		std::unordered_set<Address>& reachableAddressSet;
		std::unordered_set<Address>& newAddressSet;
		boost::optional<OutputAddresses> outputAddresses;
		RegionVariable::Attribute currentAttribute;

		bool isMarked(Address address)const
		{
			return reachableAddressSet.find(address) != reachableAddressSet.end();
		}

		void update(Address address)
		{
			if (!isMarked(address))
			{
				reachableAddressSet.emplace(address);
				newAddressSet.emplace(address);

				if (outputAddresses && outputAddresses.get().pred(address))
				{
					outputAddresses.get().outputs.emplace_back(address, currentAttribute);
				}

				if (auto opt = context.expandOpt(LRValue(address)))
				{
					boost::apply_visitor(*this, opt.get());
				}
				else
				{
					//CGL_ErrorInternal("不正なアドレスを参照しました。");
					CGL_ErrorInternal(std::string("不正なアドレスを参照しました。: Address(") + address.toString() + ")");
				}
			}
		}

		void checkExpr(const Expr& expr)
		{
			if (outputAddresses)
			{
				CheckExpr(expr, context, reachableAddressSet, newAddressSet, currentAttribute, outputAddresses.get());
			}
			else
			{
				CheckExpr(expr, context, reachableAddressSet, newAddressSet, currentAttribute);
			}
		}

		void operator()(bool node) {}

		void operator()(int node) {}

		void operator()(double node) {}

		void operator()(const CharString& node) {}

		void operator()(const List& node)
		{
			for (Address address : node.data)
			{
				update(address);
			}
		}

		void operator()(const KeyValue& node)
		{
			CGL_Error("未対応");
			//CheckValue(node.value, context, reachableAddressSet, newAddressSet);
		}

		void operator()(const Record& node)
		{
			//TODO: var(ShapeRecord in range)の場合はShapeRecordはpos,scale,angleのみ見ればよいと思ったが、
			//SatVariableBinderでも参照可能なアドレスを探して紐付けるためにこの関数を呼んでいるため、
			//これだとpos,scale,angle以外のメンバと紐付けられない問題が発生した。
			/*{
				const auto& values = node.values;
				auto posIt = values.find("pos");
				auto scaleIt = values.find("scale");
				auto angleIt = values.find("angle");
				const bool isShapeRecord = posIt != values.end() && scaleIt != values.end() && angleIt != values.end();
				if (isShapeRecord)
				{
					const RegionVariable::Attribute defaultAttribute = currentAttribute;
					currentAttribute = RegionVariable::Attribute::Position;
					update(posIt->second);
					currentAttribute = RegionVariable::Attribute::Scale;
					update(scaleIt->second);
					currentAttribute = RegionVariable::Attribute::Angle;
					update(angleIt->second);
					currentAttribute = defaultAttribute;
				}
				else
				{
					for (const auto& keyval : node.values)
					{
						update(keyval.second);
					}
				}
			}*/
			{
				const RegionVariable::Attribute defaultAttribute = currentAttribute;
				for (const auto& keyval : node.values)
				{
					if (keyval.first == "pos")
					{
						currentAttribute = RegionVariable::Attribute::Position;
						update(keyval.second);
					}
					else if (keyval.first == "scale")
					{
						currentAttribute = RegionVariable::Attribute::Scale;
						update(keyval.second);
					}
					else if (keyval.first == "angle")
					{
						currentAttribute = RegionVariable::Attribute::Angle;
						update(keyval.second);
					}
					else
					{
						currentAttribute = defaultAttribute;
						update(keyval.second);
					}
				}
				currentAttribute = defaultAttribute;
			}

			for (const auto& problem : node.problems)
			{
				if (problem.expr)
				{
					checkExpr(problem.expr.get());
				}

				for (const Address address : problem.refs)
				{
					update(address);
				}

				for (const auto& var : problem.freeVariableRefs)
				{
					update(var.address);
				}
			}

			if (node.constraint)
			{
				checkExpr(node.constraint.get());
			}

			for (const auto& varRange : node.boundedFreeVariables)
			{
				if (IsType<Accessor>(varRange.freeVariable))
				{
					const Expr expr = As<Accessor>(varRange.freeVariable);
					checkExpr(expr);
				}
				else if(IsType<Reference>(varRange.freeVariable))
				{
					const Expr expr = LRValue(As<Reference>(varRange.freeVariable));
					checkExpr(expr);
				}
				else
				{
					CGL_Error("不正な型");
				}

				if (varRange.freeRange)
				{
					checkExpr(varRange.freeRange.get());
				}
			}

			for (const auto& var: node.original.regionVars)
			{
				update(var.address);
			}

			for (const auto& oldExprs : node.original.unitConstraints)
			{
				checkExpr(oldExprs);
			}

			for (const auto& appears : node.original.variableAppearances)
			{
				for (const Address address : appears)
				{
					update(address);
				}
			}

			for (const auto& problem : node.original.groupConstraints)
			{
				if (problem.expr)
				{
					checkExpr(problem.expr.get());
				}
			}
		}

		void operator()(const FuncVal& node)
		{
			if (!node.builtinFuncAddress)
			{
				checkExpr(node.expr);
			}
		}

		void operator()(const Jump& node)
		{
			CGL_Error("未対応");
		}
	};

	void CheckExpr(const Expr& expr, const Context& context, std::unordered_set<Address>& reachableAddressSet,
		std::unordered_set<Address>& newAddressSet, RegionVariable::Attribute currentAttribute)
	{
		ExprAddressCheker cheker(context, reachableAddressSet, newAddressSet, currentAttribute);
		boost::apply_visitor(cheker, expr);
	}

	void CheckValue(const Val& evaluated, const Context& context, std::unordered_set<Address>& reachableAddressSet,
		std::unordered_set<Address>& newAddressSet, RegionVariable::Attribute currentAttribute)
	{
		ValueAddressChecker cheker(context, reachableAddressSet, newAddressSet, currentAttribute);
		boost::apply_visitor(cheker, evaluated);
	}

	void CheckExpr(const Expr& expr, const Context& context, std::unordered_set<Address>& reachableAddressSet,
		std::unordered_set<Address>& newAddressSet, RegionVariable::Attribute currentAttribute, OutputAddresses& outputAddresses)
	{
		ExprAddressCheker cheker(context, reachableAddressSet, newAddressSet, currentAttribute, outputAddresses);
		boost::apply_visitor(cheker, expr);
	}

	void CheckValue(const Val& evaluated, const Context& context, std::unordered_set<Address>& reachableAddressSet,
		std::unordered_set<Address>& newAddressSet, RegionVariable::Attribute currentAttribute, OutputAddresses& outputAddresses)
	{
		ValueAddressChecker cheker(context, reachableAddressSet, newAddressSet, currentAttribute, outputAddresses);
		boost::apply_visitor(cheker, evaluated);
	}

	/*std::unordered_set<Address> GetReachableAddressesFrom(const std::unordered_set<Address>& addresses, const Context& context)
	{
		std::unordered_set<Address> referenceableAddresses;

		const auto traverse = [&](const std::unordered_set<Address>& addresses, auto traverse)->void
		{
			for (const Address address : addresses)
			{
				std::unordered_set<Address> addressesDelta;
				CheckExpr(LRValue(address), context, referenceableAddresses, addressesDelta, RegionVariable::Attribute::Other);
				traverse(addressesDelta, traverse);
			}
		};

		traverse(addresses, traverse);

		return referenceableAddresses;
	}*/
	void GetReachableAddressesFrom(std::unordered_set<Address>& referenceableAddresses, const std::unordered_set<Address>& targetAddresses, const Context& context)
	{
		const auto traverse = [&](const std::unordered_set<Address>& addresses, auto traverse)->void
		{
			for (const Address address : addresses)
			{
				std::unordered_set<Address> addressesDelta;
				CheckExpr(LRValue(address), context, referenceableAddresses, addressesDelta, RegionVariable::Attribute::Other);
				traverse(addressesDelta, traverse);
			}
		};

		traverse(targetAddresses, traverse);
	}

	std::vector<RegionVariable> GetConstraintAddressesFrom(Address address, const Context& context, RegionVariable::Attribute attribute)
	{
		std::unordered_set<Address> initialAddresses;
		initialAddresses.emplace(address);

		std::vector<RegionVariable> result;

		std::unordered_set<Address> referenceableAddresses;
		OutputAddresses outputAddresses([&](Address address) {
			const Val value = context.expand(LRValue(address), LocationInfo());
			return IsType<int>(value) || IsType<double>(value);
		}, result);

		const auto traverse = [&](const std::unordered_set<Address>& addresses, auto traverse)->void
		{
			for (const Address address : addresses)
			{
				std::unordered_set<Address> addressesDelta;
				CheckExpr(LRValue(address), context, referenceableAddresses, addressesDelta, attribute, outputAddresses);
				traverse(addressesDelta, traverse);
			}
		};

		traverse(initialAddresses, traverse);

		return result;
	}

	Address Context::makeFuncVal(std::shared_ptr<Context> pEnv, const std::vector<Identifier>& arguments, const Expr& expr)
	{
		std::set<std::string> functionArguments;
		for (const auto& arg : arguments)
		{
			functionArguments.insert(arg);
		}

		const Expr closedFuncExpr = AsClosure(*pEnv, expr);

		FuncVal funcVal(arguments, closedFuncExpr);
		return makeTemporaryValue(funcVal);
	}

	void Context::registerBuiltInFunction(const std::string& name, const BuiltInFunction& function, bool isPlateausFunction)
	{
		const Address address1 = m_functions.add(function);
		const Address address2 = m_values.add(FuncVal(address1));

		if (address1 != address2)
		{
			CGL_Error("組み込み関数の追加に失敗");
		}

		bindValueID(name, address1);

		if (isPlateausFunction)
		{
			m_plateausFunctions[address1] = name;
		}
	}

	Val Context::callBuiltInFunction(Address functionAddress, const std::vector<Address>& arguments, const LocationInfo& info)
	{
		if (std::shared_ptr<Context> pEnv = m_weakThis.lock())
		{
			return m_functions[functionAddress](pEnv, arguments, info);
		}
			
		CGL_Error("ここは通らないはず");
		return 0;
	}

	bool Context::isPlateausBuiltInFunction(Address functionAddress)
	{
		return m_plateausFunctions.find(functionAddress) != m_plateausFunctions.end();
	}

	const Val& Context::expand(const LRValue& lrvalue, const LocationInfo& info)const
	{
		if (lrvalue.isEitherReference())
		{
			if (lrvalue.eitherReference().localReferenciable(*this))
			{
				Eval evaluator(m_weakThis.lock());
				Expr expr = lrvalue.eitherReference().local.get();
				return expand(boost::apply_visitor(evaluator, expr), info);
			}
			else if (lrvalue.eitherReference().replaced.isValid())
			{
				auto it = m_values.at(lrvalue.eitherReference().replaced);
				if (it != m_values.end())
				{
					return it->second;
				}
			}

			CGL_ErrorNode(info, std::string("値") + lrvalue.toString() + "の参照に失敗しました。");
		}
		else if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}

			CGL_ErrorNode(info, std::string("値") + lrvalue.toString() + "の参照に失敗しました。");
		}

		return lrvalue.evaluated();
	}

	Val& Context::mutableExpand(LRValue& lrvalue, const LocationInfo& info)
	{
		if (lrvalue.isEitherReference())
		{
			/*if (lrvalue.eitherReference().localReferenciable(*this))
			{
				Eval evaluator(m_weakThis.lock());
				Expr expr = lrvalue.eitherReference().local.get();
				LRValue result = boost::apply_visitor(evaluator, expr);
				return mutableExpand(result, info);
			}
			else if (lrvalue.eitherReference().replaced.isValid())
			{
				auto it = m_values.at(lrvalue.eitherReference().replaced);
				if (it != m_values.end())
				{
					return it->second;
				}
			}*/
			if (lrvalue.eitherReference().replaced.isValid())
			{
				auto it = m_values.at(lrvalue.eitherReference().replaced);
				if (it != m_values.end())
				{
					return it->second;
				}
			}

			CGL_ErrorNode(info, std::string("値") + lrvalue.toString() + "の参照に失敗しました。");
		}
		else if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}

			CGL_ErrorNode(info, std::string("値") + lrvalue.toString() + "の参照に失敗しました。");
		}

		return lrvalue.mutableVal();
	}

	boost::optional<const Val&> Context::expandOpt(const LRValue& lrvalue)const
	{
		if (lrvalue.isEitherReference())
		{
			if (lrvalue.eitherReference().localReferenciable(*this))
			{
				Eval evaluator(m_weakThis.lock());
				Expr expr = lrvalue.eitherReference().local.get();
				return expandOpt(boost::apply_visitor(evaluator, expr));
			}
			else if (lrvalue.eitherReference().replaced.isValid())
			{
				auto it = m_values.at(lrvalue.eitherReference().replaced);
				if (it != m_values.end())
				{
					return it->second;
				}
			}

			return boost::none;
		}
		else if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}

			return boost::none;
		}

		return lrvalue.evaluated();
	}

	boost::optional<Val&> Context::mutableExpandOpt(LRValue& lrvalue)
	{
		if (lrvalue.isEitherReference())
		{
			/*if (lrvalue.eitherReference().localReferenciable(*this))
			{
				Eval evaluator(m_weakThis.lock());
				Expr expr = lrvalue.eitherReference().local.get();
				return mutableExpandOpt(boost::apply_visitor(evaluator, expr));
			}
			else*/ if (lrvalue.eitherReference().replaced.isValid())
			{
				auto it = m_values.at(lrvalue.eitherReference().replaced);
				if (it != m_values.end())
				{
					return it->second;
				}
			}

			return boost::none;
		}
		else if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}

			return boost::none;
		}

		return lrvalue.mutableVal();
	}

	Address Context::evalReference(const Accessor& access)
	{
		if (auto sharedThis = m_weakThis.lock())
		{
			Eval evaluator(sharedThis);

			const Expr accessor = access;
			const LRValue refVal = boost::apply_visitor(evaluator, accessor);
			if (refVal.isLValue())
			{
				return refVal.address(*this);
			}

			CGL_Error("アクセッサの評価結果がアドレス値でない");
		}

		CGL_Error("shared this does not exist.");
		return Address::Null();
	}

	//オブジェクトの中にある全ての値への参照をリストで取得する
	std::vector<RegionVariable> Context::expandReferences(Address address, const LocationInfo& info, RegionVariable::Attribute attribute)
	{
		return GetConstraintAddressesFrom(address, *m_weakThis.lock(), attribute);
	}

	std::vector<RegionVariable> Context::expandReferences2(const Accessor& accessor, const LocationInfo& info)
	{
		auto sharedThis = m_weakThis.lock();

		Eval evaluator(sharedThis);

		using Addresses = std::vector<std::pair<Address, RegionVariable::Attribute>>;
		std::array<Addresses, 2> addressBuffer;
		unsigned int currentWriteIndex = 0;
		const auto nextIndex = [&]() {return (currentWriteIndex + 1) & 1; };
		const auto flipIndex = [&]() {currentWriteIndex = nextIndex(); };

		const auto writeBuffer = [&]()->Addresses& {return addressBuffer[currentWriteIndex]; };
		const auto readBuffer = [&]()->Addresses& {return addressBuffer[nextIndex()]; };

		RegionVariable::Attribute currentAttribute = RegionVariable::Attribute::Other;

		LRValue headValue = boost::apply_visitor(evaluator, accessor.head);
		if (headValue.isLValue())
		{
			writeBuffer().emplace_back(headValue.address(*this), currentAttribute);
		}
		else
		{
			Val evaluated = headValue.evaluated();
			if (auto opt = AsOpt<Record>(evaluated))
			{
				writeBuffer().emplace_back(makeTemporaryValue(opt.get()), currentAttribute);
			}
			else if (auto opt = AsOpt<List>(evaluated))
			{
				writeBuffer().emplace_back(makeTemporaryValue(opt.get()), currentAttribute);
			}
			else if (auto opt = AsOpt<FuncVal>(evaluated))
			{
				writeBuffer().emplace_back(makeTemporaryValue(opt.get()), currentAttribute);
			}
			else
			{
				CGL_Error("アクセッサのヘッドの評価結果が不正");
			}
		}

		for (const auto& access : accessor.accesses)
		{
			flipIndex();

			writeBuffer().clear();

			for (int i = 0; i < readBuffer().size(); ++i)
			{
				const Address address = readBuffer()[i].first;

				boost::optional<const Val&> objOpt = expandOpt(LRValue(address));
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}

				const Val& objRef = objOpt.get();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					const List& list = As<List>(objRef);

					if (listAccessOpt.get().isArbitrary)
					{
						const auto& allIndices = list.data;

						for (Address address : allIndices)
						{
							writeBuffer().emplace_back(address, currentAttribute);
						}
					}
					else
					{
						Val value = expand(boost::apply_visitor(evaluator, listAccessOpt.get().index), info);

						if (auto indexOpt = AsOpt<int>(value))
						{
							writeBuffer().emplace_back(list.get(indexOpt.get()), currentAttribute);
						}
						else if (auto indicesOpt = AsOpt<List>(value))
						{
							const List& indices = indicesOpt.get();
							for (const Address indexAddress : indices.data)
							{
								Val indexValue = expand(LRValue(indexAddress), info);
								if (auto indexOpt = AsOpt<int>(indexValue))
								{
									writeBuffer().emplace_back(list.get(indexOpt.get()), currentAttribute);
								}
								else
								{
									CGL_ErrorNode(info, "リストのインデックスが整数値ではありませんでした。");
								}
							}
						}
						else
						{
							CGL_ErrorNode(info, "リストのインデックスが整数値ではありませんでした。");
						}
					}
				}
				else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
				{
					if (!IsType<Record>(objRef))
					{
						CGL_Error("オブジェクトがレコードでない");
					}

					const Record& record = As<Record>(objRef);
					const std::string name = recordAccessOpt.get().name;
					auto it = record.values.find(name);
					if (it == record.values.end())
					{
						//CGL_Error("指定された識別子がレコード中に存在しない");
						CGL_Error(std::string() + "指定された識別子\"" + recordAccessOpt.get().name.toString() + "\"がレコード中に存在しない");
					}

					if (name == "pos")
					{
						currentAttribute = RegionVariable::Attribute::Position;
					}
					else if (name == "scale")
					{
						currentAttribute = RegionVariable::Attribute::Scale;
					}
					else if (name == "angle")
					{
						currentAttribute = RegionVariable::Attribute::Angle;
					}

					writeBuffer().emplace_back(it->second, currentAttribute);
				}
				else
				{
					auto funcAccess = As<FunctionAccess>(access);

					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("オブジェクトが関数でない");
					}

					const FuncVal& function = As<FuncVal>(objRef);

					std::vector<Address> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue currentArgument(expand(boost::apply_visitor(evaluator, expr), info));
						if (currentArgument.isLValue())
						{
							args.push_back(currentArgument.address(*this));
						}
						else
						{
							args.push_back(makeTemporaryValue(currentArgument.evaluated()));
						}
					}

					const Val returnedValue = expand(evaluator.callFunction(accessor, function, args), info);
					writeBuffer().emplace_back(makeTemporaryValue(returnedValue), currentAttribute);
				}
			}
		}

		flipIndex();

		std::vector<RegionVariable> result;
		for (const auto& var : readBuffer())
		{
			//var での省略記法 list[*] を使った場合、readBuffer() には複数のアドレスが格納される
			//ここで、var( list[*].pos in range ) と書いた場合はこの range と各アドレスをどう結び付けるかが自明でない
			//とりあえず今の実装では全てのアドレスに対して同じ range を割り当てるようにしている
			const auto expanded = expandReferences(var.first, info, var.second);
			result.insert(result.end(), expanded.begin(), expanded.end());
		}

		return result;
	}

	Reference Context::bindReference(Address address)
	{
		++m_referenceID;

		Reference reference(m_referenceID);
		
		DeepReference deepReference;
		deepReference.head = address;
		m_refAddressMap[reference] = { address };
		m_addressRefMap.insert({ address, reference });

		return reference;
	}

	Reference Context::bindReference(const Identifier& identifier)
	{
		return bindReference(findAddress(identifier));

		/*
		++m_referenceID;

		Reference reference(m_referenceID);

		const Address address = findAddress(identifier);

		
		DeepReference deepReference;
		deepReference.head = address;
		m_refAddressMap[reference] = { address };
		m_addressRefMap.insert({ address, reference });

		return reference;
		*/
	}

	Reference Context::bindReference(const Accessor& accessor, const LocationInfo& info)
	{
		if(auto pEnv = m_weakThis.lock())
		{
			DeepReference deepReference;

			Eval evaluator(pEnv);

			LRValue head = boost::apply_visitor(evaluator, accessor.head);
			if (!head.isAddress())
			{
				CGL_Error("右辺値に対して参照演算子を適用しようとしました");
			}
			
			Address address = head.address(*this);
			deepReference.head = address;

			//参照が付くという事はheadのアドレスは現時点で評価できるはずだが、その先も評価できるとは限らない

			for (const auto& access : accessor.accesses)
			{
				boost::optional<const Val&> objOpt = pEnv->expandOpt(LRValue(address));
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}
				
				const Val& objRef = objOpt.get();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					const List& list = As<List>(objRef);
					Val value = pEnv->expand(boost::apply_visitor(evaluator, listAccessOpt.get().index), info);
					if (auto indexOpt = AsOpt<int>(value))
					{
						address = list.get(indexOpt.get());
						deepReference.addList(indexOpt.get(), address);
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
					auto it = record.values.find(recordAccessOpt.get().name);
					if (it == record.values.end())
					{
						CGL_Error("指定された識別子がレコード中に存在しない");
					}

					address = it->second;
					deepReference.addRecord(it->first, address);
				}
				else
				{
					CGL_Error("右辺値に対して参照演算子を適用しようとしました");
				}
			}

			++m_referenceID;

			Reference reference(m_referenceID);

			//std::cout << "Address(" << deepReference.head.toString() << ")" << " is binded to Reference(" << reference.toString() << ")\n";
			m_refAddressMap[reference] = deepReference;
			m_addressRefMap.insert({ deepReference.head, reference });
			for (const auto& keyAddress : deepReference.tail)
			{
				//std::cout << "Address(" << keyAddress.second.toString() << ")" << " is binded to Reference(" << reference.toString() << ")\n";
				m_addressRefMap.insert({ keyAddress.second, reference });
			}

			return reference;
		}

		CGL_Error("shared this does not exist.");
		return Reference();
	}

	Address Context::getReference(Reference reference)const
	{
		auto it = m_refAddressMap.find(reference);
		const auto& deepReference = it->second;

		if (deepReference.tail.empty())
		{
			return deepReference.head;
		}
		return deepReference.tail.back().second;
	}

	Reference Context::cloneReference(Reference oldReference, const std::unordered_map<Address, Address>& addressReplaceMap)
	{
		const auto newAddress = [&](const Address address)
		{
			return addressReplaceMap.at(address);
		};

		const DeepReference& oldDeepReference = m_refAddressMap[oldReference];

		++m_referenceID;
		Reference newReference(m_referenceID);	
		DeepReference newDeepReference;

		newDeepReference.head = newAddress(oldDeepReference.head);
		m_addressRefMap.insert({ newDeepReference.head, newReference });

		for (const auto& keyval : oldDeepReference.tail)
		{
			const Address currentAddress = newAddress(keyval.second);
			newDeepReference.add(keyval.first, currentAddress);
			m_addressRefMap.insert({ currentAddress, newReference });
		}

		m_refAddressMap[newReference] = newDeepReference;
		
		return newReference;
	}

	void Context::printReference(Reference reference, std::ostream & os) const
	{
		auto it = m_refAddressMap.find(reference);
		const auto& deepReference = it->second;

		os << deepReference.head.toString();
		for (const auto& ref : deepReference.tail)
		{
			os << " -> " << ref.second.toString();
		}
	}

	void Context::bindValueID(const std::string& name, const Address ID)
	{
		for (auto scopeIt = localEnv().rbegin(); scopeIt != localEnv().rend(); ++scopeIt)
		{
			auto valIt = scopeIt->variables.find(name);
			if (valIt != scopeIt->variables.end())
			{
				changeAddress(valIt->second, ID);
				valIt->second = ID;
				return;
			}
		}

		localEnv().back().variables[name] = ID;
	}

	bool Context::existsInCurrentScope(const std::string& name)const
	{
		return localEnv().back().variables.find(name) != localEnv().back().variables.end();
	}

	bool Context::existsInLocalScope(const std::string& name)const
	{
		return std::any_of(localEnv().begin(), localEnv().end(), [&](const Scope& scope) {return scope.variables.find(name) != scope.variables.end(); });
	}

	void Context::printContext(bool flag)const {}

	void Context::printContext(std::ostream& os)const
	{
		os << "---------------------------\n";

		/*os << "Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			os << keyval.first.toString() << " : ";

			printVal(val, m_weakThis.lock(), os);
		}*/

		for (const auto& env : m_localEnvStack)
		{
			os << "Environment:\n";
			size_t d = 0;
			for (auto scopeIt = env.begin(); scopeIt != env.end(); ++scopeIt, ++d)
			{
				os << "Depth : " << d << "\n";
				for (const auto& var : scopeIt->variables)
				{
					os << "    " << var.first << " : " << var.second.toString() << "\n";
				}
			}
		}

		/*os << "References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			os << "Depth : " << d << "\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names.variables)
			{
				os << "    " << keyval.first << " : " << keyval.second.toString() << "\n";
			}
		}*/

		os << "---------------------------\n";
	}

	void Context::assignToAccessor(const Accessor& accessor, const LRValue& newValue, const LocationInfo& info)
	{
		if (auto pEnv = m_weakThis.lock())
		{
			LRValue address;

			Eval evaluator(pEnv);
			address = boost::apply_visitor(evaluator, accessor.head);
			if (address.isRValue())
			{
				CGL_Error("一時オブジェクトへの代入はできません");
			}

			for (int i = 0; i < accessor.accesses.size(); ++i)
			{
				const auto& access = accessor.accesses[i];
				const bool isLastElement = i + 1 == accessor.accesses.size();

				boost::optional<Val&> objOpt = mutableExpandOpt(address);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}

				Val& objRef = objOpt.get();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					Val indexValue = expand(boost::apply_visitor(evaluator, listAccessOpt.get().index), info);

					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					List& list = As<List>(objRef);
					if (auto indexOpt = AsOpt<int>(indexValue))
					{
						const int index = indexOpt.get();
						if (isLastElement)
						{
							const Address oldAddress = list.data[index];
							const Address newAddress = newValue.isLValue() ? newValue.address(*this) : makeTemporaryValue(newValue.evaluated());
							changeAddress(oldAddress, newAddress);
							list.data[index] = newAddress;
						}
						else
						{
							address = LRValue(list.get(index));
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

					Record& record = As<Record>(objRef);
					auto it = record.values.find(recordAccessOpt.get().name);
					if (it == record.values.end())
					{
						CGL_Error("指定された識別子がレコード中に存在しない");
					}

					if (isLastElement)
					{
						const Address oldAddress = it->second;
						const Address newAddress = newValue.isLValue() ? newValue.address(*this) : makeTemporaryValue(newValue.evaluated());
						changeAddress(oldAddress, newAddress);
						it->second = newAddress;
					}
					else
					{
						address = LRValue(it->second);
					}
				}
				else
				{
					CGL_Error("一時オブジェクトへの代入はできません");
				}
			}
		}
		else
		{
			CGL_Error("shared this does not exist.");
		}
	}

	void Context::assignToReference(const Reference& reference, const LRValue& newValue, const LocationInfo& info)
	{
		const Address oldAddress = getReference(reference);
		const Address newAddress = newValue.isLValue() ? newValue.address(*this) : makeTemporaryValue(newValue.evaluated());
		replaceGlobalContextAddress(oldAddress, newAddress);
	}

	std::shared_ptr<Context> Context::Make()
	{
		auto p = std::make_shared<Context>();
		p->m_weakThis = p;
		p->switchFrontScope();
		p->enterScope();
		p->initialize();
		return p;
	}

	std::shared_ptr<Context> Context::Make(const Context& other)
	{
		auto p = std::make_shared<Context>(other);
		p->m_weakThis = p;
		return p;
	}

	Address Context::makeTemporaryValue(const Val& value)
	{
		const Address address = m_values.add(value);

		localEnv().back().temporaryAddresses.push_back(address);

		const int thresholdGC = 20000;
		//const int thresholdGC = 5000;
		if (thresholdGC <= static_cast<int>(m_values.size()) - static_cast<int>(m_lastGCValueSize))
		{
			garbageCollect();
			m_lastGCValueSize = m_values.size();
		}

		return address;
	}

	Address Context::findAddress(const std::string& name)const
	{
		for (auto scopeIt = localEnv().rbegin(); scopeIt != localEnv().rend(); ++scopeIt)
		{
			auto variableIt = scopeIt->variables.find(name);
			if (variableIt != scopeIt->variables.end())
			{
				return variableIt->second;
			}
		}

		return Address::Null();
	}

	std::set<std::string> Context::getVisibleIdentifiers()const
	{
		std::set<std::string> result;
		for (const auto& env : m_localEnvStack)
		{
			for (auto scopeIt = env.rbegin(); scopeIt != env.rend(); ++scopeIt)
			{
				for (const auto& nameVal : scopeIt->variables)
				{
					result.insert(nameVal.first);
				}
			}
		}
		return result;
	}

	boost::optional<DefFuncWithScopeInfo&> Context::getDeferredFunction(const Identifier& deferredIdentifier)
	{
		if (!deferredIdentifier.isDeferredCall() && !deferredIdentifier.isMakeClosure())
		{
			//std::cerr << "End   getDeferredFunction(0)" << std::endl;
			return boost::none;
		}

		auto [callerScopeInfo, rawIdentifier] = deferredIdentifier.decomposed();
		
		/*std::cerr << "deferredIdentifiers: " << deferredIdentifiers.size() << std::endl;
		for (const auto& keyval : deferredIdentifiers)
		{
			std::cerr << "key: " << static_cast<std::string>(keyval.first) << std::endl;
		}*/

		// 呼び出しと一致する識別子の検索
		auto it = deferredIdentifiers.find(rawIdentifier);
		if (it == deferredIdentifiers.end() || it->second.empty())
		{
			//std::cerr << "End   getDeferredFunction(1) \"" << static_cast<std::string>(rawIdentifier) <<"\""<< std::endl;
			return boost::none;
		}
		
		// 呼び出しと一致するインデックス配列の検索
		std::vector<DefFuncWithScopeInfo>& scopeInfos = it->second;
		int maxMatchCount = -1;
		int maxMatchCountIndex = -1;
		
		for (size_t functionIndex = 0; functionIndex < scopeInfos.size(); ++functionIndex)
		{
			const DefFuncWithScopeInfo& currentFuncInfo = scopeInfos[functionIndex];
			const auto& funcScopeInfo = currentFuncInfo.scopeInfo;

			// 呼び出し側より深いスコープで定義された遅延識別子が呼ばれることはない
			if (callerScopeInfo.size() < funcScopeInfo.size())
			{
				continue;
			}

			// 呼び出しの必要条件：関数側のスコープが呼び出し側のスコープを包含している
			// <=> 関数側のスコープインデックス配列が呼び出し側のスコープインデックス配列に包含されている
			bool matched = true;
			for (size_t i = 0; i < funcScopeInfo.size(); ++i)
			{
				if (funcScopeInfo[i] != callerScopeInfo[i])
				{
					matched = false;
				}
			}

			// 呼び出し可能な遅延識別子の中ではより深いスコープで定義されたものが優先される
			if (matched && maxMatchCount < static_cast<int>(funcScopeInfo.size()))
			{
				maxMatchCount = static_cast<int>(funcScopeInfo.size());
				maxMatchCountIndex = static_cast<int>(functionIndex);
			}
		}

		if (maxMatchCountIndex == -1)
		{
			//std::cerr << "End   getDeferredFunction(2)" << std::endl;
			return boost::none;
		}

		//std::cerr << "End   getDeferredFunction(3)" << std::endl;
		return scopeInfos[maxMatchCountIndex];
	}

	boost::optional<const DefFuncWithScopeInfo&> Context::getDeferredFunction(const Identifier& deferredIdentifier)const
	{
		if (!deferredIdentifier.isDeferredCall() && !deferredIdentifier.isMakeClosure())
		{
			return boost::none;
		}

		auto [callerScopeInfo, rawIdentifier] = deferredIdentifier.decomposed();

		// 呼び出しと一致する識別子の検索
		auto it = deferredIdentifiers.find(rawIdentifier);
		if (it == deferredIdentifiers.end() || it->second.empty())
		{
			return boost::none;
		}

		// 呼び出しと一致するインデックス配列の検索
		const std::vector<DefFuncWithScopeInfo>& scopeInfos = it->second;
		int maxMatchCount = -1;
		int maxMatchCountIndex = -1;

		for (size_t functionIndex = 0; functionIndex < scopeInfos.size(); ++functionIndex)
		{
			const DefFuncWithScopeInfo& currentFuncInfo = scopeInfos[functionIndex];
			const auto& funcScopeInfo = currentFuncInfo.scopeInfo;

			// 呼び出し側より深いスコープで定義された遅延識別子が呼ばれることはない
			if (callerScopeInfo.size() < funcScopeInfo.size())
			{
				continue;
			}

			// 呼び出しの必要条件：関数側のスコープが呼び出し側のスコープを包含している
			// <=> 関数側のスコープインデックス配列が呼び出し側のスコープインデックス配列に包含されている
			bool matched = true;
			for (size_t i = 0; i < funcScopeInfo.size(); ++i)
			{
				if (funcScopeInfo[i] != callerScopeInfo[i])
				{
					matched = false;
				}
			}

			// 呼び出し可能な遅延識別子の中ではより深いスコープで定義されたものが優先される
			if (matched && maxMatchCount < static_cast<int>(funcScopeInfo.size()))
			{
				maxMatchCount = static_cast<int>(funcScopeInfo.size());
				maxMatchCountIndex = static_cast<int>(functionIndex);
			}
		}

		if (maxMatchCountIndex == -1)
		{
			return boost::none;
		}

		return scopeInfos[maxMatchCountIndex];
	}

	void Context::initialize()
	{
		m_random.seed(1);

		registerBuiltInFunction(
			"Print",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			//std::cout << "Address(" << arguments[0].toString() << "): ";
			printVal2(pEnv->expand(LRValue(arguments[0]), info), pEnv, std::cout, 0);
			std::cout << std::endl;
			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"EnableTimeLimit",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& value = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(value))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			pEnv->setTimeLimit(AsDouble(value));
			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"DisableTimeLimit",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 0)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			pEnv->setTimeLimit(boost::none);
			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"PrintContext",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 0)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			printContext(std::cout);

			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"Size",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& value = pEnv->expand(LRValue(arguments[0]), info);
			if (auto opt = AsOpt<List>(value))
			{
				return static_cast<int>(opt.get().data.size());
			}
			else if (auto opt = AsOpt<Record>(value))
			{
				return static_cast<int>(opt.get().values.size());
			}

			CGL_ErrorNode(info, "不正な式です");
			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"Reverse",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& value = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsType<List>(value))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			List original = As<List>(value);
			std::reverse(original.data.begin(), original.data.end());
			return original;
		},
			false
			);

		registerBuiltInFunction(
			"Push",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const LRValue address(arguments[0]);
			Val& listValue = pEnv->mutableExpand(LRValue(address), info);
			if (!IsType<List>(listValue))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			const Val& value = pEnv->expand(LRValue(arguments[1]), info);

			List& original = As<List>(listValue);
			original.data.push_back(pEnv->makeTemporaryValue(value));
			return original;
		},
			false
			);

		registerBuiltInFunction(
			"Pop",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const LRValue address(arguments[0]);
			Val& listValue = pEnv->mutableExpand(LRValue(address), info);
			if (!IsType<List>(listValue))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			List& original = As<List>(listValue);
			original.data.pop_back();
			return original;
		},
			false
			);

		registerBuiltInFunction(
			"Sort",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1 && arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const LRValue address(arguments[0]);
			Val& listValue = pEnv->mutableExpand(LRValue(address), info);
			if (!IsType<List>(listValue))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			List& original = As<List>(listValue);

			if (arguments.size() == 1)
			{
				std::sort(original.data.begin(), original.data.end(), 
					[&](const Address& a, const Address& b)->bool
				{
					return LessThanFunc(pEnv->expand(LRValue(a), info), pEnv->expand(LRValue(b), info), *pEnv);
				});
			}
			else
			{
				const Val& value = pEnv->expand(LRValue(arguments[1]), info);
				if (!IsType<FuncVal>(value))
				{
					CGL_ErrorNode(info, "引数の型が正しくありません");
				}
				const FuncVal& predicate = As<FuncVal>(value);

				Eval eval(pEnv);

				std::sort(original.data.begin(), original.data.end(),
					[&](const Address& a, const Address& b)->bool
				{
					std::vector<Address> args({a,b});
					const Val result = pEnv->expand(eval.callFunction(info, predicate, args), info);
					if (!IsType<bool>(result))
					{
						CGL_ErrorNode(info, "不正な型");
					}
					return As<bool>(result);
				});
			}

			return original;
		},
			false
			);

		registerBuiltInFunction(
			"Cmaes",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			return Val(true);
		},
			true
			);

		registerBuiltInFunction(
			"Rad",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& value = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(value))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return deg2rad * AsDouble(value);
		},
			false
			);

		registerBuiltInFunction(
			"Deg",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& value = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(value))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return rad2deg * AsDouble(value);
		},
			false
			);

		registerBuiltInFunction(
			"Abs",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return std::abs(AsDouble(x));
		},
			false
			);

		registerBuiltInFunction(
			"Mod",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			const Val& y = pEnv->expand(LRValue(arguments[1]), info);
			if (!IsNum(x) || !IsNum(y))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			if (IsType<int>(x) && IsType<int>(y))
			{
				return As<int>(x) % As<int>(y);
			}
			return fmod(AsDouble(x), AsDouble(y));
		},
			false
			);

		registerBuiltInFunction(
			"Floor",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return static_cast<int>(std::floor(AsDouble(x)));
		},
			false
			);

		registerBuiltInFunction(
			"Ceil",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return static_cast<int>(std::ceil(AsDouble(x)));
		},
			false
			);

		registerBuiltInFunction(
			"Sin",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return std::sin(deg2rad*AsDouble(x));
		},
			false
			);

		registerBuiltInFunction(
			"Cos",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return std::cos(deg2rad*AsDouble(x));
		},
			false
			);

		registerBuiltInFunction(
			"Log",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return std::log(AsDouble(x));
		},
			false
			);

		registerBuiltInFunction(
			"SinRad",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return std::sin(AsDouble(x));
		},
			false
			);

		registerBuiltInFunction(
			"CosRad",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& x = pEnv->expand(LRValue(arguments[0]), info);
			if (!IsNum(x))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return std::cos(AsDouble(x));
		},
			false
			);

		registerBuiltInFunction(
			"Random",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& v0 = expand(LRValue(arguments[0]), info);
			const Val& v1 = expand(LRValue(arguments[1]), info);

			if (!IsNum(v0) || !IsNum(v1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			if (IsType<int>(v0) && IsType<int>(v1))
			{
				std::uniform_int_distribution<int> distInt(As<int>(v0), As<int>(v1));
				return distInt(m_random);
			}

			const double x0 = AsDouble(v0);
			const double x1 = AsDouble(v1);
			const double x01 = m_dist(m_random);
			return x0 + (x1 - x0)*x01;
		},
			false
			);

		registerBuiltInFunction(
			"RandomSeed",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 0 && arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			if (arguments.empty())
			{
				std::random_device randomDevice;
				m_random.seed(randomDevice());
			}
			else
			{
				const Val& value = expand(LRValue(arguments[0]), info);
				if (!IsType<int>(value))
				{
					CGL_ErrorNode(info, "引数の型が正しくありません");
				}
				m_random.seed(As<int>(value));
			}

			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"BuildPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2 && arguments.size() != 3)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& passesVal = pEnv->expand(LRValue(arguments[0]), info);
			const Val& numVal = pEnv->expand(LRValue(arguments[1]), info);

			auto obstaclesValOpt = arguments.size() == 3 ? boost::optional<const Val&>(pEnv->expand(LRValue(arguments[2]), info)) : boost::none;

			if (!IsType<List>(passesVal) || !IsType<int>(numVal))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}
			if (obstaclesValOpt && !IsType<List>(obstaclesValOpt.get()))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			if (obstaclesValOpt)
			{
				return BuildPath(As<PackedList>(As<List>(passesVal).packed(*this)), pEnv, As<int>(numVal), As<PackedList>(As<List>(obstaclesValOpt.get()).packed(*this))).unpacked(*this);
			}
			return BuildPath(As<PackedList>(As<List>(passesVal).packed(*this)), pEnv, As<int>(numVal)).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"Free",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& arg1 = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(arg1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			Val clone = Clone(pEnv, arg1, info);
			Record& record = As<Record>(clone);
			record.original = OldRecordData();
			record.constraint = boost::none;
			return clone;
		},
			true
			);

		registerBuiltInFunction(
			"Fixed",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val strVal = pEnv->expand(LRValue(arguments[0]), info);
			const Val digitsVal = pEnv->expand(LRValue(arguments[1]), info);

			if (!IsNum(strVal))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}
			if (!IsType<int>(digitsVal))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			const double num = AsDouble(strVal);
			std::stringstream ss;
			ss << std::setprecision(As<int>(digitsVal));
			ss << num;
			CharString printedStr(AsUtf32(ss.str()));

			return printedStr;
		},
			true
			);

		registerBuiltInFunction(
			"Text",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1 && arguments.size() != 2 && arguments.size() != 3 && arguments.size() != 4)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			Val strVal = pEnv->expand(LRValue(arguments[0]), info);
			const auto heightOpt = 2 <= arguments.size() ? boost::optional<const Val&>(pEnv->expand(LRValue(arguments[1]), info)) : boost::none;
			const auto baseLineOpt = 3 <= arguments.size() ? boost::optional<const Val&>(pEnv->expand(LRValue(arguments[2]), info)) : boost::none;
			const auto fontNameOpt = 4 <= arguments.size() ? boost::optional<const Val&>(pEnv->expand(LRValue(arguments[3]), info)) : boost::none;

			if (!IsType<CharString>(strVal))
			{
				std::stringstream ss;
				ValuePrinter2 printer(m_weakThis.lock(), ss, 0);
				boost::apply_visitor(printer, strVal);
				CharString printedStr(AsUtf32(ss.str()));
				strVal = printedStr;
			}
			if (heightOpt && !IsNum(heightOpt.get()))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}
			if (baseLineOpt && !IsType<Record>(baseLineOpt.get()))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}
			if (fontNameOpt && !IsType<CharString>(fontNameOpt.get()))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			if (4 <= arguments.size())
			{
				return BuildText(
					As<CharString>(strVal), 
					AsDouble(heightOpt.get()),
					As<PackedRecord>(As<Record>(baseLineOpt.get()).packed(*this)), 
					As<CharString>(fontNameOpt.get())
				).unpacked(*this);
			}
			else if (3 <= arguments.size())
			{
				return BuildText(As<CharString>(strVal), AsDouble(heightOpt.get()), As<PackedRecord>(As<Record>(baseLineOpt.get()).packed(*this))).unpacked(*this);
			}
			else if (2 <= arguments.size())
			{
				return BuildText(As<CharString>(strVal), AsDouble(heightOpt.get())).unpacked(*this);
			}
			return BuildText(As<CharString>(strVal)).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"OffsetPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& arg1 = pEnv->expand(LRValue(arguments[0]), info);
			const Val& arg2 = pEnv->expand(LRValue(arguments[1]), info);

			if (!IsType<Record>(arg1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}
			if (!IsType<double>(arg2) && !IsType<int>(arg2))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetOffsetPath(As<PackedRecord>(As<Record>(arg1).packed(*this)), AsDouble(arg2)).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"SubPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 3)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& arg1 = pEnv->expand(LRValue(arguments[0]), info);
			const Val& arg2 = pEnv->expand(LRValue(arguments[1]), info);
			const Val& arg3 = pEnv->expand(LRValue(arguments[2]), info);

			if (!IsType<Record>(arg1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}
			if (!IsNum(arg2) || !IsNum(arg3))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetSubPath(As<PackedRecord>(As<Record>(arg1).packed(*this)), AsDouble(arg2), AsDouble(arg3)).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"FunctionPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 4)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& func = pEnv->expand(LRValue(arguments[0]), info);
			const Val& beginX = pEnv->expand(LRValue(arguments[1]), info);
			const Val& endX = pEnv->expand(LRValue(arguments[2]), info);
			const Val& numOfPoints = pEnv->expand(LRValue(arguments[3]), info);

			if (!IsType<FuncVal>(func) || !IsNum(beginX) || !IsNum(endX) || !IsType<int>(numOfPoints))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetFunctionPath(m_weakThis.lock(), info, As<FuncVal>(func), AsDouble(beginX), AsDouble(endX), As<int>(numOfPoints)).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"ShapeOuterPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& arg1 = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(arg1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}
			
			return GetShapeOuterPaths(As<PackedRecord>(As<Record>(arg1).packed(*this)), pEnv).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"ShapeInnerPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& arg1 = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(arg1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetShapeInnerPaths(As<PackedRecord>(As<Record>(arg1).packed(*this)), pEnv).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"ShapePath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& arg1 = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(arg1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetShapePaths(As<PackedRecord>(As<Record>(arg1).packed(*this)), pEnv).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"Touch",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return ShapeTouch(Packed(pEnv->expand(LRValue(arguments[0]), info), *pEnv), Packed(pEnv->expand(LRValue(arguments[1]), info), *pEnv), pEnv);
		},
			false
			);

		registerBuiltInFunction(
			"Near",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return ShapeNear(Packed(pEnv->expand(LRValue(arguments[0]), info), *pEnv), Packed(pEnv->expand(LRValue(arguments[1]), info), *pEnv), pEnv);
		},
			false
			);

		registerBuiltInFunction(
			"Avoid",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return ShapeAvoid(Packed(pEnv->expand(LRValue(arguments[0]), info), *pEnv), Packed(pEnv->expand(LRValue(arguments[1]), info), *pEnv), pEnv);
		},
			false
			);

		registerBuiltInFunction(
			"Diff",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return Unpacked(ShapeDiff(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), Packed(pEnv->expand(LRValue(arguments[1]), info), *this), pEnv), *this);
		},
			true
			);

		registerBuiltInFunction(
			"Union",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return Unpacked(ShapeUnion(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), Packed(pEnv->expand(LRValue(arguments[1]), info), *this), pEnv), *this);
		},
			true
			);

		registerBuiltInFunction(
			"Intersect",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return Unpacked(ShapeIntersect(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), Packed(pEnv->expand(LRValue(arguments[1]), info), *this), pEnv), *this);
		},
			true
			);

		registerBuiltInFunction(
			"SymDiff",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return Unpacked(ShapeSymDiff(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), Packed(pEnv->expand(LRValue(arguments[1]), info), *this), pEnv), *this);
		},
			true
			);

		registerBuiltInFunction(
			"Buffer",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return Unpacked(ShapeBuffer(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), Packed(pEnv->expand(LRValue(arguments[1]), info), *this), pEnv), *this);
		},
			true
			);

		registerBuiltInFunction(
			"DeformShapeByPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);
			const Val& path = pEnv->expand(LRValue(arguments[1]), info);

			if (!IsType<Record>(shape) || !IsType<Record>(path))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetBaseLineDeformedShape(As<PackedRecord>(As<Record>(shape).packed(*this)), As<PackedRecord>(As<Record>(path).packed(*this)), pEnv).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"DeformShapeByPath2",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 4)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);
			const Val& path = pEnv->expand(LRValue(arguments[1]), info);
			const Val& p0 = pEnv->expand(LRValue(arguments[2]), info);
			const Val& p1 = pEnv->expand(LRValue(arguments[3]), info);

			if (!IsType<Record>(shape) || !IsType<Record>(path) || !IsType<Record>(p0) || !IsType<Record>(p1))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetDeformedPathShape(
				As<PackedRecord>(As<Record>(shape).packed(*this)),
				As<PackedRecord>(As<Record>(p0).packed(*this)),
				As<PackedRecord>(As<Record>(p1).packed(*this)),
				As<PackedRecord>(As<Record>(path).packed(*this)), pEnv).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"BezierPath",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 5)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& p0 = pEnv->expand(LRValue(arguments[0]), info);
			const Val& n0 = pEnv->expand(LRValue(arguments[1]), info);
			const Val& p1 = pEnv->expand(LRValue(arguments[2]), info);
			const Val& n1 = pEnv->expand(LRValue(arguments[3]), info);
			const Val& num = pEnv->expand(LRValue(arguments[4]), info);

			if (!IsType<Record>(p0) || !IsType<Record>(n0) || !IsType<Record>(p1) || !IsType<Record>(n1) || !IsType<int>(num))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetBezierPath(
				As<PackedRecord>(As<Record>(p0).packed(*this)),
				As<PackedRecord>(As<Record>(n0).packed(*this)),
				As<PackedRecord>(As<Record>(p1).packed(*this)),
				As<PackedRecord>(As<Record>(n1).packed(*this)),
				As<int>(num)
			).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"CubicBezier",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 5)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& p0 = pEnv->expand(LRValue(arguments[0]), info);
			const Val& p1 = pEnv->expand(LRValue(arguments[1]), info);
			const Val& p2 = pEnv->expand(LRValue(arguments[2]), info);
			const Val& p3 = pEnv->expand(LRValue(arguments[3]), info);
			const Val& num = pEnv->expand(LRValue(arguments[4]), info);

			if (!IsType<Record>(p0) || !IsType<Record>(p1) || !IsType<Record>(p2) || !IsType<Record>(p3) || !IsType<int>(num))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetCubicBezier(
				As<PackedRecord>(As<Record>(p0).packed(*this)),
				As<PackedRecord>(As<Record>(p1).packed(*this)),
				As<PackedRecord>(As<Record>(p2).packed(*this)),
				As<PackedRecord>(As<Record>(p3).packed(*this)),
				As<int>(num)
			).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"SubDiv",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);
			const Val& num = pEnv->expand(LRValue(arguments[1]), info);

			if (!IsType<Record>(shape) || !IsType<int>(num))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeSubDiv(
				As<PackedRecord>(As<Record>(shape).packed(*this)),
				As<int>(num), pEnv
			).unpacked(*this);
		},
			true
			);

		registerBuiltInFunction(
			"Area",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return ShapeArea(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), pEnv);
		},
			true
			);

		registerBuiltInFunction(
			"Distance",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return ShapeDistance(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), Packed(pEnv->expand(LRValue(arguments[1]), info), *this), pEnv);
		},
			false
			);

		registerBuiltInFunction(
			"ClosestPoints",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			return ShapeClosestPoints(Packed(pEnv->expand(LRValue(arguments[0]), info), *this), Packed(pEnv->expand(LRValue(arguments[1]), info), *this), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignH",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignH(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignHTop",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignHTop(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignHBottom",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignHBottom(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignHCenter",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignHCenter(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignV",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignV(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignVLeft",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignVLeft(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignVRight",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignVRight(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"AlignVCenter",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<List>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return AlignVCenter(As<PackedList>(As<List>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"Left",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeLeft(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"Right",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeRight(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"Top",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeTop(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"Bottom",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeBottom(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"TopLeft",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeTopLeft(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"TopRight",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeTopRight(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"BottomLeft",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeBottomLeft(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"BottomRight",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return ShapeBottomRight(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"BoundingBox",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetBoundingBox(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"ConvexHull",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetConvexHull(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"GlobalShape",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetGlobalShape(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"TransformShape",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 4)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& pos = pEnv->expand(LRValue(arguments[0]), info);
			const Val& scale = pEnv->expand(LRValue(arguments[1]), info);
			const Val& angle = pEnv->expand(LRValue(arguments[2]), info);
			const Val& shape = pEnv->expand(LRValue(arguments[3]), info);

			if (!IsType<Record>(shape) || !IsType<Record>(pos) || !IsType<Record>(scale) || !IsNum(angle))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetTransformedShape(
				As<PackedRecord>(As<Record>(shape).packed(*this)),
				As<PackedRecord>(As<Record>(pos).packed(*this)),
				As<PackedRecord>(As<Record>(scale).packed(*this)),
				AsDouble(angle), pEnv
			).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"GetPolygon",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			const Val& shape = pEnv->expand(LRValue(arguments[0]), info);

			if (!IsType<Record>(shape))
			{
				CGL_ErrorNode(info, "引数の型が正しくありません");
			}

			return GetPolygon(As<PackedRecord>(As<Record>(shape).packed(*this)), pEnv).unpacked(*this);
		},
			false
			);

		registerBuiltInFunction(
			"GC",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 0)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			garbageCollect();

			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"EnableGC",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_ErrorNode(info, "引数の数が正しくありません");
			}

			if (auto opt = AsOpt<bool>(pEnv->expand(LRValue(arguments[0]), info)))
			{
				m_automaticGC = opt.get();
			}

			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"Error",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() == 0)
			{
				CGL_ErrorNode(info, "Error() called");
			}
			else
			{
				std::stringstream ss;
				ss << "Error() called: ";
				printVal2(pEnv->expand(LRValue(arguments[0]), info), m_weakThis.lock(), ss);
				CGL_ErrorNode(info, ss.str());
			}

			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"Assert",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments, const LocationInfo& info)->Val
		{
			if (arguments.size() == 0 || 3 <= arguments.size())
			{
				CGL_ErrorNode(info, "Assert() called");
			}

			if (arguments.size() == 1)
			{
				if (EqualFunc(pEnv->expand(LRValue(arguments[0]), info), true, *this))
				{
					return 0;
				}
				else
				{
					CGL_ErrorNode(info, "Assertion failed");
				}
			}
			else
			{
				if (EqualFunc(pEnv->expand(LRValue(arguments[0]), info), true, *this))
				{
					return 0;
				}
				else
				{
					std::stringstream ss;
					ss << "Assertion failed: ";
					printVal2(pEnv->expand(LRValue(arguments[1]), info), m_weakThis.lock(), ss);
					CGL_ErrorNode(info, ss.str());
				}
			}

			return 0;
		},
			false
			);
	}

	void Context::changeAddress(Address addressFrom, Address addressTo)
	{
		//CGL_DBG1(addressFrom.toString() + " -> " + addressTo.toString());

		if (addressFrom == addressTo)
		{
			return;
		}
		
		/*
		for (auto it = m_addressRefMap.find(addressFrom); it != m_addressRefMap.end(); it = m_addressRefMap.find(addressFrom))
		{
			m_addressRefMap.insert({ addressTo, it->second });
			m_refAddressMap[it->second] = addressTo;
			m_addressRefMap.erase(it);
		}
		*/

		//アクセッサの全てのアドレスをm_addressRefMapに紐付けているので、一つのReferenceに対して複数回処理が走るのを防止するために処理済みの参照を記録しておく
		std::unordered_set<Reference> processedList;

		for (auto it = m_addressRefMap.find(addressFrom); it != m_addressRefMap.end(); it = m_addressRefMap.find(addressFrom))
		{
			if (processedList.find(it->second) != processedList.end())
			{
				m_addressRefMap.insert({ addressTo, it->second });
				m_addressRefMap.erase(it);
				continue;
			}

			{
				DeepReference& deepReference = m_refAddressMap[it->second];

				auto& head = deepReference.head;
				auto& tail = deepReference.tail;

				Address address;
				size_t startIndex;

				if (head == addressFrom)
				{
					address = addressTo;
					head = address;
					startIndex = 0;
				}
				else
				{
					for (size_t i = 0; i < tail.size(); ++i)
					{
						if (tail[i].second == addressFrom)
						{
							address = addressTo;
							tail[i].second = address;
							startIndex = i + 1;
						}
					}
				}

				for (size_t i = startIndex; i < tail.size(); ++i)
				{
					boost::optional<const Val&> objOpt = expandOpt(LRValue(address));
					if (!objOpt)
					{
						CGL_Error("参照エラー");
					}

					const Val& objRef = objOpt.get();

					if (IsType<int>(tail[i].first))
					{
						if (!IsType<List>(objRef))
						{
							CGL_Error("参照が作られた変数に異なる型の値を代入しようとしました");
						}

						const int index = As<int>(tail[i].first);

						const List& list = As<List>(objRef);
						address = list.data[index];

						tail[i].second = address;
					}
					else
					{
						if (!IsType<Record>(objRef))
						{
							CGL_Error("参照が作られた変数に異なる型の値を代入しようとしました");
						}

						const std::string name = As<std::string>(tail[i].first);
						
						const Record& record = As<Record>(objRef);
						address = record.values.at(name);

						tail[i].second = address;
					}
				}
			}
			
			processedList.emplace(it->second);

			m_addressRefMap.insert({ addressTo, it->second });
			m_addressRefMap.erase(it);
		}
	}

	void Context::replaceGlobalContextAddress(Address addressFrom, Address addressTo)
	{
		/*m_localEnvStack;

		for (LocalContext& localContext : m_localEnvStack)
		{
			for (Scope& scope: localContext)
			{
				scope.temporaryAddresses;
			}
		}*/

		//TODO: GCと同じ要領で環境から参照可能な全てのアドレスを参照し、該当アドレスを書き換える
	}

	void Context::garbageCollect(bool force)
	{
		if (!m_automaticGC)
		{
			return;
		}

		static int count = 0;
		//std::cout << "garbageCollect(" << count << ")" << std::endl;
		//printContext(std::cout);

		//printContext(std::cout);
		CGL_DBG1("GC begin");
		std::unordered_set<Address> referenceableAddresses;

		const auto isReachable = [&](const Address address)
		{
			return referenceableAddresses.find(address) != referenceableAddresses.end();
		};

		{
			std::unordered_set<Address> addressesDelta;
			//std::cout << "Whole Local Env Stack Size: " << m_localEnvStack.size() << std::endl;
			for (const auto& env : m_localEnvStack)
			{
				int scopeCount = 0;

				//std::cout << "Current Scope Depth: " << env.size() << ", ";
				for (auto scopeIt = env.rbegin(); scopeIt != env.rend(); ++scopeIt)
				{
					const int varCount = scopeIt->variables.size() + scopeIt->temporaryAddresses.size();
					for (const auto& var : scopeIt->variables)
					{
						const Address address = var.second;
						if (!isReachable(address))
						{
							addressesDelta.emplace(address);
						}
					}

					for (const Address address : scopeIt->temporaryAddresses)
					{
						if (!isReachable(address))
						{
							addressesDelta.emplace(address);
						}
					}

					//std::cout << "var(" << varCount << "), ";
				}

				//std::cout << "\n";
			}

			GetReachableAddressesFrom(referenceableAddresses, addressesDelta, *this);
		}

		{
			for (size_t i = 0; i < currentRecords.size(); ++i)
			{
				Record& record = currentRecords[i];
				Val evaluated = record;
				std::unordered_set<Address> addressesDelta;
				CheckValue(evaluated, *this, referenceableAddresses, addressesDelta, RegionVariable::Attribute::Other);

				GetReachableAddressesFrom(referenceableAddresses, addressesDelta, *this);
			}

			if (temporaryRecord)
			{
				std::unordered_set<Address> addressesDelta;
				CheckValue(temporaryRecord.get(), *this, referenceableAddresses, addressesDelta, RegionVariable::Attribute::Other);

				GetReachableAddressesFrom(referenceableAddresses, addressesDelta, *this);
			}
		}

		for (const auto& keyval : m_functions)
		{
			referenceableAddresses.emplace(keyval.first);
		}

		const size_t prevGC = m_values.size();

		/*CGL_DBG1("referenceableAddresses: ");
		for (const Address add : referenceableAddresses)
		{
			std::cout << "Address(" << add.toString() << ")\n";
		}*/

		m_values.gc(referenceableAddresses);

		const size_t postGC = m_values.size();

		CGL_DBG1("GC end");

		++count;
		//std::cout << "GC: ValueSize(" << prevGC << " -> " << postGC << ")\n";
	}

	class ValueAccessorSearcher : public boost::static_visitor<bool>
	{
	public:
		ValueAccessorSearcher(const Context& context, const Address searchAddress) :
			context(context),
			searchAddress(searchAddress)
		{}

		const Context& context;
		const Address searchAddress;
		std::stack<std::pair<std::string, Address>> currentSearchAddresses;

		bool operator()(bool node) { return false; }

		bool operator()(int node) { return false; }

		bool operator()(double node) { return false; }

		bool operator()(const CharString& node) { return false; }

		bool operator()(const KeyValue& node) { return false; }

		bool operator()(const FuncVal& node) { return false; }

		bool operator()(const Jump& node) { return false; }

		bool operator()(const List& node)
		{
			for (size_t i = 0; i < node.data.size(); ++i)
			{
				const Address address = node.data[i];
				if (address == searchAddress)
				{
					std::stringstream ss;
					ss << "[" << i << "]";
					currentSearchAddresses.push(std::pair<std::string, Address>(ss.str(), address));
					return true;
				}

				if (auto opt = context.expandOpt(LRValue(address)))
				{
					std::stringstream ss;
					ss << "[" << i << "]";
					currentSearchAddresses.push(std::pair<std::string, Address>(ss.str(), address));
					if (boost::apply_visitor(*this, opt.get()))
					{
						return true;
					}
					currentSearchAddresses.pop();
				}
			}

			return false;
		}

		bool operator()(const Record& node)
		{
			for (const auto& keyval : node.values)
			{
				const Address address = keyval.second;
				if (keyval.second == searchAddress)
				{
					std::stringstream ss;
					ss << "." << keyval.first;
					currentSearchAddresses.push(std::pair<std::string, Address>(ss.str(), address));
					return true;
				}

				if (auto opt = context.expandOpt(LRValue(address)))
				{
					std::stringstream ss;
					ss << "." << keyval.first;
					currentSearchAddresses.push(std::pair<std::string, Address>(ss.str(), address));
					if (boost::apply_visitor(*this, opt.get()))
					{
						return true;
					}
					currentSearchAddresses.pop();
				}
			}

			return false;
		}
	};

	std::string Context::makeLabel(const Address& address)const
	{
		for (size_t i = 0; i < currentRecords.size(); ++i)
		{
			Record& record = currentRecords[i];
			Val evaluated = record;
			ValueAccessorSearcher searcher(*this, address);
			if (boost::apply_visitor(searcher, evaluated))
			{
				std::string str;
				while (!searcher.currentSearchAddresses.empty())
				{
					str = searcher.currentSearchAddresses.top().first + str;
					searcher.currentSearchAddresses.pop();
				}
				return str;
			}
		}

		if (temporaryRecord)
		{
			Val evaluated = temporaryRecord.get();
			ValueAccessorSearcher searcher(*this, address);
			if (boost::apply_visitor(searcher, evaluated))
			{
				std::string str;
				while (!searcher.currentSearchAddresses.empty())
				{
					str = searcher.currentSearchAddresses.top().first + str;
					searcher.currentSearchAddresses.pop();
				}
				return str;
			}
		}

		for (const auto& env : m_localEnvStack)
		{
			for (auto scopeIt = env.rbegin(); scopeIt != env.rend(); ++scopeIt)
			{
				auto it = std::find_if(scopeIt->variables.begin(), scopeIt->variables.end(),
					[&](const Scope::VariableMap::value_type& value)
				{
					return value.second == address;
				});

				if (it != scopeIt->variables.end())
				{
					return it->first;
				}

				for (const auto& nameVal : scopeIt->variables)
				{
					if (auto opt = expandOpt(LRValue(nameVal.second)))
					{
						ValueAccessorSearcher searcher(*this, address);
						if (boost::apply_visitor(searcher, opt.get()))
						{
							std::string str;
							while (!searcher.currentSearchAddresses.empty())
							{
								str = searcher.currentSearchAddresses.top().first + str;
								searcher.currentSearchAddresses.pop();
							}
							return nameVal.first + str;
						}
					}
				}
			}
		}

		return "(unknown)";
	}

	std::shared_ptr<Context> Context::cloneContext()
	{
		std::shared_ptr<Context> inst = Context::Make();
		inst->currentRecords = currentRecords;
		inst->temporaryRecord = temporaryRecord;
		inst->m_functions = m_functions;
		inst->m_plateausFunctions = m_plateausFunctions;
		inst->m_refAddressMap = m_refAddressMap;
		inst->m_addressRefMap = m_addressRefMap;
		inst->m_values = m_values.clone();
		inst->m_localEnvStack = m_localEnvStack;
		inst->m_referenceID = m_referenceID;
		inst->m_lastGCValueSize = m_lastGCValueSize;
		inst->m_automaticExtendMode = m_automaticExtendMode;
		inst->m_automaticGC = m_automaticGC;
		inst->m_solveTimeLimit = m_solveTimeLimit;
		inst->m_dist = m_dist;
		inst->m_random = m_random;
		inst->m_weakThis = inst;

		return inst;
	}
}
