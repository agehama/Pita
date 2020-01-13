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

	PackedPolyData GetPolygonVertices(const gg::Polygon* poly)
	{
		std::vector<PackedList> result;

		{
			PackedList polygonList;
			const gg::LineString* outer = poly->getExteriorRing();

			//TODO: Geosの頂点の向きは決まっている？
			if (IsClockWise(outer))
			{
				//始点と終点は同じ座標なので始点は飛ばす
				for (int i = 1; i < static_cast<int>(outer->getNumPoints()); ++i)
				{
					const gg::Coordinate& p = outer->getCoordinateN(i);
					polygonList.add(MakeVec2(p.x, p.y));
				}
			}
			else
			{
				for (int i = static_cast<int>(outer->getNumPoints()) - 1; 0 < i; --i)
				{
					const gg::Coordinate& p = outer->getCoordinateN(i);
					polygonList.add(MakeVec2(p.x, p.y));
				}
			}

			result.push_back(polygonList);
		}

		for (size_t i = 0; i < poly->getNumInteriorRing(); ++i)
		{
			PackedList holeVertexList;
			const gg::LineString* hole = poly->getInteriorRingN(i);

			if (IsClockWise(hole))
			{
				for (int n = static_cast<int>(hole->getNumPoints()) - 1; 0 < n; --n)
				{
					gg::Point* pp = hole->getPointN(n);
					holeVertexList.add(MakeVec2(pp->getX(), pp->getY()));
				}
			}
			else
			{
				for (int n = 1; n < static_cast<int>(hole->getNumPoints()); ++n)
				{
					gg::Point* pp = hole->getPointN(n);
					holeVertexList.add(MakeVec2(pp->getX(), pp->getY()));
				}
			}

			result.push_back(holeVertexList);
		}

		return result;
	}

	PackedList VectorToPackedList(const std::vector<PackedList>& polygons)
	{
		PackedList result;
		for (const auto& polygon : polygons)
		{
			result.add(polygon);
		}

		return result;
	}

	PackedPolyDataType GetPackedListType(const PackedList& packedList)
	{
		if (packedList.data.empty())
		{
			return PackedPolyDataType::POLYGON;
		}

		const auto& front = packedList.data.front().value;
		if (auto opt = AsOpt<PackedRecord>(front))
		{
			return PackedPolyDataType::POLYGON;
		}

		return PackedPolyDataType::MULTIPOLYGON;
	}

	PackedList GetLineStringVertices(const gg::LineString* line)
	{
		const auto appendCoord = [&](PackedList& list, double x, double y)
		{
			list.add(MakeRecord("x", x, "y", y));
		};

		PackedList result;

		//LineStringはPolygonと異なりループしない（頂点が重ならない）ので飛ばさずに読む
		for (int i = 0; i < static_cast<int>(line->getNumPoints()); ++i)
		{
			const gg::Coordinate& p = line->getCoordinateN(i);
			appendCoord(result, p.x, p.y);
		}

		return result;
	}

	PackedRecord GetLineStringPacked(const gg::LineString* line)
	{
		PackedRecord result;
		result.add("line", GetLineStringVertices(line));

		return result;
	}

#ifdef commentout
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
#endif
}
