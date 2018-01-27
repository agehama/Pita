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
		static void TryWrite(std::shared_ptr<Environment> env, const Evaluated& value)
		{
			ProgressStore& i = Instance();
			if (i.mtx.try_lock())
			{
				i.pEnv = env->clone();
				i.evaluated = value;

				i.mtx.unlock();
			}
		}

		static bool TryLock()
		{
			ProgressStore& i = Instance();
			return i.mtx.try_lock();
		}

		static void Unlock()
		{
			ProgressStore& i = Instance();
			i.mtx.unlock();
		}

		static std::shared_ptr<Environment> GetEnvironment()
		{
			ProgressStore& i = Instance();
			return i.pEnv;
		}

		static boost::optional<Evaluated>& GetEvaluated()
		{
			ProgressStore& i = Instance();
			return i.evaluated;
		}

		static void Reset()
		{
			ProgressStore& i = Instance();
			i.pEnv = nullptr;
			i.evaluated = boost::none;
		}

		static bool HasValue()
		{
			ProgressStore& i = Instance();
			return static_cast<bool>(i.evaluated);
		}

	private:
		static ProgressStore& Instance()
		{
			static ProgressStore i;
			return i;
		}

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

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}

		Expr operator()(const LRValue& node)const
		{
			if (node.isLValue())
			{
				if (auto opt = getOpt(node.address()))
				{
					return LRValue(opt.value());
				}
			}
			return node;
		}

		Expr operator()(const Identifier& node)const { return node; }

		Expr operator()(const SatReference& node)const { return node; }

		Expr operator()(const UnaryExpr& node)const
		{
			return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op);
		}

		Expr operator()(const BinaryExpr& node)const
		{
			return BinaryExpr(
				boost::apply_visitor(*this, node.lhs), 
				boost::apply_visitor(*this, node.rhs), 
				node.op);
		}

		Expr operator()(const Range& node)const
		{
			return Range(
				boost::apply_visitor(*this, node.lhs),
				boost::apply_visitor(*this, node.rhs));
		}

		Expr operator()(const Lines& node)const
		{
			Lines result;
			for (size_t i = 0; i < node.size(); ++i)
			{
				result.add(boost::apply_visitor(*this, node.exprs[i]));
			}
			return result;
		}

		Expr operator()(const DefFunc& node)const
		{
			return DefFunc(node.arguments, boost::apply_visitor(*this, node.expr));
		}

		Expr operator()(const If& node)const
		{
			If result(boost::apply_visitor(*this, node.cond_expr));
			result.then_expr = boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
			}

			return result;
		}

		Expr operator()(const For& node)const
		{
			For result;
			result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
			result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
			result.loopCounter = node.loopCounter;
			result.doExpr = boost::apply_visitor(*this, node.doExpr);
			return result;
		}

		Expr operator()(const Return& node)const
		{
			return Return(boost::apply_visitor(*this, node.expr));
		}

		Expr operator()(const ListConstractor& node)const
		{
			ListConstractor result;
			for (const auto& expr : node.data)
			{
				result.data.push_back(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const KeyExpr& node)const
		{
			KeyExpr result(node.name);
			result.expr = boost::apply_visitor(*this, node.expr);
			return result;
		}

		Expr operator()(const RecordConstractor& node)const
		{
			RecordConstractor result;
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(*this, expr));
			}
			return result;
		}
		
		Expr operator()(const RecordInheritor& node)const
		{
			RecordInheritor result;
			result.original = boost::apply_visitor(*this, node.original);

			Expr originalAdder = node.adder;
			Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
			if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
			{
				result.adder = opt.value();
				return result;
			}

			CGL_Error("node.adderの置き換え結果がRecordConstractorでない");
			return LRValue(0);
		}

		Expr operator()(const Accessor& node)const
		{
			Accessor result;
			result.head = boost::apply_visitor(*this, node.head);
			
			for (const auto& access : node.accesses)
			{
				if (auto listAccess = AsOpt<ListAccess>(access))
				{
					result.add(ListAccess(boost::apply_visitor(*this, listAccess.value().index)));
				}
				else if (auto recordAccess = AsOpt<RecordAccess>(access))
				{
					result.add(recordAccess.value());
				}
				else if (auto functionAccess = AsOpt<FunctionAccess>(access))
				{
					FunctionAccess access;
					for (const auto& argument : functionAccess.value().actualArguments)
					{
						access.add(boost::apply_visitor(*this, argument));
					}
					result.add(access);
				}
				else
				{
					CGL_Error("aaa");
				}
			}

			return result;
		}

		Expr operator()(const DeclSat& node)const
		{
			return DeclSat(boost::apply_visitor(*this, node.expr));
		}

		Expr operator()(const DeclFree& node)const
		{
			DeclFree result;
			for (const auto& accessor : node.accessors)
			{
				Expr expr = accessor;
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}
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

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}

		Evaluated operator()(bool node) { return node; }

		Evaluated operator()(int node) { return node; }

		Evaluated operator()(double node) { return node; }

		Evaluated operator()(const List& node)
		{
			List result;
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				if (auto opt = getOpt(data[i]))
				{
					result.append(opt.value());
				}
				else
				{
					const Evaluated& substance = pEnv->expand(data[i]);
					const Evaluated clone = boost::apply_visitor(*this, substance);
					const Address newAddress = pEnv->makeTemporaryValue(clone);
					result.append(newAddress);
					replaceMap[data[i]] = newAddress;
				}
			}

			return result;
		}

		Evaluated operator()(const KeyValue& node) { return node; }

		Evaluated operator()(const Record& node)
		{
			Record result;

			for (const auto& value : node.values)
			{
				if (auto opt = getOpt(value.second))
				{
					result.append(value.first, opt.value());
				}
				else
				{
					const Evaluated& substance = pEnv->expand(value.second);
					const Evaluated clone = boost::apply_visitor(*this, substance);
					const Address newAddress = pEnv->makeTemporaryValue(clone);
					result.append(value.first, newAddress);
					replaceMap[value.second] = newAddress;
				}
			}

			result.problem = node.problem;
			result.freeVariables = node.freeVariables;
			result.freeVariableRefs = node.freeVariableRefs;

			return result;
		}

		Evaluated operator()(const FuncVal& node) { return node; }

		Evaluated operator()(const Jump& node) { return node; }

		//Evaluated operator()(const DeclFree& node) { return node; }
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

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}
		Evaluated operator()(bool node) { return node; }

		Evaluated operator()(int node) { return node; }

		Evaluated operator()(double node) { return node; }

		Evaluated operator()(const List& node)
		{
			const auto& data = node.data;

			for (size_t i = 0; i < data.size(); ++i)
			{
				//ValueCloner1でクローンは既に作ったので、そのクローンを直接書き換える
				const Evaluated& substance = pEnv->expand(data[i]);
				pEnv->assignToObject(data[i], boost::apply_visitor(*this, substance));
			}

			return node;
		}

		Evaluated operator()(const KeyValue& node) { return node; }

		Evaluated operator()(const Record& node)
		{
			for (const auto& value : node.values)
			{
				//ValueCloner1でクローンは既に作ったので、そのクローンを直接書き換える
				const Evaluated& substance = pEnv->expand(value.second);
				pEnv->assignToObject(value.second, boost::apply_visitor(*this, substance));
			}

			node.problem;
			node.freeVariables;
			node.freeVariableRefs;

			return node;
		}

		Evaluated operator()(const FuncVal& node)const
		{
			if (node.builtinFuncAddress)
			{
				return node;
			}

			FuncVal result;
			result.arguments = node.arguments;
			result.builtinFuncAddress = node.builtinFuncAddress;

			AddressReplacer replacer(replaceMap);
			result.expr = boost::apply_visitor(replacer, node.expr);

			return result;
		}

		Evaluated operator()(const Jump& node) { return node; }

		//Evaluated operator()(const DeclFree& node) { return node; }
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

		boost::optional<Address> getOpt(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return boost::none;
		}

		Address replaced(Address address)const
		{
			auto it = replaceMap.find(address);
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return address;
		}

		void operator()(bool& node) {}

		void operator()(int& node) {}

		void operator()(double& node) {}

		void operator()(List& node) {}

		void operator()(KeyValue& node) {}

		void operator()(Record& node)
		{
			AddressReplacer replacer(replaceMap);

			auto& problem = node.problem;
			if (problem.candidateExpr)
			{
				problem.candidateExpr = boost::apply_visitor(replacer, problem.candidateExpr.value());
			}

			for (size_t i = 0; i < problem.refs.size(); ++i)
			{
				const Address oldAddress = problem.refs[i];
				if (auto newAddressOpt = getOpt(oldAddress))
				{
					const Address newAddress = newAddressOpt.value();
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
		}

		void operator()(FuncVal& node) {}

		void operator()(Jump& node) {}

		//Evaluated operator()(const DeclFree& node) { return node; }
	};

	inline Evaluated Clone(std::shared_ptr<Environment> pEnv, const Evaluated& value)
	{
		/*
		関数値がアドレスを内部に持っている時、クローン作成の前後でその依存関係を保存する必要があるので、クローン作成は2ステップに分けて行う。
		1. リスト・レコードの再帰的なコピー
		2. 関数の持つアドレスを新しい方に付け替える		
		*/
		ValueCloner cloner(pEnv);
		const Evaluated& evaluated = boost::apply_visitor(cloner, value);
		ValueCloner2 cloner2(pEnv, cloner.replaceMap);
		ValueCloner3 cloner3(pEnv, cloner.replaceMap);
		Evaluated evaluated2 = boost::apply_visitor(cloner2, evaluated);
		boost::apply_visitor(cloner3, evaluated2);
		return evaluated2;
	}

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

		ClosureMaker& addLocalVariable(const std::string& name)
		{
			localVariables.insert(name);
			return *this;
		}

		bool isLocalVariable(const std::string& name)const
		{
			return localVariables.find(name) != localVariables.end();
		}

		Expr operator()(const LRValue& node) { return node; }

		Expr operator()(const Identifier& node)
		{
			//その関数のローカル変数であれば関数の実行時に評価すればよいので、名前を残す
			if (isLocalVariable(node))
			{
				return node;
			}
			//ローカル変数に無ければアドレスに置き換える
			const Address address = pEnv->findAddress(node);
			if (address.isValid())
			{
				//Identifier RecordConstructor の形をしたレコード継承の head 部分
				//とりあえず参照先のレコードのメンバはローカル変数とおく
				if (isInnerRecord)
				{
					const Evaluated& evaluated = pEnv->expand(address);
					if (auto opt = AsOpt<Record>(evaluated))
					{
						const Record& record = opt.value();
						for (const auto& keyval : record.values)
						{
							addLocalVariable(keyval.first);
						}
					}
				}

				return LRValue(address);
			}

			CGL_Error("識別子が定義されていません");
			return LRValue(0);
		}

		Expr operator()(const SatReference& node) { return node; }

		Expr operator()(const UnaryExpr& node)
		{
			return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op);
		}

		Expr operator()(const BinaryExpr& node)
		{
			const Expr rhs = boost::apply_visitor(*this, node.rhs);

			if (node.op != BinaryOp::Assign)
			{
				const Expr lhs = boost::apply_visitor(*this, node.lhs);
				return BinaryExpr(lhs, rhs, node.op);
			}

			//Assignの場合、lhs は Address or Identifier or Accessor に限られる
			//つまり現時点では、(if cond then x else y) = true のような式を許可していない
			//ここで左辺に直接アドレスが入っていることは有り得る？
			//a = b = 10　のような式でも、右結合であり左側は常に識別子が残っているはずなので、あり得ないと思う
			/*if (auto valOpt = AsOpt<Evaluated>(node.lhs))
			{
				const Evaluated& val = valOpt.value();
				if (IsType<Address>(val))
				{
					if (Address address = As<Address>(val))
					{

					}
					else
					{
						ErrorLog("");
						return 0;
					}
				}
				else
				{
					ErrorLog("");
					return 0;
				}
			}
			else */if (auto valOpt = AsOpt<Identifier>(node.lhs))
			{
				const Identifier identifier = valOpt.value();

				//ローカル変数にあれば、その場で解決できる識別子なので何もしない
				if (isLocalVariable(identifier))
				{
					return BinaryExpr(node.lhs, rhs, node.op);
				}
				else
				{
					const Address address = pEnv->findAddress(identifier);

					//ローカル変数に無く、スコープにあれば、アドレスに置き換える
					if (address.isValid())
					{
						//TODO: 制約式の場合は、ここではじく必要がある
						return BinaryExpr(LRValue(address), rhs, node.op);
					}
					//スコープにも無い場合は新たなローカル変数の宣言なので、ローカル変数に追加しておく
					else
					{
						addLocalVariable(identifier);
						return BinaryExpr(node.lhs, rhs, node.op);
					}
				}
			}
			else if (auto valOpt = AsOpt<Accessor>(node.lhs))
			{
				//アクセッサの場合は少なくとも変数宣言ではない
				//ローカル変数 or スコープ
				/*～～～～～～～～～～～～～～～～～～～～～～～～～～～～～
				Accessorのheadだけ評価してアドレス値に変換したい
					headさえ分かればあとはそこから辿れるので
					今の実装ではheadは式になっているが、これだと良くない
					今は左辺にはそんなに複雑な式は許可していないので、これも識別子くらいの単純な形に制限してよいのではないか
				～～～～～～～～～～～～～～～～～～～～～～～～～～～～～*/

				//評価することにした
				const Expr lhs = boost::apply_visitor(*this, node.lhs);

				return BinaryExpr(lhs, rhs, node.op);
			}

			CGL_Error("二項演算子\"=\"の左辺は単一の左辺値でなければなりません");
			return LRValue(0);
		}

		Expr operator()(const Range& node)
		{
			return Range(
				boost::apply_visitor(*this, node.lhs),
				boost::apply_visitor(*this, node.rhs));
		}

		Expr operator()(const Lines& node)
		{
			Lines result;
			for (size_t i = 0; i < node.size(); ++i)
			{
				result.add(boost::apply_visitor(*this, node.exprs[i]));
			}
			return result;
		}

		Expr operator()(const DefFunc& node)
		{
			//ClosureMaker child(*this);
			ClosureMaker child(pEnv, localVariables, false);
			for (const auto& argument : node.arguments)
			{
				child.addLocalVariable(argument);
			}

			return DefFunc(node.arguments, boost::apply_visitor(child, node.expr));
		}

		Expr operator()(const If& node)
		{
			If result(boost::apply_visitor(*this, node.cond_expr));
			result.then_expr = boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
			}

			return result;
		}

		Expr operator()(const For& node)
		{
			For result;
			result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
			result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

			//ClosureMaker child(*this);
			ClosureMaker child(pEnv, localVariables, false);
			child.addLocalVariable(node.loopCounter);
			result.doExpr = boost::apply_visitor(child, node.doExpr);
			result.loopCounter = node.loopCounter;

			return result;
		}

		Expr operator()(const Return& node)
		{
			return Return(boost::apply_visitor(*this, node.expr));
			//これだとダメかもしれない？
			//return a = 6, a + 2
		}

		Expr operator()(const ListConstractor& node)
		{
			ListConstractor result;
			//ClosureMaker child(*this);
			ClosureMaker child(pEnv, localVariables, false);
			for (const auto& expr : node.data)
			{
				result.data.push_back(boost::apply_visitor(child, expr));
			}
			return result;
		}

		Expr operator()(const KeyExpr& node)
		{
			//変数宣言式
			//再代入の可能性もあるがどっちにしろこれ以降この識別子はローカル変数と扱ってよい
			addLocalVariable(node.name);

			KeyExpr result(node.name);
			result.expr = boost::apply_visitor(*this, node.expr);
			return result;
		}

		/*
		Expr operator()(const RecordConstractor& node)
		{
			RecordConstractor result;
			ClosureMaker child(*this);
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(child, expr));
			}
			return result;
		}
		*/

		Expr operator()(const RecordConstractor& node)
		{
			RecordConstractor result;
			
			if (isInnerRecord)
			{
				isInnerRecord = false;
				for (const auto& expr : node.exprs)
				{
					result.exprs.push_back(boost::apply_visitor(*this, expr));
				}
				isInnerRecord = true;
			}
			else
			{
				ClosureMaker child(pEnv, localVariables, false);
				for (const auto& expr : node.exprs)
				{
					result.exprs.push_back(boost::apply_visitor(child, expr));
				}
			}			
			
			return result;
		}

		//レコード継承構文については特殊で、adderを評価する時のスコープはheadと同じである必要がある。
		//つまり、headを評価する時にはその中身を、一段階だけ（波括弧一つ分だけ）展開するようにして評価しなければならない。
		Expr operator()(const RecordInheritor& node)
		{
			RecordInheritor result;

			//新たに追加
			ClosureMaker child(pEnv, localVariables, true);

			//result.original = boost::apply_visitor(*this, node.original);
			result.original = boost::apply_visitor(child, node.original);
			
			Expr originalAdder = node.adder;
			//Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
			Expr replacedAdder = boost::apply_visitor(child, originalAdder);
			if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
			{
				result.adder = opt.value();
				return result;
			}

			CGL_Error("node.adderの置き換え結果がRecordConstractorでない");
			return LRValue(0);
		}

		Expr operator()(const Accessor& node)
		{
			Accessor result;
			
			result.head = boost::apply_visitor(*this, node.head);
			//DeclSatの評価後ではsat式中のアクセッサ（の内sat式のローカル変数でないもの）のheadはアドレス値に評価されている必要がある。
			//しかし、ここでは*thisを使っているので、任意の式がアドレス値に評価されるわけではない。
			//例えば、次の式 ([f1,f2] @ [f3])[0](x) だとhead部はリストの結合式であり、Evalを通さないとアドレス値にできない。
			//しかし、ここでEvalは使いたくない（ClosureMakerが副作用を起こすのは良くない）ため、現時点ではアクセッサのhead部は単一の識別子のみで構成されるものと仮定している。
			//こうすることにより、識別子がローカル変数ならそのまま残り、外部の変数ならアドレス値に変換されることが保証できる。

			for (const auto& access : node.accesses)
			{
				if (auto listAccess = AsOpt<ListAccess>(access))
				{
					result.add(ListAccess(boost::apply_visitor(*this, listAccess.value().index)));
				}
				else if (auto recordAccess = AsOpt<RecordAccess>(access))
				{
					result.add(recordAccess.value());
				}
				else if (auto functionAccess = AsOpt<FunctionAccess>(access))
				{
					FunctionAccess access;
					for (const auto& argument : functionAccess.value().actualArguments)
					{
						access.add(boost::apply_visitor(*this, argument));
					}
					result.add(access);
				}
				else
				{
					CGL_Error("aaa");
				}
			}

			return result;
		}
		
		Expr operator()(const DeclSat& node)
		{
			DeclSat result;
			result.expr = boost::apply_visitor(*this, node.expr);
			return result;
		}

		Expr operator()(const DeclFree& node)
		{
			DeclFree result;
			for (const auto& accessor : node.accessors)
			{
				const Expr expr = accessor;
				const Expr closedAccessor = boost::apply_visitor(*this, expr);
				if (!IsType<Accessor>(closedAccessor))
				{
					CGL_Error("aaa");
				}
				result.accessors.push_back(As<Accessor>(closedAccessor));
			}
			return result;
		}
	};

	class ConstraintProblem : public cppoptlib::Problem<double>
	{
	public:
		using typename cppoptlib::Problem<double>::TVector;

		std::function<double(const TVector&)> evaluator;
		Record originalRecord;
		std::vector<Identifier> keyList;
		std::shared_ptr<Environment> pEnv;

		bool callback(const cppoptlib::Criteria<cppoptlib::Problem<double>::Scalar> &state, const TVector &x)override
		{
			Record tempRecord = originalRecord;
			
			for (size_t i = 0; i < x.size(); ++i)
			{
				Address address = originalRecord.freeVariableRefs[i];
				pEnv->assignToObject(address, x[i]);
			}

			for (const auto& key : keyList)
			{
				Address address = pEnv->findAddress(key);
				tempRecord.append(key, address);
			}

			ProgressStore::TryWrite(pEnv, tempRecord);

			return true;
		}

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

		LRValue operator()(const SatReference& node)
		{
			CGL_Error("不正な式");
			return LRValue(0);
		}

		LRValue operator()(const UnaryExpr& node)
		{
			const Evaluated lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs));

			switch (node.op)
			{
			case UnaryOp::Not:   return RValue(Not(lhs, *pEnv));
			case UnaryOp::Minus: return RValue(Minus(lhs, *pEnv));
			case UnaryOp::Plus:  return RValue(Plus(lhs, *pEnv));
			}

			CGL_Error("Invalid UnaryExpr");
			return RValue(0);
		}

		LRValue operator()(const BinaryExpr& node)
		{
			//const Evaluated lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs));
			const LRValue lhs_ = boost::apply_visitor(*this, node.lhs);
			//const Evaluated lhs = pEnv->expand(lhs_);
			Evaluated lhs;
			if (node.op != BinaryOp::Assign)
			{
				lhs = pEnv->expand(lhs_);
			}
			
			const Evaluated rhs = pEnv->expand(boost::apply_visitor(*this, node.rhs));

			switch (node.op)
			{
			case BinaryOp::And: return RValue(And(lhs, rhs, *pEnv));
			case BinaryOp::Or:  return RValue(Or(lhs, rhs, *pEnv));

			case BinaryOp::Equal:        return RValue(Equal(lhs, rhs, *pEnv));
			case BinaryOp::NotEqual:     return RValue(NotEqual(lhs, rhs, *pEnv));
			case BinaryOp::LessThan:     return RValue(LessThan(lhs, rhs, *pEnv));
			case BinaryOp::LessEqual:    return RValue(LessEqual(lhs, rhs, *pEnv));
			case BinaryOp::GreaterThan:  return RValue(GreaterThan(lhs, rhs, *pEnv));
			case BinaryOp::GreaterEqual: return RValue(GreaterEqual(lhs, rhs, *pEnv));

			case BinaryOp::Add: return RValue(Add(lhs, rhs, *pEnv));
			case BinaryOp::Sub: return RValue(Sub(lhs, rhs, *pEnv));
			case BinaryOp::Mul: return RValue(Mul(lhs, rhs, *pEnv));
			case BinaryOp::Div: return RValue(Div(lhs, rhs, *pEnv));

			case BinaryOp::Pow:    return RValue(Pow(lhs, rhs, *pEnv));
			case BinaryOp::Concat: return RValue(Concat(lhs, rhs, *pEnv));
			case BinaryOp::Assign:
			{
				//return Assign(lhs, rhs, *pEnv);

				//Assignの場合、lhs は Address or Identifier or Accessor に限られる
				//つまり現時点では、(if cond then x else y) = true のような式を許可していない
				//ここで左辺に直接アドレスが入っていることは有り得る？
				//a = b = 10　のような式でも、右結合であり左側は常に識別子が残っているはずなので、あり得ないと思う
				if (auto valOpt = AsOpt<LRValue>(node.lhs))
				{
					const LRValue& val = valOpt.value();
					if (val.isLValue())
					{
						if (val.address().isValid())
						{
							pEnv->assignToObject(val.address(), rhs);
						}
						else
						{
							CGL_Error("reference error");
						}
					}
					else
					{
						CGL_Error("");
					}

					/*const RValue& val = valOpt.value();
					if (IsType<Address>(val.value))
					{
						if (Address address = As<Address>(val.value))
						{
							pEnv->assignToObject(address, rhs);
						}
						else
						{
							CGL_Error("Invalid address");
							return 0;
						}
					}
					else
					{
						CGL_Error("");
						return 0;
					}*/
				}
				else if (auto valOpt = AsOpt<Identifier>(node.lhs))
				{
					const Identifier& identifier = valOpt.value();

					const Address address = pEnv->findAddress(identifier);
					//変数が存在する：代入式
					if (address.isValid())
					{
						CGL_DebugLog("代入式");
						pEnv->assignToObject(address, rhs);
					}
					//変数が存在しない：変数宣言式
					else
					{
						CGL_DebugLog("変数宣言式");
						pEnv->bindNewValue(identifier, rhs);
						CGL_DebugLog("");
					}

					return RValue(rhs);
				}
				else if (auto valOpt = AsOpt<Accessor>(node.lhs))
				{
					if (lhs_.isLValue())
					{
						Address address = lhs_.address();
						if (address.isValid())
						{
							pEnv->assignToObject(address, rhs);
							return RValue(rhs);
						}
						else
						{
							CGL_Error("参照エラー");
						}
					}
					else
					{
						CGL_Error("アクセッサの評価結果がアドレスでない");
					}
				}
			}
			}

			CGL_Error("Invalid BinaryExpr");
			return RValue(0);
		}

		LRValue operator()(const DefFunc& defFunc)
		{
			return pEnv->makeFuncVal(pEnv, defFunc.arguments, defFunc.expr);
		}

		//LRValue operator()(const FunctionCaller& callFunc)
		LRValue callFunction(const FuncVal& funcVal, const std::vector<Address>& expandedArguments)
		{
			std::cout << "callFunction:" << std::endl;

			CGL_DebugLog("Function Environment:");
			pEnv->printEnvironment();

			/*
			まだ参照をスコープ間で共有できるようにしていないため、引数に与えられたオブジェクトは全て展開して渡す。
			そして、引数の評価時点ではまだ関数の中に入っていないので、スコープを変える前に展開を行う。
			*/

			/*
			12/14
			全ての値はIDで管理するようにする。
			そしてスコープが変わると、変数のマッピングは変わるが、値は共通なのでどちらからも参照できる。
			*/
			/*
			std::vector<Address> expandedArguments(callFunc.actualArguments.size());
			for (size_t i = 0; i < expandedArguments.size(); ++i)
			{
				expandedArguments[i] = pEnv->makeTemporaryValue(callFunc.actualArguments[i]);
			}

			CGL_DebugLog("");

			FuncVal funcVal;

			if (auto opt = AsOpt<FuncVal>(callFunc.funcRef))
			{
				funcVal = opt.value();
			}
			else
			{
				const Address funcAddress = pEnv->findAddress(As<Identifier>(callFunc.funcRef));
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
			}
			*/

			/*if (funcVal.builtinFuncAddress)
			{
				return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments));
			}*/
			if (funcVal.builtinFuncAddress)
			{
				return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.value(), expandedArguments));
			}

			CGL_DebugLog("");

			//if (funcVal.arguments.size() != callFunc.actualArguments.size())
			if (funcVal.arguments.size() != expandedArguments.size())
			{
				CGL_Error("仮引数の数と実引数の数が合っていない");
			}

			//関数の評価
			//(1)ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える

			pEnv->switchFrontScope();
			//今の意味論ではスコープへの参照は全てアドレス値に変換している
			//ここで、関数内からグローバル変数の書き換えを行おうとすると、アドレスに紐つけられた値を直接変更することになる
			//TODO: これは、意味論的に正しいのか一度考える必要がある
			//とりあえず関数がスコープに依存することはなくなったので、単純に別のスコープに切り替えるだけで良い

			CGL_DebugLog("");

			//(2)関数の引数用にスコープを一つ追加する
			pEnv->enterScope();

			CGL_DebugLog("");

			for (size_t i = 0; i < funcVal.arguments.size(); ++i)
			{
				/*
				12/14
				引数はスコープをまたぐ時に参照先が変わらないように全てIDで渡すことにする。
				*/
				pEnv->bindValueID(funcVal.arguments[i], expandedArguments[i]);
			}

			CGL_DebugLog("Function Definition:");
			printExpr(funcVal.expr);

			//(3)関数の戻り値を元のスコープに戻す時も、引数と同じ理由で全て展開して渡す。
			//Evaluated result = pEnv->expandObject(boost::apply_visitor(*this, funcVal.expr));
			Evaluated result;
			{
				//const Evaluated resultValue = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
				//result = pEnv->expandRef(pEnv->makeTemporaryValue(resultValue));
				/*
				EliminateScopeDependency elim(pEnv);
				result = pEnv->makeTemporaryValue(boost::apply_visitor(elim, resultValue));
				*/
				result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr));
				CGL_DebugLog("Function Evaluated:");
				printEvaluated(result, nullptr);
			}
			//Evaluated result = pEnv->expandObject();

			CGL_DebugLog("");

			//(4)関数を抜ける時に、仮引数は全て解放される
			pEnv->exitScope();

			CGL_DebugLog("");

			//(5)最後にローカル変数の環境を関数の実行前のものに戻す。
			pEnv->switchBackScope();

			CGL_DebugLog("");
			
			//評価結果がreturn式だった場合はreturnを外して中身を返す
			//return以外のジャンプ命令は関数では効果を持たないのでそのまま上に返す
			if (IsType<Jump>(result))
			{
				auto& jump = As<Jump>(result);
				if (jump.isReturn())
				{
					if (jump.lhs)
					{
						return RValue(jump.lhs.value());
					}
					else
					{
						CGL_Error("return式の中身が入って無い");
					}
				}
			}

			return RValue(result);
		}

		LRValue operator()(const Range& range)
		{
			return RValue(0);
		}

		LRValue operator()(const Lines& statement)
		{
			pEnv->enterScope();

			Evaluated result;
			int i = 0;
			for (const auto& expr : statement.exprs)
			{
				//std::cout << "Evaluate expression(" << i << ")" << std::endl;
				CGL_DebugLog("Evaluate expression(" + std::to_string(i) + ")");
				pEnv->printEnvironment();

				//std::cout << "LINES_A Expr:" << std::endl;
				//printExpr(expr);
				CGL_DebugLog("");
				result = pEnv->expand(boost::apply_visitor(*this, expr));
				printEvaluated(result, pEnv);

				//std::cout << "LINES_B" << std::endl;
				CGL_DebugLog("");

				//TODO: 後で考える
				//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
				/*
				if (IsLValue(result))
				{
					//const Evaluated& resultValue = pEnv->dereference(result);
					const Evaluated resultValue = pEnv->expandRef(result);
					const bool isMacro = IsType<Jump>(resultValue);
					if (isMacro)
					{
						result = As<Jump>(resultValue);
					}
				}
				*/

				//std::cout << "LINES_C" << std::endl;
				CGL_DebugLog("");
				//途中でジャンプ命令を読んだら即座に評価を終了する
				if (IsType<Jump>(result))
				{
					//std::cout << "LINES_D" << std::endl;
					CGL_DebugLog("");
					break;
				}

				//std::cout << "LINES_D" << std::endl;
				CGL_DebugLog("");

				++i;
			}

			//std::cout << "LINES_E" << std::endl;
			CGL_DebugLog("");

			//この後すぐ解放されるので dereference しておく
			bool deref = true;
			/*
			if (auto refOpt = AsOpt<ObjectReference>(result))
			{
				if (IsType<unsigned>(refOpt.value().headValue))
				{
					deref = false;
				}
			}

			std::cout << "LINES_F" << std::endl;

			if (deref)
			{
				result = pEnv->dereference(result);
			}
			*/

			/*
			if (auto refOpt = AsOpt<Address>(result))
			{
				//result = pEnv->dereference(refOpt.value());
				//result = pEnv->expandRef(refOpt.value());
				result = pEnv->expand(refOpt.value());
			}
			*/

			//std::cout << "LINES_G" << std::endl;
			CGL_DebugLog("");

			pEnv->exitScope();

			//std::cout << "LINES_H" << std::endl;
			CGL_DebugLog("");
			return LRValue(result);
		}

		LRValue operator()(const If& if_statement)
		{
			const Evaluated cond = pEnv->expand(boost::apply_visitor(*this, if_statement.cond_expr));
			if (!IsType<bool>(cond))
			{
				//条件は必ずブール値である必要がある
				//std::cerr << "Error(" << __LINE__ << ")\n";
				CGL_Error("条件は必ずブール値である必要がある");
			}

			if (As<bool>(cond))
			{
				const Evaluated result = pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr));
				return RValue(result);

			}
			else if (if_statement.else_expr)
			{
				const Evaluated result = pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.value()));
				return RValue(result);
			}

			//else式が無いケースで cond = False であったら一応警告を出す
			//std::cerr << "Warning(" << __LINE__ << ")\n";
			CGL_WarnLog("else式が無いケースで cond = False であった");
			return RValue(0);
		}

		LRValue operator()(const For& forExpression)
		{
			std::cout << "For:" << std::endl;

			const Evaluated startVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeStart));
			const Evaluated endVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeEnd));

			//startVal <= endVal なら 1
			//startVal > endVal なら -1
			//を適切な型に変換して返す
			const auto calcStepValue = [&](const Evaluated& a, const Evaluated& b)->boost::optional<std::pair<Evaluated, bool>>
			{
				const bool a_IsInt = IsType<int>(a);
				const bool a_IsDouble = IsType<double>(a);

				const bool b_IsInt = IsType<int>(b);
				const bool b_IsDouble = IsType<double>(b);

				if (!((a_IsInt || a_IsDouble) && (b_IsInt || b_IsDouble)))
				{
					//エラー：ループのレンジが不正な型（整数か実数に評価できる必要がある）
					//std::cerr << "Error(" << __LINE__ << ")\n";
					//return boost::none;
					CGL_Error("ループのレンジが不正な型（整数か実数に評価できる必要がある）");
				}

				const bool result_IsDouble = a_IsDouble || b_IsDouble;
				//const auto lessEq = LessEqual(a, b, *pEnv);
				//if (!IsType<bool>(lessEq))
				//{
				//	//エラー：aとbの比較に失敗した
				//	//一応確かめているだけでここを通ることはないはず
				//	//LessEqualの実装ミス？
				//	//std::cerr << "Error(" << __LINE__ << ")\n";
				//	//return boost::none;
				//	CGL_Error("LessEqualの実装ミス？");
				//}

				//const bool isInOrder = As<bool>(lessEq);
				const bool isInOrder = LessEqual(a, b, *pEnv);

				const int sign = isInOrder ? 1 : -1;

				if (result_IsDouble)
				{
					/*const Evaluated xx = Mul(1.0, sign, *pEnv);
					return std::pair<Evaluated, bool>(xx, isInOrder);*/

					//std::pair<Evaluated, bool> xa(Mul(1.0, sign, *pEnv), isInOrder);

					/*const Evaluated xx = Mul(1.0, sign, *pEnv);
					std::pair<Evaluated, bool> xa(xx, isInOrder);
					return boost::optional<std::pair<Evaluated, bool>>(xa);*/

					return std::pair<Evaluated, bool>(Mul(1.0, sign, *pEnv), isInOrder);
				}

				return std::pair<Evaluated, bool>(Mul(1, sign, *pEnv), isInOrder);
			};

			const auto loopContinues = [&](const Evaluated& loopCount, bool isInOrder)->boost::optional<bool>
			{
				//isInOrder == true
				//loopCountValue <= endVal

				//isInOrder == false
				//loopCountValue > endVal

				const Evaluated result = LessEqual(loopCount, endVal, *pEnv);
				if (!IsType<bool>(result))
				{
					CGL_Error("ここを通ることはないはず");
				}

				return As<bool>(result) == isInOrder;
			};

			const auto stepOrder = calcStepValue(startVal, endVal);
			if (!stepOrder)
			{
				CGL_Error("ループのレンジが不正");
			}

			const Evaluated step = stepOrder.value().first;
			const bool isInOrder = stepOrder.value().second;

			Evaluated loopCountValue = startVal;

			Evaluated loopResult;

			pEnv->enterScope();

			while (true)
			{
				const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
				if (!isLoopContinuesOpt)
				{
					CGL_Error("ここを通ることはないはず");
				}

				//ループの継続条件を満たさなかったので抜ける
				if (!isLoopContinuesOpt.value())
				{
					break;
				}

				pEnv->bindNewValue(forExpression.loopCounter, loopCountValue);

				loopResult = pEnv->expand(boost::apply_visitor(*this, forExpression.doExpr));

				//ループカウンタの更新
				loopCountValue = Add(loopCountValue, step, *pEnv);
			}

			pEnv->exitScope();

			return RValue(loopResult);
		}

		LRValue operator()(const Return& return_statement)
		{
			const Evaluated lhs = pEnv->expand(boost::apply_visitor(*this, return_statement.expr));

			return RValue(Jump::MakeReturn(lhs));
		}

		LRValue operator()(const ListConstractor& listConstractor)
		{
			List list;
			for (const auto& expr : listConstractor.data)
			{
				/*const Evaluated value = pEnv->expand(boost::apply_visitor(*this, expr));
				CGL_DebugLog("");
				printEvaluated(value, nullptr);

				list.append(pEnv->makeTemporaryValue(value));*/
				LRValue lrvalue = boost::apply_visitor(*this, expr);
				if (lrvalue.isLValue())
				{
					list.append(lrvalue.address());
				}
				else
				{
					list.append(pEnv->makeTemporaryValue(lrvalue.evaluated()));
				}
			}

			return RValue(list);
		}

		LRValue operator()(const KeyExpr& keyExpr)
		{
			const Evaluated value = pEnv->expand(boost::apply_visitor(*this, keyExpr.expr));

			return RValue(KeyValue(keyExpr.name, value));
		}

		LRValue operator()(const RecordConstractor& recordConsractor)
		{
			std::cout << "RecordConstractor:" << std::endl;

			CGL_DebugLog("");
			pEnv->enterScope();
			CGL_DebugLog("");

			std::vector<Identifier> keyList;
			/*
			レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
			レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアスとして定義（スコープを抜けたら元に戻る≒遮蔽）
			*/

			for (size_t i = 0; i < recordConsractor.exprs.size(); ++i)
			{
				CGL_DebugLog(std::string("RecordExpr(") + ToS(i) + "): ");
				printExpr(recordConsractor.exprs[i]);
			}

			Record newRecord;

			if (temporaryRecord)
			{
				currentRecords.push(temporaryRecord.value());
				temporaryRecord = boost::none;
			}
			else
			{
				currentRecords.push(std::ref(newRecord));
			}

			Record& record = currentRecords.top();
			
			int i = 0;

			for (const auto& expr : recordConsractor.exprs)
			{
				CGL_DebugLog("");
				Evaluated value = pEnv->expand(boost::apply_visitor(*this, expr));
				CGL_DebugLog("Evaluate: ");
				printExpr(expr);
				CGL_DebugLog("Result: ");
				printEvaluated(value, pEnv);

				//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
				if (auto keyValOpt = AsOpt<KeyValue>(value))
				{
					const auto keyVal = keyValOpt.value();
					keyList.push_back(keyVal.name);

					CGL_DebugLog(std::string("assign to ") + static_cast<std::string>(keyVal.name));

					//Assign(keyVal.name, keyVal.value, *pEnv);

					//識別子はEvaluatedからはずしたので、識別子に対して直接代入を行うことはできなくなった
					//Assign(ObjectReference(keyVal.name), keyVal.value, *pEnv);

					//したがって、一度代入式を作ってからそれを評価する
					//Expr exprVal = RValue(keyVal.value);
					Expr exprVal = LRValue(keyVal.value);
					Expr expr = BinaryExpr(keyVal.name, exprVal, BinaryOp::Assign);
					boost::apply_visitor(*this, expr);

					CGL_DebugLog("");
					//Assign(ObjectReference(keyVal.name), keyVal.value, *pEnv);
				}
				/*
				else if (auto declSatOpt = AsOpt<DeclSat>(value))
				{
					//record.problem.addConstraint(declSatOpt.value().expr);
					//ここでクロージャを作る必要がある
					ClosureMaker closureMaker(pEnv, {});
					const Expr closedSatExpr = boost::apply_visitor(closureMaker, declSatOpt.value().expr);
					record.problem.addConstraint(closedSatExpr);
				}
				*/
				/*
				else if (auto declFreeOpt = AsOpt<DeclFree>(value))
				{
					for (const auto& accessor : declFreeOpt.value().accessors)
					{
						ClosureMaker closureMaker(pEnv, {});
						const Expr freeExpr = accessor;
						const Expr closedFreeExpr = boost::apply_visitor(closureMaker, freeExpr);
						if (!IsType<Accessor>(closedFreeExpr))
						{
							CGL_Error("ここは通らないはず");
						}

						record.freeVariables.push_back(As<Accessor>(closedFreeExpr));
					}
				}
				*/

				//valueは今は右辺値のみになっている
				//TODO: もう一度考察する
				/*
				//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
				if (IsLValue(value))
				{
					const Evaluated resultValue = pEnv->expandRef(value);
					const bool isMacro = IsType<Jump>(resultValue);
					if (isMacro)
					{
						value = As<Jump>(resultValue);
					}
				}

				//途中でジャンプ命令を読んだら即座に評価を終了する
				if (IsType<Jump>(value))
				{
					break;
				}
				*/

				++i;
			}

			pEnv->printEnvironment();
			CGL_DebugLog("");

			/*for (const auto& satExpr : innerSatClosures)
			{
				record.problem.addConstraint(satExpr);
			}
			innerSatClosures.clear();*/

			CGL_DebugLog("");

			std::vector<double> resultxs;
			if (record.problem.candidateExpr)
			{
				auto& problem = record.problem;
				auto& freeVariableRefs = record.freeVariableRefs;

				const auto& problemRefs = problem.refs;
				const auto& freeVariables = record.freeVariables;

				{
					//record.freeVariablesをもとにrecord.freeVariableRefsを構築
					//全てのアクセッサを展開し、各変数の参照リストを作成する
					freeVariableRefs.clear();
					for (const auto& accessor : record.freeVariables)
					{
						const Address refAddress = pEnv->evalReference(accessor);
						//const Address refAddress = accessor;
						//単一の値 or List or Record
						if (refAddress.isValid())
						{
							const auto addresses = pEnv->expandReferences(refAddress);
							freeVariableRefs.insert(freeVariableRefs.end(), addresses.begin(), addresses.end());
						}
						else
						{
							CGL_Error("accessor refers null address");
						}
					}

					CGL_DebugLog(std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size()));

					//一度sat式の中身を展開し、
					//Accessorを展開するvisitor（Expr -> Expr）を作り、実行する
					//その後Sat2Exprに掛ける
					//Expr2SatExprでは単項演算子・二項演算子の評価の際、中身をみて定数であったら畳み込む処理を行う

					//satの関数呼び出しを全て式に展開する
					//{
					//	//problem.candidateExpr = pEnv->expandFunction(problem.candidateExpr.value());
					//	Expr ee = pEnv->expandFunction(problem.candidateExpr.value());
					//	problem.candidateExpr = ee;
					//}

					//展開された式をSatExprに変換する
					//＋satの式に含まれる変数の内、freeVariableRefsに入っていないものは即時に評価して畳み込む
					//freeVariableRefsに入っているものは最適化対象の変数としてdataに追加していく
					//＋freeVariableRefsの変数についてはsat式に出現したかどうかを記録し削除する
					problem.constructConstraint(pEnv, record.freeVariableRefs);

					//設定されたdataが参照している値を見て初期値を設定する
					if (!problem.initializeData(pEnv))
					{
						return LRValue(0);
					}

					CGL_DebugLog(std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size()));
					CGL_DebugLog(std::string("Record SatExpr: "));
					std::cout << (std::string("Record FreeVariablesSize: ") + std::to_string(record.freeVariableRefs.size())) << std::endl;
				}

				//DeclFreeに出現する参照について、そのインデックス -> Problemのデータのインデックスを取得するマップ
				std::unordered_map<int, int> variable2Data;
				for (size_t freeIndex = 0; freeIndex < record.freeVariableRefs.size(); ++freeIndex)
				{
					CGL_DebugLog(ToS(freeIndex));
					CGL_DebugLog(std::string("Address(") + record.freeVariableRefs[freeIndex].toString() + ")");
					const auto& ref1 = record.freeVariableRefs[freeIndex];

					bool found = false;
					for (size_t dataIndex = 0; dataIndex < problemRefs.size(); ++dataIndex)
					{
						CGL_DebugLog(ToS(dataIndex));
						CGL_DebugLog(std::string("Address(") + problemRefs[dataIndex].toString() + ")");

						const auto& ref2 = problemRefs[dataIndex];

						if (ref1 == ref2)
						{
							//std::cout << "    " << freeIndex << " -> " << dataIndex << std::endl;

							found = true;
							variable2Data[freeIndex] = dataIndex;
							break;
						}
					}

					//DeclFreeにあってDeclSatに無い変数は意味がない。
					//単に無視しても良いが、恐らく入力のミスと思われるので警告を出す
					if (!found)
					{
						//std::cerr << "Error(" << __LINE__ << "):freeに指定された変数が無効です。\n";
						//return 0;
						CGL_WarnLog("freeに指定された変数が無効です");
					}
				}
				CGL_DebugLog("End Record MakeMap");

				/*
				ConstraintProblem constraintProblem;
				constraintProblem.evaluator = [&](const ConstraintProblem::TVector& v)->double
				{
					//-1000 -> 1000
					for (int i = 0; i < v.size(); ++i)
					{
						problem.update(variable2Data[i], v[i]);
						//problem.update(variable2Data[i], (v[i] - 0.5)*2000.0);
					}

					{
						for (const auto& keyval : problem.invRefs)
						{
							pEnv->assignToObject(keyval.first, problem.data[keyval.second]);
						}
					}

					pEnv->switchFrontScope();
					double result = problem.eval(pEnv);
					pEnv->switchBackScope();

					CGL_DebugLog(std::string("cost: ") + ToS(result, 17));
					std::cout << std::string("cost: ") << ToS(result, 17) << "\n";
					return result;
				};
				constraintProblem.originalRecord = record;
				constraintProblem.keyList = keyList;
				constraintProblem.pEnv = pEnv;

				Eigen::VectorXd x0s(record.freeVariableRefs.size());
				for (int i = 0; i < x0s.size(); ++i)
				{
					x0s[i] = problem.data[variable2Data[i]];
					//x0s[i] = (problem.data[variable2Data[i]] / 2000.0) + 0.5;
					CGL_DebugLog(ToS(i) + " : " + ToS(x0s[i]));
				}

				cppoptlib::BfgsSolver<ConstraintProblem> solver;
				solver.minimize(constraintProblem, x0s);

				resultxs.resize(x0s.size());
				for (int i = 0; i < x0s.size(); ++i)
				{
					resultxs[i] = x0s[i];
				}
				//*/

				//*
				libcmaes::FitFunc func = [&](const double *x, const int N)->double
				{
					for (int i = 0; i < N; ++i)
					{
						problem.update(variable2Data[i], x[i]);
					}

					{
						for (const auto& keyval : problem.invRefs)
						{
							pEnv->assignToObject(keyval.first, problem.data[keyval.second]);
						}
					}

					pEnv->switchFrontScope();
					double result = problem.eval(pEnv);
					pEnv->switchBackScope();

					CGL_DebugLog(std::string("cost: ") + ToS(result, 17));
					
					return result;
				};

				CGL_DebugLog("");

				std::vector<double> x0(record.freeVariableRefs.size());
				for (int i = 0; i < x0.size(); ++i)
				{
					x0[i] = problem.data[variable2Data[i]];
					CGL_DebugLog(ToS(i) + " : " + ToS(x0[i]));
				}

				CGL_DebugLog("");

				const double sigma = 0.1;

				const int lambda = 100;

				libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda);
				CGL_DebugLog("");
				libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
				CGL_DebugLog("");
				resultxs = cmasols.best_candidate().get_x();
				CGL_DebugLog("");
				//*/
			}

			CGL_DebugLog("");
			for (size_t i = 0; i < resultxs.size(); ++i)
			{
				Address address = record.freeVariableRefs[i];
				pEnv->assignToObject(address, resultxs[i]);
				//pEnv->assignToObject(address, (resultxs[i] - 0.5)*2000.0);
			}

			for (const auto& key : keyList)
			{
				//record.append(key.name, pEnv->dereference(key));

				//record.append(key.name, pEnv->makeTemporaryValue(pEnv->dereference(ObjectReference(key))));

				/*Address address = pEnv->findAddress(key);
				boost::optional<const Evaluated&> opt = pEnv->expandOpt(address);
				record.append(key, pEnv->makeTemporaryValue(opt.value()));*/

				Address address = pEnv->findAddress(key);
				record.append(key, address);
			}

			pEnv->printEnvironment();

			CGL_DebugLog("");

			currentRecords.pop();

			//pEnv->pop();
			pEnv->exitScope();

			CGL_DebugLog("--------------------------- Print Environment ---------------------------");
			pEnv->printEnvironment();
			CGL_DebugLog("-------------------------------------------------------------------------");

			return RValue(record);
		}

		LRValue operator()(const RecordInheritor& record)
		{
			boost::optional<Record> recordOpt;

			/*
			if (auto opt = AsOpt<Identifier>(record.original))
			{
				//auto originalOpt = pEnv->find(opt.value().name);
				boost::optional<const Evaluated&> originalOpt = pEnv->dereference(pEnv->findAddress(opt.value()));
				if (!originalOpt)
				{
					//エラー：未定義のレコードを参照しようとした
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}

				recordOpt = AsOpt<Record>(originalOpt.value());
				if (!recordOpt)
				{
					//エラー：識別子の指すオブジェクトがレコード型ではない
					std::cerr << "Error(" << __LINE__ << ")\n";
					return 0;
				}
			}
			else if (auto opt = AsOpt<Record>(record.original))
			{
				recordOpt = opt.value();
			}
			*/

			//Evaluated originalRecordRef = boost::apply_visitor(*this, record.original);
			//Evaluated originalRecordVal = pEnv->expandRef(originalRecordRef);
			Evaluated originalRecordVal = pEnv->expand(boost::apply_visitor(*this, record.original));
			if (auto opt = AsOpt<Record>(originalRecordVal))
			{
				recordOpt = opt.value();
			}
			else
			{
				CGL_Error("not record");
			}

			/*
			a{}を評価する手順
			(1) オリジナルのレコードaのクローン(a')を作る
			(2) a'の各キーと値に対する参照をローカルスコープに追加する
			(3) 追加するレコードの中身を評価する
			(4) ローカルスコープの参照値を読みレコードに上書きする //リストアクセスなどの変更処理
			(5) レコードをマージする //ローカル変数などの変更処理
			*/

			//(1)
			//Record clone = recordOpt.value();
			
			pEnv->printEnvironment(true);
			CGL_DebugLog("Original:");
			printEvaluated(originalRecordVal, pEnv);

			Record clone = As<Record>(Clone(pEnv, recordOpt.value()));

			CGL_DebugLog("Clone:");
			printEvaluated(clone, pEnv);

			if (temporaryRecord)
			{
				CGL_Error("レコード拡張に失敗");
			}
			temporaryRecord = clone;

			pEnv->enterScope();
			for (auto& keyval : clone.values)
			{
				pEnv->makeVariable(keyval.first, keyval.second);

				CGL_DebugLog(std::string("Bind ") + keyval.first + " -> " + "Address(" + keyval.second.toString() + ")");
			}

			Expr expr = record.adder;
			Evaluated recordValue = pEnv->expand(boost::apply_visitor(*this, expr));

			pEnv->exitScope();

			return pEnv->makeTemporaryValue(recordValue);

			/*
			//(2)
			pEnv->enterScope();
			for (auto& keyval : clone.values)
			{
				//pEnv->bindObjectRef(keyval.first, keyval.second);
				pEnv->makeVariable(keyval.first, keyval.second);

				CGL_DebugLog(std::string("Bind ") + keyval.first + " -> " + "Address(" + keyval.second.toString() + ")");
			}

			CGL_DebugLog("");

			//(3)
			Expr expr = record.adder;
			//Evaluated recordValue = boost::apply_visitor(*this, expr);
			Evaluated recordValue = pEnv->expand(boost::apply_visitor(*this, expr));
			if (auto opt = AsOpt<Record>(recordValue))
			{
				CGL_DebugLog("");

				//(4)
				for (auto& keyval : recordOpt.value().values)
				{
					clone.values[keyval.first] = pEnv->findAddress(keyval.first);
				}

				CGL_DebugLog("");

				//(5)
				MargeRecordInplace(clone, opt.value());

				CGL_DebugLog("");

				//TODO:ここで制約処理を行う

				//pEnv->pop();
				pEnv->exitScope();

				//return RValue(clone);
				return pEnv->makeTemporaryValue(clone);

				//MargeRecord(recordOpt.value(), opt.value());
			}

			CGL_DebugLog("");

			//pEnv->pop();
			pEnv->exitScope();
			*/

			//ここは通らないはず。{}で囲まれた式を評価した結果がレコードでなかった。
			//std::cerr << "Error(" << __LINE__ << ")\n";
			CGL_Error("ここは通らないはず");
			return RValue(0);
		}

		LRValue operator()(const DeclSat& node)
		{
			//std::cout << "DeclSat:" << std::endl;

			//ここでクロージャを作る必要がある
			ClosureMaker closureMaker(pEnv, {});
			const Expr closedSatExpr = boost::apply_visitor(closureMaker, node.expr);
			//innerSatClosures.push_back(closedSatExpr);

			pEnv->enterScope();
			//DeclSat自体は現在制約が満たされているかどうかを評価結果として返す
			const Evaluated result = pEnv->expand(boost::apply_visitor(*this, closedSatExpr));
			pEnv->exitScope();

			if (currentRecords.empty())
			{
				CGL_Error("sat宣言はレコードの中にしか書くことができません");
			}

			currentRecords.top().get().problem.addConstraint(closedSatExpr);

			return RValue(result);
			//return RValue(false);
		}

		LRValue operator()(const DeclFree& node)
		{
			//std::cout << "DeclFree:" << std::endl;
			for (const auto& accessor : node.accessors)
			{
				//std::cout << "  accessor:" << std::endl;
				if (currentRecords.empty())
				{
					CGL_Error("var宣言はレコードの中にしか書くことができません");
				}

				ClosureMaker closureMaker(pEnv, {});
				const Expr varExpr = accessor;
				const Expr closedVarExpr = boost::apply_visitor(closureMaker, varExpr);

				if (IsType<Accessor>(closedVarExpr))
				{
					//std::cout << "    Free Expr:" << std::endl;
					//printExpr(closedVarExpr, std::cout);
					currentRecords.top().get().freeVariables.push_back(As<Accessor>(closedVarExpr));
				}
				else if (IsType<Identifier>(closedVarExpr))
				{
					Accessor result(closedVarExpr);
					currentRecords.top().get().freeVariables.push_back(result);
				}
				else
				{
					CGL_Error("var宣言に指定された変数が無効です");
				}
				/*const LRValue result = boost::apply_visitor(*this, expr);
				if (!result.isLValue())
				{
					CGL_Error("var宣言に指定された変数は無効です");
				}*/

				//currentRecords.top().get().freeVariables.push_back(result.address());
			}

			return RValue(0);
		}

		LRValue operator()(const Accessor& accessor)
		{
			/*
			ObjectReference result;

			Evaluated headValue = boost::apply_visitor(*this, accessor.head);
			if (auto opt = AsOpt<Identifier>(headValue))
			{
				result.headValue = opt.value();
			}
			else if (auto opt = AsOpt<Record>(headValue))
			{
				result.headValue = opt.value();
			}
			else if (auto opt = AsOpt<List>(headValue))
			{
				result.headValue = opt.value();
			}
			else if (auto opt = AsOpt<FuncVal>(headValue))
			{
				result.headValue = opt.value();
			}
			else
			{
				//エラー：識別子かリテラル以外（評価結果としてオブジェクトを返すような式）へのアクセスには未対応
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
			*/

			Address address;

			/*
			Evaluated headValue = boost::apply_visitor(*this, accessor.head);
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
				//エラー：識別子かリテラル以外（評価結果としてオブジェクトを返すような式）へのアクセスには未対応
				std::cerr << "Error(" << __LINE__ << ")\n";
				return 0;
			}
			*/

			LRValue headValue = boost::apply_visitor(*this, accessor.head);
			if (headValue.isLValue())
			{
				address = headValue.address();
			}
			else
			{
				Evaluated evaluated = headValue.evaluated();
				if (auto opt = AsOpt<Record>(evaluated))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else if (auto opt = AsOpt<List>(evaluated))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else if (auto opt = AsOpt<FuncVal>(evaluated))
				{
					address = pEnv->makeTemporaryValue(opt.value());
				}
				else
				{
					CGL_Error("アクセッサのヘッドの評価結果が不正");
				}
			}

			for (const auto& access : accessor.accesses)
			{
				boost::optional<const Evaluated&> objOpt = pEnv->expandOpt(address);
				if (!objOpt)
				{
					CGL_Error("参照エラー");
				}

				const Evaluated& objRef = objOpt.value();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					Evaluated value = pEnv->expand(boost::apply_visitor(*this, listAccessOpt.value().index));

					if (!IsType<List>(objRef))
					{
						CGL_Error("オブジェクトがリストでない");
					}

					const List& list = As<const List&>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						address = list.get(indexOpt.value());
					}
					else
					{
						CGL_Error("list[index] の index が int 型でない");
					}
				}
				else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
				{
					if (!IsType<Record>(objRef))
					{
						CGL_Error("オブジェクトがレコードでない");
					}

					const Record& record = As<const Record&>(objRef);
					auto it = record.values.find(recordAccessOpt.value().name);
					if (it == record.values.end())
					{
						CGL_Error("指定された識別子がレコード中に存在しない");
					}

					address = it->second;
				}
				else
				{
					auto funcAccess = As<FunctionAccess>(access);

					if (!IsType<FuncVal>(objRef))
					{
						CGL_Error("オブジェクトが関数でない");
					}

					const FuncVal& function = As<const FuncVal&>(objRef);

					/*
					std::vector<Evaluated> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
					}
					
					Expr caller = FunctionCaller(function, args);
					//const Evaluated returnedValue = boost::apply_visitor(*this, caller);
					const Evaluated returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
					address = pEnv->makeTemporaryValue(returnedValue);
					*/

					std::vector<Address> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const LRValue currentArgument = pEnv->expand(boost::apply_visitor(*this, expr));
						if (currentArgument.isLValue())
						{
							args.push_back(currentArgument.address());
						}
						else
						{
							args.push_back(pEnv->makeTemporaryValue(currentArgument.evaluated()));
						}
					}

					const Evaluated returnedValue = pEnv->expand(callFunction(function, args));
					address = pEnv->makeTemporaryValue(returnedValue);
				}
			}

			return LRValue(address);
		}

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

		bool operator()(const LRValue& node)
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

		bool operator()(const Identifier& node)
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

		bool operator()(const SatReference& node) { return true; }

		bool operator()(const UnaryExpr& node)
		{
			return boost::apply_visitor(*this, node.lhs);
		}

		bool operator()(const BinaryExpr& node)
		{
			const bool lhs = boost::apply_visitor(*this, node.lhs);
			if (lhs)
			{
				return true;
			}

			const bool rhs = boost::apply_visitor(*this, node.rhs);
			return rhs;
		}

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

		bool operator()(const Accessor& node)
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

		boost::optional<size_t> freeVariableIndex(Address reference)
		{
			for (size_t i = 0; i < freeVariables.size(); ++i)
			{
				if (freeVariables[i] == reference)
				{
					return i;
				}
			}

			return boost::none;
		}

		boost::optional<SatReference> getSatRef(Address reference)
		{
			for (size_t i = 0; i < refs.size(); ++i)
			{
				if (refs[i] == reference)
				{
					return SatReference(refID_Offset + i);
					//return refs[i];
				}
			}

			return boost::none;
		}

		boost::optional<SatReference> addSatRef(Address reference)
		{
			if (auto indexOpt = freeVariableIndex(reference))
			{
				//以前に出現して登録済みのfree変数はそのまま返す
				if (auto satRefOpt = getSatRef(reference))
				{
					return satRefOpt;
				}

				//初めて出現したfree変数は登録してから返す
				usedInSat[indexOpt.value()] = 1;
				SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
				refs.push_back(reference);
				return satRef;
			}

			return boost::none;
		}
				
		Expr operator()(const LRValue& node)
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
		Expr operator()(const Identifier& node)
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

		Expr operator()(const SatReference& node) { return node; }

		Expr operator()(const UnaryExpr& node)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);

			switch (node.op)
			{
			case UnaryOp::Not:   return UnaryExpr(lhs, UnaryOp::Not);
			case UnaryOp::Minus: return UnaryExpr(lhs, UnaryOp::Minus);
			case UnaryOp::Plus:  return lhs;
			}

			CGL_Error("invalid expression");
			return LRValue(0);
		}

		Expr operator()(const BinaryExpr& node)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);
			const Expr rhs = boost::apply_visitor(*this, node.rhs);

			if (node.op != BinaryOp::Assign)
			{
				return BinaryExpr(lhs, rhs, node.op);
			}

			CGL_Error("invalid expression");
			return LRValue(0);
		}

		Expr operator()(const DefFunc& node) { CGL_Error("invalid expression"); return LRValue(0); }

		Expr operator()(const Range& node) { CGL_Error("invalid expression"); return LRValue(0); }

		Expr operator()(const Lines& node)
		{
			Lines result;
			for (const auto& expr : node.exprs)
			{
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const If& node)
		{
			If result;
			result.cond_expr = boost::apply_visitor(*this, node.cond_expr);
			result.then_expr = boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				result.else_expr = boost::apply_visitor(*this, node.else_expr.value());
			}

			return result;
		}

		Expr operator()(const For& node)
		{
			For result;
			result.loopCounter = node.loopCounter;
			result.rangeEnd = node.rangeEnd;
			result.rangeStart = node.rangeStart;
			result.doExpr = boost::apply_visitor(*this, node.doExpr);
			return result;
		}

		Expr operator()(const Return& node) { CGL_Error("invalid expression"); return 0; }
		
		Expr operator()(const ListConstractor& node)
		{
			ListConstractor result;
			for (const auto& expr : node.data)
			{
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const KeyExpr& node) { return node; }

		Expr operator()(const RecordConstractor& node)
		{
			RecordConstractor result;
			for (const auto& expr : node.exprs)
			{
				result.add(boost::apply_visitor(*this, expr));
			}
			return result;
		}

		Expr operator()(const RecordInheritor& node) { CGL_Error("invalid expression"); return 0; }
		Expr operator()(const DeclSat& node) { CGL_Error("invalid expression"); return 0; }
		Expr operator()(const DeclFree& node) { CGL_Error("invalid expression"); return 0; }

		Expr operator()(const Accessor& node)
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

				if (lastHeadAddress != headAddress && !selfDependsOnFreeVariables)
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
	};

	inline Evaluated evalExpr(const Expr& expr)
	{
		auto env = Environment::Make();

		Eval evaluator(env);

		const Evaluated result = env->expand(boost::apply_visitor(evaluator, expr));

		std::cout << "Environment:\n";
		env->printEnvironment();

		std::cout << "Result Evaluation:\n";
		printEvaluated(result, env);

		return result;
	}

	inline bool IsEqualEvaluated(const Evaluated& value1, const Evaluated& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
			if ((IsType<int>(value1) || IsType<double>(value1)) && (IsType<int>(value2) || IsType<double>(value2)))
			{
				const Evaluated d1 = IsType<int>(value1) ? static_cast<double>(As<int>(value1)) : As<double>(value1);
				const Evaluated d2 = IsType<int>(value2) ? static_cast<double>(As<int>(value2)) : As<double>(value2);
				return IsEqualEvaluated(d1, d2);
			}

			std::cerr << "Values are not same type." << std::endl;
			return false;
		}

		if (IsType<bool>(value1))
		{
			return As<bool>(value1) == As<bool>(value2);
		}
		else if (IsType<int>(value1))
		{
			return As<int>(value1) == As<int>(value2);
		}
		else if (IsType<double>(value1))
		{
			const double d1 = As<double>(value1);
			const double d2 = As<double>(value2);
			if (d1 == d2)
			{
				return true;
			}
			else
			{
				std::cerr << "Warning: Comparison of floating point numbers might be incorrect." << std::endl;
				const double eps = 0.001;
				const bool result = abs(d1 - d2) < eps;
				std::cerr << std::setprecision(17);
				std::cerr << "    " << d1 << " == " << d2 << " => " << (result ? "Maybe true" : "Maybe false") << std::endl;
				std::cerr << std::setprecision(6);
				return result;
			}
			//return As<double>(value1) == As<double>(value2);
		}
		/*else if (IsType<Address>(value1))
		{
			return As<Address>(value1) == As<Address>(value2);
		}*/
		else if (IsType<List>(value1))
		{
			return As<List>(value1) == As<List>(value2);
		}
		else if (IsType<KeyValue>(value1))
		{
			return As<KeyValue>(value1) == As<KeyValue>(value2);
		}
		else if (IsType<Record>(value1))
		{
			return As<Record>(value1) == As<Record>(value2);
		}
		else if (IsType<FuncVal>(value1))
		{
			return As<FuncVal>(value1) == As<FuncVal>(value2);
		}
		else if (IsType<Jump>(value1))
		{
			return As<Jump>(value1) == As<Jump>(value2);
		};
		/*else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		};*/

		std::cerr << "IsEqualEvaluated: Type Error" << std::endl;
		return false;
	}

	inline bool IsEqual(const Expr& value1, const Expr& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
			std::cerr << "Values are not same type." << std::endl;
			return false;
		}
		/*
		if (IsType<bool>(value1))
		{
			return As<bool>(value1) == As<bool>(value2);
		}
		else if (IsType<int>(value1))
		{
			return As<int>(value1) == As<int>(value2);
		}
		else if (IsType<double>(value1))
		{
			const double d1 = As<double>(value1);
			const double d2 = As<double>(value2);
			if (d1 == d2)
			{
				return true;
			}
			else
			{
				std::cerr << "Warning: Comparison of floating point numbers might be incorrect." << std::endl;
				const double eps = 0.001;
				const bool result = abs(d1 - d2) < eps;
				std::cerr << std::setprecision(17);
				std::cerr << "    " << d1 << " == " << d2 << " => " << (result ? "Maybe true" : "Maybe false") << std::endl;
				std::cerr << std::setprecision(6);
				return result;
			}
			//return As<double>(value1) == As<double>(value2);
		}
		*/
		/*if (IsType<RValue>(value1))
		{
			return IsEqualEvaluated(As<RValue>(value1).value, As<RValue>(value2).value);
		}*/
		if (IsType<LRValue>(value1))
		{
			const LRValue& lrvalue1 = As<LRValue>(value1);
			const LRValue& lrvalue2 = As<LRValue>(value2);
			if (lrvalue1.isLValue() && lrvalue2.isLValue())
			{
				return lrvalue1.address() == lrvalue2.address();
			}
			else if (lrvalue1.isRValue() && lrvalue2.isRValue())
			{
				return IsEqualEvaluated(lrvalue1.evaluated(), lrvalue2.evaluated());
			}
			std::cerr << "Values are not same type." << std::endl;
			return false;
		}
		else if (IsType<Identifier>(value1))
		{
			return As<Identifier>(value1) == As<Identifier>(value2);
		}
		else if (IsType<SatReference>(value1))
		{
			return As<SatReference>(value1) == As<SatReference>(value2);
		}
		else if (IsType<UnaryExpr>(value1))
		{
			return As<UnaryExpr>(value1) == As<UnaryExpr>(value2);
		}
		else if (IsType<BinaryExpr>(value1))
		{
			return As<BinaryExpr>(value1) == As<BinaryExpr>(value2);
		}
		else if (IsType<DefFunc>(value1))
		{
			return As<DefFunc>(value1) == As<DefFunc>(value2);
		}
		else if (IsType<Range>(value1))
		{
			return As<Range>(value1) == As<Range>(value2);
		}
		else if (IsType<Lines>(value1))
		{
			return As<Lines>(value1) == As<Lines>(value2);
		}
		else if (IsType<If>(value1))
		{
			return As<If>(value1) == As<If>(value2);
		}
		else if (IsType<For>(value1))
		{
			return As<For>(value1) == As<For>(value2);
		}
		else if (IsType<Return>(value1))
		{
			return As<Return>(value1) == As<Return>(value2);
		}
		else if (IsType<ListConstractor>(value1))
		{
			return As<ListConstractor>(value1) == As<ListConstractor>(value2);
		}
		else if (IsType<KeyExpr>(value1))
		{
			return As<KeyExpr>(value1) == As<KeyExpr>(value2);
		}
		else if (IsType<RecordConstractor>(value1))
		{
			return As<RecordConstractor>(value1) == As<RecordConstractor>(value2);
		}
		else if (IsType<RecordInheritor>(value1))
		{
			return As<RecordInheritor>(value1) == As<RecordInheritor>(value2);
		}
		else if (IsType<DeclSat>(value1))
		{
			return As<DeclSat>(value1) == As<DeclSat>(value2);
		}
		else if (IsType<DeclFree>(value1))
		{
			return As<DeclFree>(value1) == As<DeclFree>(value2);
		}
		else if (IsType<Accessor>(value1))
		{
			return As<Accessor>(value1) == As<Accessor>(value2);
		};

		std::cerr << "IsEqual: Type Error" << std::endl;
		return false;
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

	void Environment::initialize()
	{
		registerBuiltInFunction(
			"sin",
			[](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return Sin(pEnv->expand(arguments[0]));
		}
		);

		registerBuiltInFunction(
			"cos",
			[](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return Cos(pEnv->expand(arguments[0]));
		}
		);

		registerBuiltInFunction(
			"diff",
			[&](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 2)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return ShapeDifference(pEnv->expand(arguments[0]), pEnv->expand(arguments[1]), m_weakThis.lock());
		}
		);

		registerBuiltInFunction(
			"area",
			[&](std::shared_ptr<Environment> pEnv, const std::vector<Address>& arguments)->Evaluated
		{
			if (arguments.size() != 1)
			{
				CGL_Error("引数の数が正しくありません");
			}

			return ShapeArea(pEnv->expand(arguments[0]), m_weakThis.lock());
		}
		);
	}
}
