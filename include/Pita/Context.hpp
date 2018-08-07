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

		Values() = default;

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

		const Val& expand(const LRValue& lrvalue, const LocationInfo& info)const;

		Val& mutableExpand(LRValue& lrvalue, const LocationInfo& info);

		boost::optional<const Val&> expandOpt(const LRValue& lrvalue)const;

		boost::optional<Val&> mutableExpandOpt(LRValue& lrvalue);

		Address evalReference(const Accessor& access);

		std::vector<RegionVariable> expandReferences(Address address, const LocationInfo& info, RegionVariable::Attribute attribute = RegionVariable::Attribute::Other);

		std::vector<RegionVariable> expandReferences2(const Accessor& access, const LocationInfo& info);

		Reference bindReference(Address address);
		Reference bindReference(const Identifier& identifier);
		Reference bindReference(const Accessor& accessor, const LocationInfo& info);
		Address getReference(Reference reference)const;
		Reference cloneReference(Reference reference, const std::unordered_map<Address, Address>& addressReplaceMap);

		void printReference(Reference reference, std::ostream& os)const;

		void bindObjectRef(const std::string& name, Address ref)
		{
			bindValueID(name, ref);
		}

		void bindNewValue(const std::string& name, const Val& value)
		{
			makeVariable(name, makeTemporaryValue(value));
		}

		void bindValueID(const std::string& name, const Address ID);

		bool existsInCurrentScope(const std::string& name)const;
		bool existsInLocalScope(const std::string& name)const;

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

		//Accessorの示すリスト or レコードの持つアドレスを書き換える
		void assignToAccessor(const Accessor& accessor, const LRValue& newValue, const LocationInfo& info);

		//Referenceの示すアドレスを書き換える
		void assignToReference(const Reference& accessor, const LRValue& newValue, const LocationInfo& info);

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

		Address findAddress(const std::string& name)const;

		void garbageCollect(bool force = false);

		//sat/var宣言は現在の場所から見て最も内側のレコードに対して適用されるべきなので、その階層情報をスタックで持っておく
		std::vector<std::reference_wrapper<Record>> currentRecords;

		//レコード継承を行う時に、レコードを作ってから合成するのは難しいので、古いレコードを拡張する形で作ることにする
		boost::optional<Record&> temporaryRecord;

		bool isAutomaticExtendMode()
		{
			return m_automaticExtendMode;
		}

		bool hasTimeLimit()const
		{
			return static_cast<bool>(m_solveTimeLimit);
		}

		double timeLimit()const
		{
			return m_solveTimeLimit.get();
		}

		void setTimeLimit(const boost::optional<double>& limitSec)
		{
			m_solveTimeLimit = limitSec;
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

		//外部の変更を参照変数で捕捉
		void changeAddress(Address addressFrom, Address addressTo);

		//参照変数への変更を外部に伝播
		void replaceGlobalContextAddress(Address addressFrom, Address addressTo);

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
		bool m_automaticGC = false;

		boost::optional<double> m_solveTimeLimit;

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
		//ar(context.m_automaticGC);

		//std::uniform_real_distribution<double> m_dist;
		//std::mt19937 m_random;
		//std::weak_ptr<Context> m_weakThis;
	}
}
