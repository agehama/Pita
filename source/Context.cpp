#include <stack>
#include <Pita/Context.hpp>
#include <Pita/Evaluator.hpp>
#include <Pita/Geometry.hpp>
#include <Pita/Printer.hpp>
#include <Pita/BinaryEvaluator.hpp>

namespace cgl
{
	Address Context::makeFuncVal(std::shared_ptr<Context> pEnv, const std::vector<Identifier>& arguments, const Expr& expr)
	{
		std::set<std::string> functionArguments;
		for (const auto& arg : arguments)
		{
			functionArguments.insert(arg);
		}

		ClosureMaker maker(pEnv, functionArguments);
		const Expr closedFuncExpr = boost::apply_visitor(maker, expr);

		FuncVal funcVal(arguments, closedFuncExpr);
		return makeTemporaryValue(funcVal);
		//Address address = makeTemporaryValue(funcVal);
		//return address;
	}

	void Context::registerBuiltInFunction(const std::string& name, const BuiltInFunction& function, bool isPlateausFunction)
	{
		//m_valuesにFuncVal追加
		//m_functionsにfunction追加
		//m_scopeにname->FuncVal追加

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

	Evaluated Context::callBuiltInFunction(Address functionAddress, const std::vector<Address>& arguments)
	{
		if (std::shared_ptr<Context> pEnv = m_weakThis.lock())
		{
			return m_functions[functionAddress](pEnv, arguments);
		}
			
		CGL_Error("ここは通らないはず");
		return 0;
	}

	bool Context::isPlateausBuiltInFunction(Address functionAddress)
	{
		return m_plateausFunctions.find(functionAddress) != m_plateausFunctions.end();
	}

	const Evaluated& Context::expand(const LRValue& lrvalue)const
	{
		if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}
				
			CGL_Error(std::string("reference error: Address(") + lrvalue.toString() + ")");
		}

