#pragma once
#pragma warning(disable:4996)
#include <functional>
#include <thread>
#include <mutex>

#include <cppoptlib/meta.h>
#include <cppoptlib/problem.h>

#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	class ProgressStore
	{
	public:
		static void TryWrite(std::shared_ptr<Context> env, const Val& value);

		static bool TryLock();

		static void Unlock();

		static std::shared_ptr<Context> GetContext();

		static boost::optional<Val>& GetVal();

		static void Reset();

		static bool HasValue();

	private:
		static ProgressStore& Instance();

		std::shared_ptr<Context> pEnv;
		boost::optional<Val> evaluated;

		std::mutex mtx;
	};

	class AddressReplacer : public boost::static_visitor<Expr>
	{
	public:
		const std::unordered_map<Address, Address>& replaceMap;
		std::shared_ptr<Context> pEnv;

		AddressReplacer(const std::unordered_map<Address, Address>& replaceMap, std::shared_ptr<Context> pEnv) :
			replaceMap(replaceMap),
			pEnv(pEnv)
		{}

		boost::optional<Address> getOpt(Address address)const;

		Expr operator()(const LRValue& node)const;

		Expr operator()(const Identifier& node)const { return node; }

		Expr operator()(const Import& node)const { return node; }

		Expr operator()(const UnaryExpr& node)const;

		Expr operator()(const BinaryExpr& node)const;

		Expr operator()(const Range& node)const;

		Expr operator()(const Lines& node)const;

		Expr operator()(const DefFunc& node)const;

		Expr operator()(const If& node)const;

		Expr operator()(const For& node)const;

		Expr operator()(const Return& node)const;

		Expr operator()(const ListConstractor& node)const;

		Expr operator()(const KeyExpr& node)const;

		Expr operator()(const RecordConstractor& node)const;
		
		Expr operator()(const RecordInheritor& node)const;

		Expr operator()(const Accessor& node)const;

		Expr operator()(const DeclSat& node)const;

		Expr operator()(const DeclFree& node)const;
	};

	//Valのアドレス値を再帰的に展開したクローンを作成する
	class ValueCloner : public boost::static_visitor<Val>
	{
	public:
		ValueCloner(std::shared_ptr<Context> pEnv) :
			pEnv(pEnv)
		{}

		std::shared_ptr<Context> pEnv;
		std::unordered_map<Address, Address> replaceMap;

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
		ValueCloner2(std::shared_ptr<Context> pEnv, const std::unordered_map<Address, Address>& replaceMap) :
			pEnv(pEnv),
			replaceMap(replaceMap)
		{}

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

	Val Clone(std::shared_ptr<Context> pEnv, const Val& value);

	//関数式を構成する識別子が関数内部で閉じているものか、外側のスコープに依存しているものかを調べ
	//外側のスコープを参照する識別子をアドレスに置き換えた式を返す
	class ClosureMaker : public boost::static_visitor<Expr>
	{
	public:

		//関数内部で閉じているローカル変数
		std::set<std::string> localVariables;

		std::shared_ptr<Context> pEnv;

		//レコード継承の構文を扱うために必要
		bool isInnerRecord;

		//内側の未評価式における参照変数の展開を防ぐために必要
		bool isInnerClosure;

		ClosureMaker(std::shared_ptr<Context> pEnv, const std::set<std::string>& functionArguments, bool isInnerRecord = false, bool isInnerClosure = false) :
			pEnv(pEnv),
			localVariables(functionArguments),
			isInnerRecord(isInnerRecord),
			isInnerClosure(isInnerClosure)
		{}

		ClosureMaker& addLocalVariable(const std::string& name);

		bool isLocalVariable(const std::string& name)const;

		Expr operator()(const LRValue& node) { return node; }

		Expr operator()(const Identifier& node);

		Expr operator()(const Import& node) { return node; }

		Expr operator()(const UnaryExpr& node);

		Expr operator()(const BinaryExpr& node);

		Expr operator()(const Range& node);

		Expr operator()(const Lines& node);

		Expr operator()(const DefFunc& node);

		Expr operator()(const If& node);

		Expr operator()(const For& node);

		Expr operator()(const Return& node);

		Expr operator()(const ListConstractor& node);

		Expr operator()(const KeyExpr& node);
		
		Expr operator()(const RecordConstractor& node);

		//レコード継承構文については特殊で、adderを評価する時のスコープはheadと同じである必要がある。
		//つまり、headを評価する時にはその中身を、一段階だけ（波括弧一つ分だけ）展開するようにして評価しなければならない。
		Expr operator()(const RecordInheritor& node);

		Expr operator()(const Accessor& node);
		
		Expr operator()(const DeclSat& node);

		Expr operator()(const DeclFree& node);
	};

	class Eval : public boost::static_visitor<LRValue>
	{
	public:

		Eval(std::shared_ptr<Context> pEnv) :pEnv(pEnv) {}

		virtual LRValue operator()(const LRValue& node) { return node; }

		virtual LRValue operator()(const Identifier& node) { return pEnv->findAddress(node); }

		virtual LRValue operator()(const Import& node);

		virtual LRValue operator()(const UnaryExpr& node);

		virtual LRValue operator()(const BinaryExpr& node);

		virtual LRValue operator()(const DefFunc& defFunc);

		virtual LRValue callFunction(const LocationInfo& info, const FuncVal& funcVal, const std::vector<Address>& expandedArguments);

		virtual LRValue operator()(const Range& range) { return RValue(0); }

		virtual LRValue operator()(const Lines& statement);

		virtual LRValue operator()(const If& if_statement);

		virtual LRValue operator()(const For& forExpression);

		virtual LRValue operator()(const Return& return_statement);

		virtual LRValue operator()(const ListConstractor& listConstractor);

		virtual LRValue operator()(const KeyExpr& keyExpr);

		virtual LRValue operator()(const RecordConstractor& recordConsractor);

		virtual LRValue operator()(const RecordInheritor& record);

		virtual LRValue operator()(const DeclSat& node);

		virtual LRValue operator()(const DeclFree& node);

		virtual LRValue operator()(const Accessor& accessor);

	private:
		std::shared_ptr<Context> pEnv;
		
		//sat/var宣言は現在の場所から見て最も内側のレコードに対して適用されるべきなので、その階層情報をスタックで持っておく
		//std::stack<std::reference_wrapper<Record>> currentRecords;

		//レコード継承を行う時に、レコードを作ってから合成するのは難しいので、古いレコードを拡張する形で作ることにする
		//boost::optional<Record&> temporaryRecord;
	};

	class HasFreeVariables : public boost::static_visitor<bool>
	{
	public:

		HasFreeVariables(std::shared_ptr<Context> pEnv, const std::vector<Address>& freeVariables) :
			pEnv(pEnv),
			freeVariables(freeVariables)
		{}

		//AccessorからObjectReferenceに変換するのに必要
		std::shared_ptr<Context> pEnv;

		//freeに指定された変数全て
		std::vector<Address> freeVariables;

		bool operator()(const LRValue& node);

		bool operator()(const Identifier& node);

		bool operator()(const Import& node) { return false; }

		bool operator()(const UnaryExpr& node);

		bool operator()(const BinaryExpr& node);

		bool operator()(const DefFunc& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Range& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Lines& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const If& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const For& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const Return& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const ListConstractor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const KeyExpr& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const RecordConstractor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclSat& node) { CGL_Error("invalid expression"); return false; }
		bool operator()(const DeclFree& node) { CGL_Error("invalid expression"); return false; }

		bool operator()(const Accessor& node);
	};

	Val evalExpr(const Expr& expr);

	bool IsEqualVal(const Val& value1, const Val& value2);

	bool IsEqual(const Expr& value1, const Expr& value2);
}
