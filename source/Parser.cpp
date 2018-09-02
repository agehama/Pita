#include <iomanip>

#include <Pita/Parser.hpp>
#include <Pita/Printer.hpp>

namespace cgl
{
	std::stack<filesystem::path> workingDirectories;

	std::unordered_map<size_t, boost::optional<Expr>> importedParseTrees;

	Parser::Parser(SourceT first, SourceT last, const std::string& sourcePath) :
		Parser::base_type(program),
		annotate(Annotator(first, last)),
		errorHandler(ErrorHandler(first, last, sourcePath))
	{
		auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };
		auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

		program = -(expr_seq);

		expr_seq = general_expr[_val = Call(Lines::Make, _1)] >> *(
			',' >> general_expr[Call(Lines::Append, _val, _1)]
			);

		general_expr =
			if_expr[_val = _1]
			| return_expr[_val = _1]
			| logic_expr[_val = _1];

		if_expr = lit("if") >> general_expr[_val = Call(If::Make, _1)]
			>> lit("then") >> general_expr[Call(If::SetThen, _val, _1)]
			>> -(lit("else") >> general_expr[Call(If::SetElse, _val, _1)])
			;

		for_expr = lit("for") >> id[_val = Call(For::Make, _1)] >> lit("in")
			>> general_expr[Call(For::SetRangeStart, _val, _1)] >> lit(":")
			>> general_expr[Call(For::SetRangeEnd, _val, _1)] >>
			((lit("do") >> general_expr[Call(For::SetDo, _val, _1, false)]) |
			(lit("list") >> general_expr[Call(For::SetDo, _val, _1, true)]));

		import_expr = lit("import") >> char_string[_val = Call(Import::Make, _1)] >> -(lit("as") >> id[Call(Import::SetName, _val, _1)]);

		return_expr = lit("return") >> general_expr[_val = Call(Return::Make, _1)];

		def_func = arguments[_val = _1] >> lit("->") >> expr_seq[Call(applyFuncDef, _val, _1)];

		constraints = lit("sat") >> '(' >> expr_seq[_val = Call(DeclSat::Make, _1)] >> ')';