		return lrvalue.evaluated();
	}

	Evaluated& Context::mutableExpand(LRValue& lrvalue)
	{
		if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}

			CGL_Error(std::string("reference error: Address(") + lrvalue.toString() + ")");
		}

		return lrvalue.mutableEvaluated();
	}

	boost::optional<const Evaluated&> Context::expandOpt(const LRValue& lrvalue)const
	{
		if (lrvalue.isLValue())
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

	boost::optional<Evaluated&> Context::mutableExpandOpt(LRValue& lrvalue)
	{
		if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}

			return boost::none;
		}

		return lrvalue.mutableEvaluated();
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
	std::vector<Address> Context::expandReferences(Address address)
	{
		std::vector<Address> result;
		if (auto sharedThis = m_weakThis.lock())
		{
			const auto addElementRec = [&](auto rec, Address address)->void
			{
				const Evaluated value = sharedThis->expand(address);

				//追跡対象の変数にたどり着いたらそれを参照するアドレスを出力に追加
				if (IsType<int>(value) || IsType<double>(value) /*|| IsType<bool>(value)*/)//TODO:boolは将来的に対応
				{
					result.push_back(address);
				}
				else if (IsType<List>(value))
				{
					for (Address elemAddress : As<List>(value).data)
					{
						rec(rec, elemAddress);
					}
				}
				else if (IsType<Record>(value))
				{
					for (const auto& elem : As<Record>(value).values)
					{
						rec(rec, elem.second);
					}
				}
				//それ以外のデータは特に捕捉しない
				//TODO:最終的に int や double 以外のデータへの参照は持つことにするか？
			};

			const auto addElement = [&](const Address address)
			{
				addElementRec(addElementRec, address);
			};

			addElement(address);
		}

		return result;
	}

	std::vector<Address> Context::expandReferences2(const Accessor & accessor)
	{
		if (auto sharedThis = m_weakThis.lock())
		{
			Eval evaluator(sharedThis);

			std::array<std::vector<Address>, 2> addressBuffer;
			unsigned int currentWriteIndex = 0;
			const auto nextIndex = [&]() {return (currentWriteIndex + 1) & 1; };
			const auto flipIndex = [&]() {currentWriteIndex = nextIndex(); };

			const auto writeBuffer = [&]()->std::vector<Address>& {return addressBuffer[currentWriteIndex]; };
			const auto readBuffer = [&]()->std::vector<Address>& {return addressBuffer[nextIndex()]; };

			LRValue headValue = boost::apply_visitor(evaluator, accessor.head);
			if (headValue.isLValue())
			{
				writeBuffer().push_back(headValue.address(*this));
			}
			else
			{
				Evaluated evaluated = headValue.evaluated();
				if (auto opt = AsOpt<Record>(evaluated))
				{
					writeBuffer().push_back(makeTemporaryValue(opt.value()));
				}
				else if (auto opt = AsOpt<List>(evaluated))
				{
					writeBuffer().push_back(makeTemporaryValue(opt.value()));
				}
				else if (auto opt = AsOpt<FuncVal>(evaluated))
				{
					writeBuffer().push_back(makeTemporaryValue(opt.value()));
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
					const Address address = readBuffer()[i];

					//std::cout << "Address: " << address.toString() << std::endl;;

					boost::optional<const Evaluated&> objOpt = expandOpt(address);
					if (!objOpt)
					{
						CGL_Error("参照エラー");
					}

					const Evaluated& objRef = objOpt.value();

					if (auto listAccessOpt = AsOpt<ListAccess>(access))
					{
						if (!IsType<List>(objRef))
						{
							CGL_Error("オブジェクトがリストでない");
						}

						const List& list = As<const List&>(objRef);

						if (listAccessOpt.value().isArbitrary)
						{
							const auto& allIndices = list.data;

							writeBuffer().insert(writeBuffer().end(), allIndices.begin(), allIndices.end());
						}
						else
						{
							Evaluated value = expand(boost::apply_visitor(evaluator, listAccessOpt.value().index));

							if (auto indexOpt = AsOpt<int>(value))
							{
								//address = list.get(indexOpt.value());
								writeBuffer().push_back(list.get(indexOpt.value()));
								//std::cout << "List[" << indexOpt.value() << "]" << std::endl;
							}
							else
							{
								CGL_Error("list[index] の index が int 型でない");
							}
						}
					}
					else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
					{
						if (!IsType<Record>(objRef))
						{
							CGL_Error("オブジェクトがレコードでない");
						}

						const Record& record = As<const Record&>(objRef);
						auto it = record.values.find(recordAccessOpt.value().name);
						if (it == record.values.end())
						{
							CGL_Error("指定された識別子がレコード中に存在しない");
						}

						//address = it->second;
						writeBuffer().push_back(it->second);
					}
					else
					{
						auto funcAccess = As<FunctionAccess>(access);

						if (!IsType<FuncVal>(objRef))
						{
							CGL_Error("オブジェクトが関数でない");
						}

						const FuncVal& function = As<const FuncVal&>(objRef);

						std::vector<Address> args;
						for (const auto& expr : funcAccess.actualArguments)
						{
							const LRValue currentArgument = expand(boost::apply_visitor(evaluator, expr));
							if (currentArgument.isLValue())
							{
								args.push_back(currentArgument.address(*this));
							}
							else
							{
								args.push_back(makeTemporaryValue(currentArgument.evaluated()));
							}
						}

						const Evaluated returnedValue = expand(evaluator.callFunction(function, args));
						//address = makeTemporaryValue(returnedValue);
						writeBuffer().push_back(makeTemporaryValue(returnedValue));
					}
				}
			}

			flipIndex();

			std::vector<Address> result;
			for (const Address address : readBuffer())
			{
				const auto expanded = expandReferences(address);
				result.insert(result.end(), expanded.begin(), expanded.end());
			}

			return result;
		}

		CGL_Error("shared this does not exist.");
		return {};
	}

	Reference Context::bindReference(Address address)
	{
		++m_referenceID;

		Reference reference(m_referenceID);

		m_refAddressMap[reference] = address;
		m_addressRefMap.insert({ address, reference });

		return reference;
	}

	Address Context::getReference(Reference reference)const
	{
		auto it = m_refAddressMap.find(reference);
		return it->second;
		//return m_refAddressMap[reference];
	}

	void Context::cloneReference(const std::unordered_map<Address, Address>& replaceMap)
	{
		for (const auto& oldNew : replaceMap)
		{
			oldNew.first;
			oldNew.second;
		}
	}

	void Context::bindValueID(const std::string& name, const Address ID)
	{
		for (auto scopeIt = localEnv().rbegin(); scopeIt != localEnv().rend(); ++scopeIt)
		{
			auto valIt = scopeIt->find(name);
			if (valIt != scopeIt->end())
			{
				changeAddress(valIt->second, ID);
				valIt->second = ID;
				return;
			}
		}

		localEnv().back()[name] = ID;
	}

#ifdef CGL_EnableLogOutput
	void Context::printContext(bool flag)const
	{
		if (!flag)
		{
			return;
		}

		std::ostream& os = ofs;

		os << "Print Context Begin:\n";

		os << "Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			os << keyval.first.toString() << " : ";

			printEvaluated(val, m_weakThis.lock(), os);
		}

		os << "References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			os << "Depth : " << d << "\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names)
			{
				os << keyval.first << " : " << keyval.second.toString() << "\n";
			}
		}

		os << "Print Context End:\n";
	}
