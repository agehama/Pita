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

extern bool isDebugMode;
namespace cgl
{
	class VersionReducer : public ExprTransformer
	{
	public:
		VersionReducer() = default;

		static bool IsVersioned(const Identifier& identifier)
		{
			const auto& str = identifier.toString();
			return !str.empty() && str[0] == '$';
		}

		static Identifier WithoutVersion(const Identifier& identifier)
		{
			if (IsVersioned(identifier))
			{
				const std::string& str = identifier.toString();
				std::string originalName(str.begin() + str.find(')') + 1, str.end());
				return Identifier(originalName);
			}

			return identifier;
		}

		Expr operator()(const Identifier& node)override
		{
			std::cout << node.toString() << ", ";
			return WithoutVersion(node);
		}

		Expr operator()(const KeyExpr& node)override
		{
			const KeyExpr result(WithoutVersion(node.name), boost::apply_visitor(*this, node.expr));
			return result;
		}

		Expr operator()(const LRValue& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const UnaryExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const BinaryExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Lines& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DefFunc& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const If& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const For& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const ListConstractor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const RecordConstractor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Accessor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DeclSat& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DeclFree& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Import& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Range& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Return& node)override { return ExprTransformer::operator()(node); }
	};

	inline Expr VersionReduced(const Expr& expr)
	{
		VersionReducer reducer;
		auto result = boost::apply_visitor(reducer, expr);
		std::cout << std::endl;
		return result;
	}

	class NameReplacer : public ExprTransformer
	{
	public:
		NameReplacer(const std::unordered_map<std::string, Expr>& replaceMap) :
			replaceMap(replaceMap)
		{}

		const std::unordered_map<std::string, Expr>& replaceMap;

		Expr operator()(const Identifier& node)override
		{
			auto it = replaceMap.find(node.toString());
			if (it != replaceMap.end())
			{
				return it->second;
			}
			return node;
		}

		Expr operator()(const LRValue& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const UnaryExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const BinaryExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Lines& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DefFunc& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const If& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const For& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const ListConstractor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const KeyExpr& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const RecordConstractor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Accessor& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DeclSat& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const DeclFree& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Import& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Range& node)override { return ExprTransformer::operator()(node); }
		Expr operator()(const Return& node)override { return ExprTransformer::operator()(node); }
	};

	inline Expr ReplaceName(const Expr& original, const std::unordered_map<std::string, Expr>& replaceMap)
	{
		NameReplacer replacer(replaceMap);
		return boost::apply_visitor(replacer, original);
	}

	//Contextのクローンを作って評価しながら、全実行可能パスの解析を行う
	class InlineExpander : public ExprTransformer
	{
	public:
		InlineExpander(std::unordered_map<std::string, size_t>& identifierCounts,
			std::shared_ptr<Context> pContext,
			const std::unordered_map<std::string, std::string>& renameTable,
			int indent) :
			identifierCounts(identifierCounts),
			pContext(pContext),
			renameTable(renameTable),
			m_indent(indent)
		{}

		//全体で共通
		std::unordered_map<std::string, size_t>& identifierCounts;

		//パスごとに異なる
		std::shared_ptr<Context> pContext;

		//呼び出し階層ごとに異なる
		std::unordered_map<std::string, std::string> renameTable;
		int m_indent;

		static bool IsVersioned(const Identifier& identifier)
		{
			const auto& str = identifier.toString();
			return !str.empty() && str[0] == '$';
		}

		static Identifier WithoutVersion(const Identifier& identifier)
		{
			if (IsVersioned(identifier))
			{
				const std::string& str = identifier.toString();
				std::string originalName(str.begin() + str.find(')') + 1, str.end());
				return Identifier(originalName);
			}

			return identifier;
		}

		//n番目のname => $(n)name
		Identifier NewVersioned(const Identifier& identifier)
		{
			const std::string& original = identifier.toString();

			std::stringstream ss;
			ss << "$(" << identifierCounts[original] << ")" << original;
			const std::string renamed = ss.str();

			renameTable[original] = renamed;
			++identifierCounts[original];

			return Identifier(renamed);
		}

		std::string indent()const
		{
			const int tabSize = 4;
			std::string str;
			for (int i = 0; i < m_indent*tabSize; ++i)
			{
				str += ' ';
			}
			return str;
		}

		Expr operator()(const LRValue& node)override
		{
			/*
			インライン展開をした結果ここでは名前の解決を静的に行えるようになったはずなので、
			EitherReferenceがローカル変数なのかAddressなのかをここで解決する。
			また、ローカル変数であればリネームも行う。
			*/
			if (node.isEitherReference())
			{
				const auto& ref = node.eitherReference();
				if (ref.local)
				{
					auto it = renameTable.find(ref.local.get().toString());
					if (it != renameTable.end())
					{
						return Identifier(it->second);
					}
				}

				return LRValue(ref.replaced);
			}
			else
			{
				return ExprTransformer::operator()(node);
			}
		}

		Expr operator()(const Identifier& node)override
		{
			if (IsVersioned(node))
			{
				return node;
			}

			auto it = renameTable.find(node.toString());
			if (it == renameTable.end())
			{
				CGL_ErrorNode(node, msgs::Undefined(node));
			}

			return Identifier(it->second).setLocation(node);
		}

		Expr operator()(const UnaryExpr& node)override
		{
			return ExprTransformer::operator()(node);
		}

		Expr operator()(const BinaryExpr& node)override
		{
			if (node.op == BinaryOp::Assign)
			{
				Expr result;
				Expr evalExpr = node;

				//識別子の名前が変化した時も、変更前の名前でEvalしてContextに登録しておく
				//こうすれば、後からその変数を参照するときは普通に名前で検索でき、そしてrenameTableを見ればその場でリネームすることができる
				if (auto opt = AsOpt<Identifier>(node.lhs))
				{
					const auto currentName = opt.get();
					result = BinaryExpr(
						(IsVersioned(currentName) ? currentName : NewVersioned(currentName)),
						boost::apply_visitor(*this, node.rhs),
						node.op).setLocation(node);

					evalExpr = VersionReduced(node);
				}
				else
				{
					result = ExprTransformer::operator()(node);
				}

				std::cout << indent() << "====== Eval(begin) Line(" << __LINE__ << ")" << std::endl;
				Eval eval(pContext);
				boost::apply_visitor(eval, evalExpr);
				std::cout << indent() << "====== Eval(end)   Line(" << __LINE__ << ")" << std::endl;

				return result;
			}

			return ExprTransformer::operator()(node);
		}

		Expr operator()(const Lines& node)override
		{
			InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);

			Lines result;
			for (size_t i = 0; i < node.size(); ++i)
			{
				result.add(boost::apply_visitor(child, node.exprs[i]));
			}
			return result.setLocation(node);
		}

		Expr operator()(const DefFunc& node)override { return ExprTransformer::operator()(node); }

		Expr operator()(const If& node)override
		{
			If result(boost::apply_visitor(*this, node.cond_expr));

			//else式がない場合は分岐は必要ない
			if (!node.else_expr)
			{
				InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);
				result.then_expr = boost::apply_visitor(child, node.then_expr);
				return result.setLocation(node);
			}
			else
			{
				//最初にContextを保存する
				std::shared_ptr<Context> originalContext = pContext;

				{
					//保存したContextのクローンを作る
					pContext = originalContext->cloneContext();
					InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);
					result.then_expr = boost::apply_visitor(child, node.then_expr);
				}

