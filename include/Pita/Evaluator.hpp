#pragma once
#pragma warning(disable:4996)
#include <iomanip>
#include <cmath>
#include <functional>
#include <thread>
#include <mutex>

#include "Node.hpp"
#include "Environment.hpp"
#include "BinaryEvaluator.hpp"
#include "Printer.hpp"
#include "Geometry.hpp"

namespace cgl
{
	class ProgressStore
	{
	public:
		static void TryWrite(std::shared_ptr<Environment> env, const Evaluated& value);

		static bool TryLock();

		static void Unlock();

		static std::shared_ptr<Environment> GetEnvironment();

		static boost::optional<Evaluated>& GetEvaluated();

		static void Reset();

		static bool HasValue();

	private:
		static ProgressStore& Instance();

		std::shared_ptr<Environment> pEnv;
		boost::optional<Evaluated> evaluated;

		std::mutex mtx;
	};

	class AddressReplacer : public boost::static_visitor<Expr>
	{
	public:
		const std::unordered_map<Address, Address>& replaceMap;

		AddressReplacer(const std::unordered_map<Address, Address>& replaceMap) :
			replaceMap(replaceMap)
		{}

		boost::optional<Address> getOpt(Address address)const;

		Expr operator()(const LRValue& node)const;

		Expr operator()(const Identifier& node)const { return node; }

		Expr operator()(const SatReference& node)const { return node; }

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

	//Evaluatedのアドレス値を再帰的に展開したクローンを作成する
	class ValueCloner : public boost::static_visitor<Evaluated>
	{
	public:
		ValueCloner(std::shared_ptr<Environment> pEnv) :
			pEnv(pEnv)
		{}

		std::shared_ptr<Environment> pEnv;
		std::unordered_map<Address, Address> replaceMap;

		boost::optional<Address> getOpt(Address address)const;

		Evaluated operator()(bool node) { return node; }

		Evaluated operator()(int node) { return node; }

		Evaluated operator()(double node) { return node; }

		Evaluated operator()(const List& node);

		Evaluated operator()(const KeyValue& node) { return node; }

		Evaluated operator()(const Record& node);

		Evaluated operator()(const FuncVal& node) { return node; }

		Evaluated operator()(const Jump& node) { return node; }
	};

	class ValueCloner2 : public boost::static_visitor<Evaluated>
	{
	public:
		ValueCloner2(std::shared_ptr<Environment> pEnv, const std::unordered_map<Address, Address>& replaceMap) :
			pEnv(pEnv),
			replaceMap(replaceMap)
		{}

		std::shared_ptr<Environment> pEnv;
		const std::unordered_map<Address, Address>& replaceMap;

		boost::optional<Address> getOpt(Address address)const;

		Evaluated operator()(bool node) { return node; }

		Evaluated operator()(int node) { return node; }

		Evaluated operator()(double node) { return node; }

		Evaluated operator()(const List& node);

		Evaluated operator()(const KeyValue& node) { return node; }

		Evaluated operator()(const Record& node);

		Evaluated operator()(const FuncVal& node)const;

		Evaluated operator()(const Jump& node) { return node; }
	};

	class ValueCloner3 : public boost::static_visitor<void>
	{
	public:
		ValueCloner3(std::shared_ptr<Environment> pEnv, const std::unordered_map<Address, Address>& replaceMap) :
			pEnv(pEnv),
			replaceMap(replaceMap)
		{}

		std::shared_ptr<Environment> pEnv;
		const std::unordered_map<Address, Address>& replaceMap;

		boost::optional<Address> getOpt(Address address)const;

		Address replaced(Address address)const;

		void operator()(bool& node) {}

		void operator()(int& node) {}

		void operator()(double& node) {}

		void operator()(List& node) {}

		void operator()(KeyValue& node) {}

		void operator()(Record& node);

		void operator()(FuncVal& node) {}

		void operator()(Jump& node) {}
	};

	Evaluated Clone(std::shared_ptr<Environment> pEnv, const Evaluated& value);

	//関数式を構成する識別子が関数内部で閉じているものか、外側のスコープに依存しているものかを調べ
	//外側のスコープを参照する識別子をアドレスに置き換えた式を返す
	class ClosureMaker : public boost::static_visitor<Expr>
	{
	public:

		//関数内部で閉じているローカル変数
		std::set<std::string> localVariables;

		std::shared_ptr<Environment> pEnv;

		//レコード継承の構文を扱うために必要必要
		bool isInnerRecord;

		ClosureMaker(std::shared_ptr<Environment> pEnv, const std::set<std::string>& functionArguments, bool isInnerRecord = false) :
			pEnv(pEnv),
			localVariables(functionArguments),
			isInnerRecord(isInnerRecord)
		{}

		ClosureMaker& addLocalVariable(const std::string& name);

		bool isLocalVariable(const std::string& name)const;

		Expr operator()(const LRValue& node) { return node; }

		Expr operator()(const Identifier& node);

		Expr operator()(const SatReference& node) { return node; }

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

	class ConstraintProblem : public cppoptlib::Problem<double>
	{
	public:
		using typename cppoptlib::Problem<double>::TVector;

