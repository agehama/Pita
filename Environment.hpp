#pragma once
#include <stack>
#include "Node.hpp"
#include "BinaryEvaluator.hpp"

namespace cgl
{
	template<class ValueType>
	class Values
	{
	public:

		using ValueList = std::unordered_map<Address, ValueType>;

		Address add(const ValueType& value)
		{
			m_values.insert({ newAddress(), value });

			return Address(m_ID);
		}

		size_t size()const
		{
			return m_values.size();
		}

		ValueType& operator[](Address key)
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				CGL_Error("参照エラー");
			}
			return it->second;
		}

		const ValueType& operator[](Address key)const
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				CGL_Error("参照エラー");
			}
			return it->second;
		}

		typename ValueList::iterator at(Address key)
		{
			return m_values.find(key);
		}

		typename ValueList::const_iterator at(Address key)const
		{
			return m_values.find(key);
		}

		typename ValueList::const_iterator begin()const
		{
			return m_values.cbegin();
		}

		typename ValueList::const_iterator end()const
		{
			return m_values.cend();
		}

	private:

		Address newAddress()
		{
			return Address(++m_ID);
		}

		ValueList m_values;

		unsigned m_ID = 0;
	};

	class Environment
	{
	public:

		//using Scope = std::unordered_map<std::string, unsigned>;
		using Scope = std::unordered_map<std::string, Address>;

		using LocalEnvironment = std::vector<Scope>;

		using BuiltInFunction = std::function<Evaluated(std::shared_ptr<Environment>, const std::vector<Address>&)>;

		//ObjectReference makeFuncVal(const std::vector<Identifier>& arguments, const Expr& expr);
		Address makeFuncVal(std::shared_ptr<Environment> pEnv, const std::vector<Identifier>& arguments, const Expr& expr);

		//スコープの内側に入る/出る
		void enterScope()
		{
			//m_variables.emplace_back();
			localEnv().emplace_back();
		}
		void exitScope()
		{
			//m_variables.pop_back();
			localEnv().pop_back();
		}

		//関数呼び出しなど別のスコープに切り替える/戻す
		/*
		void switchFrontScope(int switchDepth)
		{
			//関数のスコープが同じ時の動作は未確認

			std::cout << "FuncScope:" << switchDepth << std::endl;
			std::cout << "Variables:" << m_variables.size() << std::endl;

			m_diffScopes.push({ switchDepth,std::vector<Scope>(m_variables.begin() + switchDepth + 1, m_variables.end()) });
			m_variables.erase(m_variables.begin() + switchDepth + 1, m_variables.end());
		}
		void switchBackScope()
		{
			const int switchDepth = m_diffScopes.top().first;
			const auto& diffScope = m_diffScopes.top().second;

			m_variables.erase(m_variables.begin() + switchDepth + 1, m_variables.end());
			m_variables.insert(m_variables.end(), diffScope.begin(), diffScope.end());
			m_diffScopes.pop();
		}
		*/
		void switchFrontScope()
		{
			m_localEnvStack.push(LocalEnvironment());
		}
		void switchBackScope()
		{
			m_localEnvStack.pop();
		}

		void registerBuiltInFunction(const std::string& name, const BuiltInFunction& function)
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

		Evaluated callBuiltInFunction(Address functionAddress, const std::vector<Address>& arguments)
		{
			if (std::shared_ptr<Environment> pEnv = m_weakThis.lock())
			{
				return m_functions[functionAddress](pEnv, arguments);
			}
			
			CGL_Error("ここは通らないはず");
			return 0;
		}

		/*
		boost::optional<Address> find(const std::string& name)const
		{
			const auto valueIDOpt = findAddress(name);
			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return boost::none;
			}

			//return m_values[valueIDOpt.value()];
		}
		*/

		/*bool isValid(Address address)const
		{
			return m_values.at(address) != m_values.end();
		}*/

		//Address dereference(const Evaluated& reference);
		/*
		boost::optional<const Evaluated&> dereference(const Evaluated& reference)const
		{
			if (!IsType<Address>(reference))
			{
				return boost::none;
			}

			//return m_values.at(As<Address>(reference));

			auto it = m_values.at(As<Address>(reference));
			if (it == m_values.end())
			{
				return boost::none;
			}

			return it->second;
		}
		*/

		/*Evaluated expandRef(const Evaluated& reference)const
		{
			if (!IsType<Address>(reference))
			{
				return reference;
			}

			if (Address address = As<Address>(reference))
			{
				if (auto opt = m_values.at(address))
				{
					return expandRef(opt.value());
				}
				else
				{
					CGL_Error("reference error");
					return 0;
				}
			}

			CGL_Error("reference error");
			return 0;
		}*/

		const Evaluated& expand(const LRValue& lrvalue)const
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

		boost::optional<const Evaluated&> expandOpt(const LRValue& lrvalue)const
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

		Address evalReference(const Accessor& access);

		//referenceで指されるオブジェクトの中にある全ての値への参照をリストで取得する
		/*std::vector<ObjectReference> expandReferences(const ObjectReference& reference, std::vector<ObjectReference>& output);
		std::vector<ObjectReference> expandReferences(const ObjectReference& reference)*/
		std::vector<Address> expandReferences(Address address)
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

		//{a=1,b=[2,3]}, [a, b] => [1, [2, 3]]
		/*
		Evaluated expandList(const Evaluated& reference)
		{
			if (auto listOpt = AsOpt<List>(reference))
			{
				List expanded;
				for (const auto& elem : listOpt.value().data)
				{
					expanded.append(expandList(elem));
				}
				return expanded;
			}

			return dereference(reference);
		}
		*/
		
		//ローカル変数を全て展開する
		//関数の戻り値などスコープが変わる時には参照を引き継げないので一度全て展開する必要がある
		/*
		Evaluated expandObject(const Evaluated& reference)
		{
			if (auto opt = AsOpt<Record>(reference))
			{
				Record expanded;
				for (const auto& elem : opt.value().values)
				{
					expanded.append(elem.first, expandObject(elem.second));
				}
				return expanded;
			}
			else if (auto opt = AsOpt<List>(reference))
			{
				List expanded;
				for (const auto& elem : opt.value().data)
				{
					expanded.append(expandObject(elem));
				}
				return expanded;
			}

			return dereference(reference);
		}
		*/

		/*void bindObjectRef(const std::string& name, const ObjectReference& ref)
		{
			if (auto valueIDOpt = AsOpt<unsigned>(ref.headValue))
			{
				bindValueID(name, valueIDOpt.value());
			}
			else
			{
				const auto& valueRhs = dereference(ref);
				bindNewValue(name, valueRhs);
			}
		}*/
		void bindObjectRef(const std::string& name, Address ref)
		{
			bindValueID(name, ref);
		}

		void bindNewValue(const std::string& name, const Evaluated& value)
		{
			CGL_DebugLog("");
			const Address newAddress = m_values.add(value);
			//Clone(m_weakThis.lock(), value);
			bindValueID(name, newAddress);
		}

		void bindReference(const std::string& nameLhs, const std::string& nameRhs)
		{
			const Address address = findAddress(nameRhs);
			if (!address.isValid())
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return;
			}

			bindValueID(nameLhs, address);
		}

		/*
		void bindValueID(const std::string& name, unsigned valueID)
		{
			//レコード
			//レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
			//レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアス（スコープを抜けたら元に戻る≒遮蔽）

			//現在の環境に変数が存在しなければ、
			//環境リストの末尾（＝最も内側のスコープ）に変数を追加する
			m_bindingNames.back().bind(name, valueID);
		}
		*/

		//void bindValueID(const std::string& name, unsigned ID)
		/*void bindValueID(const std::string& name, const Address ID)
		{
			for (auto scopeIt = m_variables.rbegin(); scopeIt != m_variables.rend(); ++scopeIt)
			{
				auto valIt = scopeIt->find(name);
				if (valIt != scopeIt->end())
				{
					valIt->second = ID;
					return;
				}
			}

			m_variables.back()[name] = ID;
		}*/
		void bindValueID(const std::string& name, const Address ID)
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

		//bindValueIDの変数宣言式用
		void makeVariable(const std::string& name, const Address ID)
		{
			localEnv().back()[name] = ID;
		}

		/*void push()
		{
			m_bindingNames.emplace_back(LocalEnvironment::Type::NormalScope);
		}

		void pop()
		{
			m_bindingNames.pop_back();
		}*/

		void printEnvironment(bool flag = false)const;

		//void assignToObject(const ObjectReference& objectRef, const Evaluated& newValue);
		void assignToObject(Address address, const Evaluated& newValue)
		{
			m_values[address] = newValue;
			
			//m_values[address] = expandRef(newValue);
		}

		//これで正しい？
		void assignAddress(Address addressTo, Address addressFrom)
		{
			m_values[addressTo] = m_values[addressFrom];
			//m_values[addressTo] = expandRef(m_values[addressFrom]);
		}

		static std::shared_ptr<Environment> Make()
		{
			auto p = std::make_shared<Environment>();
			p->m_weakThis = p;
			p->switchFrontScope();
			p->enterScope();
			p->initialize();
			return p;
		}

		static std::shared_ptr<Environment> Make(const Environment& other)
		{
			auto p = std::make_shared<Environment>(other);
			p->m_weakThis = p;
			return p;
		}

		std::shared_ptr<Environment> clone()
		{
			return Make(*this);
		}

		//値を作って返す（変数で束縛されないものはGCが走ったら即座に消される）
		//式の評価途中でGCは走らないようにするべきか？
		Address makeTemporaryValue(const Evaluated& value)
		{
			const Address address = m_values.add(value);

			//関数はスコープを抜ける時に定義式中の変数が解放されないか監視する必要があるのでIDを保存しておく
			/*if (IsType<FuncVal>(value))
			{
				m_funcValIDs.push_back(address);
			}*/

			return address;
		}

		Environment() = default;

