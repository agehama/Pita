#pragma once
#include <Eigen/Core>

#include "Node.hpp"
#include "Context.hpp"

namespace geos
{
	namespace geom
	{
		class Geometry;
		class Polygon;
	}
	namespace operation
	{
		namespace buffer {}
		namespace distance {}
	}
}

namespace cgl
{
	bool ReadDouble(double& output, const std::string& name, const Record& record, std::shared_ptr<Context> environment);

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
	};

	class BoundingRect
	{
	public:

		BoundingRect() = default;

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

	private:
		Eigen::Vector2d m_min = Eigen::Vector2d(DBL_MAX, DBL_MAX);
		Eigen::Vector2d m_max = Eigen::Vector2d(-DBL_MAX, -DBL_MAX);
	};

	bool ReadPolygon(Vector<Eigen::Vector2d>& output, const List& vertices, std::shared_ptr<Context> pEnv, const Transform& transform);

	void GetBoundingBoxImpl(BoundingRect& output, const List& list, std::shared_ptr<Context> pEnv, const Transform& transform);

	void GetBoundingBoxImpl(BoundingRect& output, const Record& record, std::shared_ptr<Context> pEnv, const Transform& parent = Transform());

	boost::optional<BoundingRect> GetBoundingBox(const Evaluated& value, std::shared_ptr<Context> pEnv);

	using PolygonsStream = std::multimap<double, std::string>;
	
	namespace gg = geos::geom;
	namespace gob = geos::operation::buffer;
	namespace god = geos::operation::distance;
	//namespace gt = geos::triangulate;

	std::string GetGeometryType(gg::Geometry* geometry);

	gg::Polygon* ToPolygon(const Vector<Eigen::Vector2d>& exterior);

	void GeosPolygonsConcat(std::vector<gg::Geometry*>& head, const std::vector<gg::Geometry*>& tail);

	std::vector<gg::Geometry*> GeosFromList(const cgl::List& list, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform);

	std::vector<gg::Geometry*> GeosFromRecordImpl(const cgl::Record& record, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& parent = cgl::Transform());

	std::vector<gg::Geometry*> GeosFromRecord(const Evaluated& value, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform = cgl::Transform());

	Record GetPolygon(const gg::Polygon* poly, std::shared_ptr<cgl::Context> pEnv);

	List GetShapesFromGeos(const std::vector<gg::Geometry*>& polygons, std::shared_ptr<cgl::Context> pEnv);

	void OutputPolygonsStream(PolygonsStream& ps, const gg::Polygon* polygon);

	bool OutputSVG(std::ostream& os, const Evaluated& value, std::shared_ptr<Context> pEnv);
}
