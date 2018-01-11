#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <Eigen/Core>
#include "Node.hpp"
#include "Evaluator.hpp"
#include "Printer.hpp"
#include "Environment.hpp"

namespace cgl
{
#ifdef commentout
	SatExpr Expr2SatExpr::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			const Evaluated& val = node.evaluated();
			if (IsType<double>(val))
			{
				return As<double>(val);
			}
			else if (IsType<int>(val))
			{
				return static_cast<double>(As<int>(val));
			}

			CGL_Error("sat �����̕ϐ��̕]���l�� int �^�������� double �^�Ŗ�����΂Ȃ�܂���");
		}
		else
		{
			const Address address = node.address();

			if (!address.isValid())
			{
				CGL_Error("���ʎq����`����Ă��܂���");
			}

			//free�ϐ��ɂ������ꍇ�͐���p�̎Q�ƒl��Ԃ�
			if (auto satRefOpt = addSatRef(address))
			{
				return satRefOpt.value();
			}
			//free�ϐ��ɂȂ������ꍇ�͕]���������ʂ�Ԃ�
			else
			{
				const Evaluated evaluated = pEnv->expand(address);
				if (IsType<double>(evaluated))
				{
					return As<double>(evaluated);
				}
				else if (IsType<int>(evaluated))
				{
					return static_cast<double>(As<int>(evaluated));
				}

				//int/double����Ȃ��ꍇ�͂Ƃ肠�����G���[�Bbool�͗L�蓾��H
				CGL_Error("sat �����̕ϐ��̕]���l�� int �^�������� double �^�Ŗ�����΂Ȃ�܂���");
			}
		}

		CGL_Error("�s���Ȓl");
		return 0;
	}
#endif

	Expr Expr2SatExpr::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			return node;
		}
		else
		{
			const Address address = node.address();

			if (!address.isValid())
			{
				CGL_Error("���ʎq����`����Ă��܂���");
			}

			//free�ϐ��ɂ������ꍇ�͐���p�̎Q�ƒl��Ԃ�
			if (auto satRefOpt = addSatRef(address))
			{
				return satRefOpt.value();
			}
			//free�ϐ��ɂȂ������ꍇ�͕]���������ʂ�Ԃ�
			else
			{
				const Evaluated evaluated = pEnv->expand(address);
				return LRValue(evaluated);
			}
		}

		CGL_Error("�����͒ʂ�Ȃ��͂�");
		return 0;
	}

	//�v�l�@�Fsat�ɂ������̒l���ȗ����Ďw��ł���悤�ɂ��邽�߂ɂ́A����(Identifier/Accessor)�ŕ����̒l��������悤�ɂ���K�v������
	/*
	SatExpr Expr2SatExpr::operator()(const Identifier& node)
	{
		ObjectReference currentRefVal(node);

		if (auto satRefOpt = addSatRef(currentRefVal))
		{
			return satRefOpt.value();
		}
		//for (size_t i = 0; i < freeVariables.size(); ++i)
		//{
		//	//freeVariables�ɑ��݂����ꍇ�́A�œK���p�̕ϐ�������A���̎Q�Ƃ�Ԃ�
		//	//�܂��AfreeVariables���ɂ����̕ϐ����g�p���邱�Ƃ�m�点��
		//	if (freeVariables[i] == currentRefVal)
		//	{
		//		if (usedInSat[i] == 1)
		//		{
		//			return satRefs[i];
		//		}
		//		else
		//		{
		//			usedInSat[i] = 1;

		//			SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
		//			satRefs[i] = satRef;

		//			refs.push_back(currentRefVal);
		//			std::cout << "NewRef(" << satRef.refID << ")\n";
		//			return satRef;
		//		}
		//	}
		//}

		//freeVariables�ɑ��݂��Ȃ������ꍇ�́A�����ɕ]�����Ă悢�i�萔���̏�ݍ��݁j
		const Evaluated evaluated = pEnv->dereference(currentRefVal);

		if (IsType<double>(evaluated))
		{
			return As<double>(evaluated);
		}
		else if (IsType<int>(evaluated))
		{
			return static_cast<double>(As<int>(evaluated));
		}

		//int/double����Ȃ��ꍇ�͂Ƃ肠�����G���[�Bbool�͗L�蓾��H
		std::cerr << "Error(" << __LINE__ << ")\n";
		return 0.0;
	}
	*/
#ifdef commentout
	SatExpr Expr2SatExpr::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		if (!address.isValid())
		{
			CGL_Error("���ʎq����`����Ă��܂���");
		}

		//free�ϐ��ɂ������ꍇ�͐���p�̎Q�ƒl��Ԃ�
		if (auto satRefOpt = addSatRef(address))
		{
			return satRefOpt.value();
		}
		//free�ϐ��ɂȂ������ꍇ�͕]���������ʂ�Ԃ�
		else
		{
			const Evaluated evaluated = pEnv->expand(address);
			if (IsType<double>(evaluated))
			{
				return As<double>(evaluated);
			}
			else if (IsType<int>(evaluated))
			{
				return static_cast<double>(As<int>(evaluated));
			}

			//int/double����Ȃ��ꍇ�͂Ƃ肠�����G���[�Bbool�͗L�蓾��H
			CGL_Error("sat �����̕ϐ��̕]���l�� int �^�������� double �^�Ŗ�����΂Ȃ�܂���");
		}

		CGL_Error("�����͒ʂ�Ȃ��͂�");
		return 0.0;
	}
#endif

	//������Identifier���c���Ă��鎞�_��ClosureMaker�Ƀ��[�J���ϐ����Ɣ��肳�ꂽ�ϐ��̂͂�
	Expr Expr2SatExpr::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		if (!address.isValid())
		{
			CGL_Error("���ʎq����`����Ă��܂���");
		}

		//free�ϐ��ɂ������ꍇ�͐���p�̎Q�ƒl��Ԃ�
		if (auto satRefOpt = addSatRef(address))
		{
			return satRefOpt.value();
		}
		//free�ϐ��ɂȂ������ꍇ�͕]���������ʂ�Ԃ�
		else
		{
			const Evaluated evaluated = pEnv->expand(address);
			return LRValue(evaluated);
		}

		CGL_Error("�����͒ʂ�Ȃ��͂�");
		return LRValue(0);
	}

	/*inline bool HasFreeVariables::operator()(const RValue& node)
	{
		const Evaluated& val = node.value;
		if (IsType<double>(val) || IsType<int>(val) || IsType<bool>(val))
		{
			return false;
		}
		else if (!IsType<Address>(val))
		{
			CGL_Error("�s���Ȓl");
		}

		Address address = As<Address>(val);
		for (const auto& freeVal : freeVariables)
		{
			if (address == freeVal)
			{
				return true;
			}
		}

		return false;
	}*/
	inline bool HasFreeVariables::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			const Evaluated& val = node.evaluated();
			if (IsType<double>(val) || IsType<int>(val) || IsType<bool>(val))
			{
				return false;
			}

			CGL_Error("�s���Ȓl");
		}

		Address address = node.address();
		for (const auto& freeVal : freeVariables)
		{
			if (address == freeVal)
			{
				return true;
			}
		}

		return false;
	}

	inline bool HasFreeVariables::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		for (const auto& freeVal : freeVariables)
		{
			if (address == freeVal)
			{
				return true;
			}
		}

		return false;
	}

	//���͊֐��̈�������freeVariables�����邩�ǂ����𒲂ׂ�Ƃ������Ƃ�ړI�ɂ��Ă���̂ŁA�����]�����邾���ōς�ł���
	//TODO: �{���̓A�N�Z�b�T�̒��̂��ꂼ��̐��ɂ��Ă݂�ׂ�
	inline bool HasFreeVariables::operator()(const Accessor& node)
	{
		//Accessor��ObjectReference���r����ɂ͂Ƃ肠�����]������΍s���邪����Eval�͐��������H
		Expr expr = node;
		Eval evaluator(pEnv);
		const LRValue value = boost::apply_visitor(evaluator, expr);

		/*
		//������ObjectReference�̂ݍl������΂悢�H
		if (auto refOpt = AsOpt<Address>(evaluated))
		{
			for (const auto& freeVal : freeVariables)
			{
				if (refOpt.value() == freeVal)
				{
					return true;
				}
			}

			return false;
		}
		*/
		if (value.isLValue())
		{
			const Address address = value.address();

			for (const auto& freeVal : freeVariables)
			{
				if (address == freeVal)
				{
					return true;
				}
			}

			return false;
		}

		//std::cerr << "Error(" << __LINE__ << "): invalid expression\n";
		CGL_Error("invalid expression");
		return false;
	}

