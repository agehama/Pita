#pragma once
#include <Eigen/Core>

#include <geos/geom/Point.h>
#include <geos/geom/Polygon.h>
#include <geos/geom/LineString.h>
#include <geos/geom/LinearRing.h>
#include <geos/geom/LineSegment.h>
#include <geos/geom/MultiLineString.h>
#include <geos/geom/CoordinateSequenceFactory.h>
#include <geos/geom/CoordinateArraySequence.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/geom/Coordinate.h>
#include <geos/geom/CoordinateFilter.h>
#include <geos/index/quadtree/Quadtree.h>
#include <geos/index/ItemVisitor.h>
#include <geos/geom/IntersectionMatrix.h>
#include <geos/geomgraph/PlanarGraph.h>
#include <geos/operation/linemerge/LineMergeGraph.h>
#include <geos/planargraph.h>
#include <geos/planargraph/Edge.h>
#include <geos/planargraph/Node.h>
#include <geos/planargraph/DirectedEdge.h>
#include <geos/operation/polygonize/Polygonizer.h>
#include <geos/opBuffer.h>
#include <geos/geom/PrecisionModel.h>
#include <geos/operation/linemerge/LineMerger.h>
#include <geos/opDistance.h>
#include <geos/operation/predicate/RectangleContains.h>
#include <geos/triangulate/VoronoiDiagramBuilder.h>

#include "Node.hpp"
#include "Environment.hpp"

namespace cgl
{
	bool ReadDouble(double& output, const std::string& name, const Record& record, std::shared_ptr<Environment> environment);

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

		Transform(const Record& record, std::shared_ptr<Environment> pEnv);

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

	template<class T>
	using Vector = std::vector<T, Eigen::aligned_allocator<T>>;

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

	bool ReadPolygon(Vector<Eigen::Vector2d>& output, const List& vertices, std::shared_ptr<Environment> pEnv, const Transform& transform);

	void GetBoundingBoxImpl(BoundingRect& output, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform);

	void GetBoundingBoxImpl(BoundingRect& output, const Record& record, std::shared_ptr<Environment> pEnv, const Transform& parent = Transform());

	boost::optional<BoundingRect> GetBoundingBox(const Evaluated& value, std::shared_ptr<Environment> pEnv);

	using PolygonsStream = std::multimap<double, std::string>;
	
	namespace gg = geos::geom;
	namespace gob = geos::operation::buffer;
	namespace god = geos::operation::distance;
	namespace gt = geos::triangulate;

	std::string GetGeometryType(gg::Geometry* geometry);

	gg::Polygon* ToPolygon(const Vector<Eigen::Vector2d>& exterior);

	void GeosPolygonsConcat(std::vector<gg::Geometry*>& head, const std::vector<gg::Geometry*>& tail);

	std::vector<gg::Geometry*> GeosFromList(const cgl::List& list, std::shared_ptr<cgl::Environment> pEnv, const cgl::Transform& transform);

	std::vector<gg::Geometry*> GeosFromRecordImpl(const cgl::Record& record, std::shared_ptr<cgl::Environment> pEnv, const cgl::Transform& parent = cgl::Transform());

	std::vector<gg::Geometry*> GeosFromRecord(const Evaluated& value, std::shared_ptr<cgl::Environment> pEnv, const cgl::Transform& transform = cgl::Transform());

	Record GetPolygon(const gg::Polygon* poly, std::shared_ptr<cgl::Environment> pEnv);

	List GetShapesFromGeos(const std::vector<gg::Geometry*>& polygons, std::shared_ptr<cgl::Environment> pEnv);

	void OutputPolygonsStream(PolygonsStream& ps, const gg::Polygon* polygon);

	bool OutputSVG(std::ostream& os, const Evaluated& value, std::shared_ptr<Environment> pEnv);
}
