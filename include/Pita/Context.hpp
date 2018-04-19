#pragma once
#include <stack>
#include <map>
#include <unordered_set>
#include <random>

#include "Node.hpp"

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
				CGL_Error(std::string() + "Address(" + key.toString() + ") is not binded.");
			}
			return it->second;
		}

		const ValueType& operator[](Address key)const
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				CGL_Error(std::string() + "Address(" + key.toString() + ") is not binded.");
			}
			return it->second;
		}

		void bind(Address key, const ValueType& value)
		{
			auto it = m_values.find(key);
			if (it != m_values.end())
			{
				it->second = value;
			}
			else
			{
				m_values.emplace(key, value);
			}
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

		typename ValueList::const_iterator find(const Address address)const
		{
			return m_values.find(address);
		}

		void gc(const std::unordered_set<Address>& ramainAddresses)
		{
			for (auto it = m_values.begin(); it != m_values.end();)
			{
				if (ramainAddresses.find(it->first) == ramainAddresses.end())
				{
					it = m_values.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

	private:
		Address newAddress()
		{
			return Address(++m_ID);
		}

	public:
		ValueList m_values;

		unsigned m_ID = 0;
	};

	struct Scope
	{
		using VriableMap = std::unordered_map<std::string, Address>;

		Scope() = default;

		VriableMap variables;
		std::vector<Address> temporaryAddresses;
	};

	class Context
	{
	public:
		using LocalContext = std::vector<Scope>;

		//using BuiltInFunction = std::function<Val(std::shared_ptr<Context>, const std::vector<Address>&)>;
		using BuiltInFunction = std::function<Val(std::shared_ptr<Context>, const std::vector<Address>&, const LocationInfo& info)>;

		Address makeFuncVal(std::shared_ptr<Context> pEnv, const std::vector<Identifier>& arguments, const Expr& expr);

		//スコープの内側に入る/出る
		void enterScope()
		{
			localEnv().emplace_back();
		}
		void exitScope()
		{
			localEnv().pop_back();
		}

		//関数呼び出しなど別のスコープに切り替える/戻す
		void switchFrontScope()
		{
			m_localEnvStack.push_back(LocalContext());
		}

		void switchBackScope()
		{
			m_localEnvStack.pop_back();
		}

		void registerBuiltInFunction(const std::string& name, const BuiltInFunction& function, bool isPlateausFunction);

		Val callBuiltInFunction(Address functionAddress, const std::vector<Address>& arguments, const LocationInfo& info);
		bool isPlateausBuiltInFunction(Address functionAddress);

		/*
		boost::optional<Address> find(const std::string& name)const
		{
			const auto valueIDOpt = findAddress(name);
			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return boost::none;
			}

			//return m_values[valueIDOpt.get()];
		}
		*/

		/*bool isValid(Address address)const
		{
			return m_values.at(address) != m_values.end();
		}*/

		//Address dereference(const Val& reference);
		/*
		boost::optional<const Val&> dereference(const Val& reference)const
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

		/*Val expandRef(const Val& reference)const
		{
			if (!IsType<Address>(reference))
			{
				return reference;
			}

			if (Address address = As<Address>(reference))
			{
				if (auto opt = m_values.at(address))
				{
					return expandRef(opt.get());
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

		const Val& expand(const LRValue& lrvalue, const LocationInfo& info)const;

		Val& mutableExpand(LRValue& lrvalue, const LocationInfo& info);

		boost::optional<const Val&> expandOpt(const LRValue& lrvalue)const;

		boost::optional<Val&> mutableExpandOpt(LRValue& lrvalue);

		Address evalReference(const Accessor& access);

		std::vector<Address> expandReferences(Address address, const LocationInfo& info);
		std::vector<std::pair<Address, VariableRange>> expandReferences(Address address, boost::optional<const PackedVal&> variableRange, const LocationInfo& info);

		//std::vector<Address> expandReferences2(const Accessor& access);
		std::vector<std::pair<Address, VariableRange>> expandReferences2(const Accessor& access, boost::optional<const PackedVal&> rangeOpt, const LocationInfo& info);

		Reference bindReference(Address address);
		Reference bindReference(const Identifier& identifier);
		Reference bindReference(const Accessor& accessor, const LocationInfo& info);
		Address getReference(Reference reference)const;
		Reference cloneReference(Reference reference, const std::unordered_map<Address, Address>& addressReplaceMap);
		
		void printReference(Reference reference, std::ostream& os)const;

		//ローカル変数を全て展開する
		//関数の戻り値などスコープが変わる時には参照を引き継げないので一度全て展開する必要がある
		/*
		Val expandObject(const Val& reference)
		{
			if (auto opt = AsOpt<Record>(reference))
			{
				Record expanded;
				for (const auto& elem : opt.get().values)
				{
					expanded.append(elem.first, expandObject(elem.second));
				}
				return expanded;
			}
			else if (auto opt = AsOpt<List>(reference))
			{
				List expanded;
				for (const auto& elem : opt.get().data)
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
				bindValueID(name, valueIDOpt.get());
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

		/*
		void bindNewValue(const std::string& name, const Val& value)
		{
			CGL_DebugLog("");
			const Address newAddress = m_values.add(value);
			//Clone(m_weakThis.lock(), value);
			bindValueID(name, newAddress);
		}
		*/
		void bindNewValue(const std::string& name, const Val& value)
		{
			CGL_DebugLog("");
			makeVariable(name, makeTemporaryValue(value));
		}

		/*void bindReference(const std::string& nameLhs, const std::string& nameRhs)
		{
			const Address address = findAddress(nameRhs);
			if (!address.isValid())
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return;
			}

			bindValueID(nameLhs, address);
		}*/

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
		void bindValueID(const std::string& name, const Address ID);

		bool existsInCurrentScope(const std::string& name)const;

		//bindValueIDの変数宣言式用
		void makeVariable(const std::string& name, const Address ID)
		{
			auto& variables = localEnv().back().variables;
			auto it = variables.find(name);
			
			//変数宣言文は、最も内側のスコープにある変数に対してのみ代入文としても振舞うことができる
			//したがってこの場合は代入文と同様にアドレスの変更も捕捉する必要がある
			if (it != variables.end())
			{
				changeAddress(it->second, ID);
				it->second = ID;
			}
			else
			{
				variables[name] = ID;
			}
		}
		
		void printContext(bool flag = false)const;
		void printContext(std::ostream& os)const;

		void TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(Address address, const Val& newValue)
		{
			//m_values[address] = newValue;
			m_values.bind(address, newValue);
		}

		/*
		void assignToObject(Address address, const Val& newValue)
		{
			m_values[address] = newValue;
		}

		//これで正しい？
		void assignAddress(Address addressTo, Address addressFrom)
		{
			m_values[addressTo] = m_values[addressFrom];
			//m_values[addressTo] = expandRef(m_values[addressFrom]);
		}*/

		//Accessorの示すリスト or レコードの持つアドレスを書き換える
		void assignToAccessor(const Accessor& accessor, const LRValue& newValue, const LocationInfo& info);

		static std::shared_ptr<Context> Make();

		static std::shared_ptr<Context> Make(const Context& other);

		std::shared_ptr<Context> clone()
		{
			return Make(*this);
		}

		//値を作って返す（変数で束縛されないものはGCが走ったら即座に消される）
		//式の評価途中でGCは走らないようにするべきか？
		Address makeTemporaryValue(const Val& value);

		Context() = default;

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
		Address findAddress(const std::string& name)const;

		void garbageCollect();

		//sat/var宣言は現在の場所から見て最も内側のレコードに対して適用されるべきなので、その階層情報をスタックで持っておく
		std::vector<std::reference_wrapper<Record>> currentRecords;

		//レコード継承を行う時に、レコードを作ってから合成するのは難しいので、古いレコードを拡張する形で作ることにする
		boost::optional<Record&> temporaryRecord;

		bool isAutomaticExtendMode()
		{
			return m_automaticExtendMode;
		}

	private:
		void initialize();

		LocalContext& localEnv()
		{
			return m_localEnvStack.back();
		}

		const LocalContext& localEnv()const
		{
			return m_localEnvStack.back();
		}

		void changeAddress(Address addressFrom, Address addressTo);

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

	public:
		Values<BuiltInFunction> m_functions;
		std::unordered_map<Address, std::string> m_plateausFunctions;

		std::unordered_map<Reference, DeepReference> m_refAddressMap;
		std::unordered_multimap<Address, Reference> m_addressRefMap;

		Values<Val> m_values;

		std::vector<LocalContext> m_localEnvStack;

		unsigned m_referenceID = 0;

		size_t m_lastGCValueSize = 0;

		bool m_automaticExtendMode = true;
		bool m_automaticGC = true;

		std::uniform_real_distribution<double> m_dist;
		std::mt19937 m_random;

		std::weak_ptr<Context> m_weakThis;
	};
}

namespace cereal
{
	template<class Archive>
	inline void serialize(Archive& ar, cgl::Scope& scope)
	{
		ar(scope.variables);
		ar(scope.temporaryAddresses);
	}

	/*template<class Archive>
	inline void serialize(Archive& ar, cgl::Context::BuiltInFunction& function)
	{
		ar(values.m_values);
		ar(values.m_ID);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Values<cgl::Context::BuiltInFunction>& values)
	{
		ar(values.m_values);
		ar(values.m_ID);
	}*/

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Values<cgl::Val>& values)
	{
		ar(values.m_values);
		ar(values.m_ID);
	}

	template<class Archive>
	inline void serialize(Archive& ar, cgl::Context& context)
	{
		//ar(context.m_functions);
		ar(context.m_plateausFunctions);
		ar(context.m_refAddressMap);
		ar(context.m_addressRefMap);
		ar(context.m_values);
		ar(context.m_localEnvStack);
		ar(context.m_referenceID);
		ar(context.m_lastGCValueSize);
		ar(context.m_automaticExtendMode);
		ar(context.m_automaticGC);

		//std::uniform_real_distribution<double> m_dist;
		//std::mt19937 m_random;
		//std::weak_ptr<Context> m_weakThis;
	}
}