#ifdef commentout
	inline boost::optional<SatFunctionReference> MakeSatFunctionReference(
		//const FuncVal& head, 
		//const std::string& head,
		Address head,
		const Accessor& node, 
		size_t accessorIndex, 
		Expr2SatExpr& converter,
		Eval& evaluator,
		std::shared_ptr<Environment> pEnv)
	{
		SatFunctionReference ref(head);

		for (size_t i = accessorIndex; i < node.accesses.size(); ++i)
		{
			const auto& access = node.accesses[i];

			if (auto listOpt = AsOpt<ListAccess>(access))
			{
				Evaluated value = pEnv->expand(boost::apply_visitor(evaluator, listOpt.value().index));

				if (auto indexOpt = AsOpt<int>(value))
				{
					ref.appendListRef(indexOpt.value());
				}
				else
				{
					//�G���[�Flist[index] �� index �� int �^�łȂ�
					//std::cerr << "Error(" << __LINE__ << ")\n";
					//return boost::none;
					CGL_Error("list[index] �� index �� int �^�łȂ�");
				}
				/*
				���݂͈ȉ��̂悤�Ȑ���̏������͋����Ă��Ȃ��B
				���p���͂��邩�H
				sat(v[x] == 0)
				free(x)
				*/
			}
			else if (auto recordOpt = AsOpt<RecordAccess>(access))
			{
				ref.appendRecordRef(recordOpt.value().name);
			}
			else
			{
				auto funcAccess = As<FunctionAccess>(access);

				SatFunctionReference::FunctionRef functionRef;

				std::vector<Evaluated> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					const SatExpr currentArg = boost::apply_visitor(converter, expr);
					functionRef.appendExpr(currentArg);

					//�S��SatExpr�ƈ������Ƃɂ���

					/*const Evaluated currentArg = boost::apply_visitor(evaluator, expr);

					if (IsType<ObjectReference>(currentArg))
					{
						const ObjectReference& currentRefVal = As<ObjectReference>(currentArg);

						if (auto refOpt = converter.addSatRef(currentRefVal))
						{
							functionRef.appendRef(refOpt.value());
						}
						else
						{
							functionRef.appendValue(pEnv->dereference(currentRefVal));
						}
					}
					else
					{
						functionRef.appendValue(currentArg);
					}*/
				}

				ref.appendFunctionRef(functionRef);
			}
		}

		return ref;
	}

	SatExpr Expr2SatExpr::operator()(const Accessor& node)
	{
		//sat�����̊֐��̈�����free�ϐ�������ꍇ�́A�A�h���X��R�t���Ȃ���΂����Ȃ��̂ň�U���g������
		//TODO: FunctionCall�����łȂ��A���X�g�̃C���f�b�N�X�A�N�Z�X������悤�ɂ���
		if (node.hasFunctionCall())
		{
			//Eval::operator()(const Accessor& accessor)���قڃR�s�y��������

			Eval evaluator(pEnv);

			//ObjectReference result;
			Address address;

			/*
			Evaluated headValue = boost::apply_visitor(evaluator, node.head);
			if (auto opt = AsOpt<Address>(headValue))
			{
				address = opt.value();
			}
			else if (auto opt = AsOpt<Record>(headValue))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<List>(headValue))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<FuncVal>(headValue))
			{
				address = pEnv->makeTemporaryValue(opt.value());
			}
			else
			{
				//�G���[�F���ʎq�����e�����ȊO�i�]�����ʂƂ��ăI�u�W�F�N�g��Ԃ��悤�Ȏ��j�ւ̃A�N�Z�X�ɂ͖��Ή�
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
			*/

			LRValue headValue = boost::apply_visitor(evaluator, node.head);
			if (headValue.isLValue())
			{
				address = headValue.address();
			}
			else
			{
				Evaluated head = headValue.evaluated();
				if (auto opt = AsOpt<Record>(head))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else if (auto opt = AsOpt<List>(head))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else if (auto opt = AsOpt<FuncVal>(head))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else
				{
					CGL_Error("���ʎq�����e�����ȊO�i�]�����ʂƂ��ăI�u�W�F�N�g��Ԃ��悤�Ȏ��j�ւ̃A�N�Z�X�ɂ͖��Ή�");
				}
			}

			//for (const auto& access : node.accesses)
			for (size_t i = 0; i < node.accesses.size(); ++i)
			{
				const auto& access = node.accesses[i];

				//boost::optional<const Evaluated&> objOpt = pEnv->dereference(address);
				boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(address);
				if (!objOpt)
				{
					CGL_Error("�A�h���X�l���s��");
				}

				const Evaluated& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					if (!IsType<List>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
					}

					Evaluated value = pEnv->expand(boost::apply_visitor(evaluator, listAccessOpt.value().index));

					const List& list = As<const List&>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						address = list.get(indexOpt.value());
					}
					/*else if (auto indexOpt = AsOpt<Address>(value))
					{
						const Evaluated indexValue = pEnv->expandRef(indexOpt.value());

						if (auto indexOpt = AsOpt<int>(indexValue))
						{
							address = list.get(indexOpt.value());
						}
						else
						{
							CGL_Error("list[index] �� index �� int �^�łȂ�");
						}
					}*/
					else
					{
						CGL_Error("list[index] �� index �� int �^�łȂ�");
					}
				}
				else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
				{
					if (!IsType<Record>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
					}

					const Record& record = As<const Record&>(objRef);
					auto it = record.values.find(recordAccessOpt.value().name);
					if (it == record.values.end())
					{
						CGL_Error("�w�肳�ꂽ���ʎq�����R�[�h���ɑ��݂��Ȃ�");
					}

					address = it->second;
				}
				else
				{
					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g���֐��łȂ�");
					}
					const FuncVal& function = As<const FuncVal&>(objRef);

					/*
					if (!IsType<Identifier>(result.headValue))
					{
						std::cerr << "Error(" << __LINE__ << "):FunctionAccess must be a built-in function\n";
						return 0;
					}
					
					const Identifier& identifier = As<Identifier>(result.headValue);
					*/
					//�ȑO�̈Ӗ��_�ł͎Q�Ƃ͑S�Ď��ʎq�ŊǗ����Ă����̂ŁA���ʎq�ɂ��֐��A�N�Z�X���\������
					//�Ӗ��_�̕ύX�ɂ��Q�Ƃ͑S�ăA�h���X�ɒu�������邱�ƂɂȂ����̂ŁA�r���g�C���֐��ɂ��S�ăA�h���X�����蓖�Ă�K�v������

					auto funcAccess = As<FunctionAccess>(access);

					bool hasFreeVariables = false;

					for (const auto& expr : funcAccess.actualArguments)
					{
						//�֐��̈������Q�ƌ^�ł���ꍇ�́AfreeVariables�ɓo�^���ꂽ�Q�Ƃ��ǂ����𒲂ׂ�
						//�o�^����Ă���ꍇ�́A�����ł͕]���ł��Ȃ��̂�SatExpr��FunctionCaller��o�^����
						//�܂��Aa.b(x).c�̂悤�ȃA�N�Z�X�ɂ��āA
						//�A�N�Z�b�T�̒��ň�x�ł��֐����Ăяo���Ă�����A����ȍ~�̃A�N�Z�X�ɂ��Q�Ƃ�freeVariables�Ɣ�邱�Ƃ͂��蓾�Ȃ��B
						//���������āASatExpr�ɂ��֐����w�b�h�Ƃ���ObjectReference�͑��݂�����B

						//���̏���������argument��identifier�̃P�[�X���l���ł��Ă��Ȃ�
						//identifier���܂ގ��̏ꍇ����͂�S�Ē��g�����Č��o���Ȃ���΂Ȃ�Ȃ�

						HasFreeVariables freeValSearcher(pEnv, freeVariables);
						if (boost::apply_visitor(freeValSearcher, expr))
						{
							hasFreeVariables = true;
							break;
						}
					}

					if (hasFreeVariables)
					{
						//satFuncRef��accesses�̎c����Ȃ��Ċ֐��Ƃ��̐�̃A�N�Z�b�T�Ƃ���B
						if (auto satFuncOpt = MakeSatFunctionReference(address, node, i, *this, evaluator, pEnv))
						{
							return satFuncOpt.value();
						}

						CGL_Error("Invalid FunctionAccess");
					}
					else
					{
						std::vector<Evaluated> args;
						for (const auto& expr : funcAccess.actualArguments)
						{
							args.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
						}

						Expr caller = FunctionCaller(function, args);

						const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
						address = pEnv->makeTemporaryValue(returnedValue);

						//result.appendFunctionRef(std::move(args));
					}
				}
			}

			//std::cerr << "Error(" << __LINE__ << "):Invalid Access\n";
			//return 0;
			CGL_Error("Invalid Access");
		}
		else
		{
			Expr expr = node;
			Eval evaluator(pEnv);
			/*
			const Evaluated refVal = boost::apply_visitor(evaluator, expr);

			if (!IsType<Address>(refVal))
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}

			Address address = As<Address>(refVal);
			if (!address)
			{
				ErrorLog("�A�h���X������");
				return 0;
			}
			*/

			Address address;

			const LRValue refVal = boost::apply_visitor(evaluator, expr);
			if (refVal.isLValue())
			{
				address = refVal.address();
			}
			else
			{
				CGL_Error("�A�N�Z�b�T�̕]�����ʂ��A�h���X�łȂ�");
			}

			//const ObjectReference& currentRefVal = As<ObjectReference>(refVal);

			for (size_t i = 0; i < freeVariables.size(); ++i)
			{
				//freeVariables�ɑ��݂����ꍇ�́A�œK���p�̕ϐ�������A���̎Q�Ƃ�Ԃ�
				//�܂��AfreeVariables���ɂ����̕ϐ����g�p���邱�Ƃ�m�点��
				if (freeVariables[i] == address)
				{
					if (usedInSat[i] == 1)
					{
						return satRefs[i];
					}
					else
					{
						usedInSat[i] = 1;

						SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
						satRefs[i] = satRef;

						refs.push_back(address);
						//std::cout << "NewRef(" << satRef.refID << ")\n";
						CGL_DebugLog("NewRef(" + std::to_string(satRef.refID) + ")");
						return satRef;
					}
				}
			}

			//freeVariables�ɑ��݂��Ȃ������ꍇ�́A�����ɕ]�����Ă悢�i�萔���̏�ݍ��݁j
			//const Evaluated evaluated = pEnv->expandRef(address);
			const Evaluated evaluated = pEnv->expand(address);

			if (IsType<double>(evaluated))
			{
				return As<double>(evaluated);
			}
			else if (IsType<int>(evaluated))
			{
				return static_cast<double>(As<int>(evaluated));
			}

			//int/double����Ȃ��ꍇ�͂Ƃ肠�����G���[�Bbool�͗L�蓾��H
			CGL_Error("int/double����Ȃ��ꍇ�͂Ƃ肠�����G���[�Bbool�͗L�蓾��H");
		}

		return 0.0;
	}
