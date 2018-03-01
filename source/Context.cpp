#include <stack>
#include <unordered_set>
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

	Val Context::callBuiltInFunction(Address functionAddress, const std::vector<Address>& arguments)
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

	const Val& Context::expand(const LRValue& lrvalue)const
	{
		if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}
				
			CGL_Error(std::string("reference error: ") + lrvalue.toString());
		}

		return lrvalue.evaluated();
	}

	Val& Context::mutableExpand(LRValue& lrvalue)
	{
		if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address(*this));
			if (it != m_values.end())
			{
				return it->second;
			}

			CGL_Error(std::string("reference error: ") + lrvalue.toString());
		}

		return lrvalue.mutableVal();
	}

	boost::optional<const Val&> Context::expandOpt(const LRValue& lrvalue)const
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

	boost::optional<Val&> Context::mutableExpandOpt(LRValue& lrvalue)
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
	std::vector<Address> Context::expandReferences(Address address)
	{
		std::vector<Address> result;
		if (auto sharedThis = m_weakThis.lock())
		{
			const auto addElementRec = [&](auto rec, Address address)->void
			{
				const Val value = sharedThis->expand(address);

				//追跡対象の変数にたどり着いたらそれを参照するアドレスを出力に追加
				if (IsType<int>(value) || IsType<double>(value) /*|| IsType<bool>(value)*/)//TODO:boolへの対応？
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

	std::vector<std::pair<Address, VariableRange>> Context::expandReferences(Address address, boost::optional<const PackedVal&> variableRange)
	{
		std::vector<std::pair<Address, VariableRange>> result;
		if (auto sharedThis = m_weakThis.lock())
		{
			const auto addElementRec = [&](auto rec, Address address, boost::optional<const PackedVal&> currentVariableRange)->void
			{
				const Val value = sharedThis->expand(address);

				//追跡対象の変数にたどり着いたらそれを参照するアドレスを出力に追加
				if (IsType<int>(value) || IsType<double>(value) /*|| IsType<bool>(value)*/)//TODO:boolへの対応？
				{
					VariableRange range;
					if (currentVariableRange)
					{
						if (auto opt = AsOpt<PackedList>(currentVariableRange.value()))
						{
							const auto& rangeData = opt.value().data;
							if (rangeData.size() == 2)
							{
								range.minimum = AsDouble(rangeData[0].value);
								range.maximum = AsDouble(rangeData[1].value);
							}
							else CGL_Error("rangeは要素数が二つでなければなりません");
						}
						else CGL_Error("rangeの形が不正です");
					}
					result.emplace_back(address, range);
				}
				else if (IsType<List>(value))
				{
					if (currentVariableRange)
					{
						if (auto opt = AsOpt<PackedList>(currentVariableRange.value()))
						{
							const auto& rangeData = opt.value().data;
							const auto& list = As<List>(value).data;

							if (rangeData.size() != list.size())
								CGL_Error("rangeと変数の形が対応していません");

							for (size_t i = 0; i < list.size(); ++i)
							{
								rec(rec, list[i], rangeData[i].value);
							}
						}
						else CGL_Error("rangeの形が不正です");
					}
					else
					{
						for (Address elemAddress : As<List>(value).data)
						{
							rec(rec, elemAddress, boost::none);
						}
					}
				}
				else if (IsType<Record>(value))
				{
					if (currentVariableRange)
					{
						if (auto opt = AsOpt<PackedRecord>(currentVariableRange.value()))
						{
							const auto& rangeData = opt.value().values;
							const auto& record = As<Record>(value).values;

							for (const auto& elem : record)
							{
								const auto rangeIt = rangeData.find(elem.first);
								if (rangeIt == rangeData.end())
									CGL_Error("rangeと変数の形が対応していません");
								
								rec(rec, elem.second, rangeIt->second.value);
							}
						}
						else CGL_Error("rangeの形が不正です");
					}
					else
					{
						for (const auto& elem : As<Record>(value).values)
						{
							rec(rec, elem.second, boost::none);
						}
					}
				}
				//それ以外のデータは特に捕捉しない
				//TODO:最終的に int や double 以外のデータへの参照は持つことにするか？
			};

			const auto addElement = [&](const Address address, boost::optional<const PackedVal&> currentVariableRange)
			{
				addElementRec(addElementRec, address, currentVariableRange);
			};

			addElement(address, variableRange);
		}

		return result;
	}

	/*std::vector<Address> Context::expandReferences2(const Accessor & accessor)
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
				Val evaluated = headValue.evaluated();
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

					boost::optional<const Val&> objOpt = expandOpt(address);
					if (!objOpt)
					{
						CGL_Error("参照エラー");
					}

					const Val& objRef = objOpt.value();

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
							Val value = expand(boost::apply_visitor(evaluator, listAccessOpt.value().index));

							if (auto indexOpt = AsOpt<int>(value))
							{
								writeBuffer().push_back(list.get(indexOpt.value()));
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

						const Val returnedValue = expand(evaluator.callFunction(function, args));
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
	}*/

	std::vector<std::pair<Address, VariableRange>> Context::expandReferences2(const Accessor & accessor, boost::optional<const PackedVal&> rangeOpt)
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
				Val evaluated = headValue.evaluated();
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

					boost::optional<const Val&> objOpt = expandOpt(address);
					if (!objOpt)
					{
						CGL_Error("参照エラー");
					}

					const Val& objRef = objOpt.value();

					if (auto listAccessOpt = AsOpt<ListAccess>(access))
					{
						if (!IsType<List>(objRef))
						{
							CGL_Error("オブジェクトがリストでない");
						}

						const List& list = As<List>(objRef);

						if (listAccessOpt.value().isArbitrary)
						{
							const auto& allIndices = list.data;

							writeBuffer().insert(writeBuffer().end(), allIndices.begin(), allIndices.end());
						}
						else
						{
							Val value = expand(boost::apply_visitor(evaluator, listAccessOpt.value().index));

							if (auto indexOpt = AsOpt<int>(value))
							{
								writeBuffer().push_back(list.get(indexOpt.value()));
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

						const Record& record = As<Record>(objRef);
						auto it = record.values.find(recordAccessOpt.value().name);
						if (it == record.values.end())
						{
							CGL_Error("指定された識別子がレコード中に存在しない");
						}

						writeBuffer().push_back(it->second);
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

						const Val returnedValue = expand(evaluator.callFunction(function, args));
						writeBuffer().push_back(makeTemporaryValue(returnedValue));
					}
				}
			}

			flipIndex();

			std::vector<std::pair<Address, VariableRange>> result;
			for (const Address address : readBuffer())
			{
				//var での省略記法 list[*] を使った場合、readBuffer() には複数のアドレスが格納される
				//ここで、var( list[*].pos in range ) と書いた場合はこの range と各アドレスをどう結び付けるかが自明でない
				//とりあえず今の実装では全てのアドレスに対して同じ range を割り当てるようにしている
				const auto expanded = expandReferences(address, rangeOpt);
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

	Reference Context::bindReference(const Accessor& accessor)
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

			for (const auto& access : accessor.accesses)
			{
				boost::optional<const Val&> objOpt = pEnv->expandOpt(address);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}
				
				const Val& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					const List& list = As<List>(objRef);
					Val value = pEnv->expand(boost::apply_visitor(evaluator, listAccessOpt.value().index));
					if (auto indexOpt = AsOpt<int>(value))
					{
						address = list.get(indexOpt.value());
						deepReference.addList(indexOpt.value(), address);
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
					deepReference.addRecord(it->first, address);
				}
				else
				{
					CGL_Error("右辺値に対して参照演算子を適用しようとしました");
				}
			}

			++m_referenceID;

			Reference reference(m_referenceID);

			m_refAddressMap[reference] = deepReference;
			m_addressRefMap.insert({ deepReference.head, reference });
			for (const auto& keyAddress : deepReference.tail)
			{
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

			printVal(val, m_weakThis.lock(), os);
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

			printVal(val, m_weakThis.lock(), os);
		}

		os << "References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			os << "Depth : " << d << "\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names.variables)
			{
				os << "    " << keyval.first << " : " << keyval.second.toString() << "\n";
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

				boost::optional<Val&> objOpt = mutableExpandOpt(address);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}

				Val& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					Val indexValue = expand(boost::apply_visitor(evaluator, listAccessOpt.value().index));

					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					List& list = As<List>(objRef);
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

					Record& record = As<Record>(objRef);
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
	Address Context::makeTemporaryValue(const Val& value)
	{
		const Address address = m_values.add(value);

		localEnv().back().temporaryAddresses.push_back(address);

		const int thresholdGC = 20000;
		if (thresholdGC <= static_cast<int>(m_values.size()) - static_cast<int>(m_lastGCValueSize))
		{
			garbageCollect();
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

	void Context::initialize()
	{
		m_random.seed(1);

		registerBuiltInFunction(
			"print",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			printVal(pEnv->expand(arguments[0]), pEnv, std::cout, 0);
			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"size",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			const Val& value = pEnv->expand(arguments[0]);
			if (auto opt = AsOpt<List>(value))
			{
				return static_cast<int>(opt.value().data.size());
			}
			else if (auto opt = AsOpt<Record>(value))
			{
				return static_cast<int>(opt.value().values.size());
			}

			CGL_Error("不正な式です");
			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"cmaes",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
		{
			return Val(true);
		},
			true
			);

		registerBuiltInFunction(
			"sin",
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
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
			[](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
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
			"random",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
		{
			if (arguments.size() != 2)
			{
				CGL_Error("引数の数が正しくありません");
			}

			const double x0 = AsDouble(expand(arguments[0]));
			const double x1 = AsDouble(expand(arguments[1]));
			const double x01 = m_dist(m_random);
			return x0 + (x1 - x0)*x01;
		},
			false
			);

		registerBuiltInFunction(
			"diff",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
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
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
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
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
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
			"gc",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
		{
			if (arguments.size() != 0)
			{
				CGL_Error("引数の数が正しくありません");
			}

			garbageCollect();

			return 0;
		},
			false
			);

		registerBuiltInFunction(
			"enable_gc",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			if (auto opt = AsOpt<bool>(pEnv->expand(arguments[0])))
			{
				m_automaticGC = opt.value();
			}

			return 0;
		},
			false
			);
		;

		registerBuiltInFunction(
			"printContext",
			[&](std::shared_ptr<Context> pEnv, const std::vector<Address>& arguments)->Val
		{
			if (arguments.size() != 0)
			{
				CGL_Error("引数の数が正しくありません");
			}

			printContext(std::cout);

			return 0;
		},
			false
			);
	}

	void Context::changeAddress(Address addressFrom, Address addressTo)
	{
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
					boost::optional<const Val&> objOpt = expandOpt(address);
					if (!objOpt)
					{
						CGL_Error("参照エラー");
					}

					const Val& objRef = objOpt.value();

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

	void CheckExpr(const Expr& expr, const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet);
	void CheckValue(const Val& evaluated, const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet);

	class ExprAddressCheker : public boost::static_visitor<void>
	{
	public:
		ExprAddressCheker(const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet) :
			context(context),
			reachableAddressSet(reachableAddressSet),
			newAddressSet(newAddressSet)
		{}

		const Context& context;
		std::unordered_set<Address>& reachableAddressSet;
		std::unordered_set<Address>& newAddressSet;

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
				CheckValue(context.expand(LRValue(address)), context, reachableAddressSet, newAddressSet);
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

		void operator()(const SatReference& node) {}

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

		void operator()(const DefFunc& node) {/*boost::apply_visitor(*this, node.expr);*/ }

		void operator()(const If& node)
		{
			boost::apply_visitor(*this, node.cond_expr);
			boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				boost::apply_visitor(*this, node.else_expr.value());
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

		void operator()(const RecordInheritor& node)
		{
			boost::apply_visitor(*this, node.original);
			const Expr adderExpr = node.adder;
			boost::apply_visitor(*this, adderExpr);
		}

		void operator()(const Accessor& node)
		{
			boost::apply_visitor(*this, node.head);

			for (const auto& access : node.accesses)
			{
				if (auto listAccess = AsOpt<ListAccess>(access))
				{
					boost::apply_visitor(*this, listAccess.value().index);
				}
				else if (auto functionAccess = AsOpt<FunctionAccess>(access))
				{
					for (const auto& argument : functionAccess.value().actualArguments)
					{
						boost::apply_visitor(*this, argument);
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
			for (const auto& accessor : node.accessors)
			{
				Expr expr = accessor;
				boost::apply_visitor(*this, expr);
			}
			for (const auto& range : node.ranges)
			{
				boost::apply_visitor(*this, range);
			}
		}
	};

	class ValueAddressChecker : public boost::static_visitor<void>
	{
	public:
		ValueAddressChecker(const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet) :
			context(context),
			reachableAddressSet(reachableAddressSet),
			newAddressSet(newAddressSet)
		{}

		const Context& context;
		std::unordered_set<Address>& reachableAddressSet;
		std::unordered_set<Address>& newAddressSet;

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
				boost::apply_visitor(*this, context.expand(LRValue(address)));
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

		void operator()(const KeyValue& node) {}

		void operator()(const Record& node)
		{
			for (const auto& keyval : node.values)
			{
				update(keyval.second);
			}

			if (node.constraint)
			{
				CheckExpr(node.constraint.value(), context, reachableAddressSet, newAddressSet);
			}

			for (const auto& problem : node.problems)
			{
				if (problem.expr)
				{
					CheckExpr(problem.expr.value(), context, reachableAddressSet, newAddressSet);
				}
			}

			/*const auto& problem = node.problem;
			if (problem.candidateExpr)
			{
				CheckExpr(problem.candidateExpr.value(), context, reachableAddressSet, newAddressSet);
			}*/

			/*
			if (problem.expr)
			{
				CheckExpr(problem.expr.value(), context, reachableAddressSet, newAddressSet);
			}
			for (Address address : problem.refs)
			{
				update(address);
			}

			for (Address address : node.freeVariableRefs)
			{
				update(address);
			}
			for (const auto& accessor : node.freeVariables)
			{
				Expr expr = accessor;
				CheckExpr(expr, context, reachableAddressSet, newAddressSet);
			}
			*/
		}

		void operator()(const FuncVal& node)
		{
			if (!node.builtinFuncAddress)
			{
				CheckExpr(node.expr, context, reachableAddressSet, newAddressSet);
			}
		}

		void operator()(const Jump& node) {}
	};

	void CheckExpr(const Expr& expr, const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet)
	{
		ExprAddressCheker cheker(context, reachableAddressSet, newAddressSet);
		boost::apply_visitor(cheker, expr);
	}

	void CheckValue(const Val& evaluated, const Context& context, std::unordered_set<Address>& reachableAddressSet, std::unordered_set<Address>& newAddressSet)
	{
		ValueAddressChecker cheker(context, reachableAddressSet, newAddressSet);
		boost::apply_visitor(cheker, evaluated);
	}

	void Context::garbageCollect()
	{
		if (!m_automaticGC)
		{
			return;
		}

		std::unordered_set<Address> referenceableAddresses;

		const auto isReachable = [&](const Address address)
		{
			return referenceableAddresses.find(address) != referenceableAddresses.end();
		};

		const auto traverse = [&](const std::unordered_set<Address>& addresses, auto traverse)->void
		{
			for (const Address address : addresses)
			{
				std::unordered_set<Address> addressesDelta;
				CheckExpr(LRValue(address), *this, referenceableAddresses, addressesDelta);
				
				traverse(addressesDelta, traverse);
			}
		};

		{
			std::unordered_set<Address> addressesDelta;
			for (const auto& env : m_localEnvStack)
			{
				for (auto scopeIt = env.rbegin(); scopeIt != env.rend(); ++scopeIt)
				{
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
				}
			}
			
			traverse(addressesDelta, traverse);
		}

		{

			for (size_t i = 0; i < currentRecords.size(); ++i)
			{
				Record& record = currentRecords[i];
				Val evaluated = record;
				std::unordered_set<Address> addressesDelta;
				CheckValue(evaluated, *this, referenceableAddresses, addressesDelta);

				traverse(addressesDelta, traverse);
			}

			if (temporaryRecord)
			{
				std::unordered_set<Address> addressesDelta;
				CheckValue(temporaryRecord.value(), *this, referenceableAddresses, addressesDelta);

				traverse(addressesDelta, traverse);
			}
		}

		for (const auto& keyval : m_functions)
		{
			referenceableAddresses.emplace(keyval.first);
		}
		
		const size_t prevGC = m_values.size();

		m_values.gc(referenceableAddresses);

		const size_t postGC = m_values.size();

		//std::cout << "GC: ValueSize(" << prevGC << " -> " << postGC << ")\n";
		m_lastGCValueSize = m_values.size();
	}
}
