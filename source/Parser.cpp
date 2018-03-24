#include <iomanip>

#include <Pita/Parser.hpp>

namespace cgl
{
	std::stack<filesystem::path> workingDirectories;
	//std::set<filesystem::path> alreadyImportedFiles;

	std::unordered_map<size_t, boost::optional<Expr>> importedParseTrees;

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

	boost::optional<Expr> Parse1(const std::string& filename)
	{
		std::cout << "parsing: \"" << filename << "\"" << std::endl;

		std::ifstream ifs(filename);
		if (!ifs.is_open())
		{
			CGL_Error(std::string() + "Error file_path \"" + filename + "\" does not exists.");
		}
		//std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		std::stringstream tabRemovedStr;
		char c;
		while (ifs.get(c))
		{
			if (c == '\t')
				tabRemovedStr << "    ";
			else
				tabRemovedStr << c;
		}
		std::string sourceCode = tabRemovedStr.str();

		const auto currentDirectory = cgl::filesystem::absolute(cgl::filesystem::path(filename)).parent_path();

		workingDirectories.emplace(currentDirectory);

		SourceT beginSource(sourceCode.begin()), endSource(sourceCode.end());
		IteratorT beginIt(beginSource), endIt(endSource);

		Lines lines;

		auto it = beginIt;
		SpaceSkipper<IteratorT> skipper;
		Parser<SpaceSkipperT> grammer(beginSource, endSource);

		if (!boost::spirit::qi::phrase_parse(it, endIt, grammer, skipper, lines))
		{
			//std::cerr << "Syntax Error: parse failed\n";
			std::cout << "Syntax Error: parse failed\n";
			workingDirectories.pop();
			return boost::none;
		}

		if (it != endIt)
		{
			//std::cout << "Syntax Error: ramains input\n" << std::string(it, program.end());
			std::cout << "Syntax Error: ramains input\n";
			workingDirectories.pop();
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