#endif

	Expr Expr2SatExpr::operator()(const Accessor& node)
	{
		Address headAddress;
		const Expr& head = node.head;

		//head��sat�����̃��[�J���ϐ�
		if (IsType<Identifier>(head))
		{
			Address address = pEnv->findAddress(As<Identifier>(head));
			if (!address.isValid())
			{
				CGL_Error("���ʎq����`����Ă��܂���");
			}

			//head�͕K�� Record/List/FuncVal �̂ǂꂩ�ł���Adouble�^�ł��邱�Ƃ͂��蓾�Ȃ��B
			//���������āAfree�ϐ��ɂ��邩�ǂ����͍l�������ifree�ϐ��͏璷�Ɏw��ł���̂ł������Ƃ��Ă��ʂɃG���[�ł͂Ȃ��j�A
			//����Evaluated�Ƃ��ēW�J����
			//result.head = LRValue(address);
			headAddress = address;
		}
		//head���A�h���X�l
		else if (IsType<LRValue>(head))
		{
			const LRValue& headAddressValue = As<LRValue>(head);
			if (!headAddressValue.isLValue())
			{
				CGL_Error("sat�����̃A�N�Z�b�T�̐擪�����s���Ȓl�ł�");
			}

			const Address address = headAddressValue.address();

			//����Identifier�Ɠ��l�ɒ��ړW�J����
			//result.head = LRValue(address);
			headAddress = address;
		}
		else
		{
			CGL_Error("sat���̃A�N�Z�b�T�̐擪���ɒP��̎��ʎq�ȊO�̎���p���邱�Ƃ͂ł��܂���");
		}

		Eval evaluator(pEnv);

		Accessor result;

		//TODO: �A�N�Z�b�T��free�ϐ��������Ȃ��ԁA���ꎩ�g��free�ϐ��w�肳���܂ł̃A�h���X����ݍ���
		bool selfDependsOnFreeVariables = false;
		bool childDependsOnFreeVariables = false;
		{
			HasFreeVariables searcher(pEnv, freeVariables);
			const Expr headExpr = LRValue(headAddress);
			selfDependsOnFreeVariables = boost::apply_visitor(searcher, headExpr);
		}

		for (const auto& access : node.accesses)
		{
			const Address lastHeadAddress = headAddress;

			boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(headAddress);
			if (!objOpt)
			{
				CGL_Error("�Q�ƃG���[");
			}

			const Evaluated& objRef = objOpt.value();

			if (IsType<ListAccess>(access))
			{
				const ListAccess& listAccess = As<ListAccess>(access);

				{
					HasFreeVariables searcher(pEnv, freeVariables);
					childDependsOnFreeVariables = childDependsOnFreeVariables || boost::apply_visitor(searcher, listAccess.index);
				}

				if (childDependsOnFreeVariables)
				{
					Expr accessIndex = boost::apply_visitor(*this, listAccess.index);
					result.add(ListAccess(accessIndex));
				}
				else
				{
					Evaluated value = pEnv->expand(boost::apply_visitor(evaluator, listAccess.index));

					if (!IsType<List>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����X�g�łȂ�");
					}

					const List& list = As<const List&>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						headAddress = list.get(indexOpt.value());
					}
					else
					{
						CGL_Error("list[index] �� index �� int �^�łȂ�");
					}
				}
			}
			else if (IsType<RecordAccess>(access))
			{
				const RecordAccess& recordAccess = As<RecordAccess>(access);

				if (childDependsOnFreeVariables)
				{
					result.add(access);
				}
				else
				{
					if (!IsType<Record>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g�����R�[�h�łȂ�");
					}

					const Record& record = As<const Record&>(objRef);
					auto it = record.values.find(recordAccess.name);
					if (it == record.values.end())
					{
						CGL_Error("�w�肳�ꂽ���ʎq�����R�[�h���ɑ��݂��Ȃ�");
					}

					headAddress = it->second;
				}
			}
			else
			{
				const FunctionAccess& funcAccess = As<FunctionAccess>(access);

				{
					HasFreeVariables searcher(pEnv, freeVariables);
					for (const auto& arg : funcAccess.actualArguments)
					{
						childDependsOnFreeVariables = childDependsOnFreeVariables || boost::apply_visitor(searcher, arg);
					}
				}

				if (childDependsOnFreeVariables)
				{
					FunctionAccess resultAccess;
					for (const auto& arg : funcAccess.actualArguments)
					{
						resultAccess.add(boost::apply_visitor(*this, arg));
					}
					result.add(resultAccess);
				}
				else
				{
					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("�I�u�W�F�N�g���֐��łȂ�");
					}

					const FuncVal& function = As<const FuncVal&>(objRef);

					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
					}

					Expr caller = FunctionCaller(function, args);
					const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
					headAddress = pEnv->makeTemporaryValue(returnedValue);
				}
			}

			if(lastHeadAddress != headAddress && !selfDependsOnFreeVariables)
			{
				HasFreeVariables searcher(pEnv, freeVariables);
				const Expr headExpr = LRValue(headAddress);
				selfDependsOnFreeVariables = boost::apply_visitor(searcher, headExpr);
			}
		}

		/*
		selfDependsOnFreeVariables��childDependsOnFreeVariables������true : �G���[
		selfDependsOnFreeVariables��true  : �A�N�Z�b�T�{�̂�free�i�A�N�Z�b�T��]������ƕK���P���double�^�ɂȂ�j
		childDependsOnFreeVariables��true : �A�N�Z�b�T�̈�����free�i���X�g�C���f�b�N�X���֐������j
		*/
		if (selfDependsOnFreeVariables && childDependsOnFreeVariables)
		{
			CGL_Error("sat�����̃A�N�Z�b�T�ɂ��āA�{�̂ƈ����̗�����free�ϐ��Ɏw�肷�邱�Ƃ͂ł��܂���");
		}
		else if (selfDependsOnFreeVariables)
		{
			if (!result.accesses.empty())
			{
				CGL_Error("�����͒ʂ�Ȃ��͂�");
			}
			
			if (auto satRefOpt = addSatRef(headAddress))
			{
				return satRefOpt.value();
			}
			else
			{
				CGL_Error("�����͒ʂ�Ȃ��͂�");
			}
		}
		/*else if (childDependsOnFreeVariables)
		{
			CGL_Error("TODO: �A�N�Z�b�T�̈�����free�ϐ��w��͖��Ή�");
		}*/

		result.head = LRValue(headAddress);

		return result;
	}

	/*
	Expr ExprFuncExpander::operator()(const Accessor& accessor)
	{
		return 0;
		//ObjectReference result;

		//Eval evaluator(pEnv);
		//Evaluated headValue = boost::apply_visitor(evaluator, accessor.head);
		//if (auto opt = AsOpt<Identifier>(headValue))
		//{
		//	result.headValue = opt.value();
		//}
		//else if (auto opt = AsOpt<Record>(headValue))
		//{
		//	result.headValue = opt.value();
		//}
		//else if (auto opt = AsOpt<List>(headValue))
		//{
		//	result.headValue = opt.value();
		//}
		//else if (auto opt = AsOpt<FuncVal>(headValue))
		//{
		//	result.headValue = opt.value();
		//}
		//else
		//{
		//	//�G���[�F���ʎq�����e�����ȊO�i�]�����ʂƂ��ăI�u�W�F�N�g��Ԃ��悤�Ȏ��j�ւ̃A�N�Z�X�ɂ͖��Ή�
		//	std::cerr << "Error(" << __LINE__ << ")\n";
		//	return 0;
		//}

		//for (const auto& access : accessor.accesses)
		//{
		//	if (auto listOpt = AsOpt<ListAccess>(access))
		//	{
		//		Evaluated value = boost::apply_visitor(evaluator, listOpt.value().index);

		//		if (auto indexOpt = AsOpt<int>(value))
		//		{
		//			result.appendListRef(indexOpt.value());
		//		}
		//		else
		//		{
		//			//�G���[�Flist[index] �� index �� int �^�łȂ�
		//			std::cerr << "Error(" << __LINE__ << ")\n";
		//			return 0;
		//		}
		//	}
		//	else if (auto recordOpt = AsOpt<RecordAccess>(access))
		//	{
		//		result.appendRecordRef(recordOpt.value().name.name);
		//	}
		//	else
		//	{
		//		auto funcAccess = As<FunctionAccess>(access);

		//		std::vector<Evaluated> args;
		//		for (const auto& expr : funcAccess.actualArguments)
		//		{
		//			args.push_back(boost::apply_visitor(evaluator, expr));
		//		}
		//		result.appendFunctionRef(std::move(args));
		//	}
		//}

		//return result;
	}
	*/

	void Environment::printEnvironment()const
	{
		std::cout << "Print Environment Begin:\n";

		std::cout << "Values:\n";
		for (const auto& keyval : m_values)
		{
			const auto& val = keyval.second;

			std::cout << keyval.first.toString() << " : ";

			printEvaluated(val, nullptr);
		}

		std::cout << "References:\n";
		for (size_t d = 0; d < localEnv().size(); ++d)
		{
			std::cout << "Depth : " << d << "\n";
			const auto& names = localEnv()[d];

			for (const auto& keyval : names)
			{
				std::cout << keyval.first << " : " << keyval.second.toString() << "\n";
			}
		}

		std::cout << "Print Environment End:\n";
	}

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

#ifdef commentout
	ObjectReference Environment::makeFuncVal(const std::vector<Identifier>& arguments, const Expr& expr)
	{
		std::cout << "makeFuncVal_A" << std::endl;
		auto referenceableVariables = currentReferenceableVariables();

		//���̊֐����Q�Ƃ��Ă���ϐ����̃��X�g�����
		{
			//referenceableVariables �͌��݂̃X�R�[�v�ŎQ�Ɖ\�ȕϐ��S�Ă�\���Ă���
			//���ۂɊ֐��̒��ŎQ�Ƃ����ϐ��͂��̒��̂����ꕔ�Ȃ̂ŁA���O�Ƀt�B���^���|������������

			//�X�R�[�v�Ԃœ����̕ϐ�������ꍇ�͓����̕ϐ��ŎՕ�����āA�O���̕ϐ��͎Q�Ƃ���Ȃ��̂ō폜���Ă悢
			for (auto scopeIt = referenceableVariables.rbegin(); scopeIt + 1 != referenceableVariables.rend(); ++scopeIt)
			{
				for (const auto& innerName : *scopeIt)
				{
					for (auto outerScopeIt = scopeIt + 1; outerScopeIt != referenceableVariables.rend(); ++outerScopeIt)
					{
						const auto outerNameIt = outerScopeIt->find(innerName);
						if (outerNameIt != outerScopeIt->end())
						{
							outerScopeIt->erase(outerNameIt);
						}
					}
				}
			}

			std::cout << "makeFuncVal_B" << std::endl;

			std::vector<std::string> names;
			for (size_t v = 0; v < referenceableVariables.size(); ++v)
			{
				const auto& ns = referenceableVariables[v];
				//for (int index = static_cast<int>(ns.size()) - 1; 0 <= index; --index)
				{
					//names.insert(names.end(), ns.begin(), ns.end());
					names.insert(names.end(), ns.rbegin(), ns.rend());
				}
			}

			std::cout << "Names: ";
			for (const auto& n : names)
			{
				std::cout << n << " ";
			}
			std::cout << std::endl;

			std::cout << "makeFuncVal_C" << std::endl;
			CheckNameAppearance checker(names);
			boost::apply_visitor(checker, expr);
			std::cout << "makeFuncVal_D" << std::endl;

			/*for (size_t v = 0, i = 0; v < referenceableVariables.size(); ++v)
			{
				auto& ns = referenceableVariables[v];
				for (int index = static_cast<int>(ns.size()) - 1; 0 <= index; ++i)
				{
					if (checker.appearances[i] == 1)
					{
						--index;
						continue;
					}
					else
					{
						ns.erase(std::next(ns.begin(), index));
					}
				}
			}*/

			
			for (int i = 0; i < names.size(); ++i)
			{
				if (checker.appearances[i] == 0)
				{
					const std::string& deleteName = checker.variableNames[i];

					//for (size_t v = 0, i = 0; v < referenceableVariables.size(); ++v)
					for (size_t v = 0; v < referenceableVariables.size(); ++v)
					{
						auto& ns = referenceableVariables[v];
						ns.erase(deleteName);
					}
				}
			}
			
		}

		std::cout << "makeFuncVal_E" << std::endl;

		FuncVal val(arguments, expr, referenceableVariables, scopeDepth());

		std::cout << "makeFuncVal_F" << std::endl;
		const unsigned valueID = makeTemporaryValue(val);
		std::cout << "makeFuncVal_G" << std::endl;
		//m_funcValIDs.push_back(valueID);

		std::cout << "makeFuncVal_H" << std::endl;
		return ObjectReference(valueID);
	}

	void Environment::exitScope()
	{
		{
			//���̃X�R�[�v�𔲂���ƍ폜�����ϐ����X�g
			const auto& deletingVariables = m_variables.back();

			{
				std::cout << "Prev Decrement:" << std::endl;
				for (const unsigned id : m_funcValIDs)
				{
					if (auto funcValOpt = AsOpt<FuncVal>(m_values[id]))
					{
						std::cout << "func depth: " << funcValOpt.value().currentScopeDepth << std::endl;
					}
				}
			}

			//���폜���悤�Ƃ��Ă���ϐ��ŁA�Ȃ�������X�R�[�v����Q�Ɖ\�ȕϐ��̃��X�g��Ԃ�
			const auto intersectNames = [&](const std::vector<std::set<std::string>>& names)->std::vector<std::string>
			{
				std::vector<std::string> resultNames;

				for (const auto& vs : names)
				{
					for (const auto& vname : vs)
					{
						if (deletingVariables.find(vname) != deletingVariables.end())
						{
							resultNames.push_back(vname);
						}
					}
				}

				return resultNames;
			};

			for (const unsigned id : m_funcValIDs)
			{
				if (auto funcValOpt = AsOpt<FuncVal>(m_values[id]))
				{
					if (funcValOpt.value().currentScopeDepth == scopeDepth())
					{
						//�֐��������ŊO�̕ϐ����Q�Ƃ��Ă��鎞�A���̕ϐ����֐�����ɉ������Ă��܂��ƍ���̂ŁA���̃^�C�~���O��pop�����ϐ��ɂ��Ċ֐��̓����ɑ��݂���ϐ������̒l�Œu��������Ƃ����������s���B

						//���̌�AcurrentScopeDepth��1�f�N�������g����B
						
						std::map<std::string, Evaluated> variableNames;

						const auto names = intersectNames(funcValOpt.value().referenceableVariables);
						for (const auto& name : names)
						{
							const auto valueOpt = find(name);
							if (!valueOpt)
							{
								//���ꂩ��폜�����ϐ���ۑ����悤�Ƃ��������ɏ�����Ă���
								std::cerr << "Error(" << __LINE__ << ")\n";
								return;
							}

							variableNames[name] = valueOpt.value();
						}

						std::cout << "exitScope: Replace Variable Names: " << std::endl;
						for (const auto& name : variableNames)
						{
							std::cout << name.first << " ";
						}
						std::cout << std::endl;
						
						//�폜�����ϐ���萔�ɒu��������
						ReplaceExprValue replacer(variableNames);
						funcValOpt.value().expr = boost::apply_visitor(replacer, funcValOpt.value().expr);

						std::cout << "exitScope: Function Expr Replaced to: " << std::endl;
						printExpr(funcValOpt.value().expr);
						std::cout << std::endl;

						std::cout << "exitScope: decrement" << std::endl;
						--funcValOpt.value().currentScopeDepth;
					}
				}
				else
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
				}
			}
			{
				std::cout << "Post Decrement:" << std::endl;
				for (const unsigned id : m_funcValIDs)
				{
					if (auto funcValOpt = AsOpt<FuncVal>(m_values[id]))
					{
						std::cout << "func depth: " << funcValOpt.value().currentScopeDepth << std::endl;
					}
				}
			}
		}

		m_variables.pop_back();
	}