		freeVals = lit("var") >> '(' >> (
			(lit("@") >> (accessor[Call(DeclFree::AddAccessorDynamic, _val, _1)] | id[Call(DeclFree::AddAccessorDynamic, _val, Cast<Identifier, Accessor>())]))
			| (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())])
			) >>
			-(lit("in") >> factor[Call(DeclFree::AddRange, _val, _1)]) >> *(
				", " >> (
				(lit("@") >> (accessor[Call(DeclFree::AddAccessorDynamic, _val, _1)] | id[Call(DeclFree::AddAccessorDynamic, _val, Cast<Identifier, Accessor>())]))
					| (accessor[Call(DeclFree::AddAccessor, _val, _1)] | id[Call(DeclFree::AddAccessor, _val, Cast<Identifier, Accessor>())])
					) >>
				-(lit("in") >> factor[Call(DeclFree::AddRange, _val, _1)])
				) >> ')';

		arguments = -(id[_val = _1] >> *(',' >> arguments[Call(concatArguments, _val, _1)]));

		logic_expr = logic_term[_val = _1] >> *('|' >> logic_term[_val = MakeBinaryExpr(BinaryOp::Or)]);

		logic_term = logic_factor[_val = _1] >> *('&' >> logic_factor[_val = MakeBinaryExpr(BinaryOp::And)]);

		logic_factor = ('!' >> compare_expr[_val = MakeUnaryExpr(UnaryOp::Not)])
			| compare_expr[_val = _1]
			;

		compare_expr = arith_expr[_val = _1] >> *(
			(lit("==") >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Equal)])
			| (lit("!=") >> arith_expr[_val = MakeBinaryExpr(BinaryOp::NotEqual)])
			| (lit("<") >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessThan)])
			| (lit("<=") >> arith_expr[_val = MakeBinaryExpr(BinaryOp::LessEqual)])
			| (lit(">") >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterThan)])
			| (lit(">=") >> arith_expr[_val = MakeBinaryExpr(BinaryOp::GreaterEqual)])
			)
			;

		//= ^ -> は右結合
		arith_expr = (key_expr[_val = _1] | basic_arith_expr[_val = _1]) >> -(
			('=' >> arith_expr[_val = MakeBinaryExpr(BinaryOp::Assign)])
			);

		key_expr = id[_val = Call(KeyExpr::Make, _1)] >> ':' >> basic_arith_expr[Call(KeyExpr::SetExpr, _val, _1)];

		basic_arith_expr = term[_val = _1] >>
			*(
			('+' >> term[_val = MakeBinaryExpr(BinaryOp::Add)]) |
				('-' >> term[_val = MakeBinaryExpr(BinaryOp::Sub)]) |
				('\\' >> term[_val = MakeBinaryExpr(BinaryOp::SetDiff)]) |
				('@' >> term[_val = MakeBinaryExpr(BinaryOp::Concat)])
				)
			;

		term = pow_term[_val = _1]
			| (factor[_val = _1] >>
				*(('*' >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Mul)]) |
				('/' >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Div)]))
				)
			;

		//最低でも1つは受け取るようにしないと、単一のfactorを受理できてしまうのでMul,Divの方に行ってくれない
		pow_term = factor[_val = _1] >> '^' >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)];
		pow_term1 = factor[_val = _1] >> -('^' >> pow_term1[_val = MakeBinaryExpr(BinaryOp::Pow)]);

		record_maker = encode::char_('{') >
			-(
			(record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]) >
				*(
					(',' > (record_keyexpr[Call(RecordConstractor::AppendKeyExpr, _val, _1)] | general_expr[Call(RecordConstractor::AppendExpr, _val, _1)]))
				)
			)
			> encode::char_('}');

		record_keyexpr = id[_val = Call(KeyExpr::Make, _1)] >> encode::char_(':') > general_expr[Call(KeyExpr::SetExpr, _val, _1)];

		list_maker = encode::char_('[') >
			-(
				general_expr[_val = Call(ListConstractor::Make, _1)] >
				*(
				(encode::char_(',') > general_expr[Call(ListConstractor::Append, _val, _1)])
					)
				)
				> encode::char_(']');

		/*
		accessor = (id[_val = Call(Accessor::Make, _1)] >> +(access[Call(Accessor::Append, _val, _1)]))
			| (list_maker[_val = Call(Accessor::Make, _1)] >> listAccess[Call(Accessor::AppendList, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]))
			| (record_maker[_val = Call(Accessor::Make, _1)] >> recordAccess[Call(Accessor::AppendRecord, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]))
			| (record_inheritor[_val = Call(Accessor::Make, _1)] >> recordAccess[Call(Accessor::AppendRecord, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]));
		*/
		accessor = (id[_val = Call(Accessor::Make, _1)] >> +(access[Call(Accessor::Append, _val, _1)]))
			| (list_maker[_val = Call(Accessor::Make, _1)] >> listAccess[Call(Accessor::AppendList, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]))
			| (record_maker[_val = Call(Accessor::Make, _1)] >> recordAccess[Call(Accessor::AppendRecord, _val, _1)] >> *(access[Call(Accessor::Append, _val, _1)]));

		//accessor = (factor[_val = Call(Accessor::Make, _1)] >> +(access[Call(Accessor::Append, _val, _1)]));

		access = functionAccess[_val = Cast<FunctionAccess, Access>()]
			| listAccess[_val = Cast<ListAccess, Access>()]
			| recordAccess[_val = Cast<RecordAccess, Access>()]
			| inheritAccess[_val = Cast<InheritAccess, Access>()];

		recordAccess = encode::char_('.') >> id[_val = Call(RecordAccess::Make, _1)];

		//listAccess = encode::char_('[') >> general_expr[Call(ListAccess::SetIndex, _val, _1)] >> encode::char_(']');
		listAccess = encode::char_('[') >> (encode::char_('*')[Call(ListAccess::SetIndexArbitrary, _val)] | general_expr[Call(ListAccess::SetIndex, _val, _1)]) >> encode::char_(']');

		functionAccess = encode::char_('(')
			>> -(general_expr[Call(FunctionAccess::Append, _val, _1)])
			>> *(encode::char_(',') >> general_expr[Call(FunctionAccess::Append, _val, _1)]) >> encode::char_(')');

		inheritAccess = record_maker[_val = Call(InheritAccess::Make, _1)];

		factor =
			import_expr[_val = _1]
			| ('(' > expr_seq[_val = _1] > ')')
			| char_string[_val = Call(BuildString, _1)]
			//| lexeme['\"' > (*(encode::char_ - encode::char_('\"')))[_val = Call(BuildString, _1)] > '\"']
			| constraints[_val = _1]
			//| record_inheritor[_val = _1]
			| freeVals[_val = _1]
			//| ("shapeOf(" > accessor[_val = _1] >')')
			| accessor[_val = _1]
			//| record_inheritor[_val = _1]
			| def_func[_val = _1]
			| for_expr[_val = _1]
			| list_maker[_val = _1]
			| record_maker[_val = _1]
			| ('+' > factor[_val = MakeUnaryExpr(UnaryOp::Plus)])
			| ('-' > factor[_val = MakeUnaryExpr(UnaryOp::Minus)])
			| ('@' > (accessor[_val = MakeUnaryExpr(UnaryOp::Dynamic)] | id[_val = MakeUnaryExpr(UnaryOp::Dynamic)]))
			| float_value[_val = Call(LRValue::Float, _1)]
			| int_[_val = Call(LRValue::Int, _1)]
			| lit("true")[_val = Call(LRValue::Bool, true)]
			| lit("false")[_val = Call(LRValue::Bool, false)]
			| id[_val = _1];

		id = unchecked_identifier[_val = Call(Identifier::MakeIdentifier, _1)] - distinct_keyword;

		//char_string = lexeme[*(encode::char_ - encode::char_('\"'))];
		char_string = lexeme['\"' > *(encode::char_ - encode::char_('\"')) > '\"'];

		distinct_keyword = lexeme[keywords >> !(encode::alnum | '_')];
		unchecked_identifier = lexeme[(encode::alpha | encode::char_('_')) >> *(encode::alnum | encode::char_('_'))];

		float_value = lexeme[+encode::char_('0', '9') >> encode::char_('.') >> +encode::char_('0', '9')];

		if (isDebugMode)
		{
			auto errorInfo = errorHandler(_1, _3, _4);
			qi::on_error<qi::fail>(general_expr, errorInfo);
			qi::on_error<qi::fail>(logic_expr, errorInfo);
			qi::on_error<qi::fail>(logic_term, errorInfo);
			qi::on_error<qi::fail>(logic_factor, errorInfo);
			qi::on_error<qi::fail>(compare_expr, errorInfo);
			qi::on_error<qi::fail>(arith_expr, errorInfo);
			qi::on_error<qi::fail>(basic_arith_expr, errorInfo);
			qi::on_error<qi::fail>(term, errorInfo);
			qi::on_error<qi::fail>(factor, errorInfo);
			qi::on_error<qi::fail>(pow_term, errorInfo);
			qi::on_error<qi::fail>(pow_term1, errorInfo);
			qi::on_error<qi::fail>(constraints, errorInfo);
			qi::on_error<qi::fail>(freeVals, errorInfo);
			//qi::on_error<qi::fail>(functionAccess, errorInfo);
			//qi::on_error<qi::fail>(recordAccess, errorInfo);
			//qi::on_error<qi::fail>(listAccess, errorInfo);
			qi::on_error<qi::fail>(accessor, errorInfo);
			//qi::on_error<qi::fail>(access, errorInfo);
			qi::on_error<qi::fail>(record_keyexpr, errorInfo);
			qi::on_error<qi::fail>(record_maker, errorInfo);
			qi::on_error<qi::fail>(list_maker, errorInfo);
			qi::on_error<qi::fail>(import_expr, errorInfo);
			qi::on_error<qi::fail>(for_expr, errorInfo);
			qi::on_error<qi::fail>(if_expr, errorInfo);
			qi::on_error<qi::fail>(return_expr, errorInfo);
			qi::on_error<qi::fail>(def_func, errorInfo);
			//qi::on_error<qi::fail>(arguments, errorInfo);
			qi::on_error<qi::fail>(id, errorInfo);
			qi::on_error<qi::fail>(key_expr, errorInfo);
			//qi::on_error<qi::fail>(char_string, errorInfo);
			qi::on_error<qi::fail>(expr_seq, errorInfo);

			auto setLocationInfo = annotate(_val, _1, _3);
			//qi::on_success(general_expr, setLocationInfo);
			qi::on_success(logic_expr, setLocationInfo);
			qi::on_success(logic_term, setLocationInfo);
			qi::on_success(logic_factor, setLocationInfo);
			qi::on_success(compare_expr, setLocationInfo);
			qi::on_success(arith_expr, setLocationInfo);
			qi::on_success(basic_arith_expr, setLocationInfo);
			//qi::on_success(term, setLocationInfo);
			//qi::on_success(factor, setLocationInfo);
			//qi::on_success(pow_term, setLocationInfo);
			//qi::on_success(pow_term1, setLocationInfo);
			qi::on_success(constraints, setLocationInfo);
			qi::on_success(freeVals, setLocationInfo);
			//qi::on_success(functionAccess, setLocationInfo);
			//qi::on_success(recordAccess, setLocationInfo);
			//qi::on_success(listAccess, setLocationInfo);
			qi::on_success(accessor, setLocationInfo);
			//qi::on_success(access, setLocationInfo);
			qi::on_success(record_keyexpr, setLocationInfo);
			qi::on_success(record_maker, setLocationInfo);
			qi::on_success(list_maker, setLocationInfo);
			qi::on_success(import_expr, setLocationInfo);
			qi::on_success(for_expr, setLocationInfo);
			qi::on_success(if_expr, setLocationInfo);
			qi::on_success(return_expr, setLocationInfo);
			//qi::on_success(def_func, setLocationInfo);
			//qi::on_success(arguments, setLocationInfo);
			qi::on_success(id, setLocationInfo);
			qi::on_success(key_expr, setLocationInfo);
			//qi::on_success(char_string, setLocationInfo);
			//qi::on_success(expr_seq, setLocationInfo);
			//qi::on_success(program, setLocationInfo);

			/*BOOST_SPIRIT_DEBUG_NODES(
			(float_value)(unchecked_identifier)(distinct_keyword)(program)
			(expr_seq)(general_expr)(logic_expr)(logic_term)(logic_factor)(compare_expr)(arith_expr)(basic_arith_expr)(term)(factor)(pow_term)(pow_term1)
			(char_string)(key_expr)(id)(arguments)(def_func)(return_expr)(if_expr)(for_expr)(import_expr)(list_maker)(record_inheritor)(record_maker)(record_keyexpr)
			(access)(accessor)(listAccess)(recordAccess)(functionAccess)(freeVals)(constraints)
			)*/
		}
	}

	template<typename T>
	using R = boost::recursive_wrapper<T>;

	inline LocationInfo& GetLocInfo(Expr& expr)
	{
		switch (expr.which())
		{
		case ExprIndex<R<LRValue>>() :           return static_cast<LocationInfo&>(As<LRValue>(expr));
		case ExprIndex<Identifier>() :           return static_cast<LocationInfo&>(As<Identifier>(expr));
		case ExprIndex<R<Import>>() :            return static_cast<LocationInfo&>(As<Import>(expr));
		case ExprIndex<R<UnaryExpr>>() :         return static_cast<LocationInfo&>(As<UnaryExpr>(expr));
		case ExprIndex<R<BinaryExpr>>() :        return static_cast<LocationInfo&>(As<BinaryExpr>(expr));
		case ExprIndex<R<DefFunc>>() :           return static_cast<LocationInfo&>(As<DefFunc>(expr));
		case ExprIndex<R<Range>>() :             return static_cast<LocationInfo&>(As<Range>(expr));
		case ExprIndex<R<Lines>>() :             return static_cast<LocationInfo&>(As<Lines>(expr));
		case ExprIndex<R<If>>() :                return static_cast<LocationInfo&>(As<If>(expr));
		case ExprIndex<R<For>>() :               return static_cast<LocationInfo&>(As<For>(expr));
		case ExprIndex<R<Return>>() :            return static_cast<LocationInfo&>(As<Return>(expr));
		case ExprIndex<R<ListConstractor>>() :   return static_cast<LocationInfo&>(As<ListConstractor>(expr));
		case ExprIndex<R<KeyExpr>>() :           return static_cast<LocationInfo&>(As<KeyExpr>(expr));
		case ExprIndex<R<RecordConstractor>>() : return static_cast<LocationInfo&>(As<RecordConstractor>(expr));
		case ExprIndex<R<DeclSat>>() :           return static_cast<LocationInfo&>(As<DeclSat>(expr));
		case ExprIndex<R<DeclFree>>() :          return static_cast<LocationInfo&>(As<DeclFree>(expr));
		case ExprIndex<R<Accessor>>() :          return static_cast<LocationInfo&>(As<Accessor>(expr));
		default:
			CGL_Error("エラー：対応する型が無い");
		};
	}

	LocationInfo GetLocationInfo(IteratorT f, IteratorT l, SourceT first, SourceT last)
	{
		LocationInfo li;

		auto sourceBeginIt = f.base();
		auto sourceEndIt = l.base();

		auto lowerBound = get_line_start(first, sourceBeginIt);

		li.locInfo_lineBegin = get_line(sourceBeginIt);
		li.locInfo_lineEnd = get_line(sourceEndIt);

		auto line = get_current_line(lowerBound, sourceBeginIt, last);

		size_t cur_pos = 0, start_pos = 0, end_pos = 0;
		for (IteratorT it = line.begin(), _eol = line.end(); ; ++it, ++cur_pos)
		{
			if (it.base() == sourceBeginIt) start_pos = cur_pos;
			if (it.base() == sourceEndIt) end_pos = cur_pos;

			if (*(it.base()) == '\n')
				cur_pos = 0;

			if (it == _eol)
				break;
		}

		li.locInfo_posBegin = start_pos;
		li.locInfo_posEnd = end_pos;

		return li;
	}

	void Annotator::doAnnotate(LocationInfo& li, IteratorT f, IteratorT l, SourceT first, SourceT last)
	{
		li = GetLocationInfo(f, l, first, last);
	}

	void Annotator::doAnnotate(Expr& li, IteratorT f, IteratorT l, SourceT first, SourceT last)
	{
		doAnnotate(GetLocInfo(li), f, l, first, last);
	}

	bool errorMessagePrinted = false;
	void ErrorHandler::doAnnotate(IteratorT f, IteratorT l, SourceT first, SourceT last, const std::string& sourcePath, const std::string& what)
	{
		if (errorMessagePrinted)
		{
			return;
		}
		errorMessagePrinted = true;

		const LocationInfo li = GetLocationInfo(f, l, first, last);

		std::cout << std::string("Error") + li.getInfo() + ": expecting " + what << std::endl;
		PrintErrorPos(sourcePath, li);
	}

	//importファイルはpathとnameの組で管理する
	//最初のパース時にimportされるpathとnameの組を登録しておき、
	//パース直後(途中だとバックトラックが走る可能性があるためパースが終わってから)にそのソースがimportするソースのパースを再帰的に行う
	//実行時には、あるpathとnameの組に紐付けられるパースツリーに対して一回のみevalが走り、その評価結果が返却される
	class ParseImports : public boost::static_visitor<void>
	{
	public:
		ParseImports() = default;

		void operator()(const Lines& node)const
		{
			for (const auto& expr : node.exprs)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const Import& node)const
		{
			if (importedParseTrees.find(node.getSeed()) != importedParseTrees.end())
			{
				return;
			}

			const std::string path = node.getImportPath();

			if (auto opt = Parse1(path))
			{
				importedParseTrees[node.getSeed()] = opt;
			}
			else
			{
				CGL_Error("Parse failed.");
			}
		}

		void operator()(const BinaryExpr& node)const
		{
			boost::apply_visitor(*this, node.lhs);
			boost::apply_visitor(*this, node.rhs);
		}

		void operator()(const LRValue& node)const {}

		void operator()(const Identifier& node)const {}

		void operator()(const UnaryExpr& node)const
		{
			boost::apply_visitor(*this, node.lhs);
		}

		void operator()(const Range& node)const
		{
			boost::apply_visitor(*this, node.lhs);
			boost::apply_visitor(*this, node.rhs);
		}

		void operator()(const DefFunc& node)const
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const If& node)const
		{
			boost::apply_visitor(*this, node.cond_expr);
			boost::apply_visitor(*this, node.then_expr);
			if (node.else_expr)
			{
				boost::apply_visitor(*this, node.else_expr.get());
			}
		}

		void operator()(const For& node)const
		{
			boost::apply_visitor(*this, node.doExpr);
		}

		void operator()(const Return& node)const {}

		void operator()(const ListConstractor& node)const
		{
			for (const auto& expr : node.data)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const KeyExpr& node)const
		{
			boost::apply_visitor(*this, node.expr);
		}

		void operator()(const RecordConstractor& node)const
		{
			for (const auto& expr : node.exprs)
			{
				boost::apply_visitor(*this, expr);
			}
		}

		void operator()(const Accessor& node)const
		{
			boost::apply_visitor(*this, node.head);
			for (const auto& access : node.accesses)
			{
				if (auto opt = AsOpt<ListAccess>(access))
				{
					boost::apply_visitor(*this, opt.get().index);
				}
				else if (auto opt = AsOpt<FunctionAccess>(access))
				{
					for (const auto& arg : opt.get().actualArguments)
					{
						boost::apply_visitor(*this, arg);
					}
				}
				else if (auto opt = AsOpt<InheritAccess>(access))
				{
					const Expr adder = opt.get().adder;
					boost::apply_visitor(*this, adder);
				}
				//RecordAccessは特に見るべきものはない
				//else if (auto opt = AsOpt<RecordAccess>(access)){}
			}
		}

		void operator()(const DeclSat& node)const {}
		void operator()(const DeclFree& node)const {}
	};

	inline std::vector<std::string> Split(const std::string& input, char delimiter)
	{
		std::istringstream stream(input);

		std::string field;
		std::vector<std::string> result;
		while (std::getline(stream, field, delimiter)) {
			result.push_back(field);
		}
		return result;
	}

	std::string EscapedSourceCode(const std::string& sourceCode)
	{
		std::stringstream escapedStr;

		//コメントのエスケープ
		{
			const auto nextCharOpt = [](std::u32string::const_iterator it, std::u32string::const_iterator itEnd)->boost::optional<std::uint32_t>
			{
				const auto nextIt = it + 1;
				if (nextIt != itEnd)
				{
					return *nextIt;
				}
				return boost::none;
			};

			const auto utf32str = AsUtf32(sourceCode);

			bool inLineComment = false;
			int scopeCommentDepth = 0;
			for (auto it = utf32str.begin(); it != utf32str.end(); ++it)
			{
				std::stringstream currentChar;
				bool skipNext = false;
				if (1 <= scopeCommentDepth)
				{
					//エラー位置を正しく捕捉するため、コメント内でも改行はちゃんと拾う必要がある
					if (*it == '\n')
					{
						currentChar << '\n';
					}
					else if (*it == '*')
					{
						if (auto opt = nextCharOpt(it, utf32str.end()))
						{
							if (opt.get() == '/')
							{
								--scopeCommentDepth;
								currentChar << "  ";
								skipNext = true;
							}
						}
					}
				}
				else if (inLineComment)
				{
					if (*it == '\n')
					{
						currentChar << '\n';
						inLineComment = false;
					}
				}

				if (*it == '\t')
				{
					currentChar << "    ";
				}
				else if (*it == '/')
				{
					if (auto opt = nextCharOpt(it, utf32str.end()))
					{
						if (opt.get() == '/' || opt.get() == '*')
						{
							if (opt.get() == '/')
							{
								inLineComment = true;
							}
							else
							{
								++scopeCommentDepth;
							}

							currentChar << "  ";
							skipNext = true;
						}
					}
				}

				if (skipNext)
				{
					++it;
				}

				if (skipNext || inLineComment || 1 <= scopeCommentDepth)
				{
					if (currentChar.str().empty())
					{
						escapedStr << ' ';
					}
					else
					{
						escapedStr << currentChar.str();
					}
				}
				else
				{
					escapedStr << AsUtf8(std::u32string(it, it + 1));
				}
			}
		}

		//行末にカンマを挿入
		{
			const std::string postExpectingSymbols1("[({:=!?&|<>*/,+-@");
			const std::string prevExpectingSymbols1("])}:=!?&|<>*/,");

			//同じ記号で囲む場合は内側か外側かの判定を行う必要がある
			const std::string postExpectingSymbols1Symmetric("\"");
			const std::string prevExpectingSymbols1Symmetric("\"");

			const std::vector<std::string> postExpectingSymbols2({ "->","<=",">=","==","!=" });
			const std::vector<std::string> prevExpectingSymbols2({ "->","<=",">=","==","!=" });

			const std::vector<std::string> postExpectingSymbols4({ "then","else" });
			const std::vector<std::string> prevExpectingSymbols4({ "then","else" });

			/*
			const std::vector<std::string> postExpectingSymbols({ "[","(","{","->",":","=","!","?","\"","&","|","<","<=",">",">=","==","!=","*","/","+","-","@" });
			const std::vector<std::string> prevExpectingSymbols({ "]",")","}","->",":","=","!","?","\"","&","|","<","<=",">",">=","==","!=","*","/" });
			*/

			auto lines = Split(escapedStr.str(), '\n');

			//空行を飛ばした比較を行うため空行以外のインデックスを保存
			std::vector<size_t> nonEmptyIndices;
			for (size_t i = 0; i < lines.size(); ++i)
			{
				if (lines[i].find_first_not_of(" \t\r") != std::string::npos)
				{
					nonEmptyIndices.push_back(i);
				}
			}

			std::set<size_t> addCommaLines;

			for (size_t i = 0; i + 1 < nonEmptyIndices.size(); ++i)
			{
				const size_t prevIndex = nonEmptyIndices[i];
				const size_t postIndex = nonEmptyIndices[i + 1];

				const auto& prevLine = lines[prevIndex];
				const auto& postLine = lines[postIndex];

				const auto countChars = [&](char ch, size_t lineIndex, size_t charIndex)->size_t
				{
					size_t count = 0;
					for (size_t index = 0; index < lineIndex; ++index)
					{
						const auto& line = lines[index];
						count += std::count(line.begin(), line.end(), ch);
					}

					const auto& line = lines[lineIndex];
					count += std::count(line.begin(), line.begin() + charIndex, ch);
					std::cout << "char count: " << count << "\n";

					return count;
				};

				const auto needComma = [&]()->bool
				{
					const size_t prevLastIndex = prevLine.find_last_not_of(" \t\r");
					const char prevLastChar = prevLine[prevLastIndex];
					if (postExpectingSymbols1.find(prevLastChar) != std::string::npos)
					{
						return false;
					}
					const size_t prevLastSymmetricSymbolPos = postExpectingSymbols1Symmetric.find(prevLastChar);
					if (prevLastSymmetricSymbolPos != std::string::npos)
					{
						if (countChars(postExpectingSymbols1Symmetric[prevLastSymmetricSymbolPos], prevIndex, prevLastIndex) % 2 == 0)
						{
							return false;
						}
					}
					if (1 <= prevLastIndex)
					{
						const std::string lastTwoChars({ prevLine[prevLastIndex - 1], prevLastChar });
						if (std::find(postExpectingSymbols2.begin(), postExpectingSymbols2.end(), lastTwoChars) != postExpectingSymbols2.end())
						{
							return false;
						}
						if (3 <= prevLastIndex)
						{
							const std::string lastFourChars({ prevLine[prevLastIndex - 3], prevLine[prevLastIndex - 2], prevLine[prevLastIndex - 1], prevLastChar });
							if (std::find(postExpectingSymbols4.begin(), postExpectingSymbols4.end(), lastFourChars) != postExpectingSymbols4.end())
							{
								return false;
							}
						}
					}

					const size_t postFirstIndex = postLine.find_first_not_of(" \t\r");
					const char postFirstChar = postLine[postFirstIndex];
					if (prevExpectingSymbols1.find(postFirstChar) != std::string::npos)
					{
						return false;
					}
					const size_t postFirstSymmetricSymbolPos = prevExpectingSymbols1Symmetric.find(postFirstChar);
					if (postFirstSymmetricSymbolPos != std::string::npos)
					{
						if (countChars(prevExpectingSymbols1Symmetric[postFirstSymmetricSymbolPos], postIndex, postFirstIndex) % 2 == 1)
						{
							return false;
						}
					}
					if (postFirstIndex + 1 < postLine.size())
					{
						const std::string firstTwoChars({ postFirstChar, postLine[postFirstIndex + 1] });
						if (std::find(prevExpectingSymbols2.begin(), prevExpectingSymbols2.end(), firstTwoChars) != prevExpectingSymbols2.end())
						{
							return false;
						}
						if (postFirstIndex + 3 < postLine.size())
						{
							const std::string firstFourChars({ postFirstChar, postLine[postFirstIndex + 1], postLine[postFirstIndex + 2], postLine[postFirstIndex + 3] });
							if (std::find(prevExpectingSymbols4.begin(), prevExpectingSymbols4.end(), firstFourChars) != prevExpectingSymbols4.end())
							{
								return false;
							}
						}
					}

					return true;
				};

				const bool comma = needComma();
				if (comma)
				{
					addCommaLines.emplace(prevIndex);
				}
			}

			escapedStr = std::stringstream();
			for (size_t i = 0; i < lines.size(); ++i)
			{
				escapedStr << lines[i];
				if (addCommaLines.find(i) != addCommaLines.end())
				{
					escapedStr << ',';
				}
				escapedStr << '\n';
			}
		}

		//for debug
		/*
		std::cout << "Escaped str:" << std::endl;
		std::cout << "----------------------------------------------" << std::endl;
		std::cout << escapedStr.str() << std::endl;
		std::cout << "----------------------------------------------" << std::endl;
		//*/

		return escapedStr.str();
	}

	boost::optional<Expr> Parse1(const std::string& filename)
	{
		std::cout << "parse \"" << filename << "\" ..." << std::endl;

		std::ifstream ifs(filename);
		if (!ifs.is_open())
		{
			CGL_Error(std::string() + "Error file_path \"" + filename + "\" does not exists.");
		}

		std::string original((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
		std::string sourceCode = EscapedSourceCode(original);

		std::cout << "escaped SourceCode:\n";
		std::cout << "--------------------------------------------------"<< std::endl;
		std::cout << sourceCode << std::endl;
		std::cout << "--------------------------------------------------" << std::endl;

		const auto currentDirectory = cgl::filesystem::absolute(cgl::filesystem::path(filename)).parent_path();

		workingDirectories.emplace(currentDirectory);

		SourceT beginSource(sourceCode.begin()), endSource(sourceCode.end());
		IteratorT beginIt(beginSource), endIt(endSource);

		Lines lines;

		auto it = beginIt;
		SkipperT skipper;
		Parser grammer(beginSource, endSource, filename);
		
		const double parseBegin = GetSec();
		if (!boost::spirit::qi::phrase_parse(it, endIt, grammer, skipper, lines))
		{
			if (!errorMessagePrinted)
			{
				std::cout << "Syntax Error: parse failed\n";
			}
			if (!workingDirectories.empty())
			{
				workingDirectories.pop();
			}
			return boost::none;
		}
		const double parseSec = GetSec() - parseBegin;
		std::cerr << "parse(Qi) : " << parseSec << "[sec]" << std::endl;

		if (it != endIt)
		{
			if (!errorMessagePrinted)
			{
				std::cout << "Syntax Error: ramains input:\n" << std::string(it.base().base(), sourceCode.cend()) << std::endl;
			}
			if (!workingDirectories.empty())
			{
				workingDirectories.pop();
			}
			return boost::none;
		}

		Expr result = lines;

		boost::apply_visitor(ParseImports(), result);

		workingDirectories.pop();
		return result;
	}

	boost::optional<Expr> ParseFromSourceCode(const std::string& originalSourceCode)
	{
		std::string escapedSourceCode = EscapedSourceCode(originalSourceCode);

		workingDirectories.emplace(cgl::filesystem::current_path());

		SourceT beginSource(escapedSourceCode.begin()), endSource(escapedSourceCode.end());
		IteratorT beginIt(beginSource), endIt(endSource);

		Lines lines;

		auto it = beginIt;
		SkipperT skipper;
		Parser grammer(beginSource, endSource, "empty source");

		if (!boost::spirit::qi::phrase_parse(it, endIt, grammer, skipper, lines))
		{
			if (!errorMessagePrinted)
			{
				std::cout << "Syntax Error: parse failed\n";
			}
			if (!workingDirectories.empty())
			{
				workingDirectories.pop();
			}
			return boost::none;
		}

		if (it != endIt)
		{
			if (!errorMessagePrinted)
			{
				std::cout << "Syntax Error: ramains input:\n" << std::string(it.base().base(), escapedSourceCode.cend()) << std::endl;
			}
			if (!workingDirectories.empty())
			{
				workingDirectories.pop();
			}
			return boost::none;
		}

		Expr result = lines;

		boost::apply_visitor(ParseImports(), result);

		workingDirectories.pop();
		return result;
	}

	void PrintErrorPosSource(const std::string& sourceCode, const LocationInfo& locationInfo)
	{
		std::cerr << "--------------------------------------------------------------------------------" << std::endl;

		std::stringstream tabRemovedStr;
		for (char c : sourceCode)
		{
			if (c == '\t')
				tabRemovedStr << "    ";
			else
				tabRemovedStr << c;
		}
		std::stringstream is;
		is << tabRemovedStr.str();

		const int errorLineBegin = static_cast<int>(locationInfo.locInfo_lineBegin) - 1;
		const int errorLineEnd = static_cast<int>(locationInfo.locInfo_lineEnd) - 1;

		const int printLines = 10;
		const int printLineBegin = std::max(errorLineBegin - printLines / 2, 0);
		const int printLineEnd = errorLineEnd + printLines / 2;

		if (printLineBegin != 0)
		{
			std::cout << "..." << std::endl;
		}

		const auto getLine = [](int line)
		{
			std::stringstream ss;
			ss << std::right << std::setw(5) << line << "|";
			return ss.str();
		};

		std::string line;
		int l = 0;
		int printedLines = 0;
		for (; std::getline(is, line); ++l)
		{
			if (printLineBegin <= l && l <= printLineEnd)
			{
				std::cout << getLine(l + 1) << line << std::endl;
				if (errorLineBegin <= l && l <= errorLineEnd)
				{
					const int startX = (l == errorLineBegin ? std::max(static_cast<int>(locationInfo.locInfo_posBegin) - 1, 0) : 0);
					const int endX = (l == errorLineEnd ? std::max(static_cast<int>(locationInfo.locInfo_posEnd) - 1, 0) : static_cast<int>(line.size()));

					std::string str(line.size(), ' ');
					str.replace(startX, endX - startX, endX - startX, '^');

					std::cout << "     |" << str << std::endl;
				}
				printedLines = l;
			}
		}

		if (printedLines + 1 != l)
		{
			std::cout << "..." << std::endl;
		}
	}

	void PrintErrorPos(const std::string& inputFilepath, const LocationInfo& locationInfo)
	{
		std::ifstream ifs(inputFilepath);
		if (!ifs.is_open())
		{
			return;
		}

		const std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		PrintErrorPosSource(sourceCode, locationInfo);
	}
}
