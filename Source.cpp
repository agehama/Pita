#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include "Node.hpp"
#include "Evaluator.hpp"
#include "Printer.hpp"
#include "Environment.hpp"

namespace cgl
{
	//要考察：satにも複数の値を省略して指定できるようにするためには、ここ(Identifier/Accessor)で複数の値を扱えるようにする必要がある
	SatExpr Expr2SatExpr::operator()(const Identifier& node)
	{
		ObjectReference currentRefVal(node);

		for (size_t i = 0; i < freeVariables.size(); ++i)
		{
			//freeVariablesに存在した場合は、最適化用の変数を一つ作り、その参照を返す
			//また、freeVariables側にもその変数を使用することを知らせる
			if (freeVariables[i] == currentRefVal)
			{
				if (usedInSat[i] == 1)
				{
					return satRefs[i];
				}
				else
				{
					usedInSat[i] = 1;

					SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
					satRefs[i] = satRef;

					refs.push_back(currentRefVal);
					std::cout << "NewRef(" << satRef.refID << ")\n";
					return satRef;
				}
			}
		}

		//freeVariablesに存在しなかった場合は、即座に評価してよい（定数式の畳み込み）
		const Evaluated evaluated = pEnv->dereference(currentRefVal);

		if (IsType<double>(evaluated))
		{
			return As<double>(evaluated);
		}
		else if (IsType<int>(evaluated))
		{
			return static_cast<double>(As<int>(evaluated));
		}

		//int/doubleじゃない場合はとりあえずエラー。boolは有り得る？
		std::cerr << "Error(" << __LINE__ << ")\n";
		return 0.0;
	}

	SatExpr Expr2SatExpr::operator()(const Accessor& node)
	{
		Expr expr = node;
		Eval evaluator(pEnv);
		const Evaluated refVal = boost::apply_visitor(evaluator, expr);

		if (!IsType<ObjectReference>(refVal))
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return 0.0;
		}

		const ObjectReference& currentRefVal = As<ObjectReference>(refVal);

		for (size_t i = 0; i < freeVariables.size(); ++i)
		{
			//freeVariablesに存在した場合は、最適化用の変数を一つ作り、その参照を返す
			//また、freeVariables側にもその変数を使用することを知らせる
			if (freeVariables[i] == currentRefVal)
			{
				if (usedInSat[i] == 1)
				{
					return satRefs[i];
				}
				else
				{
					usedInSat[i] = 1;
					
					SatReference satRef(refID_Offset + static_cast<int>(refs.size()));
					satRefs[i] = satRef;

					refs.push_back(currentRefVal);
					std::cout << "NewRef(" << satRef.refID << ")\n";
					return satRef;
				}
			}
		}

		//freeVariablesに存在しなかった場合は、即座に評価してよい（定数式の畳み込み）
		const Evaluated evaluated = pEnv->dereference(currentRefVal);

		if(IsType<double>(evaluated))
		{
			return As<double>(evaluated);
		}
		else if (IsType<int>(evaluated))
		{
			return static_cast<double>(As<int>(evaluated));
		}