		std::function<double(const TVector&)> evaluator;
		Record originalRecord;
		std::vector<Identifier> keyList;
		std::shared_ptr<Environment> pEnv;

		bool callback(const cppoptlib::Criteria<cppoptlib::Problem<double>::Scalar>& state, const TVector& x) override;

		double value(const TVector &x)
		{
			return evaluator(x);
		}
	};

	class Eval : public boost::static_visitor<LRValue>
	{
	public:

		Eval(std::shared_ptr<Environment> pEnv) :pEnv(pEnv) {}

		LRValue operator()(const LRValue& node) { return node; }

		LRValue operator()(const Identifier& node) { return pEnv->findAddress(node); }

		LRValue operator()(const SatReference& node) { CGL_Error("不正な式"); return LRValue(0); }

		LRValue operator()(const UnaryExpr& node);

		LRValue operator()(const BinaryExpr& node);

		LRValue operator()(const DefFunc& defFunc);
		
		LRValue callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments);

		LRValue operator()(const Range& range) { return RValue(0); }

		LRValue operator()(const Lines& statement);

		LRValue operator()(const If& if_statement);

		LRValue operator()(const For& forExpression);

		LRValue operator()(const Return& return_statement);

		LRValue operator()(const ListConstractor& listConstractor);

		LRValue operator()(const KeyExpr& keyExpr);

		LRValue operator()(const RecordConstractor& recordConsractor);

		LRValue operator()(const RecordInheritor& record);

		LRValue operator()(const DeclSat& node);

		LRValue operator()(const DeclFree& node);

		LRValue operator()(const Accessor& accessor);

	private:
		std::shared_ptr<Environment> pEnv;
		
		//sat/var宣言は現在の場所から見て最も内側のレコードに対して適用されるべきなので、その階層情報をスタックで持っておく
		std::stack<std::reference_wrapper<Record>> currentRecords;

		//レコード継承を行う時に、レコードを作ってから合成するのは難しいので、古いレコードを拡張する形で作ることにする
		boost::optional<Record&> temporaryRecord;
	};

	class HasFreeVariables : public boost::static_visitor<bool>
	{
	public:

		HasFreeVariables(std::shared_ptr<Environment> pEnv, const std::vector<Address>& freeVariables) :
			pEnv(pEnv),
			freeVariables(freeVariables)
		{}

		//AccessorからObjectReferenceに変換するのに必要
		std::shared_ptr<Environment> pEnv;

		//freeに指定された変数全て
		std::vector<Address> freeVariables;

		bool operator()(const LRValue& node);

		bool operator()(const Identifier& node);

		bool operator()(const SatReference& node) { return true; }

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

	class Expr2SatExpr : public boost::static_visitor<Expr>
	{
	public:

		int refID_Offset;

		//AccessorからObjectReferenceに変換するのに必要
		std::shared_ptr<Environment> pEnv;

		//freeに指定された変数全て
		std::vector<Address> freeVariables;

		//freeに指定された変数が実際にsatに現れたかどうか
		std::vector<char> usedInSat;

		//FreeVariables Index -> SatReference
		std::map<int, SatReference> satRefs;

		//TODO:vector->mapに書き換える
		std::vector<Address> refs;

		Expr2SatExpr(int refID_Offset, std::shared_ptr<Environment> pEnv, const std::vector<Address>& freeVariables) :
			refID_Offset(refID_Offset),
			pEnv(pEnv),
			freeVariables(freeVariables),
			usedInSat(freeVariables.size(), 0)
		{}

		boost::optional<size_t> freeVariableIndex(Address reference);

		boost::optional<SatReference> getSatRef(Address reference);

		boost::optional<SatReference> addSatRef(Address reference);
				
		Expr operator()(const LRValue& node);

		//ここにIdentifierが残っている時点でClosureMakerにローカル変数だと判定された変数のはず
		Expr operator()(const Identifier& node);

		Expr operator()(const SatReference& node) { return node; }

		Expr operator()(const UnaryExpr& node);

		Expr operator()(const BinaryExpr& node);

		Expr operator()(const DefFunc& node) { CGL_Error("invalid expression"); return LRValue(0); }

		Expr operator()(const Range& node) { CGL_Error("invalid expression"); return LRValue(0); }

		Expr operator()(const Lines& node);

		Expr operator()(const If& node);

		Expr operator()(const For& node);

		Expr operator()(const Return& node) { CGL_Error("invalid expression"); return LRValue(0); }
		
		Expr operator()(const ListConstractor& node);

		Expr operator()(const KeyExpr& node) { return node; }

		Expr operator()(const RecordConstractor& node);

		Expr operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return LRValue(0); }
		Expr operator()(const DeclSat& node) { CGL_Error("invalid expression"); return LRValue(0); }
		Expr operator()(const DeclFree& node) { CGL_Error("invalid expression"); return LRValue(0); }

		Expr operator()(const Accessor& node);
	};

	Evaluated evalExpr(const Expr& expr);

	bool IsEqualEvaluated(const Evaluated& value1, const Evaluated& value2);

	bool IsEqual(const Expr& value1, const Expr& value2);
}