/*
式中に現れ得る識別子は次の3種類に分けられる。

1. コロンの左側に出てくる識別子：
　　最も内側のスコープにその変数が有ればその変数への参照
　　　　　　　　　　　　　　　　　無ければ新しく作った変数への束縛

2. イコールの左側に出てくる識別子：
　　スコープのどこかにその変数が有ればその変数への参照
　　　　　　　　　　　　　　　　無ければ新しく作った変数への束縛

3. それ以外の場所に出てくる識別子：
　　スコープのどこかにその変数が有ればその変数への参照
　　　　　　　　　　　　　　　　無ければ無効な参照（エラー）

ここで、1の用法と2,3の用法を両立することは難しい（識別子を見ただけでは何を返すかが決定できないので）。
しかし、1の用法はかなり特殊であるため、単に特別扱いしてもよい気がする。
つまり、コロンの左側に出てこれるのは単一の識別子のみとする（複雑なものを書けてもそれほどメリットがなくデバッグが大変になるだけ）。
これにより、コロン式を見た時に中の識別子も一緒に見れば済むので、上記の用法を両立できる。
*/
		/*boost::optional<Address> findValueID(const std::string& name)const
		{
			for (auto scopeIt = m_variables.rbegin(); scopeIt != m_variables.rend(); ++scopeIt)
			{
				auto variableIt = scopeIt->find(name);
				if (variableIt != scopeIt->end())
				{
					return variableIt->second;
				}
			}

			return boost::none;
		}*/

		
		/*Address findAddress(const std::string& name)const
		{
			for (auto scopeIt = m_variables.rbegin(); scopeIt != m_variables.rend(); ++scopeIt)
			{
				auto variableIt = scopeIt->find(name);
				if (variableIt != scopeIt->end())
				{
					return variableIt->second;
				}
			}

			return Address::Null();
		}*/
		Address findAddress(const std::string& name)const
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

	private:

		void initialize()
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
		}

		LocalEnvironment& localEnv()
		{
			return m_localEnvStack.top();
		}

		const LocalEnvironment& localEnv()const
		{
			return m_localEnvStack.top();
		}

		/*int scopeDepth()const
		{
			return static_cast<int>(m_variables.size()) - 1;
		}*/

		//現在参照可能な変数名のリストのリストを返す
		/*
		std::vector<std::set<std::string>> currentReferenceableVariables()const
		{
			std::vector<std::set<std::string>> result;
			for (const auto& scope : m_variables)
			{
				result.emplace_back();
				auto& currentScope = result.back();
				for (const auto& var : scope)
				{
					currentScope.insert(var.first);
				}
			}

			return result;
		}
		*/

		//内側のスコープから順番に変数を探して返す
		/*boost::optional<unsigned> findValueID(const std::string& name)const
		{
			boost::optional<unsigned> valueIDOpt;

			for (auto it = m_bindingNames.rbegin(); it != m_bindingNames.rend(); ++it)
			{
				valueIDOpt = it->find(name);
				if (valueIDOpt)
				{
					break;
				}
			}

			return valueIDOpt;
		}*/

		void garbageCollect();

		Values<BuiltInFunction> m_functions;

		Values<Evaluated> m_values;

		//変数はスコープ単位で管理される
		//スコープを抜けたらそのスコープで管理している変数を環境ごと削除する
		//したがって環境はネストの浅い順にリストで管理することができる（同じ深さの環境が二つ存在することはない）
		//リストの最初の要素はグローバル変数とするとする
		//std::vector<LocalEnvironment> m_bindingNames;

		/*std::vector<Scope> m_variables;
		std::stack<std::pair<int, std::vector<Scope>>> m_diffScopes;*/
		
		
		std::stack<LocalEnvironment> m_localEnvStack;

		//std::vector<Address> m_funcValIDs;

		std::weak_ptr<Environment> m_weakThis;
	};
}
