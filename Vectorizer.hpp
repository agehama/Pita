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

#ifdef _DEBUG
#pragma comment(lib, "geos_d.lib")
#else
#pragma comment(lib, "geos.lib")
#endif

#include "Node.hpp"
#include "Environment.hpp"

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

	class BoundingRect
	{
	public:

		BoundingRect() = default;

		BoundingRect(const Vector<Eigen::Vector2d>& vs)
		{
			add(vs);
		}

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

		void add(const Vector<Eigen::Vector2d>& vs)
		{
			for (const auto& v : vs)
			{
				add(v);
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

		double area()const
		{
			const auto wh = width();
			return wh.x()*wh.y();
		}

	private:
		Eigen::Vector2d m_min = Eigen::Vector2d(DBL_MAX, DBL_MAX);
		Eigen::Vector2d m_max = Eigen::Vector2d(-DBL_MAX, -DBL_MAX);
	};

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

	inline void GetBoundingBoxImpl(BoundingRect& output, const List& list, std::shared_ptr<Environment> pEnv, const Transform& transform);

	inline void GetBoundingBoxImpl(BoundingRect& output, const Record& record, std::shared_ptr<Environment> pEnv, const Transform& parent = Transform())
	{
		const Transform current(record, pEnv);
		const Transform transform = parent * current;

		for (const auto& member : record.values)
		{
			const Evaluated value = pEnv->expand(member.second);

			if (member.first == "polygon" && IsType<List>(value))
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
			else if (member.first == "polygons" && IsType<List>(value))
			{
				const List& polygons = As<List>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					const Evaluated& polygonVertices = pEnv->expand(polygonAddress);

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygon(polygon, As<List>(polygonVertices), pEnv, transform) && !polygon.empty())
					{
						for (const auto& vertex : polygon)
						{
							output.add(vertex);
						}
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

	using PolygonsStream = std::multimap<double, std::string>;
	
	namespace gg = geos::geom;
	namespace gob = geos::operation::buffer;
	namespace god = geos::operation::distance;
	namespace gt = geos::triangulate;

	gg::Polygon* ToPolygon(const Vector<Eigen::Vector2d>& exterior)
	{
		gg::CoordinateArraySequence pts;

		for (int i = 0; i < exterior.size(); ++i)
		{
			pts.add(gg::Coordinate(exterior[i].x(), exterior[i].y()));
		}

		if (!pts.empty())
		{
			pts.add(pts.front());
		}

		gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();
		return factory->createPolygon(factory->createLinearRing(pts), {});
	}

	inline void GeosPolygonsConcat(std::vector<gg::Geometry*>& head, const std::vector<gg::Geometry*>& tail)
	{
		/*if (tail.empty())
		{
			return;
		}

		gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();

		gg::Geometry* pgeometry = factory->createEmptyGeometry();
		for (int i = 0; i < tail.size(); ++i)
		{
			pgeometry = pgeometry->Union(tail[i]);
		}

		for (int i = 0; i < head.size(); ++i)
		{
			pgeometry = pgeometry->Union(head[i]);
		}

		head.clear();
		head.push_back(pgeometry);*/
		head.insert(head.end(), tail.begin(), tail.end());
	};

	inline std::vector<gg::Geometry*> GeosFromList(const cgl::List& list, std::shared_ptr<cgl::Environment> pEnv, const cgl::Transform& transform);

	inline std::vector<gg::Geometry*> GeosFromRecordImpl(const cgl::Record& record, std::shared_ptr<cgl::Environment> pEnv, const cgl::Transform& parent = cgl::Transform())
	{
		const cgl::Transform current(record, pEnv);

		const cgl::Transform transform = parent * current;

		std::vector<gg::Geometry*> currentPolygons;
		std::vector<gg::Geometry*> currentHoles;

		for (const auto& member : record.values)
		{
			const cgl::Evaluated value = pEnv->expand(member.second);

			if (member.first == "polygon" && cgl::IsType<cgl::List>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygon(polygon, cgl::As<cgl::List>(value), pEnv, transform) && !polygon.empty())
				{
					//shapes.emplace_back(CGLPolygon(polygon));
					currentPolygons.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "hole" && cgl::IsType<cgl::List>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygon(polygon, cgl::As<cgl::List>(value), pEnv, transform) && !polygon.empty())
				{
					//shapes.emplace_back(CGLPolygon(polygon));
					currentHoles.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "polygons" && IsType<List>(value))
			{
				const List& polygons = As<List>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					const Evaluated& polygonVertices = pEnv->expand(polygonAddress);

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygon(polygon, As<List>(polygonVertices), pEnv, transform) && !polygon.empty())
					{
						currentPolygons.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "holes" && IsType<List>(value))
			{
				const List& holes = As<List>(value);
				for (const auto& holeAddress : holes.data)
				{
					const Evaluated& hole = pEnv->expand(holeAddress);

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygon(polygon, As<List>(hole), pEnv, transform) && !polygon.empty())
					{
						currentHoles.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (cgl::IsType<cgl::Record>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromRecordImpl(cgl::As<cgl::Record>(value), pEnv, transform));
			}
			else if (cgl::IsType<cgl::List>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromList(cgl::As<cgl::List>(value), pEnv, transform));
			}
		}

		gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();

		if (currentPolygons.empty())
		{
			return{};
		}
		else if (currentHoles.empty())
		{
			return currentPolygons;
		}
		else
		{
			for (int s = 0; s < currentPolygons.size(); ++s)
			{
				//std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
				gg::Geometry* erodeGeometry = currentPolygons[s];
				
				for (int d = 0; d < currentHoles.size(); ++d)
				{
					erodeGeometry = erodeGeometry->difference(currentHoles[d]);
					//std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

					if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
					{
					}
					else if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						currentPolygons.erase(currentPolygons.begin() + s);

						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);
						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							currentPolygons.insert(currentPolygons.begin() + s, polygons->getGeometryN(i)->clone());
						}

						erodeGeometry = currentPolygons[s];
					}
					else
					{
						std::cout << __FUNCTION__ << " Differenceの結果が予期せぬデータ形式" << __LINE__ << std::endl;
					}
				}
				//std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
				currentPolygons[s] = erodeGeometry;
			}

			return currentPolygons;

			/*gg::MultiPolygon* accumlatedPolygons = factory->createMultiPolygon(currentPolygons);
			gg::MultiPolygon* accumlatedHoles = factory->createMultiPolygon(currentHoles);

			return{ accumlatedPolygons->difference(accumlatedHoles) };*/
		}
	}

	inline std::vector<gg::Geometry*> GeosFromList(const cgl::List& list, std::shared_ptr<cgl::Environment> pEnv, const cgl::Transform& transform)
	{
		std::vector<gg::Geometry*> currentPolygons;
		for (const cgl::Address member : list.data)
		{
			const cgl::Evaluated value = pEnv->expand(member);

			if (cgl::IsType<cgl::Record>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromRecordImpl(cgl::As<cgl::Record>(value), pEnv, transform));
			}
			else if (cgl::IsType<cgl::List>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromList(cgl::As<cgl::List>(value), pEnv, transform));
			}
		}
		return currentPolygons;
	}

	inline std::vector<gg::Geometry*> GeosFromRecord(const Evaluated& value, std::shared_ptr<cgl::Environment> pEnv, const cgl::Transform& transform = cgl::Transform())
	{
		if (cgl::IsType<cgl::Record>(value))
		{
			const cgl::Record& record = cgl::As<cgl::Record>(value);
			return GeosFromRecordImpl(record, pEnv, transform);
		}
		if (cgl::IsType<cgl::List>(value))
		{
			const cgl::List& list = cgl::As<cgl::List>(value);
			return GeosFromList(list, pEnv, transform);
		}

		return{};
	}

	inline Record GetPolygon(const gg::Polygon* poly, std::shared_ptr<cgl::Environment> pEnv)
	{
		const auto coord = [&](double x, double y)
		{
			Record record;
			record.append("x", pEnv->makeTemporaryValue(x));
			record.append("y", pEnv->makeTemporaryValue(y));
			return record;
		};

		const auto appendCoord = [&](List& list, double x, double y)
		{
			list.append(pEnv->makeTemporaryValue(coord(x, y)));
		};

		Record result;

		{
			List polygonList;
			const gg::LineString* outer = poly->getExteriorRing();
			//for (int i = static_cast<int>(outer->getNumPoints()) - 1; 0 < i; --i)//始点と終点は同じ座標なので最後だけ飛ばす
			for (int i = 1; i < static_cast<int>(outer->getNumPoints()); ++i)//始点と終点は同じ座標なので最後だけ飛ばす
			{
				const gg::Coordinate& p = outer->getCoordinateN(i);
				appendCoord(polygonList, p.x, p.y);
			}
			result.append("polygon", pEnv->makeTemporaryValue(polygonList));
		}
		
		{
			List holeList;
			for (size_t i = 0; i < poly->getNumInteriorRing(); ++i)
			{
				List holeVertexList;

				const gg::LineString* hole = poly->getInteriorRingN(i);

				for (int n = static_cast<int>(hole->getNumPoints()) - 1; 0 < n; --n)
					//for (int n = 0; n < hole->getNumPoints(); ++n)
				{
					gg::Point* pp = hole->getPointN(n);
					appendCoord(holeVertexList, pp->getX(), pp->getY());
					//const gg::Coordinate& p = hole->getCoordinateN(n);
					//holePoints.emplace_back(p.x, p.y);
				}

				holeList.append(pEnv->makeTemporaryValue(holeVertexList));
			}
			result.append("holes", pEnv->makeTemporaryValue(holeList));
		}
		
		return result;
	}

	inline List GetShapesFromGeos(const std::vector<gg::Geometry*>& polygons, std::shared_ptr<cgl::Environment> pEnv)
	{
		List resultShapes;
		
		for (size_t i = 0; i < polygons.size(); ++i)
		{
			const gg::Geometry* shape = polygons[i];

			gg::GeometryTypeId id;
			switch (shape->getGeometryTypeId())
			{
			case geos::geom::GEOS_POINT:
				break;
			case geos::geom::GEOS_LINESTRING:
				break;
			case geos::geom::GEOS_LINEARRING:
				break;
			case geos::geom::GEOS_POLYGON:
			{
				const Record record = GetPolygon(dynamic_cast<const gg::Polygon*>(shape), pEnv);
				resultShapes.append(pEnv->makeTemporaryValue(record));
				break;
			}
			case geos::geom::GEOS_MULTIPOINT:
				break;
			case geos::geom::GEOS_MULTILINESTRING:
				break;
			case geos::geom::GEOS_MULTIPOLYGON:
				break;
			case geos::geom::GEOS_GEOMETRYCOLLECTION:
				break;
			default:
				break;
			}
		}

		return resultShapes;
	}

	inline void OutputPolygonsStream(PolygonsStream& ps, const gg::Polygon* polygon)
	{
		{
			std::stringstream ss;

			const gg::LineString* outer = polygon->getExteriorRing();
			const double area = polygon->getEnvelope()->getArea();

			ss << "<polygon fill=\"black\" points=\"";
			for (int i = static_cast<int>(outer->getNumPoints()) - 1; 0 < i; --i)
			{
				const gg::Coordinate& p = outer->getCoordinateN(i);
				ss << p.x << "," << p.y << " ";
			}
			ss << "\"/>\n";

			ps.emplace(area, ss.str());
		}

		for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
		{
			std::stringstream ss;

			const gg::LineString* hole = polygon->getInteriorRingN(i);
			const double area = hole->getEnvelope()->getArea();

			ss << "<polygon fill=\"white\" points=\"";
			for (int n = static_cast<int>(hole->getNumPoints()) - 1; 0 < n; --n)
				//for (int n = 0; n < hole->getNumPoints(); ++n)
			{
				gg::Point* p = hole->getPointN(n);
				ss << p->getX() << "," << p->getY() << " ";
			}
			ss << "\"/>\n";

			ps.emplace(area, ss.str());
		}
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

			os << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << width << R"(" height=")" << width << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << width << R"(">)" << "\n";

			PolygonsStream ps;
			std::vector<gg::Geometry*> geometries = GeosFromRecord(value, pEnv);
			for (gg::Geometry* geometry : geometries)
			{
				if (geometry->getGeometryTypeId() == gg::GeometryTypeId::GEOS_POLYGON)
				{
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);
					OutputPolygonsStream(ps, polygon);
				}
				else if (geometry->getGeometryTypeId() == gg::GeometryTypeId::GEOS_MULTIPOLYGON)
				{
					const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(geometry);
					for (int i = 0; i < polygons->getNumGeometries(); ++i)
					{
						const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(polygons->getGeometryN(i));
						OutputPolygonsStream(ps, polygon);
					}
				}
			}

			//const Record& record = As<Record>(value);
			//OutputSVGImpl(ps, record, pEnv);
			for (auto it = ps.rbegin(); it != ps.rend(); ++it)
			{
				os << it->second;
			}

			os << "</svg>" << "\n";

			return true;
		}

		return false;
	}
}