		//int/doubleじゃない場合はとりあえずエラー。boolは有り得る？
		std::cerr << "Error(" << __LINE__ << ")\n";
		return 0.0;
	}

	const Evaluated& Environment::dereference(const Evaluated& reference)
	{
		if (auto nameOpt = AsOpt<Identifier>(reference))
		{
			const boost::optional<unsigned> valueIDOpt = findValueID(nameOpt.value().name);
			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return reference;
			}

			return m_values[valueIDOpt.value()];
		}
		else if (auto objRefOpt = AsOpt<ObjectReference>(reference))
		{
			const auto& referenceProcess = objRefOpt.value();

			boost::optional<unsigned> valueIDOpt;

			if (auto opt = AsOpt<Identifier>(referenceProcess.headValue))
			{
				valueIDOpt = findValueID(opt.value().name);
			}
			else if (auto opt = AsOpt<Record>(referenceProcess.headValue))
			{
				valueIDOpt = makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<List>(referenceProcess.headValue))
			{
				valueIDOpt = makeTemporaryValue(opt.value());
			}
			else if (auto opt = AsOpt<FuncVal>(referenceProcess.headValue))
			{
				valueIDOpt = makeTemporaryValue(opt.value());
			}

			if (!valueIDOpt)
			{
				std::cerr << "Error(" << __LINE__ << ")\n";
				return reference;
			}

			std::cout << "Reference: " << objRefOpt.value().asString() << "\n";

			boost::optional<const Evaluated&> result = m_values[valueIDOpt.value()];

			for (const auto& ref : referenceProcess.references)
			{
				if (auto listRefOpt = AsOpt<ObjectReference::ListRef>(ref))
				{
					const int index = listRefOpt.value().index;

					if (auto listOpt = AsOpt<List>(result.value()))
					{
						result = listOpt.value().data[index];
					}
					else
					{
						//リストとしてアクセスするのに失敗
						std::cerr << "Error(" << __LINE__ << ")\n";
						return reference;
					}
				}
				else if (auto recordRefOpt = AsOpt<ObjectReference::RecordRef>(ref))
				{
					const std::string& key = recordRefOpt.value().key;

					if (auto recordOpt = AsOpt<Record>(result.value()))
					{
						result = recordOpt.value().values.at(key);
					}
					else
					{
						//レコードとしてアクセスするのに失敗
						std::cerr << "Error(" << __LINE__ << ")\n";
						return reference;
					}
				}
				else if (auto funcRefOpt = AsOpt<ObjectReference::FunctionRef>(ref))
				{
					if (auto recordOpt = AsOpt<FuncVal>(result.value()))
					{
						Expr caller = FunctionCaller(recordOpt.value(), funcRefOpt.value().args);

						if (auto sharedThis = m_weakThis.lock())
						{
							Eval evaluator(sharedThis);

							const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
							result = m_values[ID];
						}
						else
						{
							//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
							std::cerr << "Error(" << __LINE__ << ")\n";
							return reference;
						}
					}
					else
					{
						//関数としてアクセスするのに失敗
						std::cerr << "Error(" << __LINE__ << ")\n";
						return reference;
					}
				}
			}

			return result.value();
		}

		return reference;
	}

	boost::optional<ObjectReference> Environment::evalReference(const Accessor & access)
	{
		if (auto sharedThis = m_weakThis.lock())
		{
			Eval evaluator(sharedThis);

			const Expr accessor = access;
			const Evaluated refVal = boost::apply_visitor(evaluator, accessor);

			if (!IsType<ObjectReference>(refVal))
			{
				//存在しない参照をsatに指定した
				std::cerr << "Error(" << __LINE__ << "): accessor was not reference.\n";
				return boost::none;
			}

			return As<ObjectReference>(refVal);
		}

		std::cerr << "Error(" << __LINE__ << "): shared this does not exist.\n";

		return boost::none;
	}

	Expr Environment::expandFunction(const Expr & expr)
	{
		return expr;
	}

	std::vector<ObjectReference> Environment::expandReferences(const ObjectReference & reference, std::vector<ObjectReference>& output)
	{
		if (auto sharedThis = m_weakThis.lock())
		{
			const auto addElementRec = [&](auto rec, const ObjectReference& refVal)->void
			{
				const Evaluated value = sharedThis->dereference(refVal);

				if (IsType<int>(value) || IsType<double>(value) /*|| IsType<bool>(value)*/)//TODO:boolは将来的に対応
				{
					output.push_back(refVal);
				}
				else if (IsType<List>(value))
				{
					const auto& list = As<List>(value).data;
					for (size_t i = 0; i < list.size(); ++i)
					{
						ObjectReference newRef = refVal;
						newRef.appendListRef(i);
						rec(rec, newRef);
					}
				}
				else if (IsType<Record>(value))
				{
					for (const auto& elem : As<Record>(value).values)
					{
						ObjectReference newRef = refVal;
						newRef.appendRecordRef(elem.first);
						rec(rec, newRef);
					}
				}
				else
				{
					std::cerr << "未対応";
					//TODO:最終的にintやdouble 以外のデータへの参照は持つことにするか？
				}
			};

			const auto addElement = [&](const ObjectReference& refVal)
			{
				addElementRec(addElementRec, refVal);
			};

			addElement(reference);
		}

		return output;
	}

	inline void Environment::assignToObject(const ObjectReference & objectRef, const Evaluated & newValue)
	{
		boost::optional<unsigned> valueIDOpt;

		if (auto opt = AsOpt<Identifier>(objectRef.headValue))
		{
			valueIDOpt = findValueID(opt.value().name);
		}
		else if (auto opt = AsOpt<Record>(objectRef.headValue))
		{
			valueIDOpt = makeTemporaryValue(opt.value());
		}
		else if (auto opt = AsOpt<List>(objectRef.headValue))
		{
			valueIDOpt = makeTemporaryValue(opt.value());
		}
		else if (auto opt = AsOpt<FuncVal>(objectRef.headValue))
		{
			valueIDOpt = makeTemporaryValue(opt.value());
		}

		if (!valueIDOpt)
		{
			std::cerr << "Error(" << __LINE__ << ")\n";
			return;
		}

		boost::optional<Evaluated&> result = m_values[valueIDOpt.value()];

		for (const auto& ref : objectRef.references)
		{
			if (auto listRefOpt = AsOpt<ObjectReference::ListRef>(ref))
			{
				const int index = listRefOpt.value().index;

				if (auto listOpt = AsOpt<List>(result.value()))
				{
					result = listOpt.value().data[index];
				}
				else//リストとしてアクセスするのに失敗
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}
			}
			else if (auto recordRefOpt = AsOpt<ObjectReference::RecordRef>(ref))
			{
				const std::string& key = recordRefOpt.value().key;

				if (auto recordOpt = AsOpt<Record>(result.value()))
				{
					result = recordOpt.value().values.at(key);
				}
				else//レコードとしてアクセスするのに失敗
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}
			}
			else if (auto funcRefOpt = AsOpt<ObjectReference::FunctionRef>(ref))
			{
				if (auto recordOpt = AsOpt<FuncVal>(result.value()))
				{
					Expr caller = FunctionCaller(recordOpt.value(), funcRefOpt.value().args);

					if (auto sharedThis = m_weakThis.lock())
					{
						Eval evaluator(sharedThis);
						const unsigned ID = m_values.add(boost::apply_visitor(evaluator, caller));
						result = m_values[ID];
					}
					else//エラー：m_weakThisが空（Environment::Makeを使わず初期化した？）
					{
						std::cerr << "Error(" << __LINE__ << ")\n";
						return;
					}
				}
				else//関数としてアクセスするのに失敗
				{
					std::cerr << "Error(" << __LINE__ << ")\n";
					return;
				}
			}
		}

		result.value() = newValue;
	}

	void OptimizationProblemSat::addConstraint(const Expr& logicExpr)
	{
		if (candidateExpr)
		{
			candidateExpr = BinaryExpr(candidateExpr.value(), logicExpr, BinaryOp::And);
		}
		else
		{
			candidateExpr = logicExpr;
		}
	}

	void OptimizationProblemSat::constructConstraint(std::shared_ptr<Environment> pEnv, std::vector<ObjectReference>& freeVariables)
	{
		if (!candidateExpr)
		{
			expr = boost::none;
			return;
		}

		Expr2SatExpr evaluator(0, pEnv, freeVariables);
		expr = boost::apply_visitor(evaluator, candidateExpr.value());
		refs.insert(refs.end(), evaluator.refs.begin(), evaluator.refs.end());
		
		{
			std::cout << "Print1:\n";
			PrintSatExpr printer(data);
			boost::apply_visitor(printer, expr.value());
			std::cout << "\n";
		}

		//satに出てこないfreeVariablesの削除
		for (int i = static_cast<int>(freeVariables.size()) - 1; 0 <= i; --i)
		{
			if (evaluator.usedInSat[i] == 0)
			{
				freeVariables.erase(freeVariables.begin() + i);
			}
		}
	}

	bool OptimizationProblemSat::initializeData(std::shared_ptr<Environment> pEnv)
	{
		//std::cout << "Begin OptimizationProblemSat::initializeData" << std::endl;

		data.resize(refs.size());

		for (size_t i = 0; i < data.size(); ++i)
		{
			const Evaluated val = pEnv->dereference(refs[i]);
			if (auto opt = AsOpt<double>(val))
			{
				std::cout << "    " << i << " : " << opt.value() << std::endl;
				data[i] = opt.value();
			}
			else if (auto opt = AsOpt<int>(val))
			{
				std::cout << "    " << i << " : " << opt.value() << std::endl;
				data[i] = opt.value();
			}
			else
			{
				//存在しない参照をsatに指定した
				std::cerr << "Error(" << __LINE__ << "): reference does not exist.\n";
				return false;
			}
		}

		//std::cout << "End OptimizationProblemSat::initializeData" << std::endl;
		return true;
	}

	double OptimizationProblemSat::eval()
	{
		if (!expr)
		{
			return 0.0;
		}

		EvalSatExpr evaluator(data);
		return boost::apply_visitor(evaluator, expr.value());
	}

	void OptimizationProblemSat::debugPrint()
	{
		if (!expr)
		{
			return;
		}

		PrintSatExpr evaluator(data);
		boost::apply_visitor(evaluator, expr.value());
	}
}

