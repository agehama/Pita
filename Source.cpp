#define BOOST_RESULT_OF_USE_DECLTYPE
#define BOOST_SPIRIT_USE_PHOENIX_V3

#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix.hpp>

#include <Eigen/Core>

#define CGL_EnableLogOutput

#include "Node.hpp"
#include "Printer.hpp"
#include "Environment.hpp"

#include "Evaluator.hpp"
#include "OptimizationEvaluator.hpp"

std::ofstream ofs;

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

			//auto addCharacter = [](Identifier& identifier, char c) { identifier.name.push_back(c); };

			auto concatArguments = [](Arguments& a, const Arguments& b) { a.concat(b); };

			auto applyFuncDef = [](DefFunc& f, const Expr& expr) { f.expr = expr; };

			//auto makeDouble = [](const Identifier& str) { return std::stod(str.name); };
			//auto makeString = [](char c) { return Identifier(std::string({ c })); };
			//auto appendString = [](Identifier& str, char c) { str.name.push_back(c); };
			//auto appendString2 = [](Identifier& str, const Identifier& str2) { str.name.append(str2.name); };

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

			def_func = arguments[_val = _1] >> lit("->") >> s >> statement[Call(applyFuncDef, _val, _1)];
			//def_func = arguments[_val = _1] >> lit("->") >> s >> general_expr[Call(applyFuncDef, _val, _1)];

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

			//factor = /*double_[_val = _1]
			//	| */int_[_val = _1]
			//	| lit("true")[_val = true]
			//	| lit("false")[_val = false]
			//	| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
			//	| '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
			//	| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
			//	| constraints[_val = _1]
			//	| freeVals[_val = _1]
			//	| accessor[_val = _1]
			//	| def_func[_val = _1]
			//	| for_expr[_val = _1]
			//	| list_maker[_val = _1]
			//	| record_inheritor[_val = _1]
			//	//| (id >> record_maker)[_val = Call(RecordInheritor::MakeRecord, _1,_2)]
			//	| record_maker[_val = _1]
			//	| id[_val = _1];
			
			factor = /*double_[_val = _1]
					 | */int_[_val = Call(LRValue::Int, _1)]
				| lit("true")[_val = Call(LRValue::Bool, true)]
				| lit("false")[_val = Call(LRValue::Bool, false)]
				| '(' >> s >> expr_seq[_val = _1] >> s >> ')'
				| '+' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Plus)]
				| '-' >> s >> factor[_val = MakeUnaryExpr(UnaryOp::Minus)]
				//| constraints[_val = Call(LRValue::Sat, _1)]
				| constraints[_val = _1]
				| freeVals[_val = Call(LRValue::Free, _1)]
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

namespace cgl
{
	bool ReadDouble(double& output, const std::string& name, const Record& record, std::shared_ptr<Environment> environment)
	{
		const auto& values = record.values;
		auto it = values.find(name);
		if (it == values.end())
		{
			//CGL_DebugLog(__FUNCTION__);
			return false;
		}
		auto opt = environment->expandOpt(it->second);
		if (!opt)
		{
			//CGL_DebugLog(__FUNCTION__);
			return false;
		}
		const Evaluated& value = opt.value();
		if (!IsType<int>(value) && !IsType<double>(value))
		{
			//CGL_DebugLog(__FUNCTION__);
			return false;
		}
		output = IsType<int>(value) ? static_cast<double>(As<int>(value)) : As<double>(value);
		return true;
	}

	struct Transform
	{
		Transform()
		{
			init();
		}

		Transform(const Eigen::Matrix3d& mat) :mat(mat) {}

