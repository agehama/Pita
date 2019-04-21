#pragma warning(disable:4996)
#include <iostream>

#include <Pita/Evaluator.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/OptimizationEvaluator.hpp>
#include <Pita/Printer.hpp>
#include <Pita/IntrinsicGeometricFunctions.hpp>
#include <Pita/Program.hpp>
#include <Pita/Graph.hpp>
#include <Pita/ExprTransformer.hpp>
#include <Pita/ValueCloner.hpp>

extern double cloneTime;
extern unsigned cloneCount;
extern bool printAddressInsertion;
namespace cgl
{
	class AddressReplacer : public ExprTransformer
	{
	public:
		const std::unordered_map<Address, Address>& replaceMap;
		std::shared_ptr<Context> pEnv;

		AddressReplacer(const std::unordered_map<Address, Address>& replaceMap, std::shared_ptr<Context> pEnv) :
			replaceMap(replaceMap),
			pEnv(pEnv)
		{}

		boost::optional<Address> getOpt(Address address)const;

		Expr operator()(const LRValue& node)override;
		Expr operator()(const Identifier& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Import& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const UnaryExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const BinaryExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Range& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Lines& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DefFunc& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const If& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const For& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Return& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const ListConstractor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const KeyExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const RecordConstractor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Accessor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DeclSat& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DeclFree& node)override { return ExprTransformer::operator()(node); }
	};

	boost::optional<Address> AddressReplacer::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Expr AddressReplacer::operator()(const LRValue& node)
	{
		if (node.isRValue())
		{
			return node;
		}

		if (auto opt = getOpt(node.address(*pEnv)))
		{
			if (node.isEitherReference())
			{
				return LRValue(EitherReference(node.eitherReference().local, opt.get())).setLocation(node);
			}
			else if (node.isReference())
			{
				return LRValue(pEnv->cloneReference(node.reference(), replaceMap)).setLocation(node);
			}
			else
			{
				return LRValue(opt.get()).setLocation(node);
			}
		}

		return node;
	}

	//Valのアドレス値を再帰的に展開したクローンを作成する
	class ValueCloner : public boost::static_visitor<Val>
	{
	public:
		ValueCloner(std::shared_ptr<Context> pEnv, const LocationInfo& info) :
			pEnv(pEnv),
			info(info)
		{}

		const LocationInfo info;
		std::shared_ptr<Context> pEnv;
		std::unordered_map<Address, Address> replaceMap;

		double time1 = 0, time2 = 0, time3 = 0;

		boost::optional<Address> getOpt(Address address)const;

		Val operator()(bool node) { return node; }
		Val operator()(int node) { return node; }
		Val operator()(double node) { return node; }
		Val operator()(const CharString& node) { return node; }
		Val operator()(const List& node);
		Val operator()(const KeyValue& node) { return node; }
		Val operator()(const Record& node);
		Val operator()(const FuncVal& node) { return node; }
		Val operator()(const Jump& node) { return node; }
	};

	class ValueCloner2 : public boost::static_visitor<Val>
	{
	public:
		ValueCloner2(std::shared_ptr<Context> pEnv, const std::unordered_map<Address, Address>& replaceMap, const LocationInfo& info) :
			pEnv(pEnv),
			replaceMap(replaceMap),
			info(info)
		{}

		const LocationInfo info;
		std::shared_ptr<Context> pEnv;
		const std::unordered_map<Address, Address>& replaceMap;

		boost::optional<Address> getOpt(Address address)const;

		Val operator()(bool node) { return node; }
		Val operator()(int node) { return node; }
		Val operator()(double node) { return node; }
		Val operator()(const CharString& node) { return node; }
		Val operator()(const List& node);
		Val operator()(const KeyValue& node) { return node; }
		Val operator()(const Record& node);
		Val operator()(const FuncVal& node)const;
		Val operator()(const Jump& node) { return node; }
	};

	class ValueCloner3 : public boost::static_visitor<void>
	{
	public:
		ValueCloner3(std::shared_ptr<Context> pEnv, const std::unordered_map<Address, Address>& replaceMap) :
			pEnv(pEnv),
			replaceMap(replaceMap)
		{}

