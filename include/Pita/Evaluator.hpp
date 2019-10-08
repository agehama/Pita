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

	//プログラム実行（再帰非対応）
	//boost::optional<const Val&> evalExpr(const Expr& expr);
	LRValue ExecuteProgram(const Expr& expr, std::shared_ptr<Context> pContext);

	//プログラム実行（再帰対応）
	//boost::optional<const Val&> ExecuteProgramWithRec(const Expr& expr, std::shared_ptr<Context> pContext = nullptr);
	LRValue ExecuteProgramWithRec(const Expr& expr, std::shared_ptr<Context> pContext);

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
