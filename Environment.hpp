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
		
		//�X�R�[�v�̓����ɓ���/�o��
		void enterScope()
		{
			variables.emplace_back();
		}
		void exitScope()
		{
			variables.pop_back();
		}

		//�֐��Ăяo���ȂǕʂ̃X�R�[�v�ɐ؂�ւ���/�߂�
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

		//�X�R�[�v�̓����ɓ���/�o��
		void enterScope()
		{
			m_variables.emplace_back();
		}
		void exitScope();

		//�֐��Ăяo���ȂǕʂ̃X�R�[�v�ɐ؂�ւ���/�߂�
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

		//reference�Ŏw�����I�u�W�F�N�g�̒��ɂ���S�Ă̒l�ւ̎Q�Ƃ����X�g�Ŏ擾����
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

		//���[�J���ϐ���S�ēW�J����
		//�֐��̖߂�l�ȂǃX�R�[�v���ς�鎞�ɂ͎Q�Ƃ������p���Ȃ��̂ň�x�S�ēW�J����K�v������
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
			//���R�[�h
			//���R�[�h����:���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͐V���ɒ�`
			//���R�[�h����=���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͂��̃X�R�[�v���ł̂ݗL���Ȓl�̃G�C���A�X�i�X�R�[�v�𔲂����猳�ɖ߂���Օ��j

			//���݂̊��ɕϐ������݂��Ȃ���΁A
			//�����X�g�̖����i���ł������̃X�R�[�v�j�ɕϐ���ǉ�����
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

		//���ݎQ�Ɖ\�ȕϐ����̃��X�g�̃��X�g��Ԃ�
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

		//�l������ĕԂ��i�ϐ��ő�������Ȃ��̂�GC���������瑦���ɏ������j
		//���̕]���r����GC�͑���Ȃ��悤�ɂ���ׂ����H
		unsigned makeTemporaryValue(const Evaluated& value)
		{
			return m_values.add(value);
		}

		//�����̃X�R�[�v���珇�Ԃɕϐ���T���ĕԂ�
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

		//�ϐ��̓X�R�[�v�P�ʂŊǗ������
		//�X�R�[�v�𔲂����炻�̃X�R�[�v�ŊǗ����Ă���ϐ��������ƍ폜����
		//���������Ċ��̓l�X�g�̐󂢏��Ƀ��X�g�ŊǗ����邱�Ƃ��ł���i�����[���̊�������݂��邱�Ƃ͂Ȃ��j
		//���X�g�̍ŏ��̗v�f�̓O���[�o���ϐ��Ƃ���Ƃ���
		//std::vector<LocalEnvironment> m_bindingNames;

		std::vector<Scope> m_variables;
		std::stack<std::pair<int, std::vector<Scope>>> m_diffScopes;

		std::vector<unsigned> m_funcValIDs;

		std::weak_ptr<Environment> m_weakThis;
	};
}