#endif

	/*
	Address Environment::dereference(const Evaluated& reference)
	{
		if (auto nameOpt = AsOpt<Identifier>(reference))
		{
			const boost::optional<unsigned> valueIDOpt = findValueID(nameOpt.value().name);
			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return reference;
			}

			return m_values[valueIDOpt.value()];
		}
		else if (auto objRefOpt = AsOpt<ObjectReference>(reference))
		{
			const auto& referenceProcess = objRefOpt.value();

			boost::optional<unsigned> valueIDOpt;

			if (auto idOpt = AsOpt<unsigned>(referenceProcess.headValue))
			{
				valueIDOpt = idOpt.value();
			}
			else if (auto opt = AsOpt<Identifier>(referenceProcess.headValue))
			{
				valueIDOpt = findValueID(opt.value().name);
			}
			else if (auto opt = AsOpt<Record>(referenceProcess.headValue))
			{
				valueIDOpt = makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<List>(referenceProcess.headValue))
			{
				valueIDOpt = makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<FuncVal>(referenceProcess.headValue))
			{
				valueIDOpt = makeTemporaryValue(opt.value());
			}

			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return reference;
			}

			std::cout << "Reference: " << objRefOpt.value().asString() << "\n";

			boost::optional<const Evaluated&> result = m_values[valueIDOpt.value()];

			for (const auto& ref : referenceProcess.references)
			{
				if (auto listRefOpt = AsOpt<ObjectReference::ListRef>(ref))
				{
					const int index = listRefOpt.value().index;

					if (auto listOpt = AsOpt<List>(result.value()))
					{
						listOpt.value().data[index];;
						listOpt.value().data;
						result = listOpt.value().data[index];
					}
					else
					{
						//���X�g�Ƃ��ăA�N�Z�X����̂Ɏ��s
						std::cerr << "Error(" << __LINE__ << ")\n";
						return reference;
					}
				}
				else if (auto recordRefOpt = AsOpt<ObjectReference::RecordRef>(ref))
				{
					const std::string& key = recordRefOpt.value().key;

					if (auto recordOpt = AsOpt<Record>(result.value()))
					{
						result = recordOpt.value().values.at(key);
					}
					else
					{
						//���R�[�h�Ƃ��ăA�N�Z�X����̂Ɏ��s
						std::cerr << "Error(" << __LINE__ << ")\n";
						return reference;
					}
				}
				else if (auto funcRefOpt = AsOpt<ObjectReference::FunctionRef>(ref))
				{
					boost::optional<const FuncVal&> funcValOpt;

					std::cout << "FuncRef ";
					if (auto recordOpt = AsOpt<FuncVal>(result.value()))
					{
						std::cout << "is FuncVal ";
						funcValOpt = recordOpt.value();
					}
					else if (IsType<ObjectReference>(result.value()))
					{
						std::cout << "is ObjectReference ";
						if (auto funcValOpt2 = AsOpt<FuncVal>(dereference(result.value())))
						{
							std::cout << "is FuncVal ";
							funcValOpt = funcValOpt2.value();
						}
						else
						{
							std::cout << "isn't FuncVal ";
						}
					}
					else
					{
						std::cout << "isn't AnyRef: ";
						printEvaluated(result.value());
					}
					
					std::cout << std::endl;

					if (funcValOpt)
					{
						Expr caller = FunctionCaller(funcValOpt.value(), funcRefOpt.value().args);

						if (auto sharedThis = m_weakThis.lock())
						{
							Eval evaluator(sharedThis);

							const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
							result = m_values[ID];
						}
						else
						{
							//�G���[�Fm_weakThis����iEnvironment::Make���g�킸�����������H�j
							std::cerr << "Error(" << __LINE__ << ")\n";
							return reference;
						}
					}
					else
					{
						//�֐��Ƃ��ăA�N�Z�X����̂Ɏ��s
						std::cerr << "Error(" << __LINE__ << ")\n";
						return reference;
					}

					//if (auto recordOpt = AsOpt<FuncVal>(result.value()))
					//{
					//	Expr caller = FunctionCaller(recordOpt.value(), funcRefOpt.value().args);

					//	if (auto sharedThis = m_weakThis.lock())
					//	{
					//		Eval evaluator(sharedThis);

					//		const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
					//		result = m_values[ID];
					//	}
					//	else
					//	{
					//		//�G���[�Fm_weakThis����iEnvironment::Make���g�킸�����������H�j
					//		std::cerr << "Error(" << __LINE__ << ")\n";
					//		return reference;
					//	}
					//}
					//else
					//{
					//	//�֐��Ƃ��ăA�N�Z�X����̂Ɏ��s
					//	std::cerr << "Error(" << __LINE__ << ")\n";
					//	return reference;
					//}
				}
			}

			return result.value();
		}

		return reference;
	}
	*/

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

			CGL_Error("�A�N�Z�b�T�̕]�����ʂ��A�h���X�l�łȂ�");
		}

		CGL_Error("shared this does not exist.");
		return Address::Null();
	}

	//std::pair<FunctionCaller, std::vector<Access>> Accessor::getFirstFunction(std::shared_ptr<Environment> pEnv)
	//{
	//	for (const auto& a : accesses)
	//	{
	//		if (IsType<FunctionAccess>(a))
	//		{
	//			As<FunctionAccess>(a);
	//			;
	//			//return true;
	//		}
	//	}

	//	//return false;
	//}

	Expr Environment::expandFunction(const Expr & expr)
	{
		return expr;
	}

	std::vector<Address> Environment::expandReferences(Address address)
	{
		std::vector<Address> result;
		if (auto sharedThis = m_weakThis.lock())
		{
			const auto addElementRec = [&](auto rec, Address address)->void
			{
				const Evaluated value = sharedThis->expand(address);

				//�ǐՑΏۂ̕ϐ��ɂ��ǂ蒅�����炻����Q�Ƃ���A�h���X���o�͂ɒǉ�
				if (IsType<int>(value) || IsType<double>(value) /*|| IsType<bool>(value)*/)//TODO:bool�͏����I�ɑΉ�
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
				//����ȊO�̃f�[�^�͓��ɕߑ����Ȃ�
				//TODO:�ŏI�I��int��double �ȊO�̃f�[�^�ւ̎Q�Ƃ͎����Ƃɂ��邩�H
			};

			const auto addElement = [&](const Address address)
			{
				addElementRec(addElementRec, address);
			};

			addElement(address);
		}

		return result;
	}

