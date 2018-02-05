#include <stack>
#include <Pita/Node.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/Environment.hpp>
#include <Pita/Evaluator.hpp>
#include <Pita/Geometry.hpp>

namespace cgl
{
	Address Environment::makeFuncVal(std::shared_ptr<Environment> pEnv, const std::vector<Identifier>& arguments, const Expr& expr)
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

	void Environment::registerBuiltInFunction(const std::string& name, const BuiltInFunction& function)
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
	}

	Evaluated Environment::callBuiltInFunction(Address functionAddress, const std::vector<Address>& arguments)
	{
		if (std::shared_ptr<Environment> pEnv = m_weakThis.lock())
		{
			return m_functions[functionAddress](pEnv, arguments);
		}
			
		CGL_Error("ここは通らないはず");
		return 0;
	}

	const Evaluated& Environment::expand(const LRValue& lrvalue)const
	{
		if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address());
			if (it != m_values.end())
			{
				return it->second;
			}
				
			CGL_Error(std::string("reference error: Address(") + lrvalue.address().toString() + ")");
		}

		return lrvalue.evaluated();
	}

	boost::optional<const Evaluated&> Environment::expandOpt(const LRValue& lrvalue)const
	{
		if (lrvalue.isLValue())
		{
			auto it = m_values.at(lrvalue.address());
			if (it != m_values.end())
			{
				return it->second;
			}

			return boost::none;
		}

		return lrvalue.evaluated();
	}

	Address Environment::evalReference(const Accessor & access)
	{
		if (auto sharedThis = m_weakThis.lock())
		{
			Eval evaluator(sharedThis);

			const Expr accessor = access;
			/*
			const Evaluated refVal = boost::apply_visitor(evaluator, accessor);

			if (!IsType<Address>(refVal))
			{
			CGL_Error("a");
			}

			return As<Address>(refVal);
			*/

			const LRValue refVal = boost::apply_visitor(evaluator, accessor);
			if (refVal.isLValue())
			{
				return refVal.address();
			}

			CGL_Error("アクセッサの評価結果がアドレス値でない");
		}

		CGL_Error("shared this does not exist.");
		return Address::Null();
	}

	//referenceで指されるオブジェクトの中にある全ての値への参照をリストで取得する
	/*std::vector<ObjectReference> expandReferences(const ObjectReference& reference, std::vector<ObjectReference>& output);
	std::vector<ObjectReference> expandReferences(const ObjectReference& reference)*/
	std::vector<Address> Environment::expandReferences(Address address)
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

	void Environment::bindReference(const std::string& nameLhs, const std::string& nameRhs)
	{
		const Address address = findAddress(nameRhs);
		if (!address.isValid())
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return;
		}

		bindValueID(nameLhs, address);
	}

	void Environment::bindValueID(const std::string& name, const Address ID)
	{
		//CGL_DebugLog("");
		for (auto scopeIt = localEnv().rbegin(); scopeIt != localEnv().rend(); ++scopeIt)
		{
			auto valIt = scopeIt->find(name);
			if (valIt != scopeIt->end())
			{
				valIt->second = ID;
				return;
			}
		}
		//CGL_DebugLog("");

		localEnv().back()[name] = ID;
	}

#ifdef CGL_EnableLogOutput
	void Environment::printEnvironment(bool flag)const
	{
		if (!flag)
		{
			return;
		}

		std::ostream& os = ofs;

		os << "Print Environment Begin:\n";

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

		os << "Print Environment End:\n";
	}
#else
	void Environment::printEnvironment(bool flag)const {}
#endif

	void Environment::printEnvironment(std::ostream& os)const
	{
		os << "Print Environment Begin:\n";

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

		os << "Print Environment End:\n";
	}
		
	std::shared_ptr<Environment> Environment::Make()
	{
		auto p = std::make_shared<Environment>();
		p->m_weakThis = p;
		p->switchFrontScope();
		p->enterScope();
		p->initialize();
		return p;
	}

	std::shared_ptr<Environment> Environment::Make(const Environment& other)
	{
		auto p = std::make_shared<Environment>(other);
		p->m_weakThis = p;
		return p;
	}

	//値を作って返す（変数で束縛されないものはGCが走ったら即座に消される）
	//式の評価途中でGCは走らないようにするべきか？
	Address Environment::makeTemporaryValue(const Evaluated& value)
	{
		const Address address = m_values.add(value);

		//関数はスコープを抜ける時に定義式中の変数が解放されないか監視する必要があるのでIDを保存しておく
		/*if (IsType<FuncVal>(value))
		{
			m_funcValIDs.push_back(address);
		}*/

		return address;
	}

	Address Environment::findAddress(const std::string& name)const
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

	void Environment::initialize()
	{
		registerBuiltInFunction(
			"sin",
			[](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return Sin(pEnv->expand(arguments[0]));
		}
		);

		registerBuiltInFunction(
			"cos",
			[](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return Cos(pEnv->expand(arguments[0]));
		}
		);

		registerBuiltInFunction(
			"diff",
			[&](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 2)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return ShapeDifference(pEnv->expand(arguments[0]), pEnv->expand(arguments[1]), m_weakThis.lock());
		}
		);

		registerBuiltInFunction(
			"area",
			[&](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return ShapeArea(pEnv->expand(arguments[0]), m_weakThis.lock());
		}
		);

		registerBuiltInFunction(
			"DefaultFontString",
			[&](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
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
		}
		);
	}

	void Environment::garbageCollect()
	{

	}
}
