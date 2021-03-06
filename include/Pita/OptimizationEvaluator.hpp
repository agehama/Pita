#pragma once
#pragma warning(disable:4996)
#include "Node.hpp"
#include "Context.hpp"
#include "Evaluator.hpp"

namespace cgl
{
	//sat式の中に一つでもfree変数があればtrueを返す
	class SatVariableBinder : public boost::static_visitor<bool>
	{
	public:
		//AccessorからAddressに変換するのに必要
		std::shared_ptr<Context> pEnv;

		//free変数集合->freeに指定された変数全て
		const std::vector<RegionVariable>& freeVariables;

		//free変数集合->freeに指定された変数が実際にsatに現れたかどうか
		std::vector<char>& usedInSat;

		//参照ID->Address
		std::vector<Address>& refs;
		std::unordered_set<Address>& refsSet;

		//Address->参照ID
		std::unordered_map<Address, int>& invRefs;

		bool& hasPlateausFunction;

		//ローカル変数
		std::set<std::string> localVariables;

		SatVariableBinder(
			std::shared_ptr<Context> pEnv,
			const std::vector<RegionVariable>& freeVariables,
			std::vector<char>& usedInSat,
			std::vector<Address>& refs,
			std::unordered_set<Address>& refsSet,
			std::unordered_map<Address, int>& invRefs,
			bool& hasPlateausFunction) :
			pEnv(pEnv),
			freeVariables(freeVariables),
			usedInSat(usedInSat),
			//usedInSat(freeVariables.size(), 0),
			refs(refs),
			refsSet(refsSet),
			invRefs(invRefs),
			hasPlateausFunction(hasPlateausFunction)
		{}

		SatVariableBinder& addLocalVariable(const std::string& name);

		bool isLocalVariable(const std::string& name)const;

		/*int depth = 0;
		std::string getIndent()const
		{
			const int tabSize = 4;
			std::string str;
			for (int i = 0; i < depth*tabSize; ++i)
			{
				if (i % tabSize == 0)
				{
					const int a = (i / tabSize) % 10;
					str += '0' + a;
				}
				else
				{
					str += ' ';
				}
			}
			return str;
		}*/

		boost::optional<size_t> freeVariableIndex(Address reference)const;

		//Address -> 参照ID
		boost::optional<int> addSatRefImpl(Address reference);
		bool addSatRef(Address reference);