#ifdef commentout
	std::vector<ObjectReference> Environment::expandReferences(const ObjectReference & reference, std::vector<ObjectReference>& output)
	{
		if (auto sharedThis = m_weakThis.lock())
		{
			const auto addElementRec = [&](auto rec, const ObjectReference& refVal)->void
			{
				const Evaluated value = sharedThis->dereference(refVal);

				if (IsType<int>(value) || IsType<double>(value) /*|| IsType<bool>(value)*/)//TODO:bool�͏����I�ɑΉ�
				{
					output.push_back(refVal);
				}
				else if (IsType<List>(value))
				{
					const auto& list = As<List>(value).data;
					for (size_t i = 0; i < list.size(); ++i)
					{
						ObjectReference newRef = refVal;
						newRef.appendListRef(i);
						rec(rec, newRef);
					}
				}
				else if (IsType<Record>(value))
				{
					for (const auto& elem : As<Record>(value).values)
					{
						ObjectReference newRef = refVal;
						newRef.appendRecordRef(elem.first);
						rec(rec, newRef);
					}
				}
				else
				{
					std::cerr << "���Ή�";
					//TODO:�ŏI�I��int��double �ȊO�̃f�[�^�ւ̎Q�Ƃ͎����Ƃɂ��邩�H
				}
			};

			const auto addElement = [&](const ObjectReference& refVal)
			{
				addElementRec(addElementRec, refVal);
			};

			addElement(reference);
		}

		return output;
	}

	//inline void Environment::assignToObject(const ObjectReference & objectRef, const Evaluated & newValue)
	inline void Environment::assignToObject(Address address, const Evaluated & newValue)
	{
		boost::optional<unsigned> valueIDOpt;

		if (auto idOpt = AsOpt<unsigned>(objectRef.headValue))
		{
			valueIDOpt = idOpt.value();
		}
		if (auto opt = AsOpt<Identifier>(objectRef.headValue))
		{
			valueIDOpt = findValueID(opt.value().name);
			if (!valueIDOpt)
			{
				bindNewValue(opt.value().name, newValue);
			}
			valueIDOpt = findValueID(opt.value().name);
		}
		else if (auto opt = AsOpt<Record>(objectRef.headValue))
		{
			valueIDOpt = makeTemporaryValue(opt.value());
		}
		else if (auto opt = AsOpt<List>(objectRef.headValue))
		{
			valueIDOpt = makeTemporaryValue(opt.value());
		}
		else if (auto opt = AsOpt<FuncVal>(objectRef.headValue))
		{
			valueIDOpt = makeTemporaryValue(opt.value());
		}

		if (!valueIDOpt)
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return;
		}

		boost::optional<Evaluated&> result = m_values[valueIDOpt.value()];

		for (const auto& ref : objectRef.references)
		{
			if (auto listRefOpt = AsOpt<ObjectReference::ListRef>(ref))
			{
				const int index = listRefOpt.value().index;

				if (auto listOpt = AsOpt<List>(result.value()))
				{
					//result = listOpt.value().data[index];
					result = m_values[listOpt.value().data[index].valueID];
				}
				else//���X�g�Ƃ��ăA�N�Z�X����̂Ɏ��s
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}
			}
			else if (auto recordRefOpt = AsOpt<ObjectReference::RecordRef>(ref))
			{
				const std::string& key = recordRefOpt.value().key;

				if (auto recordOpt = AsOpt<Record>(result.value()))
				{
					//result = recordOpt.value().values.at(key);
					result = m_values[recordOpt.value().values.at(key).valueID];
				}
				else//���R�[�h�Ƃ��ăA�N�Z�X����̂Ɏ��s
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}
			}
			else if (auto funcRefOpt = AsOpt<ObjectReference::FunctionRef>(ref))
			{
				boost::optional<const FuncVal&> funcValOpt;
				if (auto recordOpt = AsOpt<FuncVal>(result.value()))
				{
					funcValOpt = recordOpt.value();
				}
				else if (IsType<ObjectReference>(result.value()))
				{
					if (auto funcValOpt = AsOpt<FuncVal>(dereference(result.value())))
					{
						funcValOpt = funcValOpt.value();
					}
				}

				if (funcValOpt)
				{
					Expr caller = FunctionCaller(funcValOpt.value(), funcRefOpt.value().args);

					if (auto sharedThis = m_weakThis.lock())
					{
						Eval evaluator(sharedThis);
						const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
						result = m_values[ID];
					}
					else
					{
						//�G���[�Fm_weakThis����iEnvironment::Make���g�킸�����������H�j
						std::cerr << "Error(" << __LINE__ << ")\n";
						return;
					}
				}
				else
				{
					//�֐��Ƃ��ăA�N�Z�X����̂Ɏ��s
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}

				/*
				if (auto recordOpt = AsOpt<FuncVal>(result.value()))
				{
					Expr caller = FunctionCaller(recordOpt.value(), funcRefOpt.value().args);

					if (auto sharedThis = m_weakThis.lock())
					{
						Eval evaluator(sharedThis);
						const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
						result = m_values[ID];
					}
					else//�G���[�Fm_weakThis����iEnvironment::Make���g�킸�����������H�j
					{
						std::cerr << "Error(" << __LINE__ << ")\n";
						return;
					}
				}
				else//�֐��Ƃ��ăA�N�Z�X����̂Ɏ��s
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}
				*/
			}
		}

		result.value() = newValue;
	}
#endif

	void OptimizationProblemSat::addConstraint(const Expr& logicExpr)
	{
		if (candidateExpr)
		{
			candidateExpr = BinaryExpr(candidateExpr.value(), logicExpr, BinaryOp::And);
		}
		else
		{
			candidateExpr = logicExpr;
		}
	}

	/*
	void OptimizationProblemSat::constructConstraint(std::shared_ptr<Environment> pEnv, std::vector<Address>& freeVariables)
	{
		if (!candidateExpr)
		{
			expr = boost::none;
			return;
		}

		Expr2SatExpr evaluator(0, pEnv, freeVariables);
		expr = boost::apply_visitor(evaluator, candidateExpr.value());
		refs.insert(refs.end(), evaluator.refs.begin(), evaluator.refs.end());
		
		{
			//std::cout << "Print:\n";
			CGL_DebugLog("Print:");
			std::stringstream ss;
			PrintSatExpr printer(data, ss);
			boost::apply_visitor(printer, expr.value());
			//std::cout << "\n";
			CGL_DebugLog(ss.str());
		}

		//sat�ɏo�Ă��Ȃ�freeVariables�̍폜
		for (int i = static_cast<int>(freeVariables.size()) - 1; 0 <= i; --i)
		{
			if (evaluator.usedInSat[i] == 0)
			{
				freeVariables.erase(freeVariables.begin() + i);
			}
		}
	}
	*/

	void OptimizationProblemSat::constructConstraint(std::shared_ptr<Environment> pEnv, std::vector<Address>& freeVariables)
	{
		if (!candidateExpr)
		{
			expr = boost::none;
			return;
		}

		Expr2SatExpr evaluator(0, pEnv, freeVariables);
		expr = boost::apply_visitor(evaluator, candidateExpr.value());
		refs.insert(refs.end(), evaluator.refs.begin(), evaluator.refs.end());

		/*
		{
			CGL_DebugLog("Print:");
			std::stringstream ss;
			PrintSatExpr printer(data, ss);
			boost::apply_visitor(printer, expr.value());
			CGL_DebugLog(ss.str());
		}
		*/

		{
			CGL_DebugLog("Print:");
			Printer printer;
			boost::apply_visitor(printer, expr.value());
			CGL_DebugLog("");
		}

		//sat�ɏo�Ă��Ȃ�freeVariables�̍폜
		for (int i = static_cast<int>(freeVariables.size()) - 1; 0 <= i; --i)
		{
			if (evaluator.usedInSat[i] == 0)
			{
				freeVariables.erase(freeVariables.begin() + i);
			}
		}
	}

	bool OptimizationProblemSat::initializeData(std::shared_ptr<Environment> pEnv)
	{
		//std::cout << "Begin OptimizationProblemSat::initializeData" << std::endl;

		data.resize(refs.size());

		for (size_t i = 0; i < data.size(); ++i)
		{
			//const Evaluated val = pEnv->expandRef(refs[i]);
			const Evaluated val = pEnv->expand(refs[i]);
			if (auto opt = AsOpt<double>(val))
			{
				CGL_DebugLog(ToS(i) + " : " + ToS(opt.value()));
				data[i] = opt.value();
			}
			else if (auto opt = AsOpt<int>(val))
			{
				CGL_DebugLog(ToS(i) + " : " + ToS(opt.value()));
				data[i] = opt.value();
			}
			else
			{
				//���݂��Ȃ��Q�Ƃ�sat�Ɏw�肵��
				/*std::cerr << "Error(" << __LINE__ << "): reference does not exist.\n";
				return false;*/
				CGL_Error("���݂��Ȃ��Q�Ƃ�sat�Ɏw�肵��");
			}
		}

		//std::cout << "End OptimizationProblemSat::initializeData" << std::endl;
		return true;
	}

	double OptimizationProblemSat::eval(std::shared_ptr<Environment> pEnv)
	{
		if (!expr)
		{
			return 0.0;
		}

		if (data.empty())
		{
			CGL_WarnLog("free���ɗL���ȕϐ����w�肳��Ă��܂���B");
			return 0.0;
		}

		EvalSatExpr evaluator(pEnv, data);
		const Evaluated evaluated = boost::apply_visitor(evaluator, expr.value());
		
		if (!IsType<double>(evaluated))
		{
			CGL_Error("sat���̕]�����ʂ��s��");
		}

		return As<double>(evaluated);
	}

	void OptimizationProblemSat::debugPrint()
	{
		if (!expr)
		{
			return;
		}

		/*std::stringstream ss;
		PrintSatExpr printer(data, ss);
		boost::apply_visitor(printer, expr.value());
		CGL_DebugLog(ss.str());*/

		Printer printer;
		boost::apply_visitor(printer, expr.value());
	}
}

namespace cgl
{
	auto MakeUnaryExpr(UnaryOp op)
	{
		return boost::phoenix::bind([](const auto & e, UnaryOp op) {
			return UnaryExpr(e, op);
		}, boost::spirit::_1, op);
	}

	auto MakeBinaryExpr(BinaryOp op)
	{
		return boost::phoenix::bind([&](const auto& lhs, const auto& rhs, BinaryOp op) {
			return BinaryExpr(lhs, rhs, op);
		}, boost::spirit::_val, boost::spirit::_1, op);
	}

	template <class F, class... Args>
	auto Call(F func, Args... args)
	{
		return boost::phoenix::bind(func, args...);
	}

	template<class FromT, class ToT>
	auto Cast()
	{
		return boost::phoenix::bind([&](const FromT& a) {return static_cast<ToT>(a); }, boost::spirit::_1);
	}

	using namespace boost::spirit;