		std::shared_ptr<Context> pEnv;
		const std::unordered_map<Address, Address>& replaceMap;

		boost::optional<Address> getOpt(Address address)const;

		Address replaced(Address address)const;

		void operator()(bool& node) {}
		void operator()(int& node) {}
		void operator()(double& node) {}
		void operator()(CharString& node) {}
		void operator()(List& node) {}
		void operator()(KeyValue& node) {}
		void operator()(Record& node);
		void operator()(FuncVal& node) {}
		void operator()(Jump& node) {}
	};

	boost::optional<Address> ValueCloner::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Val ValueCloner::operator()(const List& node)
	{
		List result;
		const auto& data = node.data;
		
		/*if (printAddressInsertion)
		{
			std::cout << data.size() << ", ";
		}*/

		for (size_t i = 0; i < data.size(); ++i)
		{
			if (auto opt = getOpt(data[i]))
			{
				result.push_back(opt.get());
			}
			else
			{
				double begin1T = GetSec();
				const Val& substance = pEnv->expand(data[i], info);
				double end1T = GetSec();

				const Val clone = boost::apply_visitor(*this, substance);

				double begin2T = GetSec();
				const Address newAddress = pEnv->makeTemporaryValue(clone);
				double begin3T = GetSec();

				result.push_back(newAddress);
				replaceMap[data[i]] = newAddress;
				double begin4T = GetSec();

				time1 += end1T - begin1T;
				time2 += begin3T - begin2T;
				time3 += begin4T - begin3T;
			}
		}

		return result;
	}

	Val ValueCloner::operator()(const Record& node)
	{
		Record result;

		for (const auto& value : node.values)
		{
			if (auto opt = getOpt(value.second))
			{
				result.append(value.first, opt.get());
			}
			else
			{
				double begin1T = GetSec();
				const Val& substance = pEnv->expand(value.second, info);
				double end1T = GetSec();

				const Val clone = boost::apply_visitor(*this, substance);

				double begin2T = GetSec();
				const Address newAddress = pEnv->makeTemporaryValue(clone);
				double begin3T = GetSec();

				result.append(value.first, newAddress);
				replaceMap[value.second] = newAddress;
				double begin4T = GetSec();

				time1 += end1T - begin1T;
				time2 += begin3T - begin2T;
				time3 += begin4T - begin3T;
			}
		}

		/*if (printAddressInsertion)
		{
			std::cout << "(" << time1 << ", " << time2 << ", " << time3 << ")\n";
		}*/

		result.problems = node.problems;
		result.type = node.type;
		result.constraint = node.constraint;
		result.isSatisfied = node.isSatisfied;
		result.boundedFreeVariables = node.boundedFreeVariables;

		//result.unitConstraints = node.unitConstraints;
		//result.variableAppearances = node.variableAppearances;
		////result.constraintGroups = node.constraintGroups;
		//result.groupConstraints = node.groupConstraints;
		result.original = node.original;

		return result;
	}

	boost::optional<Address> ValueCloner2::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Val ValueCloner2::operator()(const List& node)
	{
		const auto& data = node.data;

		for (size_t i = 0; i < data.size(); ++i)
		{
			//ValueCloner1でクローンは既に作ったので、そのクローンを直接書き換える
			const Val& substance = pEnv->expand(data[i], info);
			pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(data[i], boost::apply_visitor(*this, substance));
		}

		return node;
	}
		
	Val ValueCloner2::operator()(const Record& node)
	{
		for (const auto& value : node.values)
		{
			//ValueCloner1でクローンは既に作ったので、そのクローンを直接書き換える
			const Val& substance = pEnv->expand(value.second, info);
			pEnv->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(value.second, boost::apply_visitor(*this, substance));
		}

		return node;
	}