				{
					//再び保存したContextのクローンを作る
					pContext = originalContext->cloneContext();
					InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);
					result.else_expr = boost::apply_visitor(child, node.else_expr.get());
				}

				//最初のContextを復元する
				pContext = originalContext;
			}

			return result.setLocation(node);
		}

		Expr operator()(const For& node)override
		{
			Expr evalExpr = VersionReduced(BinaryExpr(node.loopCounter, node.rangeStart, BinaryOp::Assign));
			std::cout << indent() << "====== Eval(begin) Line(" << __LINE__ << ")" << std::endl;
			Eval eval(pContext);
			boost::apply_visitor(eval, evalExpr);
			std::cout << indent() << "====== Eval(end)   Line(" << __LINE__ << ")" << std::endl;

			//*
			For result;
			result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
			result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
			result.loopCounter = (IsVersioned(node.loopCounter) ? node.loopCounter : NewVersioned(node.loopCounter));
			result.doExpr = boost::apply_visitor(*this, node.doExpr);
			result.asList = node.asList;
			return result.setLocation(node);
			//*/

			/*
			Expr startExpr = VersionReduced(node.rangeStart);
			Expr endExpr = VersionReduced(node.rangeStart);

			Eval eval(pContext);
			const Val startVal = pContext->expand(boost::apply_visitor(eval, startExpr), LocationInfo());
			const Val endVal = pContext->expand(boost::apply_visitor(eval, endExpr), LocationInfo());

			if (IsType<int>(startVal) && IsType<int>(endVal))
			{
				for (int i = As<int>(startVal); i <= As<int>(endVal); ++i)
				{
					i;
				}
			}
			else if (IsType<double>(startVal) && IsType<double>(endVal))
			{
				;
			}
			else
			{
				CGL_Error("ループカウンタが型が不正");
			}

			node.loopCounter;

			node.doExpr;
			//*/
		}

		Expr operator()(const ListConstractor& node)override { return ExprTransformer::operator()(node); }

		Expr operator()(const KeyExpr& node)override
		{
			const auto result = KeyExpr(
				(IsVersioned(node.name) ? node.name : NewVersioned(node.name)),
				boost::apply_visitor(*this, node.expr)).setLocation(node);

			std::cout << indent() << "====== Eval(begin) Line(" << __LINE__ << ")" << std::endl;
			Eval eval(pContext);
			Expr expr = VersionReduced(node);
			{
				std::cout << "Expr:\n";
				Printer2 printer(pContext, std::cout);
				boost::apply_visitor(printer, expr);
				std::cout << std::endl;
			}
			{
				std::cout << "Expr(renamed):\n";
				Expr expr2 = result;
				Printer2 printer(pContext, std::cout);
				boost::apply_visitor(printer, expr2);
				std::cout << std::endl;
			}
			boost::apply_visitor(eval, expr);
			std::cout << indent() << "====== Eval(end)   Line(" << __LINE__ << ")" << std::endl;

			return result;
		}

		Expr operator()(const RecordConstractor& node)override { return ExprTransformer::operator()(node); }

		Expr operator()(const Accessor& accessor)override
		{
			Accessor newAccessor;

			//最初のアクセスがList/Recordアクセスの場合はheadはこのまま残る
			//Function/Inheritアクセスの場合はheadは後から書き換えられる
			newAccessor.head = accessor.head;

			Address address;

			if (IsType<Identifier>(accessor.head))
			{
				const auto head = As<Identifier>(accessor.head);
				if (!pContext->findAddress(head).isValid())
				{
					CGL_ErrorNode(accessor, msgs::Undefined(head));
				}
			}

			Eval evaluator(pContext);

			std::cout << indent() << "====== Eval(begin) Line(" << __LINE__ << ")" << std::endl;
			const auto expr_ = VersionReduced(accessor.head);
			LRValue headValue = boost::apply_visitor(evaluator, expr_);
			std::cout << indent() << "====== Eval(end)   Line(" << __LINE__ << ")" << std::endl;

			if (headValue.isLValue() && headValue.deref(*pContext))
			{
				address = headValue.deref(*pContext).get();
			}
			else
			{
				Val evaluated = headValue.evaluated();
				if (auto opt = AsOpt<Record>(evaluated))
				{
					address = pContext->makeTemporaryValue(opt.get());
				}
				else if (auto opt = AsOpt<List>(evaluated))
				{
					address = pContext->makeTemporaryValue(opt.get());
				}
				else if (auto opt = AsOpt<FuncVal>(evaluated))
				{
					address = pContext->makeTemporaryValue(opt.get());
				}
				else
				{
					CGL_ErrorNodeInternal(accessor, "アクセッサの先頭の評価結果がレコード、リスト、関数のどの値でもありませんでした。");
				}
			}

			{
				std::cout << indent() << "====== Replace Accessor HeadValue ======" << std::endl;

				InlineExpander child(identifierCounts, pContext, renameTable, m_indent);
				Expr replacedHead = boost::apply_visitor(child, newAccessor.head);
				newAccessor.head = replacedHead;
			}

			for(size_t accessIndex = 0; accessIndex < accessor.accesses.size(); ++accessIndex)
			{
				const auto& access = accessor.accesses[accessIndex];
				LRValue lrAddress = LRValue(address);
				boost::optional<Val&> objOpt = pContext->mutableExpandOpt(lrAddress);
				if (!objOpt)
				{
					CGL_ErrorNode(accessor, "アクセッサによる参照先が存在しませんでした。");
				}

				Val& objRef = objOpt.get();

				if (auto listAccessOpt = AsOpt<ListAccess>(access))
				{
					std::cout << indent() << "====== Expansion Accessor List(begin) ======" << std::endl;

					if (!IsType<List>(objRef))
					{
						CGL_ErrorNode(accessor, "リストでない値に対してリストアクセスを行おうとしました。");
					}

					{
						InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);
						Expr replacedIndex = boost::apply_visitor(child, listAccessOpt.get().index);

						ListAccess replacedListAccess;
						replacedListAccess.isArbitrary = listAccessOpt.get().isArbitrary;
						//replacedListAccess.index = boost::apply_visitor(child, replacedIndex);
						replacedListAccess.index = replacedIndex;

						newAccessor.accesses.push_back(replacedListAccess);
					}

					std::cout << indent() << "====== Eval(begin) Line(" << __LINE__ << ")" << std::endl;
					const auto expr_ = VersionReduced(listAccessOpt.get().index);
					Val value = pContext->expand(boost::apply_visitor(evaluator, expr_), accessor);
					std::cout << indent() << "====== Eval(end)   Line(" << __LINE__ << ")" << std::endl;

					List& list = As<List>(objRef);

					if (auto indexOpt = AsOpt<int>(value))
					{
						const int indexValue = indexOpt.get();
						const int listSize = static_cast<int>(list.data.size());
						const int maxIndex = listSize - 1;

						if (0 <= indexValue && indexValue <= maxIndex)
						{
							address = list.get(indexValue);
						}
						else if (indexValue < 0 || !pContext->isAutomaticExtendMode())
						{
							CGL_ErrorNode(accessor, "リストの範囲外アクセスが発生しました。");
						}
						else
						{
							while (static_cast<int>(list.data.size()) - 1 < indexValue)
							{
								list.data.push_back(pContext->makeTemporaryValue(0));
							}
							address = list.get(indexValue);
						}
					}
					else
					{
						CGL_ErrorNode(accessor, "リストアクセスのインデックスが整数値ではありませんでした。");
					}

					std::cout << indent() << "====== Expansion Accessor List(end) ======" << std::endl;
				}
				else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
				{
					std::cout << indent() << "====== Expansion Accessor Record(begin) ======" << std::endl;

					if (!IsType<Record>(objRef))
					{
						CGL_ErrorNode(accessor, "レコードでない値に対してレコードアクセスを行おうとしました。");
					}

					const Record& record = As<Record>(objRef);
					
					bool found = false;
					const std::string searchName = recordAccessOpt.get().name.toString();
					//レコードメンバにアクセッサはまだリネームされてないので、元の名前で探す
					for (const auto& value : record.values)
					{
						std::cout << indent() << "====== Matching with: " << Identifier(value.first).toString() << std::endl;
						if (WithoutVersion(Identifier(value.first)).toString() == searchName)
						{
							RecordAccess newRecordAccess = recordAccessOpt.get();
							newRecordAccess.name = Identifier(value.first);

							//Recordアクセスはリネーム後の名前をpushする
							newAccessor.accesses.emplace_back(newRecordAccess);
							
							address = value.second;
							found = true;
							break;
						}
					}

					if (!found)
					{
						CGL_ErrorNode(accessor, std::string("指定された識別子\"") + recordAccessOpt.get().name.toString() + "\"がレコード中に存在しませんでした。");
					}

					std::cout << indent() << "====== Expansion Accessor Record(end) ======" << std::endl;
				}
				else if (auto recordAccessOpt = AsOpt<FunctionAccess>(access))
				{
					std::cout << indent() << "====== Expansion Accessor Function(begin) ======" << std::endl;

					if (!IsType<FuncVal>(objRef))
					{
						CGL_ErrorNode(accessor, "関数でない値に対して関数呼び出しを行おうとしました。");
					}

					auto funcAccess = As<FunctionAccess>(access);

					const FuncVal& function = As<FuncVal>(objRef);

					//引数を展開する
					std::vector<Expr> replacedArguments;
					std::unordered_map<std::string, Expr> replaceMap;
					{
						std::cout << indent() << "====== Expansion Function Arguments ======" << std::endl;
						Printer printer(pContext, std::cout, 0);

						InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);

						for (const auto& expr : funcAccess.actualArguments)
						{
							/*std::cout << indent() << "Argument: \n";
							boost::apply_visitor(printer, expr);
							std::cout << std::endl;*/
							replacedArguments.push_back(boost::apply_visitor(child, expr));
						}

						for (size_t argIndex = 0; argIndex < function.arguments.size(); ++argIndex)
						{
							replaceMap.emplace(function.arguments[argIndex].toString(), replacedArguments[argIndex]);
						}
					}

					//組み込み関数の場合は、展開した引数をアクセッサの引数に入れておく
					if (function.builtinFuncAddress)
					{
						FunctionAccess newFunctionAccess = funcAccess;
						newFunctionAccess.actualArguments.clear();

						InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);

						//置き換えた式自体がまた関数呼び出しを含む可能性があるので再帰的に展開する
						for (size_t i = 0; i < replacedArguments.size(); ++i)
						{
							const Expr expandedArgument = boost::apply_visitor(child, replacedArguments[i]);
							newFunctionAccess.actualArguments.push_back(expandedArgument);
						}

						newAccessor.accesses.clear();
						newAccessor.accesses.push_back(newFunctionAccess);
					}
					//Pita関数の場合は、展開した引数を直接関数の定義の該当箇所に挿入する
					else
					{
						if (function.arguments.size() != funcAccess.actualArguments.size())
						{
							std::stringstream ss;
							ss << "(仮引数: " << function.arguments.size() << " | 実引数: " << funcAccess.actualArguments.size() << ")";
							CGL_ErrorNode(accessor, "仮引数と実引数の数が一致しませんでした。" + ss.str());
						}

						Expr expandedFunction;
						{
							//関数は展開するとLines(expr1,expr2,...)という形になる
							expandedFunction = ReplaceName(function.expr, replaceMap);
							if (!IsType<Lines>(expandedFunction))
							{
								CGL_ErrorNode(accessor, "関数を展開した結果の型がLinesではありませんでした。");
							}

							//普通に展開していくと、どんどんLines(Lines(...))というようにネストが増えていく
							//したがって、Linesの中に単一のLinesがいる場合は取り出すようにする
							Lines lines = As<Lines>(expandedFunction);
							while (lines.exprs.size() == 1 && IsType<Lines>(lines.exprs.front()))
							{
								expandedFunction = lines.exprs.front();
								lines = As<Lines>(expandedFunction);
							}
						}

						//置き換えた式自体がまた関数呼び出しを含む可能性があるので再帰的に展開する
						{
							std::cout << indent() << "====== Expanded Function(prev) ======" << std::endl;
							Expr prevExpand = accessor;
							Printer2 printer2(pContext, std::cout, m_indent);
							boost::apply_visitor(printer2, prevExpand);
							std::cout << std::endl;
						}
						{
							std::cout << indent() << "====== Expanded Function(args) ======" << std::endl;
							for (const auto& arg : replacedArguments)
							{
								std::cout << indent() << "====== Arg:" << std::endl;
								Printer2 printer2(pContext, std::cout, m_indent);
								boost::apply_visitor(printer2, arg);
								std::cout << std::endl;
							}
						}
						{
							std::cout << indent() << "====== Expanded Function(post) ======" << std::endl;
							Printer2 printer2(pContext, std::cout, m_indent);
							boost::apply_visitor(printer2, expandedFunction);
							std::cout << std::endl;
						}

						InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);

						std::cout << indent() << "====== Expanded Function Arguments Recursively(begin) ======" << std::endl;
						expandedFunction = boost::apply_visitor(child, expandedFunction);
						std::cout << indent() << "====== Expanded Function Arguments Recursively(end) ======" << std::endl;

						newAccessor.head = expandedFunction;
						newAccessor.accesses.clear();

						//Input  => record1.func1(arg1).record2.[list1]
						//Output => (... arg1 ...).record2.[list1]
					}

					std::cout << indent() << "====== Expansion Function Eval(begin) ======" << std::endl;
					std::vector<Address> args;
					for (const auto& expr : funcAccess.actualArguments)
					{
						const auto expr_ = VersionReduced(expr);
						const LRValue currentArgument = boost::apply_visitor(evaluator, expr_);
						currentArgument.push_back(args, *pContext);
					}

					const Val returnedValue = pContext->expand(evaluator.callFunction(accessor, function, args), accessor);
					address = pContext->makeTemporaryValue(returnedValue);
					std::cout << indent() << "====== Expansion Function Eval(end) ======" << std::endl;

					std::cout << indent() << "====== Expansion Accessor Function(end) ======" << std::endl;
				}
				else
				{
					std::cout << indent() << "====== Expansion Accessor Inherit(begin) ======" << std::endl;

					if (!IsType<Record>(objRef))
					{
						CGL_ErrorNode(accessor, "レコードでない値に対して継承式を適用しようとしました。");
					}

					auto inheritAccess = As<InheritAccess>(access);

					const Record& record = As<Record>(objRef);

					InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);

					RecordConstractor recordConstructor;
					for (const auto& keyval : record.values)
					{
						Expr expr = LRValue(keyval.second);

						const auto currentName = Identifier(keyval.first);
						recordConstructor.adds(
							(IsVersioned(currentName) ? currentName : NewVersioned(currentName)),
							boost::apply_visitor(child, expr));
					}

					const Expr adderExpr = inheritAccess.adder;
					const Expr newAdderExpr = boost::apply_visitor(child, adderExpr);
					if (!IsType<RecordConstractor>(newAdderExpr))
					{
						CGL_Error("エラー");
					}

					const RecordConstractor newAdder = As<RecordConstractor>(newAdderExpr);
					recordConstructor.exprs.insert(recordConstructor.exprs.end(), newAdder.exprs.begin(), newAdder.exprs.end());
					newAccessor.head = recordConstructor;
					newAccessor.accesses.clear();

					const Val returnedValue = pContext->expand(evaluator.inheritRecord(accessor, record, inheritAccess.adder), accessor);
					address = pContext->makeTemporaryValue(returnedValue);

					std::cout << indent() << "====== Expansion Accessor Inherit(end) ======" << std::endl;
				}
				/*{
					std::cout << indent() << "====== Expansion Accessor Inherit(begin) ======" << std::endl;

					if (!IsType<Record>(objRef))
					{
						CGL_ErrorNode(accessor, "レコードでない値に対して継承式を適用しようとしました。");
					}

					auto inheritAccess = As<InheritAccess>(access);

					const Record& record = As<Record>(objRef);

					InlineExpander child(identifierCounts, pContext, renameTable, m_indent + 1);

					RecordConstractor recordConstructor;
					for (const auto& keyval : record.values)
					{
						Expr expr = LRValue(keyval.second);

						const auto currentName = Identifier(keyval.first);
						recordConstructor.adds(
							(IsVersioned(currentName) ? currentName : NewVersioned(currentName)),
							boost::apply_visitor(child, expr));
					}

					//今見ているレコードがアクセッサの先頭要素かどうかにかかわらず、勝手に先頭に置き換える
					//func1().record{scale: ...}
					//のようなレコードも全て展開して、{ ... }{scale: ...}　このようになるが、これで問題になるケースは少ないはず。
					newAccessor.head = recordConstructor;
					newAccessor.accesses.clear();

					const Expr adderExpr = inheritAccess.adder;
					const auto newAdder = boost::apply_visitor(child, adderExpr);
					if (!IsType<RecordConstractor>(newAdder))
					{
						CGL_Error("エラー");
					}
					
					InheritAccess newInheritAccess = inheritAccess;
					newInheritAccess.adder = As<RecordConstractor>(newAdder);

					newAccessor.accesses.push_back(newInheritAccess);

					{
						std::cout << indent() << "====== Inherit(begin) Line(" << __LINE__ << ")" << std::endl;
						{
							std::cout << "Adder:\n";
							Expr expr = inheritAccess.adder;
							Printer2 printer(pContext, std::cout);
							boost::apply_visitor(printer, expr);
							std::cout << std::endl;
						}
						{
							std::cout << "Adder(renamed):\n";
							Expr expr2 = newAdder;
							Printer2 printer(pContext, std::cout);
							boost::apply_visitor(printer, expr2);
							std::cout << std::endl;
						}
						std::cout << indent() << "====== Inherit(end)   Line(" << __LINE__ << ")" << std::endl;
					}

					const Val returnedValue = pContext->expand(evaluator.inheritRecord(accessor, record, inheritAccess.adder), accessor);
					address = pContext->makeTemporaryValue(returnedValue);

					std::cout << indent() << "====== Expansion Accessor Inherit(end) ======" << std::endl;
				}*/
			}

			std::cout << indent() << "====== Return Expansion Accessor ======" << std::endl;

			if (newAccessor.accesses.empty())
			{
				return newAccessor.head;
			}

			return newAccessor;
		}

		Expr operator()(const DeclSat& node) { CGL_Error("未対応"); }
		Expr operator()(const DeclFree& node) { CGL_Error("未対応"); }
		Expr operator()(const Import& node) { CGL_Error("未対応"); }
		Expr operator()(const Range& node) { CGL_Error("未対応"); }
		Expr operator()(const Return& node) { CGL_Error("未対応"); }
	};

	inline Expr InlineExpand(Expr expr, std::shared_ptr<Context> pContext)
	{
		auto contextClone = pContext->cloneContext();
		std::unordered_map<std::string, size_t> identifierCounts;

		std::unordered_map<std::string, std::string> renameTable;
		InlineExpander expander(identifierCounts, pContext, renameTable, 0);

		return ExpandedEmptyLines(boost::apply_visitor(expander, expr));
	}
}