	template<typename Iterator>
	struct SpaceSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		SpaceSkipper() :SpaceSkipper::base_type(skip)
		{
			skip = +(lit(' ') ^ lit('\r') ^ lit('\t'));
		}
	};

	template<typename Iterator>
	struct LineSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		LineSkipper() :LineSkipper::base_type(skip)
		{
			skip = ascii::space;
		}
	};

	struct keywords_t : qi::symbols<char, qi::unused_type> {
		keywords_t() {
			add("for", qi::unused)
				("in", qi::unused)
				("sat", qi::unused)
				("free", qi::unused);
		}
	} const keywords;

	using IteratorT = std::string::const_iterator;
	using SpaceSkipperT = SpaceSkipper<IteratorT>;
	using LineSkipperT = LineSkipper<IteratorT>;

	SpaceSkipperT spaceSkipper;
	LineSkipperT lineSkipper;

	using qi::char_;

	template<typename Iterator, typename Skipper>
	struct Parser
		: qi::grammar<Iterator, Lines(), Skipper>
	{
		qi::rule<Iterator, DeclSat(), Skipper> constraints;
		qi::rule<Iterator, DeclFree(), Skipper> freeVals;

		qi::rule<Iterator, FunctionAccess(), Skipper> functionAccess;
		qi::rule<Iterator, RecordAccess(), Skipper> recordAccess;
		qi::rule<Iterator, ListAccess(), Skipper> listAccess;
		qi::rule<Iterator, Accessor(), Skipper> accessor;

		qi::rule<Iterator, Access(), Skipper> access;

		qi::rule<Iterator, KeyExpr(), Skipper> record_keyexpr;
		qi::rule<Iterator, RecordConstractor(), Skipper> record_maker;
		qi::rule<Iterator, RecordInheritor(), Skipper> record_inheritor;

		qi::rule<Iterator, ListConstractor(), Skipper> list_maker;
		qi::rule<Iterator, For(), Skipper> for_expr;
		qi::rule<Iterator, If(), Skipper> if_expr;
		qi::rule<Iterator, Return(), Skipper> return_expr;
		qi::rule<Iterator, DefFunc(), Skipper> def_func;
		qi::rule<Iterator, Arguments(), Skipper> arguments;
		qi::rule<Iterator, Identifier(), Skipper> id;
		qi::rule<Iterator, Expr(), Skipper> general_expr, logic_expr, logic_term, logic_factor, compare_expr, arith_expr, basic_arith_expr, term, factor, pow_term, pow_term1;
		qi::rule<Iterator, Lines(), Skipper> expr_seq, statement;
		qi::rule<Iterator, Lines(), Skipper> program;

		//qi::rule<Iterator, std::string(), Skipper> double_value;
		//qi::rule<Iterator, Identifier(), Skipper> double_value, double_value2;

		qi::rule<Iterator> s, s1;
		qi::rule<Iterator> distinct_keyword;
		qi::rule<Iterator, std::string(), Skipper> unchecked_identifier;
		
		Parser() : Parser::base_type(program)
		{
			auto concatLines = [](Lines& lines, Expr& expr) { lines.concat(expr); };

			auto makeLines = [](Expr& expr) { return Lines(expr); };

			auto makeDefFunc = [](const Arguments& arguments, const Expr& expr) { return DefFunc(arguments, expr); };

			//auto addCharacter = [](Identifier& identifier, char c) { identifier.name.push_back(c); };

			auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };

			auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

			//auto makeDouble = [](const Identifier& str) { return std::stod(str.name); };
			//auto makeString = [](char c) { return Identifier(std::string({ c })); };
			//auto appendString = [](Identifier& str, char c) { str.name.push_back(c); };
			//auto appendString2 = [](Identifier& str, const Identifier& str2) { str.name.append(str2.name); };

			program = s >> -(expr_seq) >> s;

			/*expr_seq = general_expr[_val = Call(makeLines, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(concatLines, _val, _1)])
				| (+(lit('\n')) >> general_expr[Call(concatLines, _val, _1)])
				);*/

			expr_seq = statement[_val = _1] >> *(
				+(lit('\n')) >> statement[Call(Lines::Concat, _val, _1)]
				);

			statement = general_expr[_val = Call(Lines::Make, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(Lines::Append, _val, _1)])
				| (lit('\n') >> general_expr[Call(Lines::Append, _val, _1)])
				);

			general_expr =
				if_expr[_val = _1]
				| return_expr[_val = _1]
				| logic_expr[_val = _1];

			if_expr = lit("if") >> s >> general_expr[_val = Call(If::Make, _1)]
				>> s >> lit("then") >> s >> general_expr[Call(If::SetThen, _val, _1)]
				>> -(s >> lit("else") >> s >> general_expr[Call(If::SetElse, _val, _1)])
				;

			for_expr = lit("for") >> s >> id[_val = Call(For::Make, _1)] >> s >> lit("in")
				>> s >> general_expr[Call(For::SetRangeStart, _val, _1)] >> s >> lit(":")
				>> s >> general_expr[Call(For::SetRangeEnd, _val, _1)] >> s >> lit("do")
				>> s >> general_expr[Call(For::SetDo, _val, _1)];

			return_expr = lit("return") >> s >> general_expr[_val = Call(Return::Make, _1)];

			def_func = arguments[_val = _1] >> lit("->") >> s >> statement[Call(applyFuncDef, _val, _1)];
			//def_func = arguments[_val = _1] >> lit("->") >> s >> general_expr[Call(applyFuncDef, _val, _1)];

			//constraint��DNF�̌`�ŗ^��������̂Ƃ���
			constraints = lit("sat") >> '(' >> s >> logic_expr[_val = Call(DeclSat::Make, _1)] >> s >> ')';

			//freeVals�����R�[�h�ւ̎Q�ƂƂ�������̂͏�����ς����A�P��̒l�ւ̎Q�ƂȂ����Ȃ��͂�
			/*freeVals = lit("free") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddIdentifier, _val, _1)]) >> *(
				s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddIdentifier, _val, _1)])
				) >> s >> ')';*/
			/*freeVals = lit("free") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, _1)]) >> *(
				s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, _1)])
				) >> s >> ')';*/

			freeVals = lit("free") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())]) >> *(
				s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())])
				) >> s >> ')';

			/*freeVals = lit("free") >> '(' >> s >> id[Call(DeclFree::AddIdentifier, _val, _1)] >> *(
				s >> "," >> s >> id[Call(DeclFree::AddIdentifier, _val, _1)]
				) >> s >> ')';*/

			arguments = -(id[_val = _1] >> *(s >> ',' >> s >> arguments[Call(concatArguments, _val, _1)]));

			logic_expr = logic_term[_val = _1] >> *(s >> '|' >> s >> logic_term[_val = MakeBinaryExpr(BinaryOp::Or)]);

			logic_term = logic_factor[_val = _1] >> *(s >> '&' >> s >> logic_factor[_val = MakeBinaryExpr(BinaryOp::And)]);

			logic_factor = ('!' >> s >> compare_expr[_val = MakeUnaryExpr(UnaryOp::Not)])
				| compare_expr[_val = _1]
				;

			compare_expr = arith_expr[_val = _1] >> *(
				(s >> lit("==") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Equal)])
				| (s >> lit("!=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::NotEqual)])
				| (s >> lit("<") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessThan)])
				| (s >> lit("<=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessEqual)])
				| (s >> lit(">") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterThan)])
				| (s >> lit(">=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterEqual)])
				)
				;

			//= ^ -> �͉E����

			arith_expr = (basic_arith_expr[_val = _1] >> -(s >> '=' >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Assign)]));

			basic_arith_expr = term[_val = _1] >>
				*((s >> '+' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Add)]) |
				(s >> '-' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Sub)]))
				;

			term = pow_term[_val = _1]
				| (factor[_val = _1] >>
					*((s >> '*' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Mul)]) |
					(s >> '/' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Div)]))
					)
				;
			
			//�Œ�ł�1�͎󂯎��悤�ɂ��Ȃ��ƁA�P���factor���󗝂ł��Ă��܂��̂�Mul,Div�̕��ɍs���Ă���Ȃ�
			pow_term = factor[_val = _1] >> s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)];
			pow_term1 = factor[_val = _1] >> -(s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)]);

			//record{} �̊Ԃɂ͉��s�͋��߂Ȃ��irecord,{}�Ƌ�ʂł��Ȃ��Ȃ�̂Łj
			record_inheritor = id[_val = Call(RecordInheritor::Make, _1)] >> record_maker[Call(RecordInheritor::AppendRecord, _val, _1)];

			record_maker = (
				char_('{') >> s >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]) >>
				*(
				(s >> ',' >> s >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]))
					| (+(char_('\n')) >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]))
					)
				>> s >> char_('}')
				)
				| (char_('{') >> s >> char_('}'));

			//���R�[�h�� name:val �� name �� : �̊Ԃɉ��s�������ׂ����H -> �����Ă���͏㋰�炭���͂Ȃ����A�Ӗ������܂�Ȃ�����
			record_keyexpr = id[_val = Call(KeyExpr::Make, _1)] >> char_(':') >> s >> general_expr[Call(KeyExpr::SetExpr, _val, _1)];

			/*list_maker = (char_('[') >> s >> general_expr[_val = Call(ListConstractor::Make, _1)] >>
				*(s >> char_(',') >> s >> general_expr[Call(ListConstractor::Append, _val, _1)]) >> s >> char_(']')
				)
				| (char_('[') >> s >> char_(']'));*/

			list_maker = (char_('[') >> s >> general_expr[_val = Call(ListConstractor::Make, _1)] >>
				*(
					(s >> char_(',') >> s >> general_expr[Call(ListConstractor::Append, _val, _1)])
					| (+(char_('\n')) >> general_expr[Call(ListConstractor::Append, _val, _1)])
					) >> s >> char_(']')
				)
				| (char_('[') >> s >> char_(']'));
			
			accessor = (id[_val = Call(Accessor::Make, _1)] >> +(access[Call(Accessor::Append, _val, _1)]))
				| (list_maker[_val = Call(Accessor::Make, _1)] >> listAccess[Call(Accessor::AppendList, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]))
				| (record_maker[_val = Call(Accessor::Make, _1)] >> recordAccess[Call(Accessor::AppendRecord, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]));
			
			access = functionAccess[_val = Cast<FunctionAccess, Access>()]
				| listAccess[_val = Cast<ListAccess, Access>()]
				| recordAccess[_val = Cast<RecordAccess, Access>()];

			recordAccess = char_('.') >> s >> id[_val = Call(RecordAccess::Make, _1)];

			listAccess = char_('[') >> s >> general_expr[Call(ListAccess::SetIndex, _val, _1)] >> s >> char_(']');

			functionAccess = char_('(')
				>> -(s >> general_expr[Call(FunctionAccess::Append, _val, _1)])
				>> *(s >> char_(',') >> s >> general_expr[Call(FunctionAccess::Append, _val, _1)]) >> s >> char_(')');

			//factor = /*double_[_val = _1]
			//	| */int_[_val = _1]
			//	| lit("true")[_val = true]
			//	| lit("false")[_val = false]
			//	| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
			//	| '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
			//	| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
			//	| constraints[_val = _1]
			//	| freeVals[_val = _1]
			//	| accessor[_val = _1]
			//	| def_func[_val = _1]
			//	| for_expr[_val = _1]
			//	| list_maker[_val = _1]
			//	| record_inheritor[_val = _1]
			//	//| (id >> record_maker)[_val = Call(RecordInheritor::MakeRecord, _1,_2)]
			//	| record_maker[_val = _1]
			//	| id[_val = _1];
			
			factor = /*double_[_val = _1]
					 | */int_[_val = Call(LRValue::Int, _1)]
				| lit("true")[_val = Call(LRValue::Bool, true)]
				| lit("false")[_val = Call(LRValue::Bool, false)]
				| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
				| '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
				| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
				//| constraints[_val = Call(LRValue::Sat, _1)]
				| constraints[_val = _1]
				| freeVals[_val = Call(LRValue::Free, _1)]
				| accessor[_val = _1]
				| def_func[_val = _1]
				| for_expr[_val = _1]
				| list_maker[_val = _1]
				| record_inheritor[_val = _1]
				//| (id >> record_maker)[_val = Call(RecordInheritor::MakeRecord, _1,_2)]
				| record_maker[_val = _1]
				| id[_val = _1];

			//id�̓r���ɂ͋󔒂��܂߂Ȃ�
			//id = lexeme[ascii::alpha[_val = _1] >> *(ascii::alnum[Call(addCharacter, _val, _1)])];
			//id = identifier_def[_val = _1];
			id = unchecked_identifier[_val = _1] - distinct_keyword;

			distinct_keyword = qi::lexeme[keywords >> !(qi::alnum | '_')];
			unchecked_identifier = qi::lexeme[(qi::alpha | qi::char_('_')) >> *(qi::alnum | qi::char_('_'))];

			/*auto const distinct_keyword = qi::lexeme[keywords >> !(qi::alnum | '_')];
			auto const unchecked_identifier = qi::lexeme[(qi::alpha | qi::char_('_')) >> *(qi::alnum | qi::char_('_'))];
			auto const identifier_def = unchecked_identifier - distinct_keyword;*/

			s = *(ascii::space);
			/*s = -(ascii::space) >> -(s1);
			s1 = ascii::space >> -(s1);*/

			//double_ ���� 1. �Ƃ����p�[�X�ł��Ă��܂������Ń����W�̃p�[�X�Ɏx�Ⴊ�o��̂ŕʂɒ�`����
			//double_value = lexeme[qi::char_('1', '9') >> *(ascii::digit) >> lit(".") >> +(ascii::digit)  [_val = Call(makeDouble, _1)]];
			/*double_value = lexeme[qi::char_('1', '9')[_val = Call(makeString, _1)] >> *(ascii::digit[Call(appendString, _val, _1)])
				>> lit(".")[Call(appendString, _val, _1)] >> +(ascii::digit[Call(appendString, _val, _1)])];*/
			/*double_value = lexeme[qi::char_('1', '9')[_val = _1] >> *(ascii::digit[Call(appendString, _val, _1)])
				>> lit(".")[Call(appendString, _val, _1)] >> +(ascii::digit[Call(appendString, _val, _1)])];*/
			
			/*double_value = lexeme[qi::char_('1', '9')[_val = _1] >> *(ascii::digit[Call(appendString, _val, _1)]) >> double_value2[Call(appendString2, _val, _1)]];

			double_value2 = lexeme[lit(".")[_val = '.'] >> +(ascii::digit[Call(appendString, _val, _1)])];*/

		}
	};
}