		Transform(const Record& record, std::shared_ptr<Environment> pEnv)
		{
			double px = 0, py = 0;
			double sx = 1, sy = 1;
			double angle = 0;

			for (const auto& member : record.values)
			{
				auto valOpt = AsOpt<Record>(pEnv->expand(member.second));

				if (member.first == "pos" && valOpt)
				{
					ReadDouble(px, "x", valOpt.value(), pEnv);
					ReadDouble(py, "y", valOpt.value(), pEnv);
				}
				else if (member.first == "scale" && valOpt)
				{
					ReadDouble(sx, "x", valOpt.value(), pEnv);
					ReadDouble(sy, "y", valOpt.value(), pEnv);
				}
				else if (member.first == "angle")
				{
					ReadDouble(angle, "angle", record, pEnv);
				}
			}

			init(px, py, sx, sy, angle);
		}

		void init(double px = 0, double py = 0, double sx = 1, double sy = 1, double angle = 0)
		{
			const double pi = 3.1415926535;
			const double cosTheta = std::cos(pi*angle/180.0);
			const double sinTheta = std::sin(pi*angle/180.0);

			mat <<
				sx*cosTheta, -sy*sinTheta, px,
				sx*sinTheta, sy*cosTheta, py,
				0, 0, 1;
		}

		Transform operator*(const Transform& other)const
		{
			return mat * other.mat;
		}

		Eigen::Vector2d product(const Eigen::Vector2d& v)const
		{
			Eigen::Vector3d xs;
			xs << v.x(), v.y(), 1;
			Eigen::Vector3d result = mat * xs;
			Eigen::Vector2d result2d;
			result2d << result.x(), result.y();
			return result2d;
		}

		void printMat()const
		{
			std::cout << "Matrix(\n";
			for (int y = 0; y < 3; ++y)
			{
				std::cout << "    ";
				for (int x = 0; x < 3; ++x)
				{
					std::cout << mat(y, x) << " ";
				}
				std::cout << "\n";
			}
			std::cout << ")\n";
		}

	private:
		Eigen::Matrix3d mat;
	};

	template<class T>
	using Vector = std::vector<T, Eigen::aligned_allocator<T>>;

	inline bool ReadPolygon(Vector<Eigen::Vector2d>& output, const List& vertices, std::shared_ptr<Environment> pEnv, const Transform& transform)
	{
		output.clear();

		for (const Address vertex : vertices.data)
		{
			//CGL_DebugLog(__FUNCTION__);
			const Evaluated value = pEnv->expand(vertex);

			//CGL_DebugLog(__FUNCTION__);
			if (IsType<Record>(value))
			{
				double x = 0, y = 0;
				const Record& pos = As<Record>(value);
				//CGL_DebugLog(__FUNCTION__);
				if (!ReadDouble(x, "x", pos, pEnv) || !ReadDouble(y, "y", pos, pEnv))
				{
					//CGL_DebugLog(__FUNCTION__);
					return false;
				}
				//CGL_DebugLog(__FUNCTION__);
				Eigen::Vector2d v;
				v << x, y;
				//CGL_DebugLog(ToS(v.x()) + ", " + ToS(v.y()));
				output.push_back(transform.product(v));
				//CGL_DebugLog(__FUNCTION__);
			}
			else
			{
				//CGL_DebugLog(__FUNCTION__);
				return false;
			}
		}

		//CGL_DebugLog(__FUNCTION__);
		return true;
	}

	inline void OutputShapeList(std::ostream& os, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform);