extern int constraintViolationCount;
extern bool isInConstraint;

namespace cgl
{
	class ExprLocationInfo : public boost::static_visitor<std::string>
	{
	public:
		ExprLocationInfo() = default;

		std::string operator()(const Lines& node)const
		{
			std::string result = static_cast<const LocationInfo&>(node).getInfo();
			for (const auto& expr : node.exprs)
			{
				result += boost::apply_visitor(*this, expr);
			}
			return result;
		}

		std::string operator()(const UnaryExpr& node)const
		{
			std::string result = static_cast<const LocationInfo&>(node).getInfo();
			result += boost::apply_visitor(*this, node.lhs);
			return result;
		}

		std::string operator()(const BinaryExpr& node)const
		{
			std::string result = static_cast<const LocationInfo&>(node).getInfo();
			result += boost::apply_visitor(*this, node.lhs);
			result += boost::apply_visitor(*this, node.rhs);
			return result;
		}

		std::string operator()(const LRValue& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Identifier& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Import& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Range& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const DefFunc& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const If& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const For& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Return& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const ListConstractor& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const KeyExpr& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const RecordConstractor& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const Accessor& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const DeclSat& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
		std::string operator()(const DeclFree& node)const { return static_cast<const LocationInfo&>(node).getInfo(); }
	};

	inline std::string GetLocationInfo(const Expr& expr)
	{
		return boost::apply_visitor(ExprLocationInfo(), expr);
	}

	void ProgressStore::TryWrite(std::shared_ptr<Context> env, const Val& value)
	{
		ProgressStore& i = Instance();
		if (i.mtx.try_lock())
		{
			i.pEnv = env->clone();
			i.evaluated = value;

			i.mtx.unlock();
		}
	}

	bool ProgressStore::TryLock()
	{
		ProgressStore& i = Instance();
		return i.mtx.try_lock();
	}

	void ProgressStore::Unlock()
	{
		ProgressStore& i = Instance();
		i.mtx.unlock();
	}

	std::shared_ptr<Context> ProgressStore::GetContext()
	{
		ProgressStore& i = Instance();
		return i.pEnv;
	}

	boost::optional<Val>& ProgressStore::GetVal()
	{
		ProgressStore& i = Instance();
		return i.evaluated;
	}

	void ProgressStore::Reset()
	{
		ProgressStore& i = Instance();
		i.pEnv = nullptr;
		i.evaluated = boost::none;
	}

	bool ProgressStore::HasValue()
	{
		ProgressStore& i = Instance();
		return static_cast<bool>(i.evaluated);
	}

	ProgressStore& ProgressStore::Instance()
	{
		static ProgressStore i;
		return i;
	}

	ClosureMaker& ClosureMaker::addLocalVariable(const std::string& name)
	{
		localVariables.insert(name);
		return *this;
	}

	bool ClosureMaker::isLocalVariable(const std::string& name)const
	{
		return localVariables.find(name) != localVariables.end();
	}

	Expr ClosureMaker::operator()(const Identifier& node)
	{
		//その関数のローカル変数であれば関数の実行時に評価すればよいので、名前を残す
		if (isLocalVariable(node))
		{
			return node;
		}
		//Closure作成時点では全てのローカル変数は捕捉できないので、ここでアドレスに置き換えるのは安全でない
		//したがって、Contextに存在したら名前とアドレスのどちらからも参照できるようにしておき、評価時に実際の参照先を決める
		const Address address = pEnv->findAddress(node);
		if (address.isValid())
		{
			//Identifier RecordConstructor の形をしたレコード継承の head 部分
			//とりあえず参照先のレコードのメンバはローカル変数とおく
			if (isInnerRecord)
			{
				const Val& evaluated = pEnv->expand(address, node);
				if (auto opt = AsOpt<Record>(evaluated))
				{
					const Record& record = opt.get();
					
					for (const auto& keyval : record.values)
					{
						addLocalVariable(keyval.first);
					}
				}
			}

			return LRValue(EitherReference(node, address)).setLocation(node);
		}

		//それ以外の場合は実行してみないと分からないため、ローカル変数と仮定する（参照エラーはEvalで出す）
		//例：f = (s -> s{g()}) のgがsのメンバかどうかはfのクロージャ生成時点では分からない
		return node;
	}

	Expr ClosureMaker::operator()(const UnaryExpr& node)
	{
		if (node.op == UnaryOp::Dynamic && !isInnerClosure)
		{
			if (IsType<Identifier>(node.lhs))
			{
				if (isLocalVariable(As<Identifier>(node.lhs)))
				{
					return node;
				}
				return LRValue(pEnv->bindReference(As<Identifier>(node.lhs))).setLocation(node);
			}
			else if (IsType<Accessor>(node.lhs))
			{
				if (IsType<Identifier>(As<Accessor>(node.lhs).head) && isLocalVariable(As<Identifier>(As<Accessor>(node.lhs).head)))
				{
					return node;
				}
				return LRValue(pEnv->bindReference(As<Accessor>(node.lhs), node)).setLocation(node);
			}

			CGL_ErrorNode(node, "参照演算子\"@\"の右辺には識別子かアクセッサしか用いることができません。");
		}

		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op).setLocation(node);
	}

	Expr ClosureMaker::operator()(const BinaryExpr& node)
	{
		const Expr rhs = boost::apply_visitor(*this, node.rhs);

		if (node.op != BinaryOp::Assign)
		{
			const Expr lhs = boost::apply_visitor(*this, node.lhs);
			return BinaryExpr(lhs, rhs, node.op).setLocation(node);
		}

		//Assignの場合、lhs は Address or Identifier or Accessor に限られる
		//つまり現時点では、(if cond then x else y) = true のような式を許可していない
		//ここで左辺に直接アドレスが入っていることは有り得る？
		//a = b = 10　のような式でも、右結合であり左側は常に識別子が残っているはずなので、あり得ないと思う
		if (auto valOpt = AsOpt<Identifier>(node.lhs))
		{
			const Identifier identifier = valOpt.get();

			//ローカル変数にあれば、その場で解決できる識別子なので何もしない
			if (isLocalVariable(identifier))
			{
				return BinaryExpr(node.lhs, rhs, node.op).setLocation(node);
			}
			else
			{
				const Address address = pEnv->findAddress(identifier);

				//ローカル変数に無く、スコープにあれば、アドレスに置き換える
				if (address.isValid())
				{
					//TODO: 制約式の場合は、ここではじく必要がある
					//return BinaryExpr(LRValue(address), rhs, node.op).setLocation(node);
					return LRValue(EitherReference(identifier, address)).setLocation(node);
				}
				//スコープにも無い場合は新たなローカル変数の宣言なので、ローカル変数に追加しておく
				else
				{
					addLocalVariable(identifier);
					return BinaryExpr(node.lhs, rhs, node.op).setLocation(node);
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

			return BinaryExpr(lhs, rhs, node.op).setLocation(node);
		}
		//左辺に参照変数が来るケース：@a = x
		else if (auto valOpt = AsOpt<UnaryExpr>(node.lhs))
		{
			if (valOpt.get().op == UnaryOp::Dynamic)
			{
				const Expr lhs = boost::apply_visitor(*this, node.lhs);

				return BinaryExpr(lhs, rhs, node.op).setLocation(node);
			}
		}

		CGL_ErrorNode(node, "二項演算子\"=\"の左辺は単一の左辺値でなければなりません。");
		return LRValue(0);
	}

	Expr ClosureMaker::operator()(const Range& node)
	{
		return Range(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs)).setLocation(node);
	}

	Expr ClosureMaker::operator()(const Lines& node)
	{
		Lines result;
		for (size_t i = 0; i < node.size(); ++i)
		{
			result.add(boost::apply_visitor(*this, node.exprs[i]));
		}
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const DefFunc& node)
	{
		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, true);
		for (const auto& argument : node.arguments)
		{
			child.addLocalVariable(argument);
		}

		return DefFunc(node.arguments, boost::apply_visitor(child, node.expr)).setLocation(node);
	}