#define DO_TEST

#define DO_TEST2

namespace cgl
{
	bool ReadDouble(double& output, const std::string& name, const Record& record, std::shared_ptr<Environment> environment)
	{
		const auto& values = record.values;
		auto it = values.find(name);
		if (it == values.end())
		{
			//CGL_DebugLog(__FUNCTION__);
			return false;
		}
		auto opt = environment->expandOpt(it->second);
		if (!opt)
		{
			//CGL_DebugLog(__FUNCTION__);
			return false;
		}
		const Evaluated& value = opt.value();
		if (!IsType<int>(value) && !IsType<double>(value))
		{
			//CGL_DebugLog(__FUNCTION__);
			return false;
		}
		output = IsType<int>(value) ? static_cast<double>(As<int>(value)) : As<double>(value);
		return true;
	}

	struct Transform
	{
		Transform()
		{
			init();
		}

		Transform(const Eigen::Matrix3d& mat) :mat(mat) {}

		Transform(const Record& record, std::shared_ptr<Environment> pEnv)
		{
			double px = 0, py = 0;
			double sx = 1, sy = 1;
			double angle = 0;

			for (const auto& member : record.values)
			{
				auto valOpt = AsOpt<Record>(pEnv->expand(member.second));

				if (member.first == "pos" && valOpt)
				{
					ReadDouble(px, "x", valOpt.value(), pEnv);
					ReadDouble(py, "y", valOpt.value(), pEnv);
				}
				else if (member.first == "scale" && valOpt)
				{
					ReadDouble(sx, "x", valOpt.value(), pEnv);
					ReadDouble(sy, "y", valOpt.value(), pEnv);
				}
				else if (member.first == "angle")
				{
					ReadDouble(angle, "angle", record, pEnv);
				}
			}

			init(px, py, sx, sy, angle);
		}

		void init(double px = 0, double py = 0, double sx = 1, double sy = 1, double angle = 0)
		{
			const double pi = 3.1415926535;
			const double cosTheta = std::cos(pi*angle/180.0);
			const double sinTheta = std::sin(pi*angle/180.0);

			mat <<
				sx*cosTheta, -sy*sinTheta, px,
				sx*sinTheta, sy*cosTheta, py,
				0, 0, 1;
		}

		Transform operator*(const Transform& other)const
		{
			return mat * other.mat;
		}

		Eigen::Vector2d product(const Eigen::Vector2d& v)const
		{
			Eigen::Vector3d xs;
			xs << v.x(), v.y(), 1;
			Eigen::Vector3d result = mat * xs;
			Eigen::Vector2d result2d;
			result2d << result.x(), result.y();
			return result2d;
		}

		void printMat()const
		{
			std::cout << "Matrix(\n";
			for (int y = 0; y < 3; ++y)
			{
				std::cout << "    ";
				for (int x = 0; x < 3; ++x)
				{
					std::cout << mat(y, x) << " ";
				}
				std::cout << "\n";
			}
			std::cout << ")\n";
		}

	private:
		Eigen::Matrix3d mat;
	};

	template<class T>
	using Vector = std::vector<T, Eigen::aligned_allocator<T>>;

	inline bool ReadPolygon(Vector<Eigen::Vector2d>& output, const List& vertices, std::shared_ptr<Environment> pEnv, const Transform& transform)
	{
		output.clear();

		for (const Address vertex : vertices.data)
		{
			//CGL_DebugLog(__FUNCTION__);
			const Evaluated value = pEnv->expand(vertex);

			//CGL_DebugLog(__FUNCTION__);
			if (IsType<Record>(value))
			{
				double x = 0, y = 0;
				const Record& pos = As<Record>(value);
				//CGL_DebugLog(__FUNCTION__);
				if (!ReadDouble(x, "x", pos, pEnv) || !ReadDouble(y, "y", pos, pEnv))
				{
					//CGL_DebugLog(__FUNCTION__);
					return false;
				}
				//CGL_DebugLog(__FUNCTION__);
				Eigen::Vector2d v;
				v << x, y;
				//CGL_DebugLog(ToS(v.x()) + ", " + ToS(v.y()));
				output.push_back(transform.product(v));
				//CGL_DebugLog(__FUNCTION__);
			}
			else
			{
				//CGL_DebugLog(__FUNCTION__);
				return false;
			}
		}

		//CGL_DebugLog(__FUNCTION__);
		return true;
	}

	inline void OutputShapeList(std::ostream& os, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform);

	inline void OutputSVGImpl(std::ostream& os, const Record& record, std::shared_ptr<Environment> pEnv, const Transform& parent = Transform())
	{
		const Transform current(record, pEnv);

		const Transform transform = parent * current;

		for (const auto& member : record.values)
		{
			const Evaluated value = pEnv->expand(member.second);

			if (member.first == "vertex" && IsType<List>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygon(polygon, As<List>(value), pEnv, transform) && !polygon.empty())
				{
					//CGL_DebugLog(__FUNCTION__);

					os << "<polygon points=\"";
					for (const auto& vertex : polygon)
					{
						os << vertex.x() << "," << vertex.y() << " ";
					}
					os << "\"/>\n";
				}
				else
				{
					//CGL_DebugLog(__FUNCTION__);
				}
					
			}
			else if (IsType<Record>(value))
			{
				OutputSVGImpl(os, As<Record>(value), pEnv, transform);
			}
			else if (IsType<List>(value))
			{
				OutputShapeList(os, As<List>(value), pEnv, transform);
			}
		}
	}

	inline void OutputShapeList(std::ostream& os, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform)
	{
		for (const Address member : list.data)
		{
			const Evaluated value = pEnv->expand(member);

			if (IsType<Record>(value))
			{
				OutputSVGImpl(os, As<Record>(value), pEnv, transform);
			}
			else if (IsType<List>(value))
			{
				OutputShapeList(os, As<List>(value), pEnv, transform);
			}
		}
	}

	inline bool OutputSVG(std::ostream& os, const Evaluated& value, std::shared_ptr<Environment> pEnv)
	{
		if (IsType<Record>(value))
		{
			os << R"(<svg xmlns="http://www.w3.org/2000/svg" width="512" height="512" viewBox="0 0 512 512">)" << "\n";

			const Record& record = As<Record>(value);
			OutputSVGImpl(os, record, pEnv);

			os << "</svg>" << "\n";

			return true;
		}

		return false;
	}

	class Program
	{
	public:

		Program() :
			pEnv(Environment::Make()),
			evaluator(pEnv)
		{}

		boost::optional<Expr> parse(const std::string& program)
		{
			using namespace cgl;

			Lines lines;

			SpaceSkipper<IteratorT> skipper;
			Parser<IteratorT, SpaceSkipperT> grammer;

			std::string::const_iterator it = program.begin();
			if (!boost::spirit::qi::phrase_parse(it, program.end(), grammer, skipper, lines))
			{
				std::cerr << "Syntax Error: parse failed\n";
				return boost::none;
			}

			if (it != program.end())
			{
				std::cerr << "Syntax Error: ramains input\n" << std::string(it, program.end());
				return boost::none;
			}

			return lines;
		}

		boost::optional<Evaluated> execute(const std::string& program)
		{
			if (auto exprOpt = parse(program))
			{
				return pEnv->expand(boost::apply_visitor(evaluator, exprOpt.value()));
			}

			return boost::none;
		}

		bool draw(const std::string& program)
		{
			if (auto exprOpt = parse(program))
			{
				const Evaluated result = pEnv->expand(boost::apply_visitor(evaluator, exprOpt.value()));
				OutputSVG(std::cout, result, pEnv);
			}

			return false;
		}

		void clear()
		{
			pEnv = Environment::Make();
			evaluator = Eval(pEnv);
		}

		bool test(const std::string& program, const Expr& expr)
		{
			clear();

			if (auto result = execute(program))
			{
				std::shared_ptr<Environment> pEnv2 = Environment::Make();
				Eval evaluator2(pEnv2);

				const Evaluated answer = pEnv->expand(boost::apply_visitor(evaluator2, expr));

				return IsEqualEvaluated(result.value(), answer);
			}

			return false;
		}

	private:

		std::shared_ptr<Environment> pEnv;
		Eval evaluator;
	};
}