	inline void OutputSVGImpl(std::ostream& os, const Record& record, std::shared_ptr<Environment> pEnv, const Transform& parent = Transform())
	{
		const Transform current(record, pEnv);

		const Transform transform = parent * current;

		for (const auto& member : record.values)
		{
			const Evaluated value = pEnv->expand(member.second);

			if (member.first == "vertex" && IsType<List>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygon(polygon, As<List>(value), pEnv, transform) && !polygon.empty())
				{
					//CGL_DebugLog(__FUNCTION__);

					os << "<polygon points=\"";
					for (const auto& vertex : polygon)
					{
						os << vertex.x() << "," << vertex.y() << " ";
					}
					os << "\"/>\n";
				}
				else
				{
					//CGL_DebugLog(__FUNCTION__);
				}
					
			}
			else if (IsType<Record>(value))
			{
				OutputSVGImpl(os, As<Record>(value), pEnv, transform);
			}
			else if (IsType<List>(value))
			{
				OutputShapeList(os, As<List>(value), pEnv, transform);
			}
		}
	}

	inline void OutputShapeList(std::ostream& os, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform)
	{
		for (const Address member : list.data)
		{
			const Evaluated value = pEnv->expand(member);

			if (IsType<Record>(value))
			{
				OutputSVGImpl(os, As<Record>(value), pEnv, transform);
			}
			else if (IsType<List>(value))
			{
				OutputShapeList(os, As<List>(value), pEnv, transform);
			}
		}
	}

	class BoundingRect
	{
	public:
		void add(const Eigen::Vector2d& v)
		{
			if (v.x() < m_min.x())
			{
				m_min.x() = v.x();
			}
			if (v.y() < m_min.y())
			{
				m_min.y() = v.y();
			}
			if (m_max.x() < v.x())
			{
				m_max.x() = v.x();
			}
			if (m_max.y() < v.y())
			{
				m_max.y() = v.y();
			}
		}

		bool intersects(const BoundingRect& other)const
		{
			return std::max(m_min.x(), other.m_min.x()) < std::min(m_max.x(), other.m_max.x())
				&& std::max(m_min.y(), other.m_min.y()) < std::min(m_max.y(), other.m_max.y());
		}

		bool includes(const Eigen::Vector2d& point)const
		{
			return m_min.x() < point.x() && point.x() < m_max.x()
				&& m_min.y() < point.y() && point.y() < m_max.y();
		}

		Eigen::Vector2d pos()const
		{
			return m_min;
		}

		Eigen::Vector2d center()const
		{
			return (m_min + m_max)*0.5;
		}
				
		Eigen::Vector2d width()const
		{
			return m_max - m_min;
		}

	private:
		Eigen::Vector2d m_min = Eigen::Vector2d(DBL_MAX, DBL_MAX);
		Eigen::Vector2d m_max = Eigen::Vector2d(-DBL_MAX, -DBL_MAX);
	};

	inline void GetBoundingBoxImpl(BoundingRect& output, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform);

	inline void GetBoundingBoxImpl(BoundingRect& output, const Record& record, std::shared_ptr<Environment> pEnv, const Transform& parent = Transform())
	{
		const Transform current(record, pEnv);
		const Transform transform = parent * current;

		for (const auto& member : record.values)
		{
			const Evaluated value = pEnv->expand(member.second);

			if (member.first == "vertex" && IsType<List>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygon(polygon, As<List>(value), pEnv, transform) && !polygon.empty())
				{
					for (const auto& vertex : polygon)
					{
						output.add(vertex);
					}
				}

			}
			else if (IsType<Record>(value))
			{
				GetBoundingBoxImpl(output, As<Record>(value), pEnv, transform);
			}
			else if (IsType<List>(value))
			{
				GetBoundingBoxImpl(output, As<List>(value), pEnv, transform);
			}
		}
	}

	inline void GetBoundingBoxImpl(BoundingRect& output, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform)
	{
		for (const Address member : list.data)
		{
			const Evaluated value = pEnv->expand(member);

			if (IsType<Record>(value))
			{
				GetBoundingBoxImpl(output, As<Record>(value), pEnv, transform);
			}
			else if (IsType<List>(value))
			{
				GetBoundingBoxImpl(output, As<List>(value), pEnv, transform);
			}
		}
	}

	inline boost::optional<BoundingRect> GetBoundingBox(const Evaluated& value, std::shared_ptr<Environment> pEnv)
	{
		if (IsType<Record>(value))
		{
			const Record& record = As<Record>(value);
			BoundingRect rect;
			GetBoundingBoxImpl(rect, record, pEnv);
			return rect;
		}

		return boost::none;
	}

	inline bool OutputSVG(std::ostream& os, const Evaluated& value, std::shared_ptr<Environment> pEnv)
	{
		auto boundingBoxOpt = GetBoundingBox(value, pEnv);
		if (IsType<Record>(value) && boundingBoxOpt)
		{
			const BoundingRect& rect = boundingBoxOpt.value();

			//const auto pos = rect.pos();
			const auto widthXY = rect.width();
			const auto center = rect.center();

			const double width = std::max(widthXY.x(), widthXY.y());
			const double halfWidth = width*0.5;

			const Eigen::Vector2d pos = center - Eigen::Vector2d(halfWidth, halfWidth);

			//os << R"(<svg xmlns="http://www.w3.org/2000/svg" width="512" height="512" viewBox="0 0 512 512">)" << "\n";
			os << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << width << R"(" height=")" << width << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << width << R"(">)" << "\n";

			const Record& record = As<Record>(value);
			OutputSVGImpl(os, record, pEnv);

			os << "</svg>" << "\n";

			return true;
		}

		return false;
	}

	class Program
	{
	public:

		Program() :
			pEnv(Environment::Make()),
			evaluator(pEnv)
		{}

		boost::optional<Expr> parse(const std::string& program)
		{
			using namespace cgl;

			Lines lines;

			SpaceSkipper<IteratorT> skipper;
			Parser<IteratorT, SpaceSkipperT> grammer;

			std::string::const_iterator it = program.begin();
			if (!boost::spirit::qi::phrase_parse(it, program.end(), grammer, skipper, lines))
			{
				std::cerr << "Syntax Error: parse failed\n";
				return boost::none;
			}

			if (it != program.end())
			{
				std::cerr << "Syntax Error: ramains input\n" << std::string(it, program.end());
				return boost::none;
			}

			return lines;
		}

		boost::optional<Evaluated> execute(const std::string& program)
		{
			if (auto exprOpt = parse(program))
			{
				try
				{
					return pEnv->expand(boost::apply_visitor(evaluator, exprOpt.value()));
				}
				catch (const cgl::Exception& e)
				{
					std::cerr << "Exception: " << e.what() << std::endl;
				}
			}

			return boost::none;
		}

		bool draw(const std::string& program)
		{
			std::cout << "parse..." << std::endl;
			if (auto exprOpt = parse(program))
			{
				try
				{
					std::cout << "parse succeeded" << std::endl;
					printExpr(exprOpt.value());

					std::cout << "execute..." << std::endl;
					const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
					const Evaluated result = pEnv->expand(lrvalue);
					std::cout << "execute succeeded" << std::endl;

					std::cout << "output SVG..." << std::endl;
					OutputSVG(std::cout, result, pEnv);
					std::cout << "output succeeded" << std::endl;
				}
				catch (const cgl::Exception& e)
				{
					std::cerr << "Exception: " << e.what() << std::endl;
				}
			}

			return false;
		}

		void clear()
		{
			pEnv = Environment::Make();
			evaluator = Eval(pEnv);
		}

		bool test(const std::string& program, const Expr& expr)
		{
			clear();

			if (auto result = execute(program))
			{
				std::shared_ptr<Environment> pEnv2 = Environment::Make();
				Eval evaluator2(pEnv2);

				const Evaluated answer = pEnv->expand(boost::apply_visitor(evaluator2, expr));

				return IsEqualEvaluated(result.value(), answer);
			}

			return false;
		}

	private:

		std::shared_ptr<Environment> pEnv;
		Eval evaluator;
	};
}

