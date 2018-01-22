#pragma once
#include <Eigen/Core>

#include "Node.hpp"
#include "Printer.hpp"
#include "Environment.hpp"
#include "Evaluator.hpp"
#include "OptimizationEvaluator.hpp"
#include "Parser.hpp"

extern std::ofstream ofs;
extern bool calculating;

namespace cgl
{
	inline bool ReadDouble(double& output, const std::string& name, const Record& record, std::shared_ptr<Environment> environment)
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
		using Mat3x3 = Eigen::Matrix<double, 3, 3, 0, 3, 3>;

		Transform()
		{
			init();
		}

		Transform(double px, double py, double sx = 1, double sy = 1, double angle = 0)
		{
			init(px, py, sx, sy, angle);
		}

		Transform(const Mat3x3& mat) :mat(mat) {}

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
			return static_cast<Mat3x3>(mat * other.mat);
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
		Mat3x3 mat;
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
				//std::cerr << "Syntax Error: parse failed\n";
				std::cout << "Syntax Error: parse failed\n";
				return boost::none;
			}

			if (it != program.end())
			{
				//std::cerr << "Syntax Error: ramains input\n" << std::string(it, program.end());
				std::cout << "Syntax Error: ramains input\n" << std::string(it, program.end());
				return boost::none;
			}

			Expr result = lines;
			return result;
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

		bool draw(const std::string& program, bool logOutput = true)
		{
			if (logOutput) std::cout << "parse..." << std::endl;
			
			if (auto exprOpt = parse(program))
			{
				try
				{
					if (logOutput)
					{
						std::cout << "parse succeeded" << std::endl;
						printExpr(exprOpt.value());
					}

					if (logOutput) std::cout << "execute..." << std::endl;
					const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
					const Evaluated result = pEnv->expand(lrvalue);
					if (logOutput) std::cout << "execute succeeded" << std::endl;

					if (logOutput) std::cout << "output SVG..." << std::endl;
					OutputSVG(std::cout, result, pEnv);
					if (logOutput) std::cout << "output succeeded" << std::endl;
				}
				catch (const cgl::Exception& e)
				{
					std::cerr << "Exception: " << e.what() << std::endl;
				}
			}

			return false;
		}

		void execute1(const std::string& program, bool logOutput = true)
		{
			clear();

			if (logOutput)
			{
				std::cout << "parse..." << std::endl;
				std::cout << program << std::endl;
			}

			if (auto exprOpt = parse(program))
			{
				try
				{
					if (logOutput)
					{
						std::cout << "parse succeeded" << std::endl;
						printExpr(exprOpt.value(), std::cout);
					}

					if (logOutput) std::cout << "execute..." << std::endl;
					const LRValue lrvalue = boost::apply_visitor(evaluator, exprOpt.value());
					evaluated = pEnv->expand(lrvalue);
					if (logOutput)
					{
						std::cout << "execute succeeded" << std::endl;
						printEvaluated(evaluated.value(), pEnv, std::cout, 0);
					}

					succeeded = true;
				}
				catch (const cgl::Exception& e)
				{
					//std::cerr << "Exception: " << e.what() << std::endl;
					std::cout << "Exception: " << e.what() << std::endl;

					succeeded = false;
				}	
			}
			else
			{
				succeeded = false;
			}

			calculating = false;
		}

		void clear()
		{
			pEnv = Environment::Make();
			evaluated = boost::none;
			evaluator = Eval(pEnv);
			succeeded = false;
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

		std::shared_ptr<Environment> getEnvironment()
		{
			return pEnv;
		}

		boost::optional<Evaluated>& getEvaluated()
		{
			return evaluated;
		}

		bool isSucceeded()const
		{
			return succeeded;
		}

	private:
		std::shared_ptr<Environment> pEnv;
		Eval evaluator;
		boost::optional<Evaluated> evaluated;
		bool succeeded;
	};
}
