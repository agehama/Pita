#pragma once
#pragma warning(disable:4996)
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	//sat式の中に一つでもfree変数があればtrueを返す
	class SatVariableBinder : public boost::static_visitor<bool>
	{
	public:

		//AccessorからObjectReferenceに変換するのに必要
		std::shared_ptr<Context> pEnv;

		//free変数集合->freeに指定された変数全て
		//std::vector<Address> freeVariables;
		const std::vector<Address>& freeVariables;

		//free変数集合->freeに指定された変数が実際にsatに現れたかどうか
		std::vector<char>& usedInSat;

		//参照ID->Address
		//std::vector<Address> refs;
		std::vector<Address>& refs;

		//Address->参照ID
		//std::unordered_map<Address, int> invRefs;
		std::unordered_map<Address, int>& invRefs;

		//bool hasPlateausFunction = false;
		bool& hasPlateausFunction;

		//ローカル変数
		std::set<std::string> localVariables;

		/*SatVariableBinder(std::shared_ptr<Context> pEnv, const std::vector<Address>& freeVariables) :
			pEnv(pEnv),
			freeVariables(freeVariables),
			usedInSat(freeVariables.size(), 0)
		{}*/
		SatVariableBinder(
			std::shared_ptr<Context> pEnv, 
			const std::vector<Address>& freeVariables, 
			std::vector<char>& usedInSat,
			std::vector<Address>& refs,
			std::unordered_map<Address, int>& invRefs,
			bool& hasPlateausFunction) :
			pEnv(pEnv),
			freeVariables(freeVariables),
			usedInSat(usedInSat),
			//usedInSat(freeVariables.size(), 0),
			refs(refs),
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

		boost::optional<size_t> freeVariableIndex(Address reference);

		//Address -> 参照ID
		boost::optional<int> addSatRef(Address reference);

		bool operator()(const LRValue& node);

		bool operator()(const Identifier& node);

		bool operator()(const SatReference& node) { return true; }

		bool operator()(const UnaryExpr& node);

		bool operator()(const BinaryExpr& node);

		bool operator()(const DefFunc& node) { CGL_Error("invalid expression"); return false; }
		
		bool callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments);

		bool operator()(const Range& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const Lines& node);

		bool operator()(const If& node);

		bool operator()(const For& node);

		bool operator()(const Return& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const ListConstractor& node);

		bool operator()(const KeyExpr& node);

		bool operator()(const RecordConstractor& node);

		bool operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclSat& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclFree& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const Accessor& node);
	};

	class EvalSatExpr : public boost::static_visitor<Val>
	{
	public:
		std::shared_ptr<Context> pEnv;
		const std::vector<double>& data;//参照ID->data
		const std::vector<Address>& refs;//参照ID->Address
		const std::unordered_map<Address, int>& invRefs;//Address->参照ID

		//TODO:このpEnvは外部の環境を書き換えたくないので、独立したものを設定する
		EvalSatExpr(
			std::shared_ptr<Context> pEnv, 
			const std::vector<double>& data, 
			const std::vector<Address>& refs,
			const std::unordered_map<Address, int>& invRefs) :
			pEnv(pEnv),
			data(data),
			refs(refs),
			invRefs(invRefs)
		{}

		bool isFreeVariable(Address address)const
		{
			return invRefs.find(address) != invRefs.end();
		}

		boost::optional<double> expandFreeOpt(Address address)const;

		Val operator()(const LRValue& node);

		Val operator()(const SatReference& node);

		Val operator()(const Identifier& node);

		Val operator()(const UnaryExpr& node);

		Val operator()(const BinaryExpr& node);

		Val operator()(const DefFunc& node) { CGL_Error("不正な式です"); return 0; }

		//Val operator()(const FunctionCaller& callFunc)
		Val callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments);

		Val operator()(const Range& node) { CGL_Error("不正な式です"); return 0; }

		Val operator()(const Lines& node);

		Val operator()(const If& if_statement);

		Val operator()(const For& node) { CGL_Error("invalid expression"); return 0; }

		Val operator()(const Return& node) { CGL_Error("invalid expression"); return 0; }

		Val operator()(const ListConstractor& node) { CGL_Error("invalid expression"); return 0; }

		Val operator()(const KeyExpr& node);

		Val operator()(const RecordConstractor& recordConsractor);

		Val operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return 0; }
		Val operator()(const DeclSat& node) { CGL_Error("invalid expression"); return 0; }
		Val operator()(const DeclFree& node) { CGL_Error("invalid expression"); return 0; }

		Val operator()(const Accessor& node);
	};
}
