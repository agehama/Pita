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

	//bool ReadDouble(double& output, const std::string& name, const Record& record, std::shared_ptr<Context> environment);
	bool ReadDoublePacked(double& output, const std::string& name, const PackedRecord& record);

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

	Path ReadPathPacked(const PackedRecord& record);

	PackedRecord WritePathPacked(const Path& path);

	/*struct Transform
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

		Transform(const Record& record, std::shared_ptr<Context> pEnv);

		void init(double px = 0, double py = 0, double sx = 1, double sy = 1, double angle = 0);

		Transform operator*(const Transform& other)const
		{
			return static_cast<Mat3x3>(mat * other.mat);
		}

		Eigen::Vector2d product(const Eigen::Vector2d& v)const;

		void printMat()const;

	private:
		Mat3x3 mat;
	};*/

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

	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record, const TransformPacked& transform = TransformPacked());

	//bool ReadPolygon(Vector<Eigen::Vector2d>& output, const List& vertices, std::shared_ptr<Context> pEnv, const Transform& transform);
	bool ReadPolygonPacked(Vector<Eigen::Vector2d>& output, const PackedList& vertices, const TransformPacked& transform);

	//void GetBoundingBoxImpl(BoundingRect& output, const List& list, std::shared_ptr<Context> pEnv, const Transform& transform);
	//void GetBoundingBoxImpl(BoundingRect& output, const Record& record, std::shared_ptr<Context> pEnv, const Transform& parent = Transform());
	//boost::optional<BoundingRect> GetBoundingBox(const Val& value, std::shared_ptr<Context> pEnv);

	void GetBoundingBoxImplPacked(BoundingRect& output, const PackedList& list, const TransformPacked& transform);
	void GetBoundingBoxImplPacked(BoundingRect& output, const PackedRecord& record, const TransformPacked& parent = TransformPacked());
	boost::optional<BoundingRect> GetBoundingBoxPacked(const PackedVal& value);

	using PolygonsStream = std::multimap<double, std::string>;
	
	std::string GetGeometryType(gg::Geometry* geometry);

	gg::Polygon* ToPolygon(const Vector<Eigen::Vector2d>& exterior);

	void GeosPolygonsConcat(std::vector<gg::Geometry*>& head, const std::vector<gg::Geometry*>& tail);

	//std::vector<gg::Geometry*> GeosFromRecord(const Val& value, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform = cgl::Transform());

	std::vector<gg::Geometry*> GeosFromRecordPacked(const PackedVal& value, const cgl::TransformPacked& transform = cgl::TransformPacked());

	BoundingRect BoundingRectRecordPacked(const PackedVal& value, const cgl::TransformPacked& transform = cgl::TransformPacked());

	//Record GetPolygon(const gg::Polygon* poly, std::shared_ptr<cgl::Context> pEnv);
	PackedRecord GetPolygonPacked(const gg::Polygon* poly);

	//List GetShapesFromGeos(const std::vector<gg::Geometry*>& polygons, std::shared_ptr<cgl::Context> pEnv);

	PackedList GetShapesFromGeosPacked(const std::vector<gg::Geometry*>& polygons);

	bool OutputSVG(std::ostream& os, const PackedVal& value);

	bool OutputSVG2(std::ostream& os, const PackedVal& value, const std::string& name);
}