namespace cgl
{
	auto MakeUnaryExpr(UnaryOp op)
	{
		return boost::phoenix::bind([](const auto & e, UnaryOp op) {
			return UnaryExpr(e, op);
		}, boost::spirit::_1, op);
	}

	auto MakeBinaryExpr(BinaryOp op)
	{
		return boost::phoenix::bind([&](const auto& lhs, const auto& rhs, BinaryOp op) {
			return BinaryExpr(lhs, rhs, op);
		}, boost::spirit::_val, boost::spirit::_1, op);
	}

	template <class F, class... Args>
	auto Call(F func, Args... args)
	{
		return boost::phoenix::bind(func, args...);
	}

	template<class FromT, class ToT>
	auto Cast()
	{
		return boost::phoenix::bind([&](const FromT& a) {return static_cast<ToT>(a); }, boost::spirit::_1);
	}

	using namespace boost::spirit;

	template<typename Iterator>
	struct SpaceSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		SpaceSkipper() :SpaceSkipper::base_type(skip)
		{
			skip = +(lit(' ') ^ lit('\r') ^ lit('\t'));
		}
	};

	template<typename Iterator>
	struct LineSkipper : public qi::grammar<Iterator>
	{
		qi::rule<Iterator> skip;

		LineSkipper() :LineSkipper::base_type(skip)
		{
			skip = ascii::space;
		}
	};

	struct keywords_t : qi::symbols<char, qi::unused_type> {
		keywords_t() {
			add("for", qi::unused)
				("in", qi::unused)
				("sat", qi::unused)
				("free", qi::unused);
		}
	} const keywords;

	using IteratorT = std::string::const_iterator;
	using SpaceSkipperT = SpaceSkipper<IteratorT>;
	using LineSkipperT = LineSkipper<IteratorT>;

	SpaceSkipperT spaceSkipper;
	LineSkipperT lineSkipper;

	using qi::char_;

	template<typename Iterator, typename Skipper>
	struct Parser
		: qi::grammar<Iterator, Lines(), Skipper>
	{
		qi::rule<Iterator, DeclSat(), Skipper> constraints;
		qi::rule<Iterator, DeclFree(), Skipper> freeVals;

		qi::rule<Iterator, FunctionAccess(), Skipper> functionAccess;
		qi::rule<Iterator, RecordAccess(), Skipper> recordAccess;
		qi::rule<Iterator, ListAccess(), Skipper> listAccess;
		qi::rule<Iterator, Accessor(), Skipper> accessor;

		qi::rule<Iterator, Access(), Skipper> access;

		qi::rule<Iterator, KeyExpr(), Skipper> record_keyexpr;
		qi::rule<Iterator, RecordConstractor(), Skipper> record_maker;
		qi::rule<Iterator, RecordInheritor(), Skipper> record_inheritor;

		qi::rule<Iterator, ListConstractor(), Skipper> list_maker;
		qi::rule<Iterator, For(), Skipper> for_expr;
		qi::rule<Iterator, If(), Skipper> if_expr;
		qi::rule<Iterator, Return(), Skipper> return_expr;
		qi::rule<Iterator, DefFunc(), Skipper> def_func;
		qi::rule<Iterator, Arguments(), Skipper> arguments;
		qi::rule<Iterator, Identifier(), Skipper> id;
		qi::rule<Iterator, Expr(), Skipper> general_expr, logic_expr, logic_term, logic_factor, compare_expr, arith_expr, basic_arith_expr, term, factor, pow_term, pow_term1;
		qi::rule<Iterator, Lines(), Skipper> expr_seq, statement;
		qi::rule<Iterator, Lines(), Skipper> program;

		//qi::rule<Iterator, std::string(), Skipper> double_value;
		//qi::rule<Iterator, Identifier(), Skipper> double_value, double_value2;

		qi::rule<Iterator> s, s1;
		qi::rule<Iterator> distinct_keyword;
		qi::rule<Iterator, std::string(), Skipper> unchecked_identifier;
		
		Parser() : Parser::base_type(program)
		{
			auto concatLines = [](Lines& lines, Expr& expr) { lines.concat(expr); };

			auto makeLines = [](Expr& expr) { return Lines(expr); };

			auto makeDefFunc = [](const Arguments& arguments, const Expr& expr) { return DefFunc(arguments, expr); };

			auto addCharacter = [](Identifier& identifier, char c) { identifier.name.push_back(c); };

			auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };

			auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

			auto makeDouble = [](const Identifier& str) { return std::stod(str.name); };
			auto makeString = [](char c) { return Identifier(std::string({ c })); };
			auto appendString = [](Identifier& str, char c) { str.name.push_back(c); };
			auto appendString2 = [](Identifier& str, const Identifier& str2) { str.name.append(str2.name); };

			program = s >> -(expr_seq) >> s;

			/*expr_seq = general_expr[_val = Call(makeLines, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(concatLines, _val, _1)])
				| (+(lit('\n')) >> general_expr[Call(concatLines, _val, _1)])
				);*/

			expr_seq = statement[_val = _1] >> *(
				+(lit('\n')) >> statement[Call(Lines::Concat, _val, _1)]
				);

			statement = general_expr[_val = Call(Lines::Make, _1)] >> *(
				(s >> ',' >> s >> general_expr[Call(Lines::Append, _val, _1)])
				| (lit('\n') >> general_expr[Call(Lines::Append, _val, _1)])
				);

			general_expr =
				if_expr[_val = _1]
				| return_expr[_val = _1]
				| logic_expr[_val = _1];

			if_expr = lit("if") >> s >> general_expr[_val = Call(If::Make, _1)]
				>> s >> lit("then") >> s >> general_expr[Call(If::SetThen, _val, _1)]
				>> -(s >> lit("else") >> s >> general_expr[Call(If::SetElse, _val, _1)])
				;

			for_expr = lit("for") >> s >> id[_val = Call(For::Make, _1)] >> s >> lit("in")
				>> s >> general_expr[Call(For::SetRangeStart, _val, _1)] >> s >> lit(":")
				>> s >> general_expr[Call(For::SetRangeEnd, _val, _1)] >> s >> lit("do")
				>> s >> general_expr[Call(For::SetDo, _val, _1)];

			return_expr = lit("return") >> s >> general_expr[_val = Call(Return::Make, _1)];

			//def_func = arguments[_val = _1] >> lit("->") >> s >> statement[Call(applyFuncDef, _val, _1)];
			def_func = arguments[_val = _1] >> lit("->") >> s >> general_expr[Call(applyFuncDef, _val, _1)];

			//constraintはDNFの形で与えられるものとする
			constraints = lit("sat") >> '(' >> s >> logic_expr[_val = Call(DeclSat::Make, _1)] >> s >> ')';

			//freeValsがレコードへの参照とかを入れるのは少し大変だが、単一の値への参照なら難しくないはず
			/*freeVals = lit("free") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddIdentifier, _val, _1)]) >> *(
				s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddIdentifier, _val, _1)])
				) >> s >> ')';*/
			/*freeVals = lit("free") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, _1)]) >> *(
				s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, _1)])
				) >> s >> ')';*/

			freeVals = lit("free") >> '(' >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())]) >> *(
				s >> ", " >> s >> (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())])
				) >> s >> ')';

			/*freeVals = lit("free") >> '(' >> s >> id[Call(DeclFree::AddIdentifier, _val, _1)] >> *(
				s >> "," >> s >> id[Call(DeclFree::AddIdentifier, _val, _1)]
				) >> s >> ')';*/

			arguments = -(id[_val = _1] >> *(s >> ',' >> s >> arguments[Call(concatArguments, _val, _1)]));

			logic_expr = logic_term[_val = _1] >> *(s >> '|' >> s >> logic_term[_val = MakeBinaryExpr(BinaryOp::Or)]);

			logic_term = logic_factor[_val = _1] >> *(s >> '&' >> s >> logic_factor[_val = MakeBinaryExpr(BinaryOp::And)]);

			logic_factor = ('!' >> s >> compare_expr[_val = MakeUnaryExpr(UnaryOp::Not)])
				| compare_expr[_val = _1]
				;

			compare_expr = arith_expr[_val = _1] >> *(
				(s >> lit("==") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Equal)])
				| (s >> lit("!=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::NotEqual)])
				| (s >> lit("<") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessThan)])
				| (s >> lit("<=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessEqual)])
				| (s >> lit(">") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterThan)])
				| (s >> lit(">=") >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterEqual)])
				)
				;

			//= ^ -> は右結合

			arith_expr = (basic_arith_expr[_val = _1] >> -(s >> '=' >> s >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Assign)]));

			basic_arith_expr = term[_val = _1] >>
				*((s >> '+' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Add)]) |
				(s >> '-' >> s >> term[_val = MakeBinaryExpr(BinaryOp::Sub)]))
				;

			term = pow_term[_val = _1]
				| (factor[_val = _1] >>
					*((s >> '*' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Mul)]) |
					(s >> '/' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Div)]))
					)
				;
			
			//最低でも1つは受け取るようにしないと、単一のfactorを受理できてしまうのでMul,Divの方に行ってくれない
			pow_term = factor[_val = _1] >> s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)];
			pow_term1 = factor[_val = _1] >> -(s >> '^' >> s >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)]);

			//record{} の間には改行は挟めない（record,{}と区別できなくなるので）
			record_inheritor = id[_val = Call(RecordInheritor::Make, _1)] >> record_maker[Call(RecordInheritor::AppendRecord, _val, _1)];

			record_maker = (
				char_('{') >> s >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]) >>
				*(
				(s >> ',' >> s >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]))
					| (+(char_('\n')) >> (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]))
					)
				>> s >> char_('}')
				)
				| (char_('{') >> s >> char_('}'));

			//レコードの name:val の name と : の間に改行を許すべきか？ -> 許しても解析上恐らく問題はないが、意味があまりなさそう
			record_keyexpr = id[_val = Call(KeyExpr::Make, _1)] >> char_(':') >> s >> general_expr[Call(KeyExpr::SetExpr, _val, _1)];

			/*list_maker = (char_('[') >> s >> general_expr[_val = Call(ListConstractor::Make, _1)] >>
				*(s >> char_(',') >> s >> general_expr[Call(ListConstractor::Append, _val, _1)]) >> s >> char_(']')
				)
				| (char_('[') >> s >> char_(']'));*/

			list_maker = (char_('[') >> s >> general_expr[_val = Call(ListConstractor::Make, _1)] >>
				*(
					(s >> char_(',') >> s >> general_expr[Call(ListConstractor::Append, _val, _1)])
					| (+(char_('\n')) >> general_expr[Call(ListConstractor::Append, _val, _1)])
					) >> s >> char_(']')
				)
				| (char_('[') >> s >> char_(']'));
			
			accessor = (id[_val = Call(Accessor::Make, _1)] >> +(access[Call(Accessor::Append, _val, _1)]))
				| (list_maker[_val = Call(Accessor::Make, _1)] >> listAccess[Call(Accessor::AppendList, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]))
				| (record_maker[_val = Call(Accessor::Make, _1)] >> recordAccess[Call(Accessor::AppendRecord, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]));
			
			access = functionAccess[_val = Cast<FunctionAccess, Access>()]
				| listAccess[_val = Cast<ListAccess, Access>()]
				| recordAccess[_val = Cast<RecordAccess, Access>()];

			recordAccess = char_('.') >> s >> id[_val = Call(RecordAccess::Make, _1)];

			listAccess = char_('[') >> s >> general_expr[Call(ListAccess::SetIndex, _val, _1)] >> s >> char_(']');

			functionAccess = char_('(')
				>> -(s >> general_expr[Call(FunctionAccess::Append, _val, _1)])
				>> *(s >> char_(',') >> s >> general_expr[Call(FunctionAccess::Append, _val, _1)]) >> s >> char_(')');

			factor = /*double_[_val = _1]
				| */int_[_val = _1]
				| lit("true")[_val = true]
				| lit("false")[_val = false]
				| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
				| '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
				| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
				| constraints[_val = _1]
				| freeVals[_val = _1]
				| accessor[_val = _1]
				| def_func[_val = _1]
				| for_expr[_val = _1]
				| list_maker[_val = _1]
				| record_inheritor[_val = _1]
				//| (id >> record_maker)[_val = Call(RecordInheritor::MakeRecord, _1,_2)]
				| record_maker[_val = _1]
				| id[_val = _1];

			//idの途中には空白を含めない
			//id = lexeme[ascii::alpha[_val = _1] >> *(ascii::alnum[Call(addCharacter, _val, _1)])];
			//id = identifier_def[_val = _1];
			id = unchecked_identifier[_val = _1] - distinct_keyword;

			distinct_keyword = qi::lexeme[keywords >> !(qi::alnum | '_')];
			unchecked_identifier = qi::lexeme[(qi::alpha | qi::char_('_')) >> *(qi::alnum | qi::char_('_'))];

			/*auto const distinct_keyword = qi::lexeme[keywords >> !(qi::alnum | '_')];
			auto const unchecked_identifier = qi::lexeme[(qi::alpha | qi::char_('_')) >> *(qi::alnum | qi::char_('_'))];
			auto const identifier_def = unchecked_identifier - distinct_keyword;*/

			s = *(ascii::space);
			/*s = -(ascii::space) >> -(s1);
			s1 = ascii::space >> -(s1);*/

			//double_ だと 1. とかもパースできてしまうせいでレンジのパースに支障が出るので別に定義する
			//double_value = lexeme[qi::char_('1', '9') >> *(ascii::digit) >> lit(".") >> +(ascii::digit)  [_val = Call(makeDouble, _1)]];
			/*double_value = lexeme[qi::char_('1', '9')[_val = Call(makeString, _1)] >> *(ascii::digit[Call(appendString, _val, _1)])
				>> lit(".")[Call(appendString, _val, _1)] >> +(ascii::digit[Call(appendString, _val, _1)])];*/
			/*double_value = lexeme[qi::char_('1', '9')[_val = _1] >> *(ascii::digit[Call(appendString, _val, _1)])
				>> lit(".")[Call(appendString, _val, _1)] >> +(ascii::digit[Call(appendString, _val, _1)])];*/
			
			/*double_value = lexeme[qi::char_('1', '9')[_val = _1] >> *(ascii::digit[Call(appendString, _val, _1)]) >> double_value2[Call(appendString2, _val, _1)]];

			double_value2 = lexeme[lit(".")[_val = '.'] >> +(ascii::digit[Call(appendString, _val, _1)])];*/

		}
	};
}

