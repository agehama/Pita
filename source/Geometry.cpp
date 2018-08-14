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
	void GetQuadraticBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, int n, bool includesEndPoint)
	{
		for (int i = 0; i < n; ++i)
		{
			const double t = 1.0*i / n;
			output.push_back(p0*(1.0 - t)*(1.0 - t) + p1 * 2.0*(1.0 - t)*t + p2 * t*t);
		}

		if (includesEndPoint)
		{
			output.push_back(p2);
		}
	}

	void GetCubicBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, const Eigen::Vector2d& p3, int n, bool includesEndPoint)
	{
		for (int i = 0; i < n; ++i)
		{
			const double t = 1.0*i / n;
			output.push_back(p0*(1.0 - t)*(1.0 - t)*(1.0 - t) + p1 * 3.0*(1.0 - t)*(1.0 - t)*t + p2 * 3.0*(1.0 - t)*t*t + p3 * t*t*t);
		}

		if (includesEndPoint)
		{
			output.push_back(p3);
		}
	}

	bool IsClockWise(const Vector<Eigen::Vector2d>& closedPath)
	{
		double sum = 0;

		for (int i = 0; i + 1 < closedPath.size(); ++i)
		{
			const auto& p1 = closedPath[i];
			//const auto& p2 = closedPath[(i + 1) % closedPath.size()];
			const auto& p2 = closedPath[i + 1];

			sum += (p2.x() - p1.x())*(p2.y() + p1.y());
		}

		{
			const auto& p1 = closedPath[closedPath.size() - 1];
			const auto& p2 = closedPath[0];

			sum += (p2.x() - p1.x())*(p2.y() + p1.y());
		}

		return sum < 0.0;
	}

	bool IsClockWise(const gg::LineString* closedPath)
	{
		double sum = 0;

		for (size_t p = 0; p + 1 < closedPath->getNumPoints(); ++p)
		{
			const gg::Coordinate& p1 = closedPath->getCoordinateN(p);
			const gg::Coordinate& p2 = closedPath->getCoordinateN(p + 1);
			sum += (p2.x - p1.x)*(p2.y + p1.y);
		}
		{
			const gg::Coordinate& p1 = closedPath->getCoordinateN(closedPath->getNumPoints() - 1);
			const gg::Coordinate& p2 = closedPath->getCoordinateN(0);
			sum += (p2.x - p1.x)*(p2.y + p1.y);
		}

		return sum < 0.0;
	}

	std::tuple<bool, std::unique_ptr<gg::Geometry>> IsClockWise(std::unique_ptr<gg::Geometry> pLineString)
	{
		double sum = 0;

		const gg::LineString* closedPath = dynamic_cast<const gg::LineString*>(pLineString.get());

		for (size_t p = 0; p + 1 < closedPath->getNumPoints(); ++p)
		{
			const gg::Coordinate& p1 = closedPath->getCoordinateN(p);
			const gg::Coordinate& p2 = closedPath->getCoordinateN(p + 1);
			sum += (p2.x - p1.x)*(p2.y + p1.y);
		}
		{
			const gg::Coordinate& p1 = closedPath->getCoordinateN(closedPath->getNumPoints() - 1);
			const gg::Coordinate& p2 = closedPath->getCoordinateN(0);
			sum += (p2.x - p1.x)*(p2.y + p1.y);
		}

		return std::make_tuple(sum < 0.0, std::move(pLineString));
	}

	std::string GetGeometryType(gg::Geometry* geometry)
	{
		switch (geometry->getGeometryTypeId())
		{
		case geos::geom::GEOS_POINT:              return "Point";
		case geos::geom::GEOS_LINESTRING:         return "LineString";
		case geos::geom::GEOS_LINEARRING:         return "LinearRing";
		case geos::geom::GEOS_POLYGON:            return "Polygon";
		case geos::geom::GEOS_MULTIPOINT:         return "MultiPoint";
		case geos::geom::GEOS_MULTILINESTRING:    return "MultiLineString";
		case geos::geom::GEOS_MULTIPOLYGON:       return "MultiPolygon";
		case geos::geom::GEOS_GEOMETRYCOLLECTION: return "GeometryCollection";
		}

		return "Unknown";
	}

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

		auto factory = gg::GeometryFactory::create();
		return factory->createPolygon(factory->createLinearRing(pts), {});
	}

	gg::LineString* ToLineString(const Vector<Eigen::Vector2d>& exterior)
	{
		gg::CoordinateArraySequence pts;

		for (int i = 0; i < exterior.size(); ++i)
		{
			pts.add(gg::Coordinate(exterior[i].x(), exterior[i].y()));
		}

		auto factory = gg::GeometryFactory::create();
		return factory->createLineString(pts);
	}

	void DebugPrint(const gg::Geometry* geometry)
	{
		CGL_DBG;
		switch (geometry->getGeometryTypeId())
		{
		case geos::geom::GEOS_POINT:
		{
			std::cout << "Point" << std::endl;
			break;
		}
		case geos::geom::GEOS_LINESTRING:
		{
			std::cout << "LineString" << std::endl;
			break;
		}
		case geos::geom::GEOS_LINEARRING:
		{
			std::cout << "LinearRing" << std::endl;
			break;
		}
		case geos::geom::GEOS_POLYGON:
		{
			std::cout << "Polygon" << std::endl;
			break;
		}
		case geos::geom::GEOS_MULTIPOINT:
		{
			std::cout << "MultiPoint" << std::endl;
			break;
		}
		case geos::geom::GEOS_MULTILINESTRING:
		{
			std::cout << "MultiLineString" << std::endl;
			break;
		}
		case geos::geom::GEOS_MULTIPOLYGON:
		{
			std::cout << "MultiPolygon" << std::endl;
			break;
		}
		case geos::geom::GEOS_GEOMETRYCOLLECTION:
		{
			std::cout << "GeometryCollection" << std::endl;
			break;
		}
		default:
		{
			std::cout << "Unknown" << std::endl;
		}
		}
		CGL_DBG;
	}

	Path Path::clone()const
	{
		Path resultPath;

		resultPath.cs = std::make_unique<gg::CoordinateArraySequence>();
		auto& csResult = resultPath.cs;
		auto& distancesResult = resultPath.distances;

		for (size_t i = 0; i < cs->size(); ++i)
		{
			csResult->add(cs->getAt(i));
		}
		distancesResult = distances;

		return std::move(resultPath);
	}

	BaseLineOffset Path::getOffset(double offset)const
	{
		BaseLineOffset result;

		auto it = std::upper_bound(distances.begin(), distances.end(), offset);
		if (it == distances.end())
		{
			const double innerDistance = offset - distances[distances.size() - 2];

			Eigen::Vector2d p0(cs->getAt(cs->size() - 2).x, cs->getAt(cs->size() - 2).y);
			Eigen::Vector2d p1(cs->getAt(cs->size() - 1).x, cs->getAt(cs->size() - 1).y);

			const Eigen::Vector2d v = (p1 - p0);
			const double currentLineLength = sqrt(v.dot(v));
			const double progress = innerDistance / currentLineLength;

			const Eigen::Vector2d targetPos = p0 + v * progress;
			result.x = targetPos.x();
			result.y = targetPos.y();

			const auto n = v.normalized();
			result.angle = rad2deg * atan2(n.y(), n.x());
			result.nx = n.y();
			result.ny = -n.x();
		}
		else
		{
			const int lineIndex = std::distance(distances.begin(), it) - 1;
			const double innerDistance = offset - distances[lineIndex];

			Eigen::Vector2d p0(cs->getAt(lineIndex).x, cs->getAt(lineIndex).y);
			Eigen::Vector2d p1(cs->getAt(lineIndex + 1).x, cs->getAt(lineIndex + 1).y);

			const Eigen::Vector2d v = (p1 - p0);
			const double currentLineLength = sqrt(v.dot(v));
			const double progress = innerDistance / currentLineLength;

			const Eigen::Vector2d targetPos = p0 + v * progress;
			result.x = targetPos.x();
			result.y = targetPos.y();

			const auto n = v.normalized();
			result.angle = rad2deg * atan2(n.y(), n.x());
			result.nx = n.y();
			result.ny = -n.x();
		}

		return result;
	}

	void BoundingRect::add(const Eigen::Vector2d& v)
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

	void BoundingRect::add(const Vector<Eigen::Vector2d>& vs)
	{
		for (const auto& v : vs)
		{
			add(v);
		}
	}

	TransformPacked::TransformPacked(const PackedRecord& record)
	{
		double px = 0, py = 0;
		double sx = 1, sy = 1;
		double angle = 0;

		for (const auto& member : record.values)
		{
			const PackedVal& value = member.second.value;
			const auto valOpt = AsOpt<PackedRecord>(value);

			if (valOpt)
			{
				const PackedRecord& childRecord = valOpt.get();
				if (member.first == "pos")
				{
					ReadDoublePacked(px, "x", childRecord);
					ReadDoublePacked(py, "y", childRecord);
				}
				else if (member.first == "scale")
				{
					ReadDoublePacked(sx, "x", childRecord);
					ReadDoublePacked(sy, "y", childRecord);
				}
			}
			else if (member.first == "angle")
			{
				ReadDoublePacked(angle, "angle", record);
			}
		}

		init(px, py, sx, sy, angle);
	}

	void TransformPacked::init(double px, double py, double sx, double sy, double angle)
	{
		const double pi = 3.1415926535;
		const double cosTheta = std::cos(pi*angle / 180.0);
		const double sinTheta = std::sin(pi*angle / 180.0);

		mat <<
			sx * cosTheta, -sy * sinTheta, px,
			sx*sinTheta, sy*cosTheta, py,
			0, 0, 1;
	}

	Eigen::Vector2d TransformPacked::product(const Eigen::Vector2d& v)const
	{
		Eigen::Vector3d xs;
		xs << v.x(), v.y(), 1;
		Eigen::Vector3d result = mat * xs;
		Eigen::Vector2d result2d;
		result2d << result.x(), result.y();
		return result2d;
	}

	void TransformPacked::printMat()const
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
}