int main()
{
	using namespace cgl;

	const auto parse = [](const std::string& str, Lines& lines)->bool
	{
		using namespace cgl;

		SpaceSkipper<IteratorT> skipper;
		Parser<IteratorT, SpaceSkipperT> grammer;

		std::string::const_iterator it = str.begin();
		if (!boost::spirit::qi::phrase_parse(it, str.end(), grammer, skipper, lines))
		{
			std::cerr << "error: parse failed\n";
			return false;
		}

		if (it != str.end())
		{
			std::cerr << "error: ramains input\n" << std::string(it, str.end());
			return false;
		}

		return true;
	};

#ifdef DO_TEST

	std::vector<std::string> test_ok({
		"(1*2 + -3*-(4 + 5/6))",
		"1/(3+4)^6^7",
		"1 + 2, 3 + 4",
		"\n 4*5",
		"1 + 1 \n 2 + 3",
		"1 + 2 \n 3 + 4 \n",
		"1 + 1, \n 2 + 3",
		"1 + 1 \n \n \n 2 + 3",
		"1 + \n \n \n 2",
		"1 + 2 \n 3 + 4 \n\n\n\n\n",
		"1 + 2 \n , 4*5",
		"1 + 3 * \n 4 + 5",
		"(-> 1 + 2)",
		"(-> 1 + 2 \n 3)",
		"x, y -> x + y",
		"fun = (a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i)))",
		"fun = a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i))",
		"gcd1 = (m, n ->\n if m == 0\n then return m\n else self(mod(m, n), m)\n)",
		"gcd2 = (m, n ->\r\n if m == 0\r\n then return m\r\n else self(mod(m, n), m)\r\n)",
		"gcd3 = (m, n ->\n\tif m == 0\n\tthen return m\n\telse self(mod(m, n), m)\n)",
		"gcd4 = (m, n ->\r\n\tif m == 0\r\n\tthen return m\r\n\telse self(mod(m, n), m)\r\n)",
		R"(
gcd5 = (m, n ->
	if m == 0
	then return m
	else self(mod(m, n), m)
)
)",
R"(
func = x ->  
    x + 1
    return x

func2 = x, y -> x + y)",
"a = {a: 1, b: [1, {a: 2, b: 4}, 3]}, a.b[1].a = {x: 3, y: 5}, a",
"x = 10, f = ->x + 1, f()",
"x = 10, g = (f = ->x + 1, f), g()"
	});
	
	std::vector<std::string> test_ng({
		", 3*4",
		"1 + 1 , , 2 + 3",
		"1 + 2, 3 + 4,",
		"1 + 2, \n , 3 + 4",
		"1 + 3 * , 4 + 5",
		"(->)"
	});

	int ok_wrongs = 0;
	int ng_wrongs = 0;

	std::cout << "==================== Test Case OK ====================" << std::endl;
	for (size_t i = 0; i < test_ok.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ok[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool succeed = parse(test_ok[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (succeed)
		{
			/*
			std::cout << "eval:\n";
			printEvaluated(evalExpr(expr));
			std::cout << "\n";
			*/
		}
		else
		{
			std::cout << "[Wrong]\n";
			++ok_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "==================== Test Case NG ====================" << std::endl;
	for (size_t i = 0; i < test_ng.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ng[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool failed = !parse(test_ng[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (failed)
		{
			std::cout << "no result\n";
		}
		else
		{
			//std::cout << "eval:\n";
			//printEvaluated(evalExpr(expr));
			//std::cout << "\n";
			std::cout << "[Wrong]\n";
			++ng_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "Result:\n";
	std::cout << "Correct programs: (Wrong / All) = (" << ok_wrongs << " / " << test_ok.size() << ")\n";
	std::cout << "Wrong   programs: (Wrong / All) = (" << ng_wrongs << " / " << test_ng.size() << ")\n";

#endif

#ifdef DO_TEST2

	int eval_wrongs = 0;

	const auto testEval1 = [&](const std::string& source, std::function<bool(const Evaluated&)> pred)
	{
		std::cout << "----------------------------------------------------------\n";
		std::cout << "input:\n";
		std::cout << source << "\n\n";

		std::cout << "parse:\n";

		Lines lines;
		const bool succeed = parse(source, lines);

		if (succeed)
		{
			printLines(lines);

			std::cout << "eval:\n";
			Evaluated result = evalExpr(lines);

			std::cout << "result:\n";
			printEvaluated(result, nullptr);

			const bool isCorrect = pred(result);

			std::cout << "test: ";

			if (isCorrect)
			{
				std::cout << "Correct\n";
			}
			else
			{
				std::cout << "Wrong\n";
				++eval_wrongs;
			}
		}
		else
		{
			std::cerr << "Parse error!!\n";
			++eval_wrongs;
		}
	};

	const auto testEval = [&](const std::string& source, const Evaluated& answer)
	{
		testEval1(source, [&](const Evaluated& result) {return IsEqualEvaluated(result, answer); });
	};

testEval(R"(

{a: 3}.a

)", 3);

testEval(R"(

f = (x -> {a: x+10})
f(3).a

)", 13);

/*
testEval(R"(

vec3 = (v -> {
	x:v, y : v, z : v
})
vec3(3)

)", Record("x", 3).append("y", 3).append("z", 3));

using Li = cgl::ListConstractor;
Program program;

program.test(R"(

vec2 = (v -> [
	v, v
])
a = vec2(3)
vec2(a)

)", Li(Li({ 3, 3 }))(Li({ 3, 3 })));


program.test(R"(

vec2 = (v -> [
	v, v
])
vec2(vec2(3))

)", Li(Li({ 3, 3 }))(Li({ 3, 3 })));
*/


/*
testEval(R"(

vec2 = (v -> [
	v, v
])
a = vec2(3)
vec2(a)

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));
*/
/*
testEval(R"(

vec2 = (v -> [
	v, v
])
vec2(vec2(3))

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));
*/

/*
testEval(R"(

vec2 = (v -> {
	x:v, y : v
})
a = vec2(3)
vec2(a)

)", Record("x", Record("x", 3).append("y", 3)).append("y", Record("x", 3).append("y", 3)));

testEval(R"(

vec2 = (v -> {
	x:v, y : v
})
vec2(vec2(3))

)", Record("x", Record("x", 3).append("y", 3)).append("y", Record("x", 3).append("y", 3)));


//���̃v���O�����ɂ��āALINES_A���o�͂����LINES_B���o�͂����O�ɗ�����o�O�L��
testEval(R"(

vec3 = (v -> {
	x:v, y : v, z : v
})
mul = (v1, v2 -> {
	x:v1.x*v2.x, y : v1.y*v2.y, z : v1.z*v2.z
})
mul(vec3(3), vec3(2))

)", Record("x", 6).append("y", 6).append("z", 6));

testEval(R"(

r = {x: 0, y:10, sat(x == y), free(x)}
r.x

)", 10.0);

testEval(R"(

a = [1, 2]
b = [a, 3]

)", List().append(List().append(1).append(2)).append(3));

testEval(R"(

a = {a:1, b:2}
b = {a:a, b:3}

)", Record("a", Record("a", 1).append("b", 2)).append("b", 3));

testEval1(R"(

shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
}

line = shape{
	vertex: [
		{x:0, y:0}
		{x:1, y:0}
	]
}

main = {
	l1: line{
		vertex[1].y = 10
		color: {r:255, g:0, b:0}
	}
	l2: line{
		vertex[0] = {x: 2, y:3}
		color: {r:0, g:255, b:0}
	}

	sat(l1.vertex[1].x == l2.vertex[0].x & l1.vertex[1].y == l2.vertex[0].y)
	free(l1.vertex[1])
}

)", [](const Evaluated& result) {
	const Evaluated l1vertex1 = As<List>(As<Record>(As<Record>(result).values.at("l1")).values.at("vertex"));
	const Evaluated answer = List().append(Record("x", 0).append("y", 0)).append(Record("x", 2).append("y", 3));
	printEvaluated(answer);
	return IsEqual(l1vertex1, answer);
});

testEval1(R"(

main = {
    x: 1
    y: 2
    r: 1
    theta: 0
    sat(r*cos(theta) == x & r*sin(theta) == y)
    free(r, theta)
}

)", [](const Evaluated& result) {
	return IsEqual(As<Record>(result).values.at("theta"),1.1071487177940905030170654601785);
});
*/


/*

rod = {
    r: 10
    verts: [
        {x:0, y:0}
        {x:r, y:0}
    ]
}

newRod = (x, y -> rod{verts:[{x:x, y:y}, {x:x+r, y:y}]})

rod2 = {
    rods: [newRod(0,0),newRod(10,10),newRod(20,20),newRod(30,30)]

    sat(rods[0].verts[1].x == rods[0+1].verts[0].x & rods[0].verts[1].y == rods[0+1].verts[0].y)
    sat(rods[1].verts[1].x == rods[1+1].verts[0].x & rods[1].verts[1].y == rods[1+1].verts[0].y)
    sat(rods[2].verts[1].x == rods[2+1].verts[0].x & rods[2].verts[1].y == rods[2+1].verts[0].y)

    sat((rods[0].verts[0].x - rods[0].verts[1].x)^2 + (rods[0].verts[0].y - rods[0].verts[1].y)^2 == rods[0].r^2)
    sat((rods[1].verts[0].x - rods[1].verts[1].x)^2 + (rods[1].verts[0].y - rods[1].verts[1].y)^2 == rods[1].r^2)
    sat((rods[2].verts[0].x - rods[2].verts[1].x)^2 + (rods[2].verts[0].y - rods[2].verts[1].y)^2 == rods[2].r^2)
    sat((rods[3].verts[0].x - rods[3].verts[1].x)^2 + (rods[3].verts[0].y - rods[3].verts[1].y)^2 == rods[3].r^2)

    sat(rods[0].verts[0].x == 0 & rods[0].verts[0].y == 0)
    sat(rods[3].verts[1].x == 30 & rods[3].verts[1].y == 0)

    free(rods[0].verts, rods[1].verts, rods[2].verts, rods[3].verts)
}

*/

/*
rod = {
    r: 10
    verts: [
        {x:0, y:0}
        {x:r, y:0}
    ]
}

newRod = (x, y -> rod{verts:[{x:x, y:y}, {x:x+r, y:y}]})

rod2 = {
    rods: [newRod(0,0),newRod(10,10),newRod(20,20),newRod(30,30)]

    for i in 0:2 do(
        sat(rods[i].verts[1].x == rods[i+1].verts[0].x & rods[i].verts[1].y == rods[i+1].verts[0].y)
    )

    for i in 0:3 do(
        sat((rods[i].verts[0].x - rods[i].verts[1].x)^2 + (rods[i].verts[0].y - rods[i].verts[1].y)^2 == rods[i].r^2)
    )

    sat(rods[0].verts[0].x == 0 & rods[0].verts[0].y == 0)
    sat(rods[3].verts[1].x == 30 & rods[3].verts[1].y == 0)

    free(rods[0].verts, rods[1].verts, rods[2].verts, rods[3].verts)
}

EOF
*/
	std::cerr<<"Test Wrong Count: " << eval_wrongs<<std::endl;
	
#endif
	
	
	/*
	��邱��

	shape = {
		pos: {x:0, y:0}
		scale: {x:1, y:1}
		angle: 0
	}

	square = shape{
		vertex: [
			{x: -1, y: -1}, {x: +1, y: -1}
			{x: +1, y: +1}, {x: -1, y: +1}
		]
	}

	twosquare = shape{
		a: square{pos.x = 2, angle = 45}
		b: square{pos.y = 5}
	}
	*/

/*
shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

stick = shape{
	scale = {x:1, y:3}
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

plus = shape{
	scale = {x:50, y:50}
	a: stick{}
	b: stick{angle = 90}
}

cross = shape{
	pos = {x:256, y:256}
	a: plus{angle = 45}
}

EOF
	*/

	{
		cgl::Program program;
		program.draw(R"(
shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

shape{
	vertex: [
		{x: 100, y: 100}, {x: 300, y: 100}
		{x: 300, y: 200}
	]
}
)");
	}

	while (true)
	{
		std::string source;

		std::string buffer;
		while (std::cout << ">> ", std::getline(std::cin, buffer))
		{
			if (buffer == "quit()")
			{
				return 0;
			}
			else if (buffer == "EOF")
			{
				break;
			}

			source.append(buffer + '\n');
		}

		Program program;
		try
		{
			program.draw(source);
		}
		catch (const cgl::Exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}
	}

	return 0;
}