		bool operator()(const LRValue& node);
		bool operator()(const Identifier& node);
		bool operator()(const Import& node) { return false; }
		bool operator()(const UnaryExpr& node);
		bool operator()(const BinaryExpr& node);
		bool operator()(const DefFunc& node) { CGL_Error("invalid expression"); return false; }
		bool callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments, const LocationInfo& info);
		bool operator()(const Range& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Lines& node);
		bool operator()(const If& node);
		bool operator()(const For& node);
		bool operator()(const Return& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const ListConstractor& node);
		bool operator()(const KeyExpr& node);
		bool operator()(const RecordConstractor& node);
		bool operator()(const DeclSat& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclFree& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Accessor& node);
	};

	//制約を上から論理積で分割する
	class ConjunctionSeparater : public boost::static_visitor<void>
	{
	public:
		ConjunctionSeparater() = default;

		std::vector<Expr> conjunctions;

		void operator()(const LRValue& node) { conjunctions.push_back(node); }
		void operator()(const Identifier& node) { conjunctions.push_back(node); }
		void operator()(const Import& node) { conjunctions.push_back(node); }
		void operator()(const UnaryExpr& node) { conjunctions.push_back(node); }
		void operator()(const BinaryExpr& node)
		{
			if (node.op == BinaryOp::And)
			{
				boost::apply_visitor(*this, node.lhs);
				boost::apply_visitor(*this, node.rhs);
			}
			else
			{
				conjunctions.push_back(node);
			}
		}
		void operator()(const DefFunc& node) { conjunctions.push_back(node); }
		void operator()(const Range& node) { conjunctions.push_back(node); }
		void operator()(const Lines& node) { conjunctions.push_back(node); }
		void operator()(const If& node) { conjunctions.push_back(node); }
		void operator()(const For& node) { conjunctions.push_back(node); }
		void operator()(const Return& node) { conjunctions.push_back(node); }
		void operator()(const ListConstractor& node) { conjunctions.push_back(node); }
		void operator()(const KeyExpr& node) { conjunctions.push_back(node); }
		void operator()(const RecordConstractor& node) { conjunctions.push_back(node); }
		void operator()(const DeclSat& node) { conjunctions.push_back(node); }
		void operator()(const DeclFree& node) { conjunctions.push_back(node); }
		void operator()(const Accessor& node) { conjunctions.push_back(node); }

	private:
	};

	class EvalSatExpr : public Eval
	{
	public:
		const std::vector<double>& data;//参照ID->data
		const std::vector<Address>& refs;//参照ID->Address
		const std::unordered_map<Address, int>& invRefs;//Address->参照ID

		//TODO:このpEnvは外部の環境を書き換えたくないので、独立したものを設定する
		EvalSatExpr(
			std::shared_ptr<Context> pEnv,
			const std::vector<double>& data,
			const std::vector<Address>& refs,
			const std::unordered_map<Address, int>& invRefs) :
			Eval(pEnv),
			data(data),
			refs(refs),
			invRefs(invRefs)
		{}

		bool isFreeVariable(Address address)const
		{
			return invRefs.find(address) != invRefs.end();
		}

		boost::optional<double> expandFreeOpt(Address address)const;

		LRValue operator()(const LRValue& node)override { return Eval::operator()(node); }
		LRValue operator()(const Identifier& node)override { return Eval::operator()(node); }
		LRValue operator()(const Import& node)override { return Eval::operator()(node); }
		LRValue operator()(const UnaryExpr& node)override;
		LRValue operator()(const BinaryExpr& node)override;
		LRValue operator()(const DefFunc& node)override { return Eval::operator()(node); }
		LRValue callFunction(const LocationInfo& info, const FuncVal& funcVal, const std::vector<Address>& expandedArguments)override
		{ return Eval::callFunction(info, funcVal, expandedArguments); }
		LRValue inheritRecord(const LocationInfo& info, const Record& original, const RecordConstractor& adder)override
		{ return Eval::inheritRecord(info, original, adder); }
		LRValue operator()(const Range& node)override { return Eval::operator()(node); }
		LRValue operator()(const Lines& node)override { return Eval::operator()(node); }
		LRValue operator()(const If& node)override { return Eval::operator()(node); }
		LRValue operator()(const For& node)override { return Eval::operator()(node); }
		LRValue operator()(const Return& node)override { return Eval::operator()(node); }
		LRValue operator()(const ListConstractor& node)override { return Eval::operator()(node); }
		LRValue operator()(const KeyExpr& node)override;
		LRValue operator()(const RecordConstractor& node)override { return Eval::operator()(node); }
		LRValue operator()(const DeclSat& node)override { return Eval::operator()(node); }
		LRValue operator()(const DeclFree& node)override { return Eval::operator()(node); }
		LRValue operator()(const Accessor& node)override { return Eval::operator()(node); }

		/*
		LRValue operator()(const LRValue& node)override {
			CGL_DBG; 
			auto result = Eval::operator()(node); CGL_DBG; return result;
		}
		LRValue operator()(const Identifier& node)override { std::cerr << "Identifier\"" << node.toString() << "\"" << std::endl; return Eval::operator()(node); }
		LRValue operator()(const Import& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const UnaryExpr& node)override;
		LRValue operator()(const BinaryExpr& node)override;
		LRValue operator()(const DefFunc& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue callFunction(const LocationInfo& info, const FuncVal& funcVal, const std::vector<Address>& expandedArguments)override
		{
			CGL_DBG;
			auto result = Eval::callFunction(info, funcVal, expandedArguments);
			CGL_DBG; return result;
		}
		LRValue inheritRecord(const LocationInfo& info, const Record& original, const RecordConstractor& adder)override
		{
			CGL_DBG;
			auto result = Eval::inheritRecord(info, original, adder); CGL_DBG; return result;
		}
		LRValue operator()(const Range& node)override { CGL_DBG;
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const Lines& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const If& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const For& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const Return& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const ListConstractor& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const KeyExpr& node)override;
		LRValue operator()(const RecordConstractor& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const DeclSat& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const DeclFree& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		LRValue operator()(const Accessor& node)override { CGL_DBG; 
		auto result = Eval::operator()(node); CGL_DBG; return result; }
		*/
	};
}
