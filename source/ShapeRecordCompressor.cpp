#include <Eigen/Core>

#include <geos/geom.h>
#include <geos/opBuffer.h>
#include <geos/opDistance.h>

#include <Pita/Node.hpp>
#include <Pita/Context.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/Printer.hpp>

namespace cgl
{
	PackedRecord WritePathPacked(const Path& path)
	{
		PackedRecord result;
		auto& cs = path.cs;

		PackedList polygonList;
		for (size_t i = 0; i < cs->size(); ++i)
		{
			polygonList.add(MakeRecord("x", cs->getX(i), "y", cs->getY(i)));
		}

		result.add("line", polygonList);
		return result;
	}

	PackedRecord GetPolygonPacked(const gg::Polygon* poly)
	{
		const auto appendCoord = [&](PackedList& list, double x, double y)
		{
			list.add(MakeRecord("x", x, "y", y));
		};

		PackedRecord result;
		{
			PackedList polygonList;
			const gg::LineString* outer = poly->getExteriorRing();

			int pointsCount = 0;
			for (int i = 1; i < static_cast<int>(outer->getNumPoints()); ++i)//始点と終点は同じ座標なので最後だけ飛ばす
			{
				const gg::Coordinate& p = outer->getCoordinateN(i);
				appendCoord(polygonList, p.x, p.y);
				++pointsCount;
				//std::cout << pointsCount << " | (" << p.x << ", " << p.y << ")\n";
			}

			result.add("polygon", polygonList);
		}

		{
			PackedList holeList;
			for (size_t i = 0; i < poly->getNumInteriorRing(); ++i)
			{
				PackedList holeVertexList;
				const gg::LineString* hole = poly->getInteriorRingN(i);

				for (int n = static_cast<int>(hole->getNumPoints()) - 1; 0 < n; --n)
				{
					gg::Point* pp = hole->getPointN(n);
					appendCoord(holeVertexList, pp->getX(), pp->getY());
				}

				holeList.add(holeVertexList);
			}

			result.add("holes", holeList);
		}

		return result;
	}

	PackedRecord GetLineStringPacked(const gg::LineString* line)
	{
		const auto appendCoord = [&](PackedList& list, double x, double y)
		{
			list.add(MakeRecord("x", x, "y", y));
		};

		PackedRecord result;
		{
			PackedList polygonList;

			//LineStringはPolygonと異なりループしない（頂点が重ならない）ので飛ばさずに読む
			for (int i = 0; i < static_cast<int>(line->getNumPoints()); ++i)
			{
				const gg::Coordinate& p = line->getCoordinateN(i);
				appendCoord(polygonList, p.x, p.y);
			}

			result.add("line", polygonList);
		}

		return result;
	}

	PackedList GetShapesFromGeosPacked(const std::vector<gg::Geometry*>& polygons)
	{
		PackedList resultShapes;

		for (size_t i = 0; i < polygons.size(); ++i)
		{
			const gg::Geometry* shape = polygons[i];

			gg::GeometryTypeId id;
			switch (shape->getGeometryTypeId())
			{
			case geos::geom::GEOS_POINT:
				break;
			case geos::geom::GEOS_LINESTRING:
			{
				resultShapes.add(GetLineStringPacked(dynamic_cast<const gg::LineString*>(shape)));
				break;
			}
			case geos::geom::GEOS_LINEARRING:
				break;
			case geos::geom::GEOS_POLYGON:
			{
				resultShapes.add(GetPolygonPacked(dynamic_cast<const gg::Polygon*>(shape)));
				break;
			}
			case geos::geom::GEOS_MULTIPOINT:
				break;
			case geos::geom::GEOS_MULTILINESTRING:
			{
				const gg::MultiLineString* lines = dynamic_cast<const gg::MultiLineString*>(shape);
				for (int i = 0; i < lines->getNumGeometries(); ++i)
				{
					const gg::LineString* line = dynamic_cast<const gg::LineString*>(lines->getGeometryN(i));
					resultShapes.add(GetLineStringPacked(line));
				}
				break;
			}
			case geos::geom::GEOS_MULTIPOLYGON:
			{
				const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(shape);
				for (int i = 0; i < polygons->getNumGeometries(); ++i)
				{
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(polygons->getGeometryN(i));
					resultShapes.add(GetPolygonPacked(polygon));
				}
				break;
			}
			case geos::geom::GEOS_GEOMETRYCOLLECTION:
				break;
			default:
				break;
			}
		}

		return resultShapes;
	}
}
