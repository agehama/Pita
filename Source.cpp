#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <Eigen/Core>
#include "Node.hpp"
#include "Evaluator.hpp"
#include "Printer.hpp"
#include "Environment.hpp"

std::ofstream ofs;

namespace cgl
{
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
				CGL_Error("識別子が定義されていません");
			}

			//free変数にあった場合は制約用の参照値を返す
			if (auto satRefOpt = addSatRef(address))
			{
				return satRefOpt.value();
			}
			//free変数になかった場合は評価した結果を返す
			else
			{
				const Evaluated evaluated = pEnv->expand(address);
				return LRValue(evaluated);
			}
		}

		CGL_Error("ここは通らないはず");
		return 0;
	}

	//ここにIdentifierが残っている時点でClosureMakerにローカル変数だと判定された変数のはず
	Expr Expr2SatExpr::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		if (!address.isValid())
		{
			CGL_Error("識別子が定義されていません");
		}

		//free変数にあった場合は制約用の参照値を返す
		if (auto satRefOpt = addSatRef(address))
		{
			return satRefOpt.value();
		}
		//free変数になかった場合は評価した結果を返す
		else
		{
			const Evaluated evaluated = pEnv->expand(address);
			return LRValue(evaluated);
		}

		CGL_Error("ここは通らないはず");
		return LRValue(0);
	}

	bool SatVariableBinder::operator()(const LRValue& node)
	{
		if (node.isLValue())
		{
			const Address address = node.address();

			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//free変数にあった場合は制約用の参照値を追加する
			return static_cast<bool>(addSatRef(address));
		}

		return false;
	}

	bool SatVariableBinder::operator()(const Identifier& node)
	{
		Address address = pEnv->findAddress(node);
		if (!address.isValid())
		{
			//CGL_Error("識別子が定義されていません");
			//この中で作られた変数だった場合、定義されていない可能性がある
			return false;
		}

		//free変数にあった場合は制約用の参照値を追加する
		return static_cast<bool>(addSatRef(address));
	}

	inline bool HasFreeVariables::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			const Evaluated& val = node.evaluated();
			if (IsType<double>(val) || IsType<int>(val) || IsType<bool>(val))
			{
				return false;
			}

			CGL_Error("不正な値");
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

	inline bool HasFreeVariables::operator()(const Accessor& node)
	{
		Expr expr = node;
		Eval evaluator(pEnv);
		const LRValue value = boost::apply_visitor(evaluator, expr);

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

		CGL_Error("invalid expression");
		return false;
	}

	Expr Expr2SatExpr::operator()(const Accessor& node)
	{
		Address headAddress;
		const Expr& head = node.head;

		//headがsat式中のローカル変数
		if (IsType<Identifier>(head))
		{
			Address address = pEnv->findAddress(As<Identifier>(head));
			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//headは必ず Record/List/FuncVal のどれかであり、double型であることはあり得ない。
			//したがって、free変数にあるかどうかは考慮せず（free変数は冗長に指定できるのであったとしても別にエラーではない）、
			//直接Evaluatedとして展開する
			//result.head = LRValue(address);
			headAddress = address;
		}
		//headがアドレス値
		else if (IsType<LRValue>(head))
		{
			const LRValue& headAddressValue = As<LRValue>(head);
			if (!headAddressValue.isLValue())
			{
				CGL_Error("sat式中のアクセッサの先頭部が不正な値です");
			}

			const Address address = headAddressValue.address();

			//↑のIdentifierと同様に直接展開する
			//result.head = LRValue(address);
			headAddress = address;
		}
		else
		{
			CGL_Error("sat中のアクセッサの先頭部に単一の識別子以外の式を用いることはできません");
		}

		Eval evaluator(pEnv);

		Accessor result;

		//TODO: アクセッサはfree変数を持たない間、それ自身がfree変数指定されるまでのアドレスを畳み込む
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
				CGL_Error("参照エラー");
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
						CGL_Error("オブジェクトがリストでない");
					}

					const List& list = As<const List&>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						headAddress = list.get(indexOpt.value());
					}
					else
					{
						CGL_Error("list[index] の index が int 型でない");
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
						CGL_Error("オブジェクトがレコードでない");
					}

					const Record& record = As<const Record&>(objRef);
					auto it = record.values.find(recordAccess.name);
					if (it == record.values.end())
					{
						CGL_Error("指定された識別子がレコード中に存在しない");
					}

					headAddress = it->second;
				}
			}
			else
			{
				const FunctionAccess& funcAccess = As<FunctionAccess>(access);

				{
					//TODO: HasFreeVariablesの実装は不完全で、アクセッサが関数呼び出しでさらにその引数がfree変数の場合に対応していない
					//      一度式を評価して、その過程でfree変数で指定したアドレスへのアクセスが発生するかどうかで見るべき
					//      AddressAppearanceCheckerのようなものを作る(Evalの簡易版)
					//inline bool HasFreeVariables::operator()(const Accessor& node)

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
						CGL_Error("オブジェクトが関数でない");
					}

					const FuncVal& function = As<const FuncVal&>(objRef);

					/*
					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
					}

					Expr caller = FunctionCaller(function, args);
					const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
					headAddress = pEnv->makeTemporaryValue(returnedValue);
					*/
					std::vector<Address> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue lrvalue = boost::apply_visitor(evaluator, expr);
						if (lrvalue.isLValue())
						{
							args.push_back(lrvalue.address());
						}
						else
						{
							args.push_back(pEnv->makeTemporaryValue(lrvalue.evaluated()));
						}
					}

					const Evaluated returnedValue = pEnv->expand(evaluator.callFunction(function, args));
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
		selfDependsOnFreeVariablesとchildDependsOnFreeVariablesが両方true : エラー
		selfDependsOnFreeVariablesがtrue  : アクセッサ本体がfree（アクセッサを評価すると必ず単一のdouble型になる）
		childDependsOnFreeVariablesがtrue : アクセッサの引数がfree（リストインデックスか関数引数）
		*/
		if (selfDependsOnFreeVariables && childDependsOnFreeVariables)
		{
			CGL_Error("sat式中のアクセッサについて、本体と引数の両方をfree変数に指定することはできません");
		}
		else if (selfDependsOnFreeVariables)
		{
			if (!result.accesses.empty())
			{
				CGL_Error("ここは通らないはず");
			}
			
			if (auto satRefOpt = addSatRef(headAddress))
			{
				return satRefOpt.value();
			}
			else
			{
				CGL_Error("ここは通らないはず");
			}
		}
		/*else if (childDependsOnFreeVariables)
		{
			CGL_Error("TODO: アクセッサの引数のfree変数指定は未対応");
		}*/

		result.head = LRValue(headAddress);

		return result;
	}

	//bool SatVariableBinder::operator()(const FunctionCaller& node)
	bool SatVariableBinder::callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments)
	{
		/*FuncVal funcVal;

		if (auto opt = AsOpt<FuncVal>(node.funcRef))
		{
			funcVal = opt.value();
		}
		else
		{
			const Address funcAddress = pEnv->findAddress(As<Identifier>(node.funcRef));
			if (funcAddress.isValid())
			{
				if (auto funcOpt = pEnv->expandOpt(funcAddress))
				{
					if (IsType<FuncVal>(funcOpt.value()))
					{
						funcVal = As<FuncVal>(funcOpt.value());
					}
					else
					{
						CGL_Error("指定された変数名に紐つけられた値が関数でない");
					}
				}
				else
				{
					CGL_Error("ここは通らないはず");
				}
			}
			else
			{
				CGL_Error("指定された変数名に値が紐つけられていない");
			}
		}*/

		if (funcVal.builtinFuncAddress)
		{
			return false;
		}

		/*std::vector<Address> expandedArguments(node.actualArguments.size());
		for (size_t i = 0; i < expandedArguments.size(); ++i)
		{
			expandedArguments[i] = pEnv->makeTemporaryValue(node.actualArguments[i]);
		}*/

		if (funcVal.arguments.size() != expandedArguments.size())
		{
			CGL_Error("仮引数の数と実引数の数が合っていない");
		}

		pEnv->switchFrontScope();
		pEnv->enterScope();

		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
		}
		const bool result = boost::apply_visitor(*this, funcVal.expr);

		pEnv->exitScope();
		pEnv->switchBackScope();

		return result;
	}

	bool SatVariableBinder::operator()(const Accessor& node)
	{
		CGL_DebugLog("SatVariableBinder::operator()(const Accessor& node)");
		{
			Expr expr = node;
			printExpr(expr);
		}
		
		Address headAddress;
		const Expr& head = node.head;

		//headがsat式中のローカル変数
		if (IsType<Identifier>(head))
		{
			Address address = pEnv->findAddress(As<Identifier>(head));
			if (!address.isValid())
			{
				CGL_Error("識別子が定義されていません");
			}

			//headは必ず Record/List/FuncVal のどれかであり、double型であることはあり得ない。
			//したがって、free変数にあるかどうかは考慮せず（free変数は冗長に指定できるのであったとしても別にエラーにはしない）、
			//直接Addressとして展開する
			headAddress = address;
		}
		//headがアドレス値
		else if (IsType<LRValue>(head))
		{
			const LRValue& headAddressValue = As<LRValue>(head);
			if (!headAddressValue.isLValue())
			{
				CGL_Error("sat式中のアクセッサの先頭部が不正な値です");
			}

			headAddress = headAddressValue.address();
		}
		else
		{
			CGL_Error("sat中のアクセッサの先頭部に単一の識別子以外の式を用いることはできません");
		}

		Eval evaluator(pEnv);

		Accessor result;

		//isDeterministicがtrueであれば、Evalしないとわからないところまで見に行く
		//isDeterministicがfalseの時は、評価するまでアドレスが不定なので関数の中身までは見に行かない
		bool isDeterministic = true;

		for (const auto& access : node.accesses)
		{
			boost::optional<const Evaluated&> objOpt;
			if (isDeterministic)
			{
				objOpt = pEnv->expandOpt(headAddress);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}
			}
			
			if (IsType<ListAccess>(access))
			{
				const ListAccess& listAccess = As<ListAccess>(access);

				isDeterministic = !boost::apply_visitor(*this, listAccess.index) && isDeterministic;

				if (isDeterministic)
				{
					const Evaluated& objRef = objOpt.value();
					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}
					const List& list = As<const List&>(objRef);

					Evaluated indexValue = pEnv->expand(boost::apply_visitor(evaluator, listAccess.index));
					if (auto indexOpt = AsOpt<int>(indexValue))
					{
						headAddress = list.get(indexOpt.value());
					}
					else
					{
						CGL_Error("list[index] の index が int 型でない");
					}
				}
			}
			else if (IsType<RecordAccess>(access))
			{
				const RecordAccess& recordAccess = As<RecordAccess>(access);

				if (isDeterministic)
				{
					const Evaluated& objRef = objOpt.value();
					if (!IsType<Record>(objRef))
					{
						CGL_Error("オブジェクトがレコードでない");
					}
					const Record& record = As<const Record&>(objRef);

					auto it = record.values.find(recordAccess.name);
					if (it == record.values.end())
					{
						CGL_Error("指定された識別子がレコード中に存在しない");
					}

					headAddress = it->second;
				}
			}
			else
			{
				const FunctionAccess& funcAccess = As<FunctionAccess>(access);
				
				if (isDeterministic)
				{
					//Case2(関数引数がfree)への対応
					for (const auto& argument : funcAccess.actualArguments)
					{
						isDeterministic = !boost::apply_visitor(*this, argument) && isDeterministic;
					}
					
					//呼ばれる関数の実体はその引数には依存しないため、ここでisDeterministicがfalseになっても問題ない
					
					//Case4以降への対応は関数の中身を見に行く必要がある
					const Evaluated& objRef = objOpt.value();
					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("オブジェクトが関数でない");
					}
					const FuncVal& function = As<const FuncVal&>(objRef);

					//Case4,6への対応
					/*
					std::vector<Evaluated> arguments;
					for (const auto& expr : funcAccess.actualArguments)
					{
						//ここでexpandして大丈夫なのか？
						arguments.push_back(pEnv->expand(boost::apply_visitor(evaluator, expr)));
					}
					Expr caller = FunctionCaller(function, arguments);
					isDeterministic = !boost::apply_visitor(*this, caller) && isDeterministic;
					*/
					std::vector<Address> arguments;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue lrvalue = boost::apply_visitor(evaluator, expr);
						if (lrvalue.isLValue())
						{
							arguments.push_back(lrvalue.address());
						}
						else
						{
							arguments.push_back(pEnv->makeTemporaryValue(lrvalue.evaluated()));
						}
					}
					isDeterministic = !callFunction(function, arguments) && isDeterministic;

					//ここまでで一つもfree変数が出てこなければこの先の中身も見に行く
					if (isDeterministic)
					{
						//const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(evaluator, caller));
						const Evaluated returnedValue = pEnv->expand(evaluator.callFunction(function, arguments));
						headAddress = pEnv->makeTemporaryValue(returnedValue);
					}
				}
			}
		}

		if (isDeterministic)
		{
			return static_cast<bool>(addSatRef(headAddress));
		}

		return true;
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
		//	//エラー：識別子かリテラル以外（評価結果としてオブジェクトを返すような式）へのアクセスには未対応
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
		//			//エラー：list[index] の index が int 型でない
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

			//printEvaluated(val, nullptr, os);
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

	void Environment::bindNewValue(const std::string& name, const Evaluated& value)
	{
		CGL_DebugLog("");
		const Address newAddress = m_values.add(value);
		//Clone(m_weakThis.lock(), value);
		bindValueID(name, newAddress);
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

		//その関数が参照している変数名のリストを作る
		{
			//referenceableVariables は現在のスコープで参照可能な変数全てを表している
			//実際に関数の中で参照される変数はこの中のごく一部なので、事前にフィルタを掛けた方がいい

			//スコープ間で同名の変数がある場合は内側の変数で遮蔽されて、外側の変数は参照されないので削除してよい
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
			//このスコープを抜けると削除される変数リスト
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

			//今削除しようとしている変数で、なおかつあるスコープから参照可能な変数のリストを返す
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
						//関数が内部で外の変数を参照している時、その変数が関数より先に解放されてしまうと困るので、このタイミングでpopされる変数について関数の内部に存在する変数をその値で置き換えるという処理を行う。

						//その後、currentScopeDepthを1つデクリメントする。
						
						std::map<std::string, Evaluated> variableNames;

						const auto names = intersectNames(funcValOpt.value().referenceableVariables);
						for (const auto& name : names)
						{
							const auto valueOpt = find(name);
							if (!valueOpt)
							{
								//これから削除される変数を保存しようとしたが既に消されていた
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
						
						//削除される変数を定数に置き換える
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
						//リストとしてアクセスするのに失敗
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
						//レコードとしてアクセスするのに失敗
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
							//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
							std::cerr << "Error(" << __LINE__ << ")\n";
							return reference;
						}
					}
					else
					{
						//関数としてアクセスするのに失敗
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
					//		//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
					//		std::cerr << "Error(" << __LINE__ << ")\n";
					//		return reference;
					//	}
					//}
					//else
					//{
					//	//関数としてアクセスするのに失敗
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

			CGL_Error("アクセッサの評価結果がアドレス値でない");
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
				//TODO:最終的にintやdouble 以外のデータへの参照は持つことにするか？
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

				if (IsType<int>(value) || IsType<double>(value) /*|| IsType<bool>(value)*/)//TODO:boolは将来的に対応
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
					std::cerr << "未対応";
					//TODO:最終的にintやdouble 以外のデータへの参照は持つことにするか？
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
				else//リストとしてアクセスするのに失敗
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
				else//レコードとしてアクセスするのに失敗
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
						//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
						std::cerr << "Error(" << __LINE__ << ")\n";
						return;
					}
				}
				else
				{
					//関数としてアクセスするのに失敗
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
					else//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
					{
						std::cerr << "Error(" << __LINE__ << ")\n";
						return;
					}
				}
				else//関数としてアクセスするのに失敗
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

		//satに出てこないfreeVariablesの削除
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

		/*Expr2SatExpr evaluator(0, pEnv, freeVariables);
		expr = boost::apply_visitor(evaluator, candidateExpr.value());
		refs.insert(refs.end(), evaluator.refs.begin(), evaluator.refs.end());

		{
			CGL_DebugLog("Print:");
			Printer printer;
			boost::apply_visitor(printer, expr.value());
			CGL_DebugLog("");
		}*/

		
		CGL_DebugLog("freeVariables:");
		for (const auto& val : freeVariables)
		{
			CGL_DebugLog(std::string("  Address(") + val.toString() + ")");
		}

		SatVariableBinder binder(pEnv, freeVariables);
		if (boost::apply_visitor(binder, candidateExpr.value()))
		{
			expr = candidateExpr.value();
			refs = binder.refs;
			invRefs = binder.invRefs;

			//satに出てこないfreeVariablesの削除
			for (int i = static_cast<int>(freeVariables.size()) - 1; 0 <= i; --i)
			{
				if (binder.usedInSat[i] == 0)
				{
					freeVariables.erase(freeVariables.begin() + i);
				}
			}
		}
		else
		{
			expr = boost::none;
			refs.clear();
			invRefs.clear();
			freeVariables.clear();
		}

		{
			CGL_DebugLog("env:");
			pEnv->printEnvironment(true);

			CGL_DebugLog("expr:");
			printExpr(candidateExpr.value());
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
				//存在しない参照をsatに指定した
				/*std::cerr << "Error(" << __LINE__ << "): reference does not exist.\n";
				return false;*/
				CGL_Error("存在しない参照をsatに指定した");
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
			CGL_WarnLog("free式に有効な変数が指定されていません。");
			return 0.0;
		}

		/*{
			CGL_DebugLog("data:");
			for(int i=0;i<data.size();++i)
			{
				CGL_DebugLog(std::string("  ID(") + ToS(i) + ") -> " + ToS(data[i]));
			}

			CGL_DebugLog("refs:");
			for (int i = 0; i<refs.size(); ++i)
			{
				CGL_DebugLog(std::string("  ID(") + ToS(i) + ") -> Address(" + refs[i].toString() + ")");
			}

			CGL_DebugLog("invRefs:");
			for(const auto& keyval : invRefs)
			{
				CGL_DebugLog(std::string("  Address(") + keyval.first.toString() + ") -> ID(" + ToS(keyval.second) + ")");
			}

			CGL_DebugLog("env:");
			pEnv->printEnvironment();

			CGL_DebugLog("expr:");
			printExpr(expr.value());
		}*/
		
		EvalSatExpr evaluator(pEnv, data, refs, invRefs);
		const Evaluated evaluated = boost::apply_visitor(evaluator, expr.value());
		
		if (IsType<double>(evaluated))
		{
			return As<double>(evaluated);
		}
		else if (IsType<int>(evaluated))
		{
			return As<int>(evaluated);
		}

		CGL_Error("sat式の評価結果が不正");
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

		printExpr(expr.value());
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

			//constraintはDNFの形で与えられるものとする
			constraints = lit("sat") >> '(' >> s >> logic_expr[_val = Call(DeclSat::Make, _1)] >> s >> ')';

			//freeValsがレコードへの参照とかを入れるのは少し大変だが、単一の値への参照なら難しくないはず
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

			//= ^ -> は右結合

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
			
			//最低でも1つは受け取るようにしないと、単一のfactorを受理できてしまうのでMul,Divの方に行ってくれない
			pow_term = factor[_val = _1] >> s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)];
			pow_term1 = factor[_val = _1] >> -(s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)]);

			//record{} の間には改行は挟めない（record,{}と区別できなくなるので）
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

			//レコードの name:val の name と : の間に改行を許すべきか？ -> 許しても解析上恐らく問題はないが、意味があまりなさそう
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

			//idの途中には空白を含めない
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

			//double_ だと 1. とかもパースできてしまうせいでレンジのパースに支障が出るので別に定義する
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
				try
				{
					return pEnv->expand(boost::apply_visitor(evaluator, exprOpt.value()));
				}
				catch (const cgl::Exception& e)
				{
					std::cerr << "Exception: " << e.what() << std::endl;
				}
			}

			return boost::none;
		}

		bool draw(const std::string& program)
		{
			if (auto exprOpt = parse(program))
			{
				try
				{
					printExpr(exprOpt.value());
					CGL_DebugLog("printExpr");
					const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
					CGL_DebugLog("evaluate");
					const Evaluated result = pEnv->expand(lrvalue);
					CGL_DebugLog("expand");
					OutputSVG(std::cout, result, pEnv);
					CGL_DebugLog("outputSVG");
				}
				catch (const cgl::Exception& e)
				{
					std::cerr << "Exception: " << e.what() << std::endl;
				}
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
	ofs = std::ofstream("log.txt");

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
			Expr expr = lines;
			printExpr(expr);

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


//このプログラムについて、LINES_Aが出力されてLINES_Bが出力される前に落ちるバグ有り
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

	/*
	{
		cgl::Program program;
		program.draw(R"(
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
)");
	}

	{
		cgl::Program program;
		program.draw(R"(









































shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

transform = (t, p -> {
	cosT:cos(t.angle)
	sinT:sin(t.angle)
	x:t.pos.x + cosT * t.scale.x * p.x - sinT * t.scale.y * p.y
	y:t.pos.y + sinT* t.scale.x* p.x + cosT* t.scale.y*p.y
})

contact = (p, q -> p.x == q.x & p.y == q.y)

square = shape{
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

main = shape{
	a: square{scale: {x:10,y:30}}
	b: square{scale: {x:20,y:10}}
	sat(contact(transform(a, a.vertex[0]), transform(b, b.vertex[2])))
	free(a.pos)
}

)");
	}
	*/

	{
		cgl::Program program;
		program.draw(R"(









































transform = (pos, scale, angle, vertex -> {
	cosT:cos(angle)
	sinT:sin(angle)
	x:pos.x + cosT * scale.x * vertex.x - sinT * scale.y * vertex.y
	y:pos.y + sinT* scale.x * vertex.x + cosT* scale.y * vertex.y
})

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
	topLeft:     (->transform(pos, scale, angle, vertex[0]))
	topRight:    (->transform(pos, scale, angle, vertex[1]))
	bottomLeft:  (->transform(pos, scale, angle, vertex[2]))
	bottomRight: (->transform(pos, scale, angle, vertex[3]))
}

contact = (p, q -> (p.x - q.x)*(p.x - q.x) < 1  & (p.y - q.y)*(p.y - q.y) < 1)

main = shape{

    a: square{scale: {x: 50, y: 50}}
    b: square{scale: {x: 100, y: 50}}
    c: b{}

    sat( contact(a.topRight(), b.bottomLeft()) & contact(a.bottomRight(), c.topLeft()) )
    sat( contact(b.bottomRight(), c.topRight()) )
    free(b.pos, b.angle, c.pos, c.angle)
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
		program.draw(source);
		/*Program program;
		try
		{
			program.draw(source);
		}
		catch (const cgl::Exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}*/
	}

	return 0;
}