#else
	void Context::printContext(bool flag)const {}
#endif

	void Context::printContext(std::ostream& os)const
	{
		os << "Print Context Begin:\n";

		os << "Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			os << keyval.first.toString() << " : ";

			printEvaluated(val, m_weakThis.lock(), os);
		}

		os << "References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			os << "Depth : " << d << "\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names)
			{
				os << keyval.first << " : " << keyval.second.toString() << "\n";
			}
		}

		os << "Print Context End:\n";
	}

	void Context::assignToAccessor(const Accessor& accessor, const LRValue& newValue)
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

				boost::optional<Evaluated&> objOpt = mutableExpandOpt(address);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}

				Evaluated& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					Evaluated indexValue = expand(boost::apply_visitor(evaluator, listAccessOpt.value().index));

					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					List& list = As<List&>(objRef);

					if (auto indexOpt = AsOpt<int>(indexValue))
					{
						const int index = indexOpt.value();
						if (isLastElement)
						{
							const Address oldAddress = list.data[index];
							const Address newAddress = newValue.isLValue() ? newValue.address(*this) : makeTemporaryValue(newValue.evaluated());
							changeAddress(oldAddress, newAddress);
							list.data[index] = newAddress;
						}
						else
						{
							address = list.get(index);
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

					Record& record = As<Record&>(objRef);
					auto it = record.values.find(recordAccessOpt.value().name);
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
						address = it->second;
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

	//値を作って返す（変数で束縛されないものはGCが走ったら即座に消される）
	//式の評価途中でGCは走らないようにするべきか？
	Address Context::makeTemporaryValue(const Evaluated& value)
	{
		const Address address = m_values.add(value);

		//関数はスコープを抜ける時に定義式中の変数が解放されないか監視する必要があるのでIDを保存しておく
		/*if (IsType<FuncVal>(value))
		{
			m_funcValIDs.push_back(address);
		}*/

		return address;
	}

	Address Context::findAddress(const std::string& name)const
	{
		for (auto scopeIt = localEnv().rbegin(); scopeIt != localEnv().rend(); ++scopeIt)
		{
			auto variableIt = scopeIt->find(name);
			if (variableIt != scopeIt->end())
			{
				return variableIt->second;
			}
		}

		return Address::Null();
	}

	void Context::initialize()
	{
		registerBuiltInFunction(
			"print",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			printEvaluated(pEnv->expand(arguments[0]), pEnv, std::cout, 0);
			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"sin",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return Sin(pEnv->expand(arguments[0]));
		},
			false
			);

		registerBuiltInFunction(
			"cos",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return Cos(pEnv->expand(arguments[0]));
		},
			false
			);

		registerBuiltInFunction(
			"diff",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 2)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return ShapeDifference(pEnv->expand(arguments[0]), pEnv->expand(arguments[1]), m_weakThis.lock());
		},
			true
			);

		registerBuiltInFunction(
			"buffer",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 2)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return ShapeBuffer(pEnv->expand(arguments[0]), pEnv->expand(arguments[1]), m_weakThis.lock());
		},
			true
			);

		registerBuiltInFunction(
			"area",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return ShapeArea(pEnv->expand(arguments[0]), m_weakThis.lock());
		},
			true
			);

		registerBuiltInFunction(
			"DefaultFontString",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			const Evaluated evaluated = pEnv->expand(arguments[0]);
			if (!IsType<FuncVal>(evaluated))
			{
				CGL_Error("不正な式です");
			}

			const Expr expr = As<FuncVal>(evaluated).expr;
			if (!IsType<Identifier>(expr))
			{
				CGL_Error("不正な式です");
			}

			std::cout << "Fontの取得：" << static_cast<std::string>(As<Identifier>(expr)) << "\n";

			return GetDefaultFontString(static_cast<std::string>(As<Identifier>(expr)), m_weakThis.lock());
			//return ShapeArea(pEnv->expand(arguments[0]), m_weakThis.lock());
			//return 0;
		},
			false
			);
	}

	void Context::changeAddress(Address addressFrom, Address addressTo)
	{
		//m_refAddressMap[reference] = address;
		//m_addressRefMap.insert({ address, reference });
		//m_values[addressTo] = m_values[addressFrom];

		if (addressFrom == addressTo)
		{
			return;
		}

		for (auto it = m_addressRefMap.find(addressFrom); it != m_addressRefMap.end(); it = m_addressRefMap.find(addressFrom))
		{
			m_addressRefMap.insert({ addressTo, it->second });
			m_refAddressMap[it->second] = addressTo;
			m_addressRefMap.erase(it);
		}
	}

	void Context::garbageCollect()
	{

	}
}
