#pragma once
#include "Node.hpp"

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

class LocalEnvironment
{
public:

	enum Type { NormalScope, RecordScope };

	LocalEnvironment(Type scopeType):
		type(scopeType)
	{}

	using LocalVariables = std::unordered_map<std::string, unsigned>;

	void bind(const std::string& name, unsigned ID)
	{
		localVariables[name] = ID;
	}

	boost::optional<unsigned> find(const std::string& name)const
	{
		const auto it = localVariables.find(name);
		if (it == localVariables.end())
		{
			return boost::none;
		}

		return it->second;
	}

	LocalVariables::const_iterator begin()const
	{
		return localVariables.cbegin();
	}

	LocalVariables::const_iterator end()const
	{
		return localVariables.cend();
	}

private:

	LocalVariables localVariables;

	Type type;

	//isInnerScope = false �̎�GC�̑ΏۂƂȂ�
	bool isInnerScope = true;
};

class Environment
{
public:

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

	void bindValueID(const std::string& name, unsigned valueID)
	{
		/*
		���R�[�h
		���R�[�h����:���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͐V���ɒ�`
		���R�[�h����=���@�����K�w�ɓ�����:��������ꍇ�͂���ւ̍đ���A�����ꍇ�͂��̃X�R�[�v���ł̂ݗL���Ȓl�̃G�C���A�X�i�X�R�[�v�𔲂����猳�ɖ߂���Օ��j
		*/

		//���݂̊��ɕϐ������݂��Ȃ���΁A
		//�����X�g�̖����i���ł������̃X�R�[�v�j�ɕϐ���ǉ�����
		m_bindingNames.back().bind(name, valueID);
	}

	void pushNormal()
	{
		m_bindingNames.emplace_back(LocalEnvironment::Type::NormalScope);
	}

	void pushRecord()
	{
		m_bindingNames.emplace_back(LocalEnvironment::Type::RecordScope);
	}

	void pop()
	{
		m_bindingNames.pop_back();
	}

	void printEnvironment()const
	{
		/*std::cout << "Print Environment:\n";
		
		std::cout << "Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			std::cout << keyval.first << " : ";

			printEvaluated(val);
		}

		std::cout << "References:\n";
		for (size_t d = 0; d < m_bindingNames.size(); ++d)
		{
			std::cout << "Depth : " << d << "\n";
			const auto& names = m_bindingNames[d];
			
			for (const auto& keyval : names)
			{
				std::cout << keyval.first << " : " << keyval.second << "\n";
			}
		}*/
	}

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

	//�l������ĕԂ��i�ϐ��ő�������Ȃ��̂�GC���������瑦���ɏ������j
	//���̕]���r����GC�͑���Ȃ��悤�ɂ���ׂ����H
	unsigned makeTemporaryValue(const Evaluated& value)
	{
		return m_values.add(value);
	}

	//�����̃X�R�[�v���珇�Ԃɕϐ���T���ĕԂ�
	boost::optional<unsigned> findValueID(const std::string& name)const
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
	}

	void garbageCollect();

	Values m_values;

	//�ϐ��̓X�R�[�v�P�ʂŊǗ������
	//�X�R�[�v�𔲂����炻�̃X�R�[�v�ŊǗ����Ă���ϐ��������ƍ폜����
	//���������Ċ��̓l�X�g�̐󂢏��Ƀ��X�g�ŊǗ����邱�Ƃ��ł���i�����[���̊�������݂��邱�Ƃ͂Ȃ��j
	//���X�g�̍ŏ��̗v�f�̓O���[�o���ϐ��Ƃ���Ƃ���
	std::vector<LocalEnvironment> m_bindingNames;

	std::weak_ptr<Environment> m_weakThis;
};