	Val ValueCloner2::operator()(const FuncVal& node)const
	{
		if (node.builtinFuncAddress)
		{
			return node;
		}

		FuncVal result;
		result.arguments = node.arguments;
		result.builtinFuncAddress = node.builtinFuncAddress;

		AddressReplacer replacer(replaceMap, pEnv);
		result.expr = boost::apply_visitor(replacer, node.expr);

		return result;
	}
	
	boost::optional<Address> ValueCloner3::getOpt(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return boost::none;
	}

	Address ValueCloner3::replaced(Address address)const
	{
		auto it = replaceMap.find(address);
		if (it != replaceMap.end())
		{
			return it->second;
		}
		return address;
	}

	void ValueCloner3::operator()(Record& node)
	{
		AddressReplacer replacer(replaceMap, pEnv);

		if (node.constraint)
		{
			node.constraint = boost::apply_visitor(replacer, node.constraint.get());
		}

		for (auto& problem : node.problems)
		{
			if (problem.expr)
			{
				problem.expr = boost::apply_visitor(replacer, problem.expr.get());
			}

			for (size_t i = 0; i < problem.refs.size(); ++i)
			{
				const Address oldAddress = problem.refs[i];
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					problem.refs[i] = newAddress;
					problem.invRefs[newAddress] = problem.invRefs[oldAddress];
					problem.invRefs.erase(oldAddress);
				}
			}

			for (size_t i = 0; i < problem.freeVariableRefs.size(); ++i)
			{
				auto& currentElem = problem.freeVariableRefs[i];
				const Address oldAddress = currentElem.address;
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					currentElem.address = newAddress;
				}
			}
		}

		auto& original = node.original;

		/*for (auto& currentElem : original.freeVariableAddresses)
		{
			const Address oldAddress = currentElem.first;
			if (auto newAddressOpt = getOpt(oldAddress))
			{
				const Address newAddress = newAddressOpt.get();
				currentElem.first = newAddress;
			}
		}*/
		/*
		for (auto& currentElem : original.freeVars)
		{
			if (IsType<Accessor>(currentElem))
			{
				Expr expr = As<Accessor>(currentElem);
				currentElem = As<Accessor>(boost::apply_visitor(replacer, expr));
				//アクセッサの場合、ClosureMakerを通っているので先頭は必ずアドレスになっている
			}
			//elseの場合はReferenceであり、この場合はContextが勝手に参照先を切り替えるのでここでは何もしなくてよい
		}
		*/
		for (RegionVariable& var : original.regionVars)
		{
			if (auto newAddressOpt = getOpt(var.address))
			{
				var.address = newAddressOpt.get();
			}
		}

		for (auto& unitConstraint : original.unitConstraints)
		{
			unitConstraint = boost::apply_visitor(replacer, unitConstraint);
		}

		if (!original.variableAppearances.empty())
		{
			std::vector<ConstraintAppearance> newAppearance;

			for (auto& appearance : original.variableAppearances)
			{
				ConstraintAppearance currentAppearance;
				for (const Address address : appearance)
				{
					if (auto newAddressOpt = getOpt(address))
					{
						currentAppearance.insert(newAddressOpt.get());
					}
					else
					{
						currentAppearance.insert(address);
					}
				}
				newAppearance.push_back(currentAppearance);
			}

			original.variableAppearances = newAppearance;
		}

		for (auto& problem : original.groupConstraints)
		{
			if (problem.expr)
			{
				problem.expr = boost::apply_visitor(replacer, problem.expr.get());
			}

			for (size_t i = 0; i < problem.refs.size(); ++i)
			{
				const Address oldAddress = problem.refs[i];
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					problem.refs[i] = newAddress;
					problem.invRefs[newAddress] = problem.invRefs[oldAddress];
					problem.invRefs.erase(oldAddress);
				}
			}

			for (size_t i = 0; i < problem.freeVariableRefs.size(); ++i)
			{
				auto& currentElem = problem.freeVariableRefs[i];
				const Address oldAddress = currentElem.address;
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.get();
					currentElem.address = newAddress;
				}
			}
		}

		//constraintGroupsはただの数字だからそのままでよい

		/*
		for (auto& freeVariable : node.freeVariables)
		{
			if (IsType<Accessor>(freeVariable))
			{
				const Expr expr = As<Accessor>(freeVariable);
				freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
				//アクセッサの場合、ClosureMakerを通っているので先頭は必ずアドレスになっている
			}
			//Expr expr = freeVariable;
			//freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
		}
		*/

		for (auto& freeVar : node.boundedFreeVariables)
		{
			if (IsType<Accessor>(freeVar.freeVariable))
			{
				const Expr expr = As<Accessor>(freeVar.freeVariable);
				freeVar.freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
			}
			else
			{
				const Reference reference = As<Reference>(freeVar.freeVariable);
				freeVar.freeVariable = pEnv->cloneReference(reference, replaceMap);
			}
		}
		
		/*
		auto& problem = node.problem;
		if (problem.candidateExpr)
		{
			problem.candidateExpr = boost::apply_visitor(replacer, problem.candidateExpr.get());
		}

		for (size_t i = 0; i < problem.refs.size(); ++i)
		{
			const Address oldAddress = problem.refs[i];
			if (auto newAddressOpt = getOpt(oldAddress))
			{
				const Address newAddress = newAddressOpt.get();
				problem.refs[i] = newAddress;
				problem.invRefs[newAddress] = problem.invRefs[oldAddress];
				problem.invRefs.erase(oldAddress);
			}
		}

		for (auto& freeVariable : node.freeVariables)
		{
			Expr expr = freeVariable;
			freeVariable = As<Accessor>(boost::apply_visitor(replacer, expr));
		}
		*/
	}

	Val Clone(std::shared_ptr<Context> pEnv, const Val& value, const LocationInfo& info)
	{
		/*
		関数値がアドレスを内部に持っている時、クローン作成の前後でその依存関係を保存する必要があるので、クローン作成は2ステップに分けて行う。
		1. リスト・レコードの再帰的なコピー
		2. 関数の持つアドレスを新しい方に付け替える
		*/
		/*++cloneCount;
		double beginT = GetSec();

		ValueCloner cloner(pEnv, info);
		const Val& evaluated = boost::apply_visitor(cloner, value);
		double begin2T = GetSec();

		ValueCloner2 cloner2(pEnv, cloner.replaceMap, info);
		ValueCloner3 cloner3(pEnv, cloner.replaceMap);
		Val evaluated2 = boost::apply_visitor(cloner2, evaluated);
		
		double begin3T = GetSec();
		boost::apply_visitor(cloner3, evaluated2);
		double endT = GetSec();
		
		cloneTime += GetSec() - beginT;*/

		//++cloneCount;
		double beginT = GetSec();
		ValueCloner cloner(pEnv, info);
		const Val& evaluated = boost::apply_visitor(cloner, value);
		
		ValueCloner2 cloner2(pEnv, cloner.replaceMap, info);
		ValueCloner3 cloner3(pEnv, cloner.replaceMap);
		Val evaluated2 = boost::apply_visitor(cloner2, evaluated);

		boost::apply_visitor(cloner3, evaluated2);
		cloneTime += GetSec() - beginT;
		/*if (printAddressInsertion)
		{
			std::cout << "(" << cloner.time1 << ", " << cloner.time2 << ", " << cloner.time3 << ")\n";
		}*/
		/*if (printAddressInsertion)
		{
			std::cout << "(" << (begin2T - beginT) << ", " << (begin3T - begin2T) << ", " << (endT - begin3T) << ")\n";
		}*/

		/*{
			std::cout << "Cloner replaceMap:\n";
			std::vector<std::pair<Address, Address>> as;
			for (const auto& keyval : cloner.replaceMap)
			{
				as.emplace_back(keyval.first, keyval.second);
			}

			std::sort(as.begin(), as.end(), [](const std::pair<Address, Address>& a, const std::pair<Address, Address>& b) {return a.first < b.first; });
			for (const auto& keyval : as)
			{
				std::cout << keyval.first.toString() << " -> " << keyval.second.toString() << "\n";
			}
		}*/

		return evaluated2;
	}
}