int main()
{
	ofs = std::ofstream("log.txt");

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
			Expr expr = lines;
			printExpr(expr);

			std::cout << "eval:\n";
			Evaluated result = evalExpr(lines);

			std::cout << "result:\n";
			printEvaluated(result, nullptr);

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
		testEval1(source, [&](const Evaluated& result) {return IsEqualEvaluated(result, answer); });
	};

testEval(R"(

{a: 3}.a

)", 3);

testEval(R"(

f = (x -> {a: x+10})
f(3).a

)", 13);

/*
testEval(R"(

vec3 = (v -> {
	x:v, y : v, z : v
})
vec3(3)

)", Record("x", 3).append("y", 3).append("z", 3));

using Li = cgl::ListConstractor;
Program program;

program.test(R"(

vec2 = (v -> [
	v, v
])
a = vec2(3)
vec2(a)

)", Li(Li({ 3, 3 }))(Li({ 3, 3 })));


program.test(R"(

vec2 = (v -> [
	v, v
])
vec2(vec2(3))

)", Li(Li({ 3, 3 }))(Li({ 3, 3 })));
*/


/*
testEval(R"(

vec2 = (v -> [
	v, v
])
a = vec2(3)
vec2(a)

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));
*/
/*
testEval(R"(

vec2 = (v -> [
	v, v
])
vec2(vec2(3))

)", List().append(List().append(3).append(3)).append(List().append(3).append(3)));
*/

