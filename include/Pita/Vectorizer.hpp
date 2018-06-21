#pragma once
#include <tuple>
#include <cfloat>
#include <numeric>
#include <Eigen/Core>

#include "Node.hpp"
#include "Context.hpp"

namespace geos
{
	namespace geom
	{
		class Geometry;
		class Polygon;
		class LineString;
		class CoordinateArraySequence;
	}
	namespace operation
	{
		namespace buffer {}
		namespace distance {}
	}
}

namespace cgl
{
	namespace gg = geos::geom;
	namespace gob = geos::operation::buffer;
	namespace god = geos::operation::distance;

	void GetQuadraticBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, int n, bool includesEndPoint);
	void GetCubicBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, const Eigen::Vector2d& p3, int n, bool includesEndPoint);

	std::string GetGeometryType(gg::Geometry* geometry);
	gg::Polygon* ToPolygon(const Vector<Eigen::Vector2d>& exterior);
	gg::LineString* ToLineString(const Vector<Eigen::Vector2d>& exterior);
	void GeosPolygonsConcat(std::vector<gg::Geometry*>& head, const std::vector<gg::Geometry*>& tail);

	struct Color
	{
		int r = 0, g = 0, b = 0;

		std::string toString()const
		{
			std::stringstream ss;
			ss << "rgb(" << r << ", " << g << ", " << b << ")";
			return ss.str();
		}
	};

	class PitaGeometry
	{
	public:
		PitaGeometry() = default;

		PitaGeometry(gg::Geometry* shape, const Color& color) :
			shape(shape),
			color(color)
		{}

		gg::Geometry* shape;
		Color color;
	};

	struct BaseLineOffset
	{
		double x = 0, y = 0;
		double nx = 0, ny = 0;
		double angle = 0;
	};

	struct Path
	{
		std::unique_ptr<gg::CoordinateArraySequence> cs;
		std::vector<double> distances;

		double length()const
		{
			return distances.back();
			//return std::accumulate(distances.begin(), distances.end(), 0.0);
		}

		bool empty()const
		{
			return distances.empty();
		}

		Path clone()const;

		BaseLineOffset getOffset(double offset)const;
	};

	class BoundingRect
	{
	public:
		BoundingRect() = default;

		BoundingRect(double min_x, double min_y, double max_x, double max_y) :
			m_min(min_x, min_y),
			m_max(max_x, max_y)
		{}

		BoundingRect(const Vector<Eigen::Vector2d>& vs)
		{
			add(vs);
		}

		void add(const Eigen::Vector2d& v);

		void add(const Vector<Eigen::Vector2d>& vs);

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

		double area()const
		{
			const auto wh = width();
			return wh.x()*wh.y();
		}

		Eigen::Vector2d minPos()const
		{
			return m_min;
		}

		Eigen::Vector2d maxPos()const
		{
			return m_max;
		}

		bool isEmpty()const
		{
			return m_min.x() == DBL_MAX && m_min.y() == DBL_MAX
				&& m_max.x() == -DBL_MAX && m_max.y() == -DBL_MAX;
		}

	private:
		Eigen::Vector2d m_min = Eigen::Vector2d(DBL_MAX, DBL_MAX);
		Eigen::Vector2d m_max = Eigen::Vector2d(-DBL_MAX, -DBL_MAX);
	};

	struct TransformPacked
	{
		using Mat3x3 = Eigen::Matrix<double, 3, 3, 0, 3, 3>;

		TransformPacked()
		{
			init();
		}

		TransformPacked(double px, double py, double sx = 1, double sy = 1, double angle = 0)
		{
			init(px, py, sx, sy, angle);
		}

		TransformPacked(const Mat3x3& mat) :mat(mat) {}

		TransformPacked(const PackedRecord& record);

		void init(double px = 0, double py = 0, double sx = 1, double sy = 1, double angle = 0);

		TransformPacked operator*(const TransformPacked& other)const
		{
			return static_cast<Mat3x3>(mat * other.mat);
		}

		Eigen::Vector2d product(const Eigen::Vector2d& v)const;

		void printMat()const;

	private:
		Mat3x3 mat;
	};

	//Compress to ShapeRecord
	PackedRecord WritePathPacked(const Path& path);
	PackedRecord GetPolygonPacked(const gg::Polygon* poly);
	PackedList GetShapesFromGeosPacked(const std::vector<gg::Geometry*>& polygons);

	//Convert ShapeRecord
	bool ReadDoublePacked(double& output, const std::string& name, const PackedRecord& record);
	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record);
	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record, const TransformPacked& transform);
	Path ReadPathPacked(const PackedRecord& record);
	bool ReadColorPacked(Color& output, const PackedRecord& record, const TransformPacked& transform);
	bool ReadPolygonPacked(Vector<Eigen::Vector2d>& output, const PackedList& vertices, const TransformPacked& transform);

	//Interpret ShapeRecord
	BoundingRect BoundingRectRecordPacked(const PackedVal& value);
	std::vector<gg::Geometry*> GeosFromRecordPacked(const PackedVal& value, const cgl::TransformPacked& transform = cgl::TransformPacked());
	bool OutputSVG2(std::ostream& os, const PackedVal& value, const std::string& name);
}
