#pragma once
#include <stack>
#include "Node.hpp"

namespace cgl
{
	class Values
	{
	public:

		using ValueList = std::unordered_map<unsigned, Evaluated>;

		unsigned add(const Evaluated& value)
		{
			m_values.insert({ nextID(), value });

			return m_ID;
		}

		size_t size()const
		{
			return m_values.size();
		}

		Evaluated& operator[](unsigned key)
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
			}
			return it->second;
		}

		const Evaluated& operator[](unsigned key)const
		{
			auto it = m_values.find(key);
			if (it == m_values.end())
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
			}
			return it->second;
		}

		ValueList::const_iterator begin()const
		{
			return m_values.cbegin();
		}

		ValueList::const_iterator end()const
		{
			return m_values.cend();
		}

	private:

		unsigned nextID()
		{
			return ++m_ID;
		}

		ValueList m_values;

		unsigned m_ID = 0;

	};

	/*
	class GlobalEnvironment
	{
	public:

		using Scope = std::unordered_map<std::string, unsigned>;

		void bind(const std::string& name, unsigned ID)
		{
			variables.back()[name] = ID;
		}

		boost::optional<unsigned> find(const std::string& name)const
		{
			for (auto scopeIt = variables.rbegin(); scopeIt != variables.rend(); ++scopeIt)
			{
				auto variableIt = scopeIt->find(name);
				if (variableIt != scopeIt->end())
				{
					return variableIt->second;
				}
			}

			return boost::none;
		}
		
		//スコープの内側に入る/出る
		void enterScope()
		{
			variables.emplace_back();
		}
		void exitScope()
		{
			variables.pop_back();
		}

		//関数呼び出しなど別のスコープに切り替える/戻す
		void switchFrontScope(int switchDepth)
		{
			diffScopes.push({ switchDepth,std::vector<Scope>(variables.begin() + switchDepth + 1, variables.end()) });
			variables.erase(variables.begin() + switchDepth + 1, variables.end());
		}
		void switchBackScope()
		{
			const int switchDepth = diffScopes.top().first;
			const auto& diffScope = diffScopes.top().second;
			
			variables.erase(variables.begin() + switchDepth + 1, variables.end());
			variables.insert(variables.end(), diffScope.begin(), diffScope.end());
			diffScopes.pop();
		}

	private:

		std::vector<Scope> variables;
		std::stack<std::pair<int, std::vector<Scope>>> diffScopes;
	};
	*/

	class Environment
	{
	public:

		using Scope = std::unordered_map<std::string, unsigned>;

		ObjectReference makeFuncVal(const std::vector<Identifier>& arguments, const Expr& expr);

		//スコープの内側に入る/出る
		void enterScope()
		{
			m_variables.emplace_back();
		}
		void exitScope();

		//関数呼び出しなど別のスコープに切り替える/戻す
		void switchFrontScope(int switchDepth)
		{
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

		boost::optional<const Evaluated&> find(const std::string& name)const
		{
			const auto valueIDOpt = findValueID(name);
			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return boost::none;
			}

			return m_values[valueIDOpt.value()];
		}

		const Evaluated& dereference(const Evaluated& reference);
		
		//const Evaluated& dereference(const Accessor& access);
		//boost::optional<const Evaluated&> evalReference(const Accessor& access);
		boost::optional<ObjectReference> evalReference(const Accessor& access);

		Expr expandFunction(const Expr& expr);

		//referenceで指されるオブジェクトの中にある全ての値への参照をリストで取得する
		std::vector<ObjectReference> expandReferences(const ObjectReference& reference, std::vector<ObjectReference>& output);
		std::vector<ObjectReference> expandReferences(const ObjectReference& reference)
		{
			std::vector<ObjectReference> result;
			expandReferences(reference, result);
			return result;
		}

		//{a=1,b=[2,3]}, [a, b] => [1, [2, 3]]
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

		//ローカル変数を全て展開する
		//関数の戻り値などスコープが変わる時には参照を引き継げないので一度全て展開する必要がある
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

		void bindObjectRef(const std::string& name, const ObjectReference& ref)
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
		}

		void bindNewValue(const std::string& name, const Evaluated& value)
		{
			const unsigned newID = m_values.add(value);
			bindValueID(name, newID);
		}

		void bindReference(const std::string& nameLhs, const std::string& nameRhs)
		{
			const auto valueIDOpt = findValueID(nameRhs);
			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return;
			}

			bindValueID(nameLhs, valueIDOpt.value());
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

		void bindValueID(const std::string& name, unsigned ID)
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
		}

		/*void push()
		{
			m_bindingNames.emplace_back(LocalEnvironment::Type::NormalScope);
		}

		void pop()
		{
			m_bindingNames.pop_back();
		}*/

		void printEnvironment()const;

		void assignToObject(const ObjectReference& objectRef, const Evaluated& newValue);

		static std::shared_ptr<Environment> Make()
		{
			auto p = std::make_shared<Environment>();
			p->m_weakThis = p;
			return p;
		}

		static std::shared_ptr<Environment> Make(const Environment& other)
		{
			auto p = std::make_shared<Environment>(other);
			p->m_weakThis = p;
			return p;
		}

		Environment() = default;

	private:

		int scopeDepth()const
		{
			return static_cast<int>(m_variables.size()) - 1;
		}

		//現在参照可能な変数名のリストのリストを返す
		std::vector<std::vector<std::string>> currentReferenceableVariables()const
		{
			std::vector<std::vector<std::string>> result;
			for (const auto& scope : m_variables)
			{
				result.emplace_back();
				auto& currentScope = result.back();
				for (const auto& var : scope)
				{
					currentScope.push_back(var.first);
				}
			}

			return result;
		}

		//値を作って返す（変数で束縛されないのでGCが走ったら即座に消される）
		//式の評価途中でGCは走らないようにするべきか？
		unsigned makeTemporaryValue(const Evaluated& value)
		{
			return m_values.add(value);
		}

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

		boost::optional<unsigned> findValueID(const std::string& name)const
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
		}

		void garbageCollect();

		Values m_values;

		//変数はスコープ単位で管理される
		//スコープを抜けたらそのスコープで管理している変数を環境ごと削除する
		//したがって環境はネストの浅い順にリストで管理することができる（同じ深さの環境が二つ存在することはない）
		//リストの最初の要素はグローバル変数とするとする
		//std::vector<LocalEnvironment> m_bindingNames;

		std::vector<Scope> m_variables;
		std::stack<std::pair<int, std::vector<Scope>>> m_diffScopes;

		std::vector<unsigned> m_funcValIDs;

		std::weak_ptr<Environment> m_weakThis;
	};
}