#define DO_TEST

#define DO_TEST2

int main()
{
	using namespace cgl;

	const auto parse = [](const std::string& str, Lines& lines)->bool
	{
		using namespace cgl;

		SpaceSkipper<IteratorT> skipper;
		Parser<IteratorT, SpaceSkipperT> grammer;

		std::string::const_iterator it = str.begin();
		if (!boost::spirit::qi::phrase_parse(it, str.end(), grammer, skipper, lines))
		{
			std::cerr << "error: parse failed\n";
			return false;
		}

		if (it != str.end())
		{
			std::cerr << "error: ramains input\n" << std::string(it, str.end());
			return false;
		}

		return true;
	};

#ifdef DO_TEST

	std::vector<std::string> test_ok({
		"(1*2 + -3*-(4 + 5/6))",
		"1/(3+4)^6^7",
		"1 + 2, 3 + 4",
		"\n 4*5",
		"1 + 1 \n 2 + 3",
		"1 + 2 \n 3 + 4 \n",
		"1 + 1, \n 2 + 3",
		"1 + 1 \n \n \n 2 + 3",
		"1 + \n \n \n 2",
		"1 + 2 \n 3 + 4 \n\n\n\n\n",
		"1 + 2 \n , 4*5",
		"1 + 3 * \n 4 + 5",
		"(-> 1 + 2)",
		"(-> 1 + 2 \n 3)",
		"x, y -> x + y",
		"fun = (a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i)))",
		"fun = a, b, c -> d, e, f -> g, h, i -> a = processA(b, c), d = processB((a, e), f), g = processC(h, (a, d, i))",
		"gcd1 = (m, n ->\n if m == 0\n then return m\n else self(mod(m, n), m)\n)",
		"gcd2 = (m, n ->\r\n if m == 0\r\n then return m\r\n else self(mod(m, n), m)\r\n)",
		"gcd3 = (m, n ->\n\tif m == 0\n\tthen return m\n\telse self(mod(m, n), m)\n)",
		"gcd4 = (m, n ->\r\n\tif m == 0\r\n\tthen return m\r\n\telse self(mod(m, n), m)\r\n)",
		R"(
gcd5 = (m, n ->
	if m == 0
	then return m
	else self(mod(m, n), m)
)
)",
R"(
func = x ->  
    x + 1
    return x

func2 = x, y -> x + y)",
"a = {a: 1, b: [1, {a: 2, b: 4}, 3]}, a.b[1].a = {x: 3, y: 5}, a",
"x = 10, f = ->x + 1, f()",
"x = 10, g = (f = ->x + 1, f), g()"
	});
	
	std::vector<std::string> test_ng({
		", 3*4",
		"1 + 1 , , 2 + 3",
		"1 + 2, 3 + 4,",
		"1 + 2, \n , 3 + 4",
		"1 + 3 * , 4 + 5",
		"(->)"
	});

	int ok_wrongs = 0;
	int ng_wrongs = 0;

	std::cout << "==================== Test Case OK ====================" << std::endl;
	for (size_t i = 0; i < test_ok.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ok[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool succeed = parse(test_ok[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (succeed)
		{
			/*
			std::cout << "eval:\n";
			printEvaluated(evalExpr(expr));
			std::cout << "\n";
			*/
		}
		else
		{
			std::cout << "[Wrong]\n";
			++ok_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "==================== Test Case NG ====================" << std::endl;
	for (size_t i = 0; i < test_ng.size(); ++i)
	{
		std::cout << "Case[" << i << "]\n\n";

		std::cout << "input:\n";
		std::cout << test_ng[i] << "\n\n";

		std::cout << "parse:\n";

		Lines expr;
		const bool failed = !parse(test_ng[i], expr);

		printExpr(expr);

		std::cout << "\n";

		if (failed)
		{
			std::cout << "no result\n";
		}
		else
		{
			//std::cout << "eval:\n";
			//printEvaluated(evalExpr(expr));
			//std::cout << "\n";
			std::cout << "[Wrong]\n";
			++ng_wrongs;
		}

		std::cout << "-------------------------------------" << std::endl;
	}

	std::cout << "Result:\n";
	std::cout << "Correct programs: (Wrong / All) = (" << ok_wrongs << " / " << test_ok.size() << ")\n";
	std::cout << "Wrong   programs: (Wrong / All) = (" << ng_wrongs << " / " << test_ng.size() << ")\n";

#endif

#ifdef DO_TEST2

	int eval_wrongs = 0;

	const auto testEval1 = [&](const std::string& source, std::function<bool(const Evaluated&)> pred)
	{
		std::cout << "----------------------------------------------------------\n";
		std::cout << "input:\n";
		std::cout << source << "\n\n";

		std::cout << "parse:\n";

		Lines lines;
		const bool succeed = parse(source, lines);

		if (succeed)
		{
			printLines(lines);

			std::cout << "eval:\n";
			Evaluated result = evalExpr(lines);

			std::cout << "result:\n";
			printEvaluated(result);

			const bool isCorrect = pred(result);

			std::cout << "test: ";

			if (isCorrect)
			{
				std::cout << "Correct\n";
			}
			else
			{
				std::cout << "Wrong\n";
				++eval_wrongs;
			}
		}
		else
		{
			std::cerr << "Parse error!!\n";
			++eval_wrongs;
		}
	};

	const auto testEval = [&](const std::string& source, const Evaluated& answer)
	{
		testEval1(source, [&](const Evaluated& result) {return IsEqual(result, answer); });
	};

testEval(R"(

{a: 3}.a

)", 3);

testEval(R"(

f = (x -> {a: x+10})
f(3).a

)", 13);

testEval(R"(

vec3 = (v -> {
	x:v, y : v, z : v
})
vec3(3)

)", Record("x", 3).append("y", 3).append("z", 3));

testEval(R"(

vec2 = (v -> [
	v, v
])
a = vec2(3)
vec2(a)

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));

testEval(R"(

vec2 = (v -> [
	v, v
])
vec2(vec2(3))

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));

testEval(R"(

vec2 = (v -> {
	x:v, y : v
})
a = vec2(3)
vec2(a)

)", Record("x", Record("x", 3).append("y", 3)).append("y", Record("x", 3).append("y", 3)));

testEval(R"(

vec2 = (v -> {
	x:v, y : v
})
vec2(vec2(3))

)", Record("x", Record("x", 3).append("y", 3)).append("y", Record("x", 3).append("y", 3)));

testEval(R"(

vec3 = (v -> {
	x:v, y : v, z : v
})
mul = (v1, v2 -> {
	x:v1.x*v2.x, y : v1.y*v2.y, z : v1.z*v2.z
})
mul(vec3(3), vec3(2))

)", Record("x", 6).append("y", 6).append("z", 6));

testEval(R"(

r = {x: 0, y:10, sat(x == y), free(x)}
r.x

)", 10.0);

testEval(R"(

a = [1, 2]
b = [a, 3]

)", List().append(List().append(1).append(2)).append(3));

testEval(R"(

a = {a:1, b:2}
b = {a:a, b:3}

)", Record("a", Record("a", 1).append("b", 2)).append("b", 3));

testEval1(R"(

shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
}

line = shape{
	vertex: [
		{x:0, y:0}
		{x:1, y:0}
	]
}

main = {
	l1: line{
		vertex[1].y = 10
		color: {r:255, g:0, b:0}
	}
	l2: line{
		vertex[0] = {x: 2, y:3}
		color: {r:0, g:255, b:0}
	}

	sat(l1.vertex[1].x == l2.vertex[0].x & l1.vertex[1].y == l2.vertex[0].y)
	free(l1.vertex[1])
}

)", [](const Evaluated& result) {
	const auto l1vertex1 = As<List>(As<Record>(As<Record>(result).values.at("l1")).values.at("vertex"));
	const auto answer = List().append(Record("x", 0).append("y", 0)).append(Record("x", 2).append("y", 3));
	printEvaluated(answer);
	return IsEqual(l1vertex1, answer);
});

	std::cerr<<"Test Wrong Count: " << eval_wrongs<<std::endl;

#endif
	
	while (true)
	{
		std::string source;

		std::string buffer;
		while (std::cout << ">> ", std::getline(std::cin, buffer))
		{
			if (buffer == "quit()")
			{
				return 0;
			}
			else if (buffer == "EOF")
			{
				break;
			}

			source.append(buffer + '\n');
		}

		std::cout << "input:\n";
		std::cout << source << "\n\n";

		std::cout << "parse:\n";

		Lines lines;
		const bool succeed = parse(source, lines);

		if (!succeed)
		{
			std::cerr << "Parse error!!\n";
		}

		printLines(lines);

		if (succeed)
		{
			Evaluated result = evalExpr(lines);
			std::cout << "Result Evaluation:\n";
			printEvaluated(result);
		}
	}

	return 0;
}