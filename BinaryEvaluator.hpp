#pragma once

#include "Node.hpp"

inline Evaluated Not(const Evaluated& lhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);

	if (IsType<bool>(lhs))
	{
		return !As<bool>(lhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Plus(const Evaluated& lhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);

	if (IsType<int>(lhs) || IsType<double>(lhs))
	{
		return lhs;
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Minus(const Evaluated& lhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);

	if (IsType<int>(lhs))
	{
		return -As<int>(lhs);
	}
	else if (IsType<double>(lhs))
	{
		return -As<double>(lhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated And(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<bool>(lhs) && IsType<bool>(rhs))
	{
		return As<bool>(lhs) && As<bool>(rhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Or(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<bool>(lhs) && IsType<bool>(rhs))
	{
		return As<bool>(lhs) || As<bool>(rhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Equal(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) == As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) == As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) == As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) == As<double>(rhs);
		}
	}

	if (IsType<bool>(lhs) && IsType<bool>(rhs))
	{
		return As<bool>(lhs) == As<bool>(rhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated NotEqual(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) != As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) != As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) != As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) != As<double>(rhs);
		}
	}

	if (IsType<bool>(lhs) && IsType<bool>(rhs))
	{
		return As<bool>(lhs) != As<bool>(rhs);
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated LessThan(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) < As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) < As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) < As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) < As<double>(rhs);
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated LessEqual(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) <= As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) <= As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) <= As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) <= As<double>(rhs);
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated GreaterThan(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) > As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) > As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) > As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) > As<double>(rhs);
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated GreaterEqual(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) >= As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) >= As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) >= As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) >= As<double>(rhs);
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Max(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return std::max(As<int>(lhs), As<int>(rhs));
		}
		else if (IsType<double>(rhs))
		{
			return std::max<double>(As<int>(lhs), As<double>(rhs));
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return std::max<double>(As<double>(lhs), As<int>(rhs));
		}
		else if (IsType<double>(rhs))
		{
			return std::max(As<double>(lhs), As<double>(rhs));
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Add(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) + As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) + As<double>(rhs);
		}	
	}
	else if(IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) + As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) + As<double>(rhs);
		}
	}
	
	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Sub(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) - As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) - As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) - As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) - As<double>(rhs);
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Mul(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) * As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) * As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) * As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) * As<double>(rhs);
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Div(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<int>(lhs) / As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<int>(lhs) / As<double>(rhs);
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return As<double>(lhs) / As<int>(rhs);
		}
		else if (IsType<double>(rhs))
		{
			return As<double>(lhs) / As<double>(rhs);
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Pow(const Evaluated& lhs_, const Evaluated& rhs_, Environment& env)
{
	const Evaluated lhs = env.dereference(lhs_);
	const Evaluated rhs = env.dereference(rhs_);

	if (IsType<int>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return pow(As<int>(lhs), As<int>(rhs));
		}
		else if (IsType<double>(rhs))
		{
			return pow(As<int>(lhs), As<double>(rhs));
		}
	}
	else if (IsType<double>(lhs))
	{
		if (IsType<int>(rhs))
		{
			return pow(As<double>(lhs), As<int>(rhs));
		}
		else if (IsType<double>(rhs))
		{
			return pow(As<double>(lhs), As<double>(rhs));
		}
	}

	std::cerr << "Error(" << __LINE__ << ")\n";
	return 0;
}

inline Evaluated Assign(const Evaluated& lhs, const Evaluated& rhs, Environment& env)
{
	if (!IsLValue(lhs))
	{
		//������̍��ӂ����Ӓl�łȂ�
		std::cerr << "Error(" << __LINE__ << ")\n";
		return 0;
	}

	if (auto nameLhsOpt = AsOpt<Identifier>(lhs))
	{
		/*
		������́A���ӂƉE�ӂ̌`�̑g�ݍ��킹�ɂ��4�p�^�[���̋������N���蓾��B

		���̍��ӁF���݂̊��ɂ��łɑ��݂��鎯�ʎq���A�V���ɑ������s�����ʎq��
		���̉E�ӁF���Ӓl�ł��邩�A�E�Ӓl�ł��邩

		���Ӓl�̏ꍇ�́A���̎Q�Ɛ���擾���V����
		*/

		const auto& nameLhs = As<Identifier>(lhs).name;
		
		const bool rhsIsIdentifier = IsType<Identifier>(rhs);
		const bool rhsIsObjRef = IsType<ObjectReference>(rhs);
		if (rhsIsIdentifier)
		{
			const auto& nameRhs = As<Identifier>(rhs).name;

			env.bindReference(nameLhs, nameRhs);
		}
		else if (rhsIsObjRef)
		{
			const auto& valueRhs = env.dereference(rhs);
			env.bindNewValue(nameLhs, valueRhs);
		}
		else if (IsType<List>(rhs))
		{
			env.bindNewValue(nameLhs, env.expandList(rhs));
		}
		else
		{
			env.bindNewValue(nameLhs, rhs);
		}
	}
	else if (auto objLhsOpt = AsOpt<ObjectReference>(lhs))
	{
		const auto& objRefLhs = objLhsOpt.value();

		const bool rhsIsIdentifier = IsType<Identifier>(rhs);
		const bool rhsIsObjRef = IsType<ObjectReference>(rhs);
		if (rhsIsIdentifier)
		{
			const auto& nameRhs = As<Identifier>(rhs).name;
			const auto& valueRhs = env.dereference(nameRhs);

			env.assignToObject(objRefLhs, valueRhs);
		}
		else if (rhsIsObjRef)
		{
			const auto& valueRhs = env.dereference(rhs);
			env.assignToObject(objRefLhs, valueRhs);
		}
		else if (IsType<List>(rhs))
		{
			env.assignToObject(objRefLhs, env.expandList(rhs));
		}
		else
		{
			env.assignToObject(objRefLhs, rhs);
		}
	}

	return lhs;
}
