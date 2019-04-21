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