	Expr ClosureMaker::operator()(const If& node)
	{
		If result(boost::apply_visitor(*this, node.cond_expr));
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (node.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.get());
		}

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const For& node)
	{
		For result;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);

		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, isInnerClosure);
		child.addLocalVariable(node.loopCounter);
		result.doExpr = boost::apply_visitor(child, node.doExpr);
		result.loopCounter = node.loopCounter;
		result.asList = node.asList;

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const Return& node)
	{
		return Return(boost::apply_visitor(*this, node.expr)).setLocation(node);
		//これだとダメかもしれない？
		//return a = 6, a + 2
	}

	Expr ClosureMaker::operator()(const ListConstractor& node)
	{
		ListConstractor result;
		//ClosureMaker child(*this);
		ClosureMaker child(pEnv, localVariables, false, isInnerClosure);
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(child, expr));
		}
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const KeyExpr& node)
	{
		//変数宣言式
		//再代入の可能性もあるがどっちにしろこれ以降この識別子はローカル変数と扱ってよい
		addLocalVariable(node.name);

		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const RecordConstractor& node)
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
			ClosureMaker child(pEnv, localVariables, false, isInnerClosure);
			for (const auto& expr : node.exprs)
			{
				result.exprs.push_back(boost::apply_visitor(child, expr));
			}
		}

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const Accessor& node)
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
				if (listAccess.get().isArbitrary)
				{
					result.add(listAccess.get());
				}
				else
				{
					result.add(ListAccess(boost::apply_visitor(*this, listAccess.get().index)));
				}
			}
			else if (auto recordAccess = AsOpt<RecordAccess>(access))
			{
				result.add(recordAccess.get());
			}
			else if (auto functionAccess = AsOpt<FunctionAccess>(access))
			{
				FunctionAccess access;
				for (const auto& argument : functionAccess.get().actualArguments)
				{
					access.add(boost::apply_visitor(*this, argument));
				}
				result.add(access);
			}
			else if (auto inheritAccess = AsOpt<InheritAccess>(access))
			{
				ClosureMaker child(pEnv, localVariables, true, isInnerClosure);

				Expr originalAdder = inheritAccess.get().adder;
				Expr replacedAdder = boost::apply_visitor(child, originalAdder);
				if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
				{
					InheritAccess access(opt.get());
					result.add(access);
				}
				else
				{
					CGL_ErrorNodeInternal(node, "node.adderの評価結果がRecordConstractorでありませんでした。");
				}
			}
			else
			{
				CGL_ErrorNodeInternal(node, "アクセッサの評価結果が不正です。");
			}
		}

		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const DeclSat& node)
	{
		DeclSat result;
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
	}

	Expr ClosureMaker::operator()(const DeclFree& node)
	{
		DeclFree result;
		for (const auto& freeVar : node.accessors)
		{
			const Expr expr = freeVar.accessor;
			const Expr closedAccessor = boost::apply_visitor(*this, expr);
			if (IsType<Accessor>(closedAccessor) || IsType<UnaryExpr>(closedAccessor))
			{
				result.addAccessor(closedAccessor);
			}
			else
			{
				CGL_ErrorNodeInternal(node, "アクセッサの評価結果が不正です。");
			}

			if (freeVar.range)
			{
				const Expr closedRange = boost::apply_visitor(*this, freeVar.range.get());
				result.addRange(closedRange);
			}
		}
		return result.setLocation(node);
	}

	LRValue Eval::operator()(const Identifier& node)
	{
		const Address address = pEnv->findAddress(node);
		if (address.isValid())
		{
			return LRValue(address);
		}

		CGL_ErrorNode(node, msgs::Undefined(node));
	}

	LRValue Eval::operator()(const Import& node)
	{
		return node.eval(pEnv);
	}

	LRValue Eval::operator()(const UnaryExpr& node)
	{
		const Val lhs = pEnv->expand(boost::apply_visitor(*this, node.lhs), node);

		switch (node.op)
		{
		case UnaryOp::Not:     return RValue(NotFunc(lhs, *pEnv));
		case UnaryOp::Minus:   return RValue(MinusFunc(lhs, *pEnv));
		case UnaryOp::Plus:    return RValue(PlusFunc(lhs, *pEnv));
		case UnaryOp::Dynamic: return RValue(lhs);
		}

		CGL_ErrorNodeInternal(node, "不明な単項演算子です。");
		return RValue(0);
	}

	LRValue Eval::operator()(const BinaryExpr& node)
	{
		UpdateCurrentLocation(node);

		const LRValue rhs_ = boost::apply_visitor(*this, node.rhs);

		Val lhs;
		if (node.op != BinaryOp::Assign)
		{
			const LRValue lhs_ = boost::apply_visitor(*this, node.lhs);
			lhs = pEnv->expand(lhs_, node);
		}

		Val rhs = pEnv->expand(rhs_, node);

		switch (node.op)
		{
		case BinaryOp::And: return RValue(AndFunc(lhs, rhs, *pEnv));
		case BinaryOp::Or:  return RValue(OrFunc(lhs, rhs, *pEnv));

		case BinaryOp::Equal:        return RValue(EqualFunc(lhs, rhs, *pEnv));
		case BinaryOp::NotEqual:     return RValue(NotEqualFunc(lhs, rhs, *pEnv));
		case BinaryOp::LessThan:     return RValue(LessThanFunc(lhs, rhs, *pEnv));
		case BinaryOp::LessEqual:    return RValue(LessEqualFunc(lhs, rhs, *pEnv));
		case BinaryOp::GreaterThan:  return RValue(GreaterThanFunc(lhs, rhs, *pEnv));
		case BinaryOp::GreaterEqual: return RValue(GreaterEqualFunc(lhs, rhs, *pEnv));

		case BinaryOp::Add: return RValue(AddFunc(lhs, rhs, *pEnv));
		case BinaryOp::Sub: return RValue(SubFunc(lhs, rhs, *pEnv));
		case BinaryOp::Mul: return RValue(MulFunc(lhs, rhs, *pEnv));
		case BinaryOp::Div: return RValue(DivFunc(lhs, rhs, *pEnv));

		case BinaryOp::Pow:    return RValue(PowFunc(lhs, rhs, *pEnv));
		case BinaryOp::Concat: return RValue(ConcatFunc(lhs, rhs, *pEnv));

		case BinaryOp::SetDiff: return RValue(SetDiffFunc(lhs, rhs, *pEnv));

		case BinaryOp::Assign:
		{
			//Assignの場合、lhs は Address or Identifier or Accessor に限られる
			//つまり現時点では、(if cond then x else y) = true のような式を許可していない
			//ここで左辺に直接アドレスが入っていることは有り得る？
			//a = b = 10　のような式でも、右結合であり左側は常に識別子が残っているはずなので、あり得ないと思う
			if (auto valOpt = AsOpt<LRValue>(node.lhs))
			{
				//代入式の左辺が参照変数のケースは許す：@a = 10
				if (valOpt.get().isReference())
				{
					pEnv->assignToReference(valOpt.get().reference(), rhs_, node);
					return RValue(rhs);
				}
				else if (valOpt.get().isEitherReference())
				{
					const auto& ref = valOpt.get().eitherReference();
					if (ref.local)
					{
						Expr assignExpr = BinaryExpr(*ref.local, rhs_, BinaryOp::Assign);
						return pEnv->expand(boost::apply_visitor(*this, assignExpr), node);
					}
					else
					{
						//ref.replaced;
						CGL_ErrorNodeInternal(node, "一時オブジェクトへの代入はできません。");
					}
				}
				else
				{
					printExpr(node.lhs, pEnv, std::cout);
					CGL_ErrorNodeInternal(node, "一時オブジェクトへの代入はできません。");
				}
			}
			else if (auto valOpt = AsOpt<Identifier>(node.lhs))
			{
				const Identifier& identifier = valOpt.get();

				const Address address = pEnv->findAddress(identifier);
				//変数が存在する：代入式
				if (address.isValid())
				{
					//CGL_DebugLog("代入式");
					//pEnv->assignToObject(address, rhs);
					pEnv->bindValueID(identifier, pEnv->makeTemporaryValue(rhs));
				}
				//変数が存在しない：変数宣言式
				else
				{
					//CGL_DebugLog("変数宣言式");
					pEnv->bindNewValue(identifier, rhs);
					//CGL_DebugLog("");
				}

				return RValue(rhs);
			}
			else if (auto accessorOpt = AsOpt<Accessor>(node.lhs))
			{
				const LRValue lhs_ = boost::apply_visitor(*this, node.lhs);
				if (lhs_.isLValue())
				{
					if (lhs_.isValid())
					{
						pEnv->assignToAccessor(accessorOpt.get(), rhs_, node);
						return RValue(rhs);
					}
					else
					{
						CGL_ErrorNodeInternal(node, "アクセッサの評価結果が無効なアドレス値です。");
					}
				}
				else
				{
					CGL_ErrorNodeInternal(node, "アクセッサの評価結果が無効な値です。");
				}
			}
		}
		}

		CGL_ErrorNodeInternal(node, "不明な二項演算子です。");
		return RValue(0);
	}

	LRValue Eval::operator()(const DefFunc& defFunc)
	{
		UpdateCurrentLocation(defFunc);
		return pEnv->makeFuncVal(pEnv, defFunc.arguments, defFunc.expr);
	}

	LRValue Eval::callFunction(const LocationInfo& info, const FuncVal& funcVal, const std::vector<Address>& arguments)
	{
		CGL_DebugLog("Function Context:");
		pEnv->printContext();

		if (funcVal.builtinFuncAddress)
		{
			return LRValue(pEnv->callBuiltInFunction(funcVal.builtinFuncAddress.get(), arguments, info));
		}

		//if (funcVal.arguments.size() != callFunc.actualArguments.size())
		if (funcVal.arguments.size() != arguments.size())
		{
			CGL_ErrorNode(info, "仮引数の数と実引数の数が合っていません。");
		}

		//関数の評価
		//(1)ここでのローカル変数は関数を呼び出した側ではなく、関数が定義された側のものを使うのでローカル変数を置き換える

		pEnv->switchFrontScope();
		//今の意味論ではスコープへの参照は全てアドレス値に変換している
		//ここで、関数内からグローバル変数の書き換えを行おうとすると、アドレスに紐つけられた値を直接変更することになる
		//TODO: これは、意味論的に正しいのか一度考える必要がある
		//とりあえず関数がスコープに依存することはなくなったので、単純に別のスコープに切り替えるだけで良い

		//(2)関数の引数用にスコープを一つ追加する
		pEnv->enterScope();

		for (size_t i = 0; i < funcVal.arguments.size(); ++i)
		{
			/*
			12/14
			引数はスコープをまたぐ時に参照先が変わらないように全てIDで渡すことにする。
			*/
			pEnv->bindValueID(funcVal.arguments[i], arguments[i]);
		}

		CGL_DebugLog("Function Definition:");
		printExpr(funcVal.expr);

		//(3)関数の戻り値を元のスコープに戻す時も、引数と同じ理由で全て展開して渡す。
		Val result;
		{
			result = pEnv->expand(boost::apply_visitor(*this, funcVal.expr), info);
			CGL_DebugLog("Function Val:");
			printVal(result, nullptr);
		}

		//(4)関数を抜ける時に、仮引数は全て解放される
		pEnv->exitScope();

		//(5)最後にローカル変数の環境を関数の実行前のものに戻す。
		pEnv->switchBackScope();

		//評価結果がreturn式だった場合はreturnを外して中身を返す
		//return以外のジャンプ命令は関数では効果を持たないのでそのまま上に返す
		if (IsType<Jump>(result))
		{
			auto& jump = As<Jump>(result);
			if (jump.isReturn())
			{
				if (jump.lhs)
				{
					return RValue(jump.lhs.get());
				}
				else
				{
					CGL_ErrorNode(info, "return式の右辺が無効な値です。");
				}
			}
		}

		return RValue(result);
	}

	LRValue Eval::operator()(const Lines& statement)
	{
		//UpdateCurrentLocation(statement);

		pEnv->enterScope();

		Val result;
		int i = 0;
		for (const auto& expr : statement.exprs)
		{
			CGL_DebugLog("Evaluate expression(" + std::to_string(i) + ")");
			pEnv->printContext();

			result = pEnv->expand(boost::apply_visitor(*this, expr), statement);
			printVal(result, pEnv);

			//TODO: 後で考える
			//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
			/*
			if (IsLValue(result))
			{
				//const Val& resultValue = pEnv->dereference(result);
				const Val resultValue = pEnv->expandRef(result);
				const bool isMacro = IsType<Jump>(resultValue);
				if (isMacro)
				{
					result = As<Jump>(resultValue);
				}
			}
			*/

			//途中でジャンプ命令を読んだら即座に評価を終了する
			if (IsType<Jump>(result))
			{
				break;
			}

			++i;
		}

		pEnv->exitScope();
		return LRValue(result);
	}

	LRValue Eval::operator()(const If& if_statement)
	{
		UpdateCurrentLocation(if_statement);

		const Val cond = pEnv->expand(boost::apply_visitor(*this, if_statement.cond_expr), if_statement);
		if (!IsType<bool>(cond))
		{
			CGL_ErrorNode(if_statement, "条件式の評価結果がブール値ではありませんでした。");
		}

		if (As<bool>(cond))
		{
			const Val result = pEnv->expand(boost::apply_visitor(*this, if_statement.then_expr), if_statement);
			return RValue(result);
		}
		else if (if_statement.else_expr)
		{
			const Val result = pEnv->expand(boost::apply_visitor(*this, if_statement.else_expr.get()), if_statement);
			return RValue(result);
		}

		return RValue(0);
	}

	LRValue Eval::operator()(const For& forExpression)
	{
		UpdateCurrentLocation(forExpression);

		const Val startVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeStart), forExpression);
		const Val endVal = pEnv->expand(boost::apply_visitor(*this, forExpression.rangeEnd), forExpression);

		//startVal <= endVal なら 1
		//startVal > endVal なら -1
		//を適切な型に変換して返す
		const auto calcStepValue = [&](const Val& a, const Val& b)->boost::optional<std::pair<Val, bool>>
		{
			const bool a_IsInt = IsType<int>(a);
			const bool a_IsDouble = IsType<double>(a);

			const bool b_IsInt = IsType<int>(b);
			const bool b_IsDouble = IsType<double>(b);

			if (!((a_IsInt || a_IsDouble) && (b_IsInt || b_IsDouble)))
			{
				CGL_ErrorNode(forExpression, "ループ範囲式の評価結果が数値ではありませんでした。");
			}

			const bool result_IsDouble = a_IsDouble || b_IsDouble;

			const bool isInOrder = LessEqualFunc(a, b, *pEnv);

			const int sign = isInOrder ? 1 : -1;

			if (result_IsDouble)
			{
				return std::pair<Val, bool>(MulFunc(1.0, sign, *pEnv), isInOrder);
			}

			return std::pair<Val, bool>(MulFunc(1, sign, *pEnv), isInOrder);
		};

		const auto loopContinues = [&](const Val& loopCount, bool isInOrder)->boost::optional<bool>
		{
			//isInOrder == true
			//loopCountValue <= endVal

			//isInOrder == false
			//loopCountValue > endVal

			const Val result = LessEqualFunc(loopCount, endVal, *pEnv);
			if (!IsType<bool>(result))
			{
				CGL_ErrorNodeInternal(forExpression, "loopCountの評価結果がブール値ではありませんでした。");
			}

			return As<bool>(result) == isInOrder;
		};

		const auto stepOrder = calcStepValue(startVal, endVal);
		if (!stepOrder)
		{
			CGL_ErrorNodeInternal(forExpression, "ループ範囲式の評価結果が不正な値です。");
		}

		const Val step = stepOrder.get().first;
		const bool isInOrder = stepOrder.get().second;

		Val loopCountValue = startVal;

		Val loopResult;

		pEnv->enterScope();

		PackedList loopResults;
		while (true)
		{
			const auto isLoopContinuesOpt = loopContinues(loopCountValue, isInOrder);
			if (!isLoopContinuesOpt)
			{
				CGL_ErrorNodeInternal(forExpression, "loopContinuesの評価結果が不正な値です。");
			}

			//ループの継続条件を満たさなかったので抜ける
			if (!isLoopContinuesOpt.get())
			{
				break;
			}

			pEnv->bindNewValue(forExpression.loopCounter, loopCountValue);

			loopResult = pEnv->expand(boost::apply_visitor(*this, forExpression.doExpr), forExpression);

			if (forExpression.asList)
			{
				loopResults.add(Packed(loopResult, *pEnv));
			}

			//ループカウンタの更新
			loopCountValue = AddFunc(loopCountValue, step, *pEnv);
		}

		pEnv->exitScope();

		if (forExpression.asList)
		{
			return RValue(Unpacked(loopResults, *pEnv));
		}

		return RValue(loopResult);
	}

	LRValue Eval::operator()(const Return& return_statement)
	{
		const Val lhs = pEnv->expand(boost::apply_visitor(*this, return_statement.expr), return_statement);

		return RValue(Jump::MakeReturn(lhs));
	}

	LRValue Eval::operator()(const ListConstractor& listConstractor)
	{
		UpdateCurrentLocation(listConstractor);

		List list;
		for (const auto& expr : listConstractor.data)
		{
			LRValue lrvalue = boost::apply_visitor(*this, expr);
			lrvalue.push_back(list, *pEnv);
		}

		return RValue(list);
	}

	LRValue Eval::operator()(const KeyExpr& node)
	{
		UpdateCurrentLocation(node);

		const LRValue rhs_ = boost::apply_visitor(*this, node.expr);
		Val rhs = pEnv->expand(rhs_, node);
		if (pEnv->existsInCurrentScope(node.name))
		{
			pEnv->bindValueID(node.name, pEnv->makeTemporaryValue(rhs));
		}
		else
		{
			pEnv->bindNewValue(node.name, rhs);
		}

		return RValue(rhs);
	}

	LRValue Eval::operator()(const RecordConstractor& recordConsractor)
	{
		UpdateCurrentLocation(recordConsractor);
		
		//CGL_DebugLog("");
		//pEnv->enterScope();
		//CGL_DebugLog("");

		std::vector<Identifier> keyList;
		/*
		レコード内の:式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合は新たに定義
		レコード内の=式　同じ階層に同名の:式がある場合はそれへの再代入、無い場合はそのスコープ内でのみ有効な値のエイリアスとして定義（スコープを抜けたら元に戻る≒遮蔽）
		*/

		/*for (size_t i = 0; i < recordConsractor.exprs.size(); ++i)
		{
			CGL_DebugLog(std::string("RecordExpr(") + ToS(i) + "): ");
			printExpr(recordConsractor.exprs[i]);
		}*/

		Record newRecord;

		const bool isNewScope = !static_cast<bool>(pEnv->temporaryRecord);

		if (pEnv->temporaryRecord)
		{
			pEnv->currentRecords.push_back(pEnv->temporaryRecord.get());
			pEnv->temporaryRecord = boost::none;
		}
		else
		{
			//現在のレコードが継承でない場合のみスコープを作る
			pEnv->enterScope();
			pEnv->currentRecords.push_back(std::ref(newRecord));
		}

		//Record& record = pEnv->currentRecords.top();
		Record& record = pEnv->currentRecords.back();

		int i = 0;
		
		for (const auto& expr : recordConsractor.exprs)
		{
			//キーに紐づけられる値はこの後の手続きで更新されるかもしれないので、今は名前だけ控えておいて後で値を参照する
			if (IsType<KeyExpr>(expr))
			{
				keyList.push_back(As<KeyExpr>(expr).name);
			}

			Val value = pEnv->expand(boost::apply_visitor(*this, expr), recordConsractor);

			//valueは今は右辺値のみになっている
			//TODO: もう一度考察する
			/*
			//式の評価結果が左辺値の場合は中身も見て、それがマクロであれば中身を展開した結果を式の評価結果とする
			if (IsLValue(value))
			{
				const Val resultValue = pEnv->expandRef(value);
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
		
		pEnv->printContext();

		//各free変数の範囲をまとめたレコードを作成する
		const auto makePackedRanges = [&](std::shared_ptr<Context> pContext, const std::vector<BoundedFreeVar>& freeVars)->std::vector<PackedVal>
		{
			Eval evaluator(pContext);
			std::vector<PackedVal> packedRanges;
			for (const auto& rangeExpr : freeVars)
			{
				if (rangeExpr.freeRange)
				{
					//TODO:ここでrangeの型が妥当か判定すべき（図形かもしくはIntervalでなければはじく）
					if (isDebugMode, false)
					{
						std::cout << "Has range\n";
					}
					packedRanges.push_back(Packed(pContext->expand(boost::apply_visitor(evaluator, rangeExpr.freeRange.get()), recordConsractor), *pContext));
				}
				else
				{
					if (isDebugMode, false)
					{
						std::cout << "Has not range\n";
					}
					packedRanges.push_back(0);
				}
			}
			return packedRanges;
		};

		//declfreeで指定されたアクセッサを全て展開して辿れるアドレスを返す（範囲指定なし）
		/*
		const auto makeFreeVariableAddresses = [&](std::shared_ptr<Context> pContext, const Record& record)
			->std::vector<std::pair<Address, VariableRange>>
		{
			std::vector<std::pair<Address, VariableRange>> freeVariableAddresses;
			for (size_t i = 0; i < record.freeVariables.size(); ++i)
			{
				const auto& accessor = record.freeVariables[i];
				if (IsType<Accessor>(accessor))
				{
					const auto addresses = pContext->expandReferences2(accessor, boost::none, recordConsractor);
					freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
				}
				else if (IsType<LRValue>(accessor) && As<LRValue>(accessor).isReference())
				{
					const auto reference = As<LRValue>(accessor).reference();
					const auto addresses = pContext->expandReferences(pEnv->getReference(reference), boost::none, recordConsractor);
					freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
				}
				//const auto addresses = pContext->expandReferences2(accessor, boost::none, recordConsractor);
				//freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
			}
			return freeVariableAddresses;
		};
		*/

		//declfreeで指定されたアクセッサを全て展開して辿れるアドレスを返す（範囲指定あり）
		/*
		const auto makeFreeVariableAddressesRange = [&](std::shared_ptr<Context> pContext, const Record& record, const std::vector<PackedVal>& packedRanges)
			->std::vector<std::pair<Address, VariableRange>>
		{
			std::vector<std::pair<Address, VariableRange>> freeVariableAddresses;
			for (size_t i = 0; i < record.freeVariables.size(); ++i)
			{
				const auto& accessor = record.freeVariables[i];
				if (IsType<Accessor>(accessor))
				{
					const auto addresses = pContext->expandReferences2(accessor, packedRanges[i], recordConsractor);
					freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
				}
				else if (IsType<LRValue>(accessor) && As<LRValue>(accessor).isReference())
				{
					const auto reference = As<LRValue>(accessor).reference();
					const auto addresses = pContext->expandReferences(pEnv->getReference(reference), packedRanges[i], recordConsractor);
					freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
				}
				//const auto addresses = pContext->expandReferences2(accessor, packedRanges[i], recordConsractor);
				//freeVariableAddresses.insert(freeVariableAddresses.end(), addresses.begin(), addresses.end());
			}
			return freeVariableAddresses;
		};
		*/

		//declfreeで指定されたアクセッサを全て展開して辿れるアドレスを返す（範囲指定なし）
		const auto makeFreeVariableAddresses = [&](std::shared_ptr<Context> pContext, const std::vector<FreeVarType>& freeVariables)
			->std::pair<std::vector<RegionVariable>, std::vector<OptimizeRegion>>
		{
			std::pair<std::vector<RegionVariable>, std::vector<OptimizeRegion>> results;
			auto& resultAddresses = results.first;
			auto& resultRegions = results.second;
			for (size_t i = 0; i < freeVariables.size(); ++i)
			{
				std::vector<RegionVariable> currentResult;
				const auto& accessor = freeVariables[i];
				if (IsType<Accessor>(accessor))
				{
					/*std::cout << "expand accessor: ";
					const Expr expr = As<Accessor>(accessor);
					printExpr2(expr, pContext, std::cout);
					std::cout << "\n";*/

					currentResult = pContext->expandReferences2(As<Accessor>(accessor), recordConsractor);
				}
				else if(IsType<Reference>(accessor))
				{
					/*std::cout << "expand reference: ";
					std::cout << As<Reference>(accessor).toString();
					std::cout << "\n";*/

					currentResult = pContext->expandReferences(pEnv->getReference(As<Reference>(accessor)), recordConsractor);
				}

				const size_t startIndex = resultAddresses.size();
				const size_t numOfIndices = currentResult.size();
				resultAddresses.insert(resultAddresses.end(), currentResult.begin(), currentResult.end());

				OptimizeRegion region;
				region.region = Interval();
				for (const auto& var : currentResult)
				{
					region.addresses.push_back(var.address);
				}
				resultRegions.push_back(region);
			}

			return results;
		};

		//declfreeで指定されたアクセッサを全て展開して辿れるアドレスを返す（範囲指定あり）
		const auto makeFreeVariableAddressesRange = [&](std::shared_ptr<Context> pContext, const std::vector<BoundedFreeVar>& freeVariables,
			const std::vector<PackedVal>& packedRanges)->std::pair<std::vector<RegionVariable>, std::vector<OptimizeRegion>>
		{
			std::pair<std::vector<RegionVariable>, std::vector<OptimizeRegion>> results;
			auto& resultAddresses = results.first;
			auto& resultRegions = results.second;
			for (size_t i = 0; i < freeVariables.size(); ++i)
			{
				std::vector<RegionVariable> currentResult;
				const auto& accessor = freeVariables[i].freeVariable;
				if (IsType<Accessor>(accessor))
				{
					/*std::cout << "expand accessor: ";
					const Expr expr = As<Accessor>(accessor);
					printExpr2(expr, pContext, std::cout);
					std::cout << "\n";*/

					currentResult = pContext->expandReferences2(As<Accessor>(accessor), recordConsractor);
				}
				else if (IsType<Reference>(accessor))
				{
					/*std::cout << "expand reference: ";
					std::cout << As<Reference>(accessor).toString();
					std::cout << "\n";*/

					currentResult = pContext->expandReferences(pEnv->getReference(As<Reference>(accessor)), recordConsractor);
				}

				const size_t startIndex = resultAddresses.size();
				const size_t numOfIndices = currentResult.size();
				resultAddresses.insert(resultAddresses.end(), currentResult.begin(), currentResult.end());

				/*
				RegionVariableは最小のdouble型単位であるのに対し、OptimizeRegionはvar宣言に指定された形がそのまま保持される。
				したがって、各RegionVaribaleごとの範囲はここでは計算せず、実際に評価する前に計算することにする。
				*/
				OptimizeRegion region;
				region.region = packedRanges[i];
				for (const auto& var : currentResult)
				{
					region.addresses.push_back(var.address);
				}
				resultRegions.push_back(region);
			}

			return results;
		};

		//論理積で繋がれた制約をリストに分解して返す
		const auto separateUnitConstraints = [](const Expr& constraint)->std::vector<Expr>
		{
			ConjunctionSeparater separater;
			boost::apply_visitor(separater, constraint);
			return separater.conjunctions;
		};

		//それぞれのunitConstraintについて、出現するアドレスの集合を求めたものを返す
		const auto searchFreeVariablesOfConstraint = [](std::shared_ptr<Context> pContext, const Expr& constraint,
			const std::vector<RegionVariable>& regionVars)->ConstraintAppearance
		{
			//制約ID -> ConstraintAppearance
			ConstraintAppearance appearingList;

			std::vector<char> usedInSat(regionVars.size(), 0);
			std::vector<Address> refs;
			std::unordered_map<Address, int> invRefs;
			bool hasPlateausFunction = false;

			if (isDebugMode, false)
			{
				CGL_DBG1("Expr: ");
				printExpr2(constraint, pContext, std::cout);

				CGL_DBG1("freeVariables: " + ToS(regionVars.size()));
				for (const auto& val : regionVars)
				{
					CGL_DBG1(std::string("  Address(") + val.address.toString() + ")");
				}
			}

			SatVariableBinder binder(pContext, regionVars, usedInSat, refs, appearingList, invRefs, hasPlateausFunction);
			boost::apply_visitor(binder, constraint);

			if (isDebugMode, false)
			{
				CGL_DBG1("appearingList: " + ToS(appearingList.size()));
				for (const auto& a : appearingList)
				{
					CGL_DBG1(std::string("  Address(") + a.toString() + ")");
				}
			}

			return appearingList;
		};

		//二つのAddress集合が共通する要素を持っているかどうかを返す
		const auto intersects = [](const ConstraintAppearance& addressesA, const ConstraintAppearance& addressesB)
		{
			if (addressesA.empty() || addressesB.empty())
			{
				return false;
			}

			for (const Address address : addressesA)
			{
				if (addressesB.find(address) != addressesB.end())
				{
					return true;
				}
			}

			return false;
		};

		//ある制約の変数であるAddress集合が、ある制約グループの変数であるAddress集合と共通するAddressを持っているかどうかを返す
		const auto intersectsToConstraintGroup = [&intersects](const ConstraintAppearance& addressesA, const ConstraintGroup& targetGroup, 
			const std::vector<ConstraintAppearance>& variableAppearances)
		{
			for (const size_t targetConstraintID : targetGroup)
			{
				const auto& targetAppearingAddresses = variableAppearances[targetConstraintID];
				if (intersects(addressesA, targetAppearingAddresses))
				{
					return true;
				}
			}
			return false;
		};

		//各制約に出現するAddressの集合をまとめ、独立な制約の組にグループ分けする
		//variableAppearances = [{a, b, c}, {a, d}, {e, f}, {g, h}]
		//constraintsGroups(variableAppearances) = [{0, 1}, {2}, {3}]
		const auto addConstraintsGroups = [&intersectsToConstraintGroup](std::vector<ConstraintGroup>& constraintsGroups,
			size_t unitConstraintID, const ConstraintAppearance& unitConstraintAppearance, const std::vector<ConstraintAppearance>& variableAppearances)
		{
			std::set<size_t> currentIntersectsGroupIDs;
			for (size_t constraintGroupID = 0; constraintGroupID < constraintsGroups.size(); ++constraintGroupID)
			{
				if (intersectsToConstraintGroup(unitConstraintAppearance, constraintsGroups[constraintGroupID], variableAppearances))
				{
					currentIntersectsGroupIDs.insert(constraintGroupID);
				}
			}

			if (currentIntersectsGroupIDs.empty())
			{
				ConstraintGroup newGroup;
				newGroup.insert(unitConstraintID);
				constraintsGroups.push_back(newGroup);
			}
			else
			{
				ConstraintGroup newGroup;
				newGroup.insert(unitConstraintID);

				for (const size_t groupID : currentIntersectsGroupIDs)
				{
					const auto& targetGroup = constraintsGroups[groupID];
					for (const size_t targetConstraintID : targetGroup)
					{
						newGroup.insert(targetConstraintID);
					}
				}

				for (auto it = currentIntersectsGroupIDs.rbegin(); it != currentIntersectsGroupIDs.rend(); ++it)
				{
					const size_t existingGroupIndex = *it;
					constraintsGroups.erase(constraintsGroups.begin() + existingGroupIndex);
				}

				constraintsGroups.push_back(newGroup);
			}
		};

		const auto mergeConstraintAppearance = [](ConstraintAppearance& a, const ConstraintAppearance& b)
		{
			for (const Address address : b)
			{
				a.insert(address);
			}
		};

		const auto readResult = [](std::shared_ptr<Context> pContext, const std::vector<double>& resultxs, const OptimizationProblemSat& problem)
		{
			for (size_t i = 0; i < resultxs.size(); ++i)
			{
				Address address = problem.freeVariableRefs[i].address;
				//const auto range = problem.freeVariableRefs[i].second;
				//std::cout << "Address(" << address.toString() << "): [" << range.minimum << ", " << range.maximum << "]\n";
				std::cout << "Address(" << address.toString() << "): " << resultxs[i] << "\n";
				pContext->TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, resultxs[i]);
				//pEnv->assignToObject(address, (resultxs[i] - 0.5)*2000.0);
			}
		};

		const auto checkSatisfied = [&](std::shared_ptr<Context> pContext, const OptimizationProblemSat& problem)
		{
			if (problem.expr)
			{
				Eval evaluator(pContext);
				const Val result = pContext->expand(boost::apply_visitor(evaluator, problem.expr.get()), recordConsractor);
				if (IsType<bool>(result))
				{
					return As<bool>(result);
				}
				else if (IsNum(result))
				{
					return EqualFunc(AsDouble(result), 0.0, *pEnv);
				}
				else
				{
					CGL_ErrorNode(recordConsractor, "制約式の評価結果が不正な値です。");
				}

				const bool currentConstraintIsSatisfied = As<bool>(result);
				return currentConstraintIsSatisfied;
			}

			return true;
		};

		//RegionVariablesとOptimizeRegionsについて実際に制約グループに登場するAddressでマスクをかける
		const auto maskedRegionVariables = [&](const std::vector<RegionVariable>& regionVariables, const std::vector<OptimizeRegion>& optimizeRegions,
			const ConstraintAppearance& constraintAppearance)->std::pair<std::vector<RegionVariable>, std::vector<OptimizeRegion>>
		{
			std::vector<RegionVariable> currentConstraintRegionVars;
			for (size_t i = 0; i < regionVariables.size(); ++i)
			{
				if (constraintAppearance.find(regionVariables[i].address) != constraintAppearance.end())
				{
					currentConstraintRegionVars.push_back(regionVariables[i]);
				}
			}

			std::vector<OptimizeRegion> currentConstraintOptimizeRegions;
			for (size_t i = 0; i < optimizeRegions.size(); ++i)
			{
				const auto& originalAddresses = optimizeRegions[i].addresses;

				OptimizeRegion newRegion = optimizeRegions[i];
				newRegion.addresses.clear();
				for (const Address address : optimizeRegions[i].addresses)
				{
					if (constraintAppearance.find(address) != constraintAppearance.end())
					{
						newRegion.addresses.push_back(address);
					}
				}

				if (!newRegion.addresses.empty())
				{
					currentConstraintOptimizeRegions.push_back(newRegion);
				}
			}

			return { currentConstraintRegionVars, currentConstraintOptimizeRegions };
		};

		auto& original = record.original;
		
		//TODO: record.constraintには継承時のoriginalが持つ制約は含まれていないものとする
		if (record.constraint || !original.unitConstraints.empty())
		{
			isInConstraint = true;
			std::cout << "--------------------------------------------------------------\n";
			std::cout << "Solve Record Constraints:" << std::endl;
			record.problems.clear();
			
			std::cout << "  1. Address expansion" << std::endl;
			///////////////////////////////////
			//1. free変数に指定されたアドレスの展開

			const std::vector<PackedVal> recordPackedRanges = makePackedRanges(pEnv, record.boundedFreeVariables);
			std::cout << "recordPackedRanges  : " << recordPackedRanges.size() << std::endl;

			/*{
				std::cout << "margedFreeVars: ";
				for (const auto& var : margedFreeVars)
				{
					if (IsType<Accessor>(var))
					{
						const Expr expr = As<Accessor>(var);
						printExpr(expr, pEnv, std::cout);
					}
					else
					{
						const Expr expr = LRValue(As<Reference>(var));
						printExpr(expr, pEnv, std::cout);
					}
					std::cout << "\n";
				}
			}*/
			auto recordVarAddresses = makeFreeVariableAddressesRange(pEnv, record.boundedFreeVariables, recordPackedRanges);

			if (isDebugMode, false)
			{
				for (const auto& val : recordVarAddresses.first)
				{
					std::cout << "Address(" << val.address.toString() << "): ";
					if (val.attributes.empty())
					{
						std::cout << "No attribute";
					}
					else
					{
						switch (*val.attributes.begin())
						{
						case cgl::RegionVariable::Position:
							std::cout << "Position"; break;
						case cgl::RegionVariable::Scale:
							std::cout << "Scale"; break;
						case cgl::RegionVariable::Angle:
							std::cout << "Angle"; break;
						case cgl::RegionVariable::Other:
							std::cout << "Other"; break;
						default:
							break;
						}
					}
					std::cout << "\n";
				}
				std::cout << "\n";
			}

			std::vector<RegionVariable> mergedRegionVars = original.regionVars;
			std::vector<OptimizeRegion> mergedOptimizeRegions = original.optimizeRegions;
			{
				mergedRegionVars.insert(mergedRegionVars.end(), recordVarAddresses.first.begin(), recordVarAddresses.first.end());
				mergedOptimizeRegions.insert(mergedOptimizeRegions.end(), recordVarAddresses.second.begin(), recordVarAddresses.second.end());
			}

			if (isDebugMode, false)
			{
				for (const OptimizeRegion& r : mergedOptimizeRegions)
				{
					if (IsType<Interval>(r.region))
					{
						auto interval = As<Interval>(r.region);
						std::cout << "Region: [" << interval.minimum << ", " << interval.maximum << "]" << std::endl;
					}
					else if (IsType<PackedVal>(r.region))
					{
						std::cout << "Shape Region" << std::endl;
					}
					else
					{
						CGL_Error("Error");
					}
				}
			}

			std::cout << "  2. Constraints separation" << std::endl;
			///////////////////////////////////
			//2. 変数の依存関係を見て独立した制約を分解
			
			//分解された単位制約
			const std::vector<Expr> adderUnitConstraints = record.constraint ? separateUnitConstraints(record.constraint.get()) : std::vector<Expr>();

			//単位制約ごとの依存するfree変数の集合
			std::vector<ConstraintAppearance> adderVariableAppearances;
			for (const auto& constraint : adderUnitConstraints)
			{
				adderVariableAppearances.push_back(searchFreeVariablesOfConstraint(pEnv, constraint, mergedRegionVars));
				/*std::cout << "constraint:\n";
				printExpr(constraint, pEnv, std::cout);
				std::stringstream ss;
				for (const Address address: adderVariableAppearances.back())
				{
					ss << "Address(" << address.toString() << "), ";
				}
				std::cout << ss.str() << "\n\n";*/
			}

			std::cout << "1 mergedRegionVars.size(): " << mergedRegionVars.size() << std::endl;

			//現在のレコードが継承前の制約を持っているならば、制約が独立かどうかを判定して必要ならば合成を行う
			{
				std::cout << "  3. Dependency analysis" << std::endl;

				//std::vector<Address> addressesOriginal;
				//std::vector<Address> addressesAdder;

				ConstraintAppearance wholeAddressesOriginal, wholeAddressesAdder;
				for (const auto& appearance : original.variableAppearances)
				{
					mergeConstraintAppearance(wholeAddressesOriginal, appearance);
				}
				for (const auto& appearance : adderVariableAppearances)
				{
					mergeConstraintAppearance(wholeAddressesAdder, appearance);
				}

				const size_t adderOffset = original.unitConstraints.size();

				std::vector<Expr> mergedUnitConstraints = original.unitConstraints;
				mergedUnitConstraints.insert(mergedUnitConstraints.end(), adderUnitConstraints.begin(), adderUnitConstraints.end());

				std::vector<ConstraintAppearance> mergedVariableAppearances = original.variableAppearances;
				mergedVariableAppearances.insert(mergedVariableAppearances.end(), adderVariableAppearances.begin(), adderVariableAppearances.end());

#define useDependencyAnalysis
#ifdef useDependencyAnalysis
				if (isDebugMode && record.constraint, false)
				{
					const auto dependencyGraphAnalysis = [&](const Expr& constraint)
					{
						{
							std::cout << "====== Before expansion ======" << std::endl;
							Printer2 printer(pEnv, std::cout, 0);
							boost::apply_visitor(printer, constraint);
							std::cout << std::endl;
						}

						{
							std::cout << "====== After expansion ======" << std::endl;
							Printer2 printer(pEnv, std::cout, 0);
							auto expanded = InlineExpand(constraint, pEnv);
							boost::apply_visitor(printer, expanded);
							std::cout << std::endl;

							std::ofstream graphFile;
							graphFile.open("constraint_CFG.dot");
							
							ConstraintAppearance flattenAddresses;
							for (const auto& appears : mergedVariableAppearances)
							{
								flattenAddresses.insert(appears.begin(), appears.end());
							}

							ConstraintGraph graph = ConstructConstraintGraph(pEnv, expanded);
							graph.addVariableAddresses(*pEnv, flattenAddresses);
							graph.outputGraphViz(graphFile, pEnv);
						}
					};

					if(!mergedUnitConstraints.empty())
					{
						Expr wholeConstraint = mergedUnitConstraints[0];
						
						for (int i = 1; i < mergedUnitConstraints.size(); ++i)
						{
							wholeConstraint = BinaryExpr(wholeConstraint, mergedUnitConstraints[i], BinaryOp::And);
						}

						dependencyGraphAnalysis(wholeConstraint);
					}
				}
#endif

				/*{
					for (const Address address : wholeAddressesOriginal)
					{
						addressesOriginal.push_back(address);
					}
					for (const Address address : wholeAddressesAdder)
					{
						addressesAdder.push_back(address);
					}
					std::sort(addressesOriginal.begin(), addressesOriginal.end());
					std::sort(addressesAdder.begin(), addressesAdder.end());
				}

				{
					std::stringstream ss;
					for (const Address address : addressesOriginal)
					{
						ss << address.toString() << ", ";
					}
					std::cout << "original: ";
					std::cout << ss.str() << "\n";
				}
				{
					std::stringstream ss;
					for (const Address address : addressesAdder)
					{
						ss << address.toString() << ", ";
					}
					std::cout << "adder   : ";
					std::cout << ss.str() << "\n";
				}*/

				//ケースB: 独立でない
				if (intersects(wholeAddressesAdder, wholeAddressesOriginal))
				{
					std::cout << "Case B" << std::endl; 

					std::vector<ConstraintGroup> mergedConstraintGroups = original.constraintGroups;
					for (size_t adderConstraintID = 0; adderConstraintID < adderUnitConstraints.size(); ++adderConstraintID)
					{
						addConstraintsGroups(mergedConstraintGroups, adderOffset + adderConstraintID, adderVariableAppearances[adderConstraintID], mergedVariableAppearances);
					}

					std::cout << "Record constraint separated to " << std::to_string(mergedConstraintGroups.size()) << " independent constraints" << std::endl;

					/*if (mergedConstraintGroups.size() == 1)
					{
						std::cout << "group   : ";
						const auto& group = mergedConstraintGroups.front();
						for (const auto val : group)
						{
							std::cout << val << ", ";
						}
						std::cout << std::endl;
					}*/

					//original.freeVariableAddresses = mergedFreeVariableAddresses;
					original.regionVars = mergedRegionVars;
					original.optimizeRegions = mergedOptimizeRegions;
					original.unitConstraints = mergedUnitConstraints;
					original.variableAppearances = mergedVariableAppearances;
					original.constraintGroups = mergedConstraintGroups;
					
					original.groupConstraints.clear();

					record.problems.resize(mergedConstraintGroups.size());
					for (size_t constraintGroupID = 0; constraintGroupID < mergedConstraintGroups.size(); ++constraintGroupID)
					{
						auto& currentProblem = record.problems[constraintGroupID];
						const auto& currentConstraintIDs = mergedConstraintGroups[constraintGroupID];

						ConstraintAppearance currentGroupDependentAddresses;
						for (size_t constraintID : currentConstraintIDs)
						{
							//currentProblem.addUnitConstraint(adderUnitConstraints[constraintID]);
							currentProblem.addUnitConstraint(mergedUnitConstraints[constraintID]);
							currentGroupDependentAddresses.insert(mergedVariableAppearances[constraintID].begin(), mergedVariableAppearances[constraintID].end());
						}

						const auto maskedVars = maskedRegionVariables(mergedRegionVars, mergedOptimizeRegions, currentGroupDependentAddresses);

						currentProblem.freeVariableRefs = maskedVars.first;
						currentProblem.optimizeRegions = maskedVars.second;

						std::cout << "Current constraint freeVariablesSize: " << std::to_string(currentProblem.freeVariableRefs.size()) << std::endl;
						std::cout << "2 mergedRegionVars.size(): " << mergedRegionVars.size() << "\n";
						std::vector<double> resultxs = currentProblem.solve(pEnv, recordConsractor, record, keyList);

						readResult(pEnv, resultxs, currentProblem);

						const bool currentConstraintIsSatisfied = checkSatisfied(pEnv, currentProblem);
						record.isSatisfied = record.isSatisfied && currentConstraintIsSatisfied;
						if (!currentConstraintIsSatisfied)
						{
							++constraintViolationCount;
						}

						original.groupConstraints.push_back(currentProblem);
					}
				}
				//ケースA: 独立 or originalのみ or adderのみ
				else
				{
					std::cout << "Case A" << std::endl; 
					//独立な場合は、まずoriginalの制約がadderの評価によって満たされなくなっていないかを確認する
					//満たされなくなっていた制約は解きなおす
					for (auto& oldConstraint : original.groupConstraints)
					{
						std::cout << "Old Constraint" << std::endl;
						const Val result = pEnv->expand(boost::apply_visitor(*this, oldConstraint.expr.get()), recordConsractor);
						/*if (!IsType<bool>(result))
						{
							CGL_ErrorNode(recordConsractor, "制約式の評価結果がブール値ではありませんでした。");
						}*/

						//const bool currentConstraintIsSatisfied = As<bool>(result);

						bool currentConstraintIsSatisfied = false;
						if (IsType<bool>(result))
						{
							currentConstraintIsSatisfied = As<bool>(result);
						}
						else if (IsNum(result))
						{
							currentConstraintIsSatisfied = EqualFunc(AsDouble(result), 0.0, *pEnv);
						}
						else
						{
							CGL_ErrorNode(recordConsractor, "制約式の評価結果が不正な値です。");
						}

						if (!currentConstraintIsSatisfied)
						{
							oldConstraint.freeVariableRefs = original.regionVars;
							oldConstraint.optimizeRegions = original.optimizeRegions;

							//TODO: SatVariableBinderをやり直す必要まではない？
							//クローンでずれたアドレスを張り替えるだけで十分かもしれない？
							//oldConstraint.constructConstraint(pEnv);

							//std::cout << "Current constraint freeVariablesSize: " << std::to_string(oldConstraint.freeVariableRefs.size()) << std::endl;

							std::vector<double> resultxs = oldConstraint.solve(pEnv, recordConsractor, record, keyList);

							readResult(pEnv, resultxs, oldConstraint);

							const bool currentConstraintIsSatisfied = checkSatisfied(pEnv, oldConstraint);
							record.isSatisfied = record.isSatisfied && currentConstraintIsSatisfied;
							if (!currentConstraintIsSatisfied)
							{
								++constraintViolationCount;
							}
						}
					}

					std::cout << "New Constraints" << std::endl;

					//次に、新たに追加される制約について解く

					//同じfree変数への依存性を持つ単位制約の組
					std::vector<ConstraintGroup> constraintGroups;
					for (size_t constraintID = 0; constraintID < adderUnitConstraints.size(); ++constraintID)
					{
						addConstraintsGroups(constraintGroups, constraintID, adderVariableAppearances[constraintID], adderVariableAppearances);
					}

					std::vector<ConstraintGroup> mergedConstraintGroups = original.constraintGroups;
					for (const auto& adderGroup : constraintGroups)
					{
						ConstraintGroup newGroup;
						for (size_t adderConstraintID : adderGroup)
						{
							newGroup.insert(adderOffset + adderConstraintID);
						}
						mergedConstraintGroups.push_back(newGroup);
					}

					if (!constraintGroups.empty())
					{
						std::cout << "Record constraint separated to " << std::to_string(constraintGroups.size()) << " independent constraints" << std::endl;
					}

					original.regionVars = mergedRegionVars;
					original.optimizeRegions = mergedOptimizeRegions;
					original.unitConstraints = mergedUnitConstraints;
					original.variableAppearances = mergedVariableAppearances;
					original.constraintGroups = mergedConstraintGroups;

					original.groupConstraints.clear();

					record.problems.resize(constraintGroups.size());
					for (size_t constraintGroupID = 0; constraintGroupID < constraintGroups.size(); ++constraintGroupID)
					{
						auto& currentProblem = record.problems[constraintGroupID];
						const auto& currentConstraintIDs = constraintGroups[constraintGroupID];

						ConstraintAppearance currentGroupDependentAddresses;
						for (size_t constraintID : currentConstraintIDs)
						{
							//CGL_DBG1("Constraint:");
							//printExpr(adderUnitConstraints[constraintID], pEnv, std::cout);
							currentProblem.addUnitConstraint(adderUnitConstraints[constraintID]);
							//ここで現在の制約グループに対応したAddressを引っ張ってくる
							currentGroupDependentAddresses.insert(adderVariableAppearances[constraintID].begin(), adderVariableAppearances[constraintID].end());
						}

						const auto maskedVars = maskedRegionVariables(recordVarAddresses.first, recordVarAddresses.second, currentGroupDependentAddresses);

						currentProblem.freeVariableRefs = maskedVars.first;
						currentProblem.optimizeRegions = maskedVars.second;

						std::vector<double> resultxs = currentProblem.solve(pEnv, recordConsractor, record, keyList);

						readResult(pEnv, resultxs, currentProblem);

						const bool currentConstraintIsSatisfied = checkSatisfied(pEnv, currentProblem);
						record.isSatisfied = record.isSatisfied && currentConstraintIsSatisfied;
						if (!currentConstraintIsSatisfied)
						{
							++constraintViolationCount;
						}

						original.groupConstraints.push_back(currentProblem);
					}
				}
			}

			record.problems.clear();
			record.constraint = boost::none;
			record.boundedFreeVariables.clear();

			isInConstraint = false;
		}
		
		for (const auto& key : keyList)
		{
			//record.append(key.name, pEnv->dereference(key));

			//record.append(key.name, pEnv->makeTemporaryValue(pEnv->dereference(ObjectReference(key))));

			/*Address address = pEnv->findAddress(key);
			boost::optional<const Val&> opt = pEnv->expandOpt(address);
			record.append(key, pEnv->makeTemporaryValue(opt.get()));*/

			Address address = pEnv->findAddress(key);
			record.append(key, address);
		}

		pEnv->printContext();

		CGL_DebugLog("");

		pEnv->currentRecords.pop_back();
		
		if (isNewScope)
		{
			pEnv->exitScope();
		}

		const Address address = pEnv->makeTemporaryValue(record);
		
		//pEnv->garbageCollect();

		CGL_DebugLog("--------------------------- Print Context ---------------------------");
		pEnv->printContext();
		CGL_DebugLog("-------------------------------------------------------------------------");

		//return RValue(record);
		return LRValue(address);
	}

	LRValue Eval::inheritRecord(const LocationInfo& info, const Record& original, const RecordConstractor& adder)
	{
		//(1)オリジナルのレコードaのクローン(a')を作る
		Record clone = As<Record>(Clone(pEnv, original, info));

		CGL_DebugLog("Clone:");
		printVal(clone, pEnv);

		if (pEnv->temporaryRecord)
		{
			CGL_ErrorNodeInternal(info, "二重にレコード継承式を適用しようとしました。temporaryRecordは既に存在しています。");
		}
		pEnv->temporaryRecord = clone;

		//(2) a'の各キーと値に対する参照をローカルスコープに追加する
		pEnv->enterScope();
		for (auto& keyval : clone.values)
		{
			pEnv->makeVariable(keyval.first, keyval.second);

			CGL_DebugLog(std::string("Bind ") + keyval.first + " -> " + "Address(" + keyval.second.toString() + ")");
		}

		//(3) 追加するレコードの中身を評価する
		Expr expr = adder;
		Val recordValue = pEnv->expand(boost::apply_visitor(*this, expr), info);

		//(4) ローカルスコープの参照値を読みレコードに上書きする
		//レコード中のコロン式はレコードの最後でkeylistを見て値が紐づけられるので問題ないが
		//レコード中の代入式については、そのローカル環境の更新を手動でレコードに反映する必要がある
		if (auto opt = AsOpt<Record>(recordValue))
		{
			Record& newRecord = opt.get();
			for (const auto& keyval : clone.values)
			{
				const Address newAddress = pEnv->findAddress(keyval.first);
				if (newAddress.isValid() && newAddress != keyval.second)
				{
					CGL_DebugLog(std::string("Updated ") + keyval.first + ": " + "Address(" + keyval.second.toString() + ") -> Address(" + newAddress.toString() + ")");
					newRecord.values[keyval.first] = newAddress;
				}
			}
		}

		pEnv->exitScope();

		return pEnv->makeTemporaryValue(recordValue);
	}

	LRValue Eval::operator()(const DeclSat& node)
	{
		UpdateCurrentLocation(node);

		//ここでクロージャを作る必要がある
		ClosureMaker closureMaker(pEnv, std::set<std::string>());
		const Expr closedSatExpr = boost::apply_visitor(closureMaker, node.expr);
		//innerSatClosures.push_back(closedSatExpr);

		pEnv->enterScope();
		//DeclSat自体は現在制約が満たされているかどうかを評価結果として返す
		const Val result = pEnv->expand(boost::apply_visitor(*this, closedSatExpr), node);
		pEnv->exitScope();

		if (pEnv->currentRecords.empty())
		{
			CGL_ErrorNode(node, "制約宣言はレコード式の中にしか書くことができません。");
		}

		//pEnv->currentRecords.back().get().problem.addConstraint(closedSatExpr);
		pEnv->currentRecords.back().get().addConstraint(closedSatExpr);

		return RValue(result);
		//return RValue(false);
	}

	LRValue Eval::operator()(const DeclFree& node)
	{
		UpdateCurrentLocation(node);

		//std::cout << "Begin LRValue Eval::operator()(const DeclFree& node)" << std::endl;
		for (const auto& decl : node.accessors)
		{
			//std::cout << "  accessor:" << std::endl;
			if (pEnv->currentRecords.empty())
			{
				CGL_ErrorNode(node, "var宣言はレコード式の中にしか書くことができません。");
			}

			ClosureMaker closureMaker(pEnv, std::set<std::string>());

			const Expr varExpr = decl.accessor;
			const Expr closedVarExpr = boost::apply_visitor(closureMaker, varExpr);

			auto& freeVars = pEnv->currentRecords.back().get().boundedFreeVariables;
			/*std::cout << "VarExpr: ";
			printExpr2(varExpr, pEnv, std::cout);
			std::cout << "\n";
			std::cout << "closedVarExpr: ";
			printExpr2(closedVarExpr,pEnv,std::cout);
			std::cout << "\n";*/

			if (IsType<Accessor>(closedVarExpr))
			{
				//pEnv->currentRecords.back().get().freeVariables.push_back(As<Accessor>(closedVarExpr));
				freeVars.emplace_back(As<Accessor>(closedVarExpr));
			}
			else if (IsType<Identifier>(closedVarExpr))
			{
				Accessor result(closedVarExpr);
				//pEnv->currentRecords.back().get().freeVariables.push_back(result);
				freeVars.emplace_back(result);
			}
			else if (IsType<LRValue>(closedVarExpr) && As<LRValue>(closedVarExpr).isReference())
			{
				//pEnv->currentRecords.back().get().freeVariables.push_back(As<LRValue>(closedVarExpr).reference());
				freeVars.emplace_back(As<LRValue>(closedVarExpr).reference());
			}
			else
			{
				CGL_ErrorNode(node, "var宣言には識別子かアクセッサしか用いることができません。");
			}

			if (decl.range)
			{
				ClosureMaker closureMaker(pEnv, std::set<std::string>());
				const Expr closedRangeExpr = boost::apply_visitor(closureMaker, decl.range.get());
				freeVars.back().freeRange = closedRangeExpr;
			}
		}

		//std::cout << "End LRValue Eval::operator()(const DeclFree& node)" << std::endl;
		return RValue(0);
	}

	LRValue Eval::operator()(const Accessor& accessor)
	{
		Address address;

		if (IsType<Identifier>(accessor.head))
		{
			const auto head = As<Identifier>(accessor.head);
			if (!pEnv->findAddress(head).isValid())
			{
				CGL_ErrorNode(accessor, msgs::Undefined(head));
			}
		}

		LRValue headValue = boost::apply_visitor(*this, accessor.head);
		if (headValue.isLValue() && headValue.deref(*pEnv))
		{
			address = headValue.deref(*pEnv).get();
		}
		else
		{
			Val evaluated = headValue.evaluated();
			if (auto opt = AsOpt<Record>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.get());
			}
			else if (auto opt = AsOpt<List>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.get());
			}
			else if (auto opt = AsOpt<FuncVal>(evaluated))
			{
				address = pEnv->makeTemporaryValue(opt.get());
			}
			else
			{
				CGL_ErrorNodeInternal(accessor, "アクセッサの先頭の評価結果がレコード、リスト、関数のどの値でもありませんでした。");
			}
		}

		for (const auto& access : accessor.accesses)
		{
			//boost::optional<Val&> objOpt = pEnv->expandOpt(address);
			LRValue lrAddress = LRValue(address);
			boost::optional<Val&> objOpt = pEnv->mutableExpandOpt(lrAddress);
			if (!objOpt)
			{
				CGL_ErrorNode(accessor, "アクセッサによる参照先が存在しませんでした。");
			}

			Val& objRef = objOpt.get();

			if (auto listAccessOpt = AsOpt<ListAccess>(access))
			{
				if (!IsType<List>(objRef))
				{
					CGL_ErrorNode(accessor, "リストでない値に対してリストアクセスを行おうとしました。");
				}

				Val value = pEnv->expand(boost::apply_visitor(*this, listAccessOpt.get().index), accessor);

				List& list = As<List>(objRef);

				if (auto indexOpt = AsOpt<int>(value))
				{
					const int indexValue = indexOpt.get();
					const int listSize = static_cast<int>(list.data.size());
					const int maxIndex = listSize - 1;

					if (0 <= indexValue && indexValue <= maxIndex)
					{
						address = list.get(indexValue);
					}
					else if (indexValue < 0 || !pEnv->isAutomaticExtendMode())
					{
						CGL_ErrorNode(accessor, "リストの範囲外アクセスが発生しました。");
					}
					else
					{
						while (static_cast<int>(list.data.size()) - 1 < indexValue)
						{
							list.data.push_back(pEnv->makeTemporaryValue(0));
						}
						address = list.get(indexValue);
					}
				}
				else
				{
					CGL_ErrorNode(accessor, "リストアクセスのインデックスが整数値ではありませんでした。");
				}
			}
			else if (auto recordAccessOpt = AsOpt<RecordAccess>(access))
			{
				if (!IsType<Record>(objRef))
				{
					CGL_ErrorNode(accessor, "レコードでない値に対してレコードアクセスを行おうとしました。");
				}

				const Record& record = As<Record>(objRef);
				auto it = record.values.find(recordAccessOpt.get().name);
				if (it == record.values.end())
				{
					CGL_ErrorNode(accessor, std::string("指定された識別子\"") + recordAccessOpt.get().name.toString() + "\"がレコード中に存在しませんでした。");
				}

				address = it->second;
			}
			else if (auto recordAccessOpt = AsOpt<FunctionAccess>(access))
			{
				if (!IsType<FuncVal>(objRef))
				{
					CGL_ErrorNode(accessor, "関数でない値に対して関数呼び出しを行おうとしました。");
				}

				auto funcAccess = As<FunctionAccess>(access);

				const FuncVal& function = As<FuncVal>(objRef);

				/*
				std::vector<Val> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					args.push_back(pEnv->expand(boost::apply_visitor(*this, expr)));
				}
					
				Expr caller = FunctionCaller(function, args);
				//const Val returnedValue = boost::apply_visitor(*this, caller);
				const Val returnedValue = pEnv->expand(boost::apply_visitor(*this, caller));
				address = pEnv->makeTemporaryValue(returnedValue);
				*/

				std::vector<Address> args;
				for (const auto& expr : funcAccess.actualArguments)
				{
					const LRValue currentArgument = boost::apply_visitor(*this, expr);
					currentArgument.push_back(args, *pEnv);
				}

				const Val returnedValue = pEnv->expand(callFunction(accessor, function, args), accessor);
				address = pEnv->makeTemporaryValue(returnedValue);
			}
			else
			{
				if (!IsType<Record>(objRef))
				{
					CGL_ErrorNode(accessor, "レコードでない値に対して継承式を適用しようとしました。");
				}

				auto inheritAccess = As<InheritAccess>(access);

				const Record& record = As<Record>(objRef);
				const Val returnedValue = pEnv->expand(inheritRecord(accessor, record, inheritAccess.adder), accessor);
				address = pEnv->makeTemporaryValue(returnedValue);
			}
		}

		return LRValue(address);
	}

	boost::optional<const Val&> evalExpr(const Expr& expr)
	{
		auto env = Context::Make();

		Eval evaluator(env);

		boost::optional<const Val&> result = env->expandOpt(boost::apply_visitor(evaluator, expr));

		//std::cout << "Context:\n";
		//env->printContext();

		//std::cout << "Result Evaluation:\n";
		//printVal(result, env);

		return result;
	}

	bool IsEqualVal(const Val& value1, const Val& value2)
	{
		if (!SameType(value1.type(), value2.type()))
		{
			if ((IsType<int>(value1) || IsType<double>(value1)) && (IsType<int>(value2) || IsType<double>(value2)))
			{
				const Val d1 = IsType<int>(value1) ? static_cast<double>(As<int>(value1)) : As<double>(value1);
				const Val d2 = IsType<int>(value2) ? static_cast<double>(As<int>(value2)) : As<double>(value2);
				return IsEqualVal(d1, d2);
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
				const bool result = std::abs(d1 - d2) < eps;
				std::cerr << std::setprecision(17);
				std::cerr << "    " << d1 << " == " << d2 << " => " << (result ? "Maybe true" : "Maybe false") << std::endl;
				std::cerr << std::setprecision(6);
				return result;
			}
			//return As<double>(value1) == As<double>(value2);
		}
		else if (IsType<CharString>(value1))
		{
			return As<CharString>(value1) == As<CharString>(value2);
		}
		else if (IsType<List>(value1))
		{
			//return As<List>(value1) == As<List>(value2);
			CGL_Error("TODO:未対応");
			return false;
		}
		else if (IsType<KeyValue>(value1))
		{
			return As<KeyValue>(value1) == As<KeyValue>(value2);
		}
		else if (IsType<Record>(value1))
		{
			//return As<Record>(value1) == As<Record>(value2);
			CGL_Error("TODO:未対応");
			return false;
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

		std::cerr << "IsEqualVal: Type Error" << std::endl;
		return false;
	}

	bool IsEqual(const Expr& value1, const Expr& value2)
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
			return IsEqualVal(As<RValue>(value1).value, As<RValue>(value2).value);
		}*/
		if (IsType<LRValue>(value1))
		{
			const LRValue& lrvalue1 = As<LRValue>(value1);
			const LRValue& lrvalue2 = As<LRValue>(value2);
			if (lrvalue1.isLValue() && lrvalue2.isLValue())
			{
				return lrvalue1 == lrvalue2;
			}
			else if (lrvalue1.isRValue() && lrvalue2.isRValue())
			{
				return IsEqualVal(lrvalue1.evaluated(), lrvalue2.evaluated());
			}
			std::cerr << "Values are not same type." << std::endl;
			return false;
		}
		else if (IsType<Identifier>(value1))
		{
			return As<Identifier>(value1) == As<Identifier>(value2);
		}
		else if (IsType<Import>(value1))
		{
			//return As<SatReference>(value1) == As<SatReference>(value2);
			return false;
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
}