/*
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


//このプログラムについて、LINES_Aが出力されてLINES_Bが出力される前に落ちるバグ有り
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
	const Evaluated l1vertex1 = As<List>(As<Record>(As<Record>(result).values.at("l1")).values.at("vertex"));
	const Evaluated answer = List().append(Record("x", 0).append("y", 0)).append(Record("x", 2).append("y", 3));
	printEvaluated(answer);
	return IsEqual(l1vertex1, answer);
});

testEval1(R"(

main = {
    x: 1
    y: 2
    r: 1
    theta: 0
    sat(r*cos(theta) == x & r*sin(theta) == y)
    free(r, theta)
}

)", [](const Evaluated& result) {
	return IsEqual(As<Record>(result).values.at("theta"),1.1071487177940905030170654601785);
});
*/


/*

rod = {
    r: 10
    verts: [
        {x:0, y:0}
        {x:r, y:0}
    ]
}

newRod = (x, y -> rod{verts:[{x:x, y:y}, {x:x+r, y:y}]})

rod2 = {
    rods: [newRod(0,0),newRod(10,10),newRod(20,20),newRod(30,30)]

    sat(rods[0].verts[1].x == rods[0+1].verts[0].x & rods[0].verts[1].y == rods[0+1].verts[0].y)
    sat(rods[1].verts[1].x == rods[1+1].verts[0].x & rods[1].verts[1].y == rods[1+1].verts[0].y)
    sat(rods[2].verts[1].x == rods[2+1].verts[0].x & rods[2].verts[1].y == rods[2+1].verts[0].y)

    sat((rods[0].verts[0].x - rods[0].verts[1].x)^2 + (rods[0].verts[0].y - rods[0].verts[1].y)^2 == rods[0].r^2)
    sat((rods[1].verts[0].x - rods[1].verts[1].x)^2 + (rods[1].verts[0].y - rods[1].verts[1].y)^2 == rods[1].r^2)
    sat((rods[2].verts[0].x - rods[2].verts[1].x)^2 + (rods[2].verts[0].y - rods[2].verts[1].y)^2 == rods[2].r^2)
    sat((rods[3].verts[0].x - rods[3].verts[1].x)^2 + (rods[3].verts[0].y - rods[3].verts[1].y)^2 == rods[3].r^2)

    sat(rods[0].verts[0].x == 0 & rods[0].verts[0].y == 0)
    sat(rods[3].verts[1].x == 30 & rods[3].verts[1].y == 0)

    free(rods[0].verts, rods[1].verts, rods[2].verts, rods[3].verts)
}

*/

