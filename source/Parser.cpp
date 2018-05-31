#include <iomanip>

#include <Pita/Parser.hpp>

namespace cgl
{
	std::stack<filesystem::path> workingDirectories;
	//std::set<filesystem::path> alreadyImportedFiles;

	std::unordered_map<size_t, boost::optional<Expr>> importedParseTrees;

	class GetInfo : public boost::static_visitor<LocationInfo&>
	{
	public:
		GetInfo() = default;

		LocationInfo& operator()(Lines& node) { return node; }
		LocationInfo& operator()(BinaryExpr& node) { return node; }
		LocationInfo& operator()(LRValue& node) { return node; }
		LocationInfo& operator()(Identifier& node) { return node; }
		LocationInfo& operator()(Import& node) { return node; }
		LocationInfo& operator()(UnaryExpr& node) { return node; }
		LocationInfo& operator()(Range& node) { return node; }
		LocationInfo& operator()(DefFunc& node) { return node; }
		LocationInfo& operator()(If& node) { return node; }
		LocationInfo& operator()(For& node) { return node; }
		LocationInfo& operator()(Return& node) { return node; }
		LocationInfo& operator()(ListConstractor& node) { return node; }
		LocationInfo& operator()(KeyExpr& node) { return node; }
		LocationInfo& operator()(RecordConstractor& node) { return node; }
		LocationInfo& operator()(RecordInheritor& node) { return node; }
		LocationInfo& operator()(Accessor& node) { return node; }
		LocationInfo& operator()(DeclSat& node) { return node; }
		LocationInfo& operator()(DeclFree& node) { return node; }
	};

	inline LocationInfo& GetLocInfo(Expr& expr)
	{
		//GetInfo getter;
		//return boost::apply_visitor(getter, expr);
		if(IsType<Lines>(expr)){return static_cast<LocationInfo&>(As<Lines>(expr));}
		else if(IsType<BinaryExpr>(expr)){return static_cast<LocationInfo&>(As<BinaryExpr>(expr));}
		else if(IsType<LRValue>(expr)){return static_cast<LocationInfo&>(As<LRValue>(expr));}
		else if(IsType<Identifier>(expr)){return static_cast<LocationInfo&>(As<Identifier>(expr));}
		else if(IsType<Import>(expr)){return static_cast<LocationInfo&>(As<Import>(expr));}
		else if(IsType<UnaryExpr>(expr)){return static_cast<LocationInfo&>(As<UnaryExpr>(expr));}
		else if(IsType<Range>(expr)){return static_cast<LocationInfo&>(As<Range>(expr));}
		else if(IsType<DefFunc>(expr)){return static_cast<LocationInfo&>(As<DefFunc>(expr));}
		else if(IsType<If>(expr)){return static_cast<LocationInfo&>(As<If>(expr));}
		else if(IsType<For>(expr)){return static_cast<LocationInfo&>(As<For>(expr));}
		else if(IsType<Return>(expr)){return static_cast<LocationInfo&>(As<Return>(expr));}
		else if(IsType<ListConstractor>(expr)){return static_cast<LocationInfo&>(As<ListConstractor>(expr));}
		else if(IsType<KeyExpr>(expr)){return static_cast<LocationInfo&>(As<KeyExpr>(expr));}
		else if(IsType<RecordConstractor>(expr)){return static_cast<LocationInfo&>(As<RecordConstractor>(expr));}
		else if(IsType<RecordInheritor>(expr)){return static_cast<LocationInfo&>(As<RecordInheritor>(expr));}
		else if(IsType<Accessor>(expr)){return static_cast<LocationInfo&>(As<Accessor>(expr));}
		else if(IsType<DeclSat>(expr)){return static_cast<LocationInfo&>(As<DeclSat>(expr));}
		else{return static_cast<LocationInfo&>(As<DeclFree>(expr));}
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

		void operator()(const BinaryExpr& node)const {}
		void operator()(const LRValue& node)const {}
		void operator()(const Identifier& node)const {}
		void operator()(const UnaryExpr& node)const {}
		void operator()(const Range& node)const {}
		void operator()(const DefFunc& node)const {}
		void operator()(const If& node)const {}
		void operator()(const For& node)const {}
		void operator()(const Return& node)const {}
		void operator()(const ListConstractor& node)const {}
		void operator()(const KeyExpr& node)const {}
		void operator()(const RecordConstractor& node)const {}
		void operator()(const RecordInheritor& node)const {}
		void operator()(const Accessor& node)const {}
		void operator()(const DeclSat& node)const {}
		void operator()(const DeclFree& node)const {}
	};

	std::string EscapedSourceCode(const std::string& sourceCode)
	{
		std::stringstream escapedStr;
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
			bool inScopeComment = false;
			for (auto it = utf32str.begin(); it != utf32str.end(); ++it)
			{
				if (inScopeComment)
				{
					//エラー位置を正しく捕捉するため、コメント内でも改行はちゃんと拾う必要がある
					if (*it == '\n')
					{
						escapedStr << '\n';
						continue;
					}
					else if (*it == '*')
					{
						if (auto opt = nextCharOpt(it, utf32str.end()))
						{
							if (opt.get() == '/')
							{
								inScopeComment = false;
								++it;
								escapedStr << "  ";
								continue;
							}
						}
					}

					escapedStr << ' ';
					continue;
				}

				if (inLineComment)
				{
					if (*it == '\n')
					{
						escapedStr << '\n';
						inLineComment = false;
					}
					else
					{
						escapedStr << ' ';
					}
					continue;
				}

				if (*it == '\t')
				{
					escapedStr << "    ";
				}
				else if (*it == '/')
				{
					if (auto opt = nextCharOpt(it, utf32str.end()))
					{
						if (opt.get() == '/' || opt.get() == '*')
						{
							(opt.get() == '/' ? inLineComment : inScopeComment) = true;
							++it;
							escapedStr << "  ";
							continue;
						}
					}
				}

				escapedStr << AsUtf8(std::u32string(it, it + 1));
			}
		}

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

		const auto currentDirectory = cgl::filesystem::absolute(cgl::filesystem::path(filename)).parent_path();

		workingDirectories.emplace(currentDirectory);

		SourceT beginSource(sourceCode.begin()), endSource(sourceCode.end());
		IteratorT beginIt(beginSource), endIt(endSource);

		Lines lines;

		auto it = beginIt;
		SpaceSkipper<IteratorT> skipper;
		Parser<SpaceSkipperT> grammer(beginSource, endSource, filename);
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
		SpaceSkipper<IteratorT> skipper;
		Parser<SpaceSkipperT> grammer(beginSource, endSource, "empty source");

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

	void PrintErrorPos(const std::string& input_filepath, const LocationInfo& locationInfo)
	{
		std::cerr << "--------------------------------------------------------------------------------" << std::endl;
		std::ifstream ifs(input_filepath);
		if (!ifs.is_open())
		{
			return;
		}

		std::stringstream tabRemovedStr;
		char c;
		while (ifs.get(c))
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
		//for (; std::getline(ifs, line); ++l)
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

		std::cerr << "--------------------------------------------------------------------------------" << std::endl;
	}
}
