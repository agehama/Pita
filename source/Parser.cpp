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

	//inline boost::optional<Expr> Parse(const std::string& program)
	//{
	//	boost::u8_to_u32_iterator<std::string::const_iterator> tbegin(program.begin()), tend(program.end());

	//	Lines lines;

	//	SpaceSkipper<IteratorT> skipper;
	//	Parser<IteratorT, SpaceSkipperT> grammer;

	//	auto it = tbegin;
	//	if (!boost::spirit::qi::phrase_parse(it, tend, grammer, skipper, lines))
	//	{
	//		//std::cerr << "Syntax Error: parse failed\n";
	//		std::cout << "Syntax Error: parse failed\n";
	//		workingDirectories.pop();
	//		return boost::none;
	//	}

	//	if (it != tend)
	//	{
	//		//std::cout << "Syntax Error: ramains input\n" << std::string(it, program.end());
	//		std::cout << "Syntax Error: ramains input\n";
	//		workingDirectories.pop();
	//		return boost::none;
	//	}

	//	Expr result = lines;
	//	workingDirectories.pop();
	//	return result;
	//}

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
}