/*
rod = {
    r: 10
    verts: [
        {x:0, y:0}
        {x:r, y:0}
    ]
}

newRod = (x, y -> rod{verts:[{x:x, y:y}, {x:x+r, y:y}]})

rod2 = {
    rods: [newRod(0,0),newRod(10,10),newRod(20,20),newRod(30,30)]

    for i in 0:2 do(
        sat(rods[i].verts[1].x == rods[i+1].verts[0].x & rods[i].verts[1].y == rods[i+1].verts[0].y)
    )

    for i in 0:3 do(
        sat((rods[i].verts[0].x - rods[i].verts[1].x)^2 + (rods[i].verts[0].y - rods[i].verts[1].y)^2 == rods[i].r^2)
    )

    sat(rods[0].verts[0].x == 0 & rods[0].verts[0].y == 0)
    sat(rods[3].verts[1].x == 30 & rods[3].verts[1].y == 0)

    free(rods[0].verts, rods[1].verts, rods[2].verts, rods[3].verts)
}

EOF
*/
	std::cerr<<"Test Wrong Count: " << eval_wrongs<<std::endl;
	
#endif

/*
shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

stick = shape{
	scale = {x:1, y:3}
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

plus = shape{
	scale = {x:50, y:50}
	a: stick{}
	b: stick{angle = 90}
}

cross = shape{
	pos = {x:256, y:256}
	a: plus{angle = 45}
}

EOF
	*/

	
	{
		cgl::Program program;
		program.draw(R"(
shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

stick = shape{
	scale = {x:1, y:3}
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

plus = shape{
	scale = {x:50, y:50}
	a: stick{}
	b: stick{angle = 90}
}

cross = shape{
	pos = {x:256, y:256}
	a: plus{angle = 45}
}
)");
	}

	{
		cgl::Program program;
		program.draw(R"(

shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

transform = (t, p -> {
	cosT:cos(t.angle)
	sinT:sin(t.angle)
	x:t.pos.x + cosT * t.scale.x * p.x - sinT * t.scale.y * p.y
	y:t.pos.y + sinT* t.scale.x* p.x + cosT* t.scale.y*p.y
})

contact = (p, q -> p.x == q.x & p.y == q.y)

square = shape{
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}
		{x: +1, y: +1}, {x: -1, y: +1}
	]
}

main = shape{
	a: square{scale: {x:10,y:30}}
	b: square{scale: {x:20,y:10}}
	sat(contact(transform(a, a.vertex[0]), transform(b, b.vertex[2])))
	free(a.pos)
}

)");
	}
	

	{
		cgl::Program program;
		program.draw(R"(

transform = (pos, scale, angle, vertex -> {
	cosT:cos(angle)
	sinT:sin(angle)
	x:pos.x + cosT * scale.x * vertex.x - sinT * scale.y * vertex.y
	y:pos.y + sinT* scale.x * vertex.x + cosT* scale.y * vertex.y
})

shape = {
	pos: {x:0, y:0}
	scale: {x:1, y:1}
	angle: 0
}

square = shape{
	vertex: [
		{x: -1, y: -1}, {x: +1, y: -1}, {x: +1, y: +1}, {x: -1, y: +1}
	]
	topLeft:     (->transform(pos, scale, angle, vertex[0]))
	topRight:    (->transform(pos, scale, angle, vertex[1]))
	bottomRight: (->transform(pos, scale, angle, vertex[2]))
	bottomLeft:  (->transform(pos, scale, angle, vertex[3]))
}

contact = (p, q -> (p.x - q.x)*(p.x - q.x) < 1  & (p.y - q.y)*(p.y - q.y) < 1)

main = shape{

    a: square{scale: {x: 50, y: 50}}
    b: square{scale: {x: 100, y: 50}}
    c: b{}

    sat( contact(a.topRight(), b.bottomLeft()) & contact(a.bottomRight(), c.topLeft()) )
    sat( contact(b.bottomRight(), c.topRight()) )
    free(b.pos, b.angle, c.pos, c.angle)
}
)");
	}
	

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

		Program program;
		program.draw(source);
		/*Program program;
		try
		{
			program.draw(source);
		}
		catch (const cgl::Exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
		}*/
	}

	return 0;
}