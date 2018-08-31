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
	//Expr -> Expr の変換を行う処理全般のひな型
	class ExprTransformer : public boost::static_visitor<Expr>
	{
	public:
		ExprTransformer() = default;
		virtual ~ExprTransformer() = default;

		virtual Expr operator()(const LRValue& node);
		virtual Expr operator()(const Identifier& node);
		virtual Expr operator()(const UnaryExpr& node);
		virtual Expr operator()(const BinaryExpr& node);
		virtual Expr operator()(const Lines& node);
		virtual Expr operator()(const DefFunc& node);
		virtual Expr operator()(const If& node);
		virtual Expr operator()(const For& node);
		virtual Expr operator()(const ListConstractor& node);
		virtual Expr operator()(const KeyExpr& node);
		virtual Expr operator()(const RecordConstractor& node);
		virtual Expr operator()(const Accessor& node);
		virtual Expr operator()(const DeclSat& node);
		virtual Expr operator()(const DeclFree& node);
		virtual Expr operator()(const Import& node);
		virtual Expr operator()(const Range& node);
		virtual Expr operator()(const Return& node);
	};

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

	Val Clone(std::shared_ptr<Context> pEnv, const Val& value, const LocationInfo& info);

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
		Expr operator()(const Accessor& node);
		Expr operator()(const DeclSat& node);
		Expr operator()(const DeclFree& node);
	};

	class Eval : public boost::static_visitor<LRValue>
	{
	public:
		Eval(std::shared_ptr<Context> pEnv) :pEnv(pEnv) {}

		virtual LRValue operator()(const LRValue& node) { return node; }
		virtual LRValue operator()(const Identifier& node);
		virtual LRValue operator()(const Import& node);
		virtual LRValue operator()(const UnaryExpr& node);
		virtual LRValue operator()(const BinaryExpr& node);
		virtual LRValue operator()(const DefFunc& defFunc);
		virtual LRValue callFunction(const LocationInfo& info, const FuncVal& funcVal, const std::vector<Address>& arguments);
		virtual LRValue inheritRecord(const LocationInfo& info, const Record& original, const RecordConstractor& adder);
		virtual LRValue operator()(const Range& range) { return RValue(0); }
		virtual LRValue operator()(const Lines& statement);
		virtual LRValue operator()(const If& if_statement);
		virtual LRValue operator()(const For& forExpression);
		virtual LRValue operator()(const Return& return_statement);
		virtual LRValue operator()(const ListConstractor& listConstractor);
		virtual LRValue operator()(const KeyExpr& keyExpr);
		virtual LRValue operator()(const RecordConstractor& recordConsractor);
		virtual LRValue operator()(const DeclSat& node);
		virtual LRValue operator()(const DeclFree& node);
		virtual LRValue operator()(const Accessor& accessor);

	protected:
		std::shared_ptr<Context> pEnv;
	};

	boost::optional<const Val&> evalExpr(const Expr& expr);

	bool IsEqualVal(const Val& value1, const Val& value2);

	bool IsEqual(const Expr& value1, const Expr& value2);

	inline Expr ExpandedEmptyLines(const Expr& expr)
	{
		Expr result = expr;

		//一番外側の空のLinesは展開して返す
		while (IsType<Lines>(result) && As<Lines>(result).exprs.size() == 1)
		{
			const Expr copy = As<Lines>(result).exprs.front();
			result = copy;
		}

		return result;
	}
}
