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

	std::vector<gg::Geometry*> GeosFromListPacked(const cgl::PackedList& list, const cgl::TransformPacked& transform);

	void BoundingRectListPacked(BoundingRect& output, const cgl::PackedList& list, const cgl::TransformPacked& transform);

	std::vector<PitaGeometry> GeosFromListDrawablePacked(const cgl::PackedList& list, const cgl::TransformPacked& transform);

	std::vector<PitaGeometry> GeosFromRecordDrawablePacked(const cgl::PackedVal& value, const cgl::TransformPacked& transform = cgl::TransformPacked());

	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record)
	{
		const auto& values = record.values;

		auto itX = values.find("x");
		auto itY = values.find("y");
		
		return std::tuple<double, double>(AsDouble(itX->second.value), AsDouble(itY->second.value));
	}

	Path ReadPathPacked(const PackedRecord& pathRecord)
	{
		Path resultPath;

		resultPath.cs = std::make_unique<gg::CoordinateArraySequence>();
		auto& cs = resultPath.cs;
		auto& distances = resultPath.distances;

		const TransformPacked transform(pathRecord);

		auto itLine = pathRecord.values.find("line");
		if (itLine == pathRecord.values.end())
			CGL_Error("不正な式です");

		const PackedVal lineVal = itLine->second.value;
		if (auto pointListOpt = AsOpt<PackedList>(lineVal))
		{
			for (const auto pointVal : pointListOpt.value().data)
			{
				if (auto posRecordOpt = AsOpt<PackedRecord>(pointVal.value))
				{
					const auto v = ReadVec2Packed(posRecordOpt.value());
					const double x = std::get<0>(v);
					const double y = std::get<1>(v);
					//cs->add(gg::Coordinate(x, y));
					Eigen::Vector2d xy;
					xy << x, y;
					const auto xy_ = transform.product(xy);

					cs->add(gg::Coordinate(xy_.x(), xy_.y()));
				}
				else CGL_Error("不正な式です");
			}

			distances.push_back(0);
			for (int i = 1; i < cs->size(); ++i)
			{
				const double newDistance = cs->getAt(i - 1).distance(cs->getAt(i));
				distances.push_back(distances.back() + newDistance);
			}
		}
		else CGL_Error("不正な式です");

		return std::move(resultPath);
	}

	PackedRecord WritePathPacked(const Path& path)
	{
		PackedRecord result;
		auto& cs = path.cs;

		const auto coord = [&](double x, double y)
		{
			PackedRecord record;
			record.add("x", x);
			record.add("y", y);
			return record;
		};
		const auto appendCoord = [&](PackedList& list, double x, double y)
		{
			const auto record = coord(x, y);
			list.add(record);
		};
		
		PackedList polygonList;
		for (size_t i = 0; i < cs->size(); ++i)
		{
			appendCoord(polygonList, cs->getX(i), cs->getY(i));
		}

		result.add("line", polygonList);
		return result;
	}

	bool ReadDoublePacked(double& output, const std::string& name, const PackedRecord& record)
	{
		const auto& values = record.values;

		auto it = values.find(name);
		if (it == values.end())
		{
			return false;
		}
		const PackedVal& value = it->second.value;
		if (!IsType<int>(value) && !IsType<double>(value))
		{
			return false;
		}
		output = IsType<int>(value) ? static_cast<double>(As<int>(value)) : As<double>(value);
		return true;
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
				const PackedRecord& childRecord = valOpt.value();
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
			sx*cosTheta, -sy*sinTheta, px,
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

	bool ReadPolygonPacked(Vector<Eigen::Vector2d>& output, const PackedList& vertices, const TransformPacked& transform)
	{
		output.clear();

		for (const auto& val: vertices.data)
		{
			const PackedVal& value = val.value;

			if (IsType<PackedRecord>(value))
			{
				double x = 0, y = 0;
				const PackedRecord& pos = As<PackedRecord>(value);
				if (!ReadDoublePacked(x, "x", pos) || !ReadDoublePacked(y, "y", pos))
				{
					return false;
				}
				Eigen::Vector2d v;
				v << x, y;
				output.push_back(transform.product(v));
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	bool ReadColorPacked(Color& output, const PackedRecord& record, const TransformPacked& transform)
	{
		double r = 0, g = 0, b = 0;

		if (!(
			ReadDoublePacked(r, "r", record) &&
			ReadDoublePacked(g, "g", record) &&
			ReadDoublePacked(b, "b", record)
			))
		{
			return false;
		}

		output.r = r;
		output.g = g;
		output.b = b;

		return true;
	}

	void GetBoundingBoxImplPacked(BoundingRect& output, const PackedRecord& record, const TransformPacked& parent)
	{
		const TransformPacked current(record);
		const TransformPacked transform = parent * current;

		for (const auto& member : record.values)
		{
			const auto& value = member.second.value;

			if ((member.first == "polygon" || member.first == "line") && IsType<PackedList>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygonPacked(polygon, As<PackedList>(value), transform) && !polygon.empty())
				{
					for (const auto& vertex : polygon)
					{
						output.add(vertex);
					}
				}
			}
			else if (member.first == "polygons" && IsType<PackedList>(value))
			{
				const PackedList& polygons = As<PackedList>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					//const Val& polygonVertices = pEnv->expand(polygonAddress);
					const auto& polygonVertices = polygonAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(polygonVertices), transform) && !polygon.empty())
					{
						for (const auto& vertex : polygon)
						{
							output.add(vertex);
						}
					}
				}
			}
			else if (IsType<PackedRecord>(value))
			{
				GetBoundingBoxImplPacked(output, As<PackedRecord>(value), transform);
			}
			else if (IsType<PackedList>(value))
			{
				GetBoundingBoxImplPacked(output, As<PackedList>(value), transform);
			}
		}
	}

	void GetBoundingBoxImplPacked(BoundingRect& output, const PackedList& list, const TransformPacked& transform)
	{
		for (const auto& member : list.data)
		{
			const PackedVal& value = member.value;

			if (IsType<PackedRecord>(value))
			{
				GetBoundingBoxImplPacked(output, As<PackedRecord>(value), transform);
			}
			else if (IsType<PackedList>(value))
			{
				GetBoundingBoxImplPacked(output, As<PackedList>(value), transform);
			}
		}
	}

	boost::optional<BoundingRect> GetBoundingBoxPacked(const PackedVal& value)
	{
		if (IsType<PackedRecord>(value))
		{
			const PackedRecord& record = As<PackedRecord>(value);
			BoundingRect rect;
			GetBoundingBoxImplPacked(rect, record);
			return rect;
		}

		return boost::none;
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

		//gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();
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

	void GeosPolygonsConcat(std::vector<gg::Geometry*>& head, const std::vector<gg::Geometry*>& tail)
	{
		head.insert(head.end(), tail.begin(), tail.end());
	}

	void GeosPolygonsConcatDrawable(std::vector<PitaGeometry>& head, const std::vector<PitaGeometry>& tail)
	{
		head.insert(head.end(), tail.begin(), tail.end());
	}

	std::vector<PitaGeometry> GeosFromRecordImplDrawablePacked(const PackedRecord& record, const TransformPacked& parent)
	{
		const TransformPacked current(record);
		const TransformPacked transform = parent * current;

		std::vector<PitaGeometry> wholePolygons;

		std::vector<gg::Geometry*> currentPolygons;
		std::vector<gg::Geometry*> currentHoles;

		gg::Geometry* currentLine = nullptr;

		Color currentColor;

		for (const auto& member : record.values)
		{
			const auto& value = member.second.value;

			if (member.first == "polygon" && IsType<PackedList>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygonPacked(polygon, As<PackedList>(value), transform) && !polygon.empty())
				{
					currentPolygons.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "hole" && IsType<PackedList>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygonPacked(polygon, As<PackedList>(value), transform) && !polygon.empty())
				{
					currentHoles.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "polygons" && IsType<PackedList>(value))
			{
				const PackedList& polygons = As<PackedList>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					const auto& polygonVertices = polygonAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(polygonVertices), transform) && !polygon.empty())
					{
						currentPolygons.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "holes" && IsType<PackedList>(value))
			{
				const PackedList& holes = As<PackedList>(value);
				for (const auto& holeAddress : holes.data)
				{
					const auto& hole = holeAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(hole), transform) && !polygon.empty())
					{
						currentHoles.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "line" && IsType<PackedList>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygonPacked(polygon, As<PackedList>(value), transform) && !polygon.empty())
				{
					currentLine = ToLineString(polygon);
				}
			}
			else if (member.first == "color" && IsType<PackedRecord>(value))
			{
				ReadColorPacked(currentColor, As<PackedRecord>(value), transform);
			}
			else if (IsType<PackedRecord>(value))
			{
				GeosPolygonsConcatDrawable(wholePolygons, GeosFromRecordImplDrawablePacked(As<PackedRecord>(value), transform));
			}
			else if (cgl::IsType<cgl::List>(value))
			{
				GeosPolygonsConcatDrawable(wholePolygons, GeosFromListDrawablePacked(As<PackedList>(value), transform));
			}
		}

		//gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();
		auto factory = gg::GeometryFactory::create();

		if (currentPolygons.empty() && currentLine == nullptr)
		{
			return wholePolygons;
		}
		else if (currentPolygons.empty())
		{
			wholePolygons.emplace_back(currentLine, currentColor);
			return wholePolygons;
		}
		else if (currentHoles.empty())
		{
			for (gg::Geometry* geometry : currentPolygons)
			{
				wholePolygons.emplace_back(geometry, currentColor);
			}

			if (currentLine)
			{
				wholePolygons.emplace_back(currentLine, currentColor);
			}

			return wholePolygons;
		}
		else
		{
			for (int s = 0; s < currentPolygons.size(); ++s)
			{
				gg::Geometry* erodeGeometry = currentPolygons[s];

				for (int d = 0; d < currentHoles.size(); ++d)
				{
					erodeGeometry = erodeGeometry->difference(currentHoles[d]);

					if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						currentPolygons.erase(currentPolygons.begin() + s);

						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);
						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							currentPolygons.insert(currentPolygons.begin() + s, polygons->getGeometryN(i)->clone());
						}

						erodeGeometry = currentPolygons[s];
					}
					else if (erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_POLYGON
						&& erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_GEOMETRYCOLLECTION)
					{
						std::cout << __FUNCTION__ << " Differenceの結果が予期せぬデータ形式" << __LINE__ << std::endl;
					}
				}

				currentPolygons[s] = erodeGeometry;
			}

			for (gg::Geometry* geometry : currentPolygons)
			{
				wholePolygons.emplace_back(geometry, currentColor);
			}

			if (currentLine)
			{
				wholePolygons.emplace_back(currentLine, currentColor);
			}

			return wholePolygons;
		}
	}

	std::vector<PitaGeometry> GeosFromListDrawablePacked(const cgl::PackedList& list, const cgl::TransformPacked& transform)
	{
		std::vector<PitaGeometry> currentPolygons;

		for (const auto& member : list.data)
		{
			const auto& value = member.value;

			if (IsType<PackedRecord>(value))
			{
				GeosPolygonsConcatDrawable(currentPolygons, GeosFromRecordImplDrawablePacked(As<PackedRecord>(value), transform));
			}
			else if (IsType<List>(value))
			{
				GeosPolygonsConcatDrawable(currentPolygons, GeosFromListDrawablePacked(As<PackedList>(value), transform));
			}
		}

		return currentPolygons;
	}

	std::vector<PitaGeometry> GeosFromRecordDrawablePacked(const cgl::PackedVal& value, const cgl::TransformPacked& transform)
	{
		if (IsType<PackedRecord>(value))
		{
			const PackedRecord& record = As<PackedRecord>(value);
			return GeosFromRecordImplDrawablePacked(record, transform);
		}
		if (IsType<PackedList>(value))
		{
			const PackedList& list = As<PackedList>(value);
			return GeosFromListDrawablePacked(list, transform);
		}

		return{};
	}

	std::vector<gg::Geometry*> GeosFromRecordPackedImpl(const cgl::PackedRecord& record, const cgl::TransformPacked& parent)
	{
		const cgl::TransformPacked current(record);

		const cgl::TransformPacked transform = parent * current;

		std::vector<gg::Geometry*> currentPolygons;
		std::vector<gg::Geometry*> currentHoles;

		std::vector<gg::Geometry*> currentLines;

		for (const auto& member : record.values)
		{
			const cgl::PackedVal& value = member.second.value;

			if (member.first == "polygon" && cgl::IsType<cgl::PackedList>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygonPacked(polygon, cgl::As<cgl::PackedList>(value), transform) && !polygon.empty())
				{
					currentPolygons.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "hole" && cgl::IsType<cgl::PackedList>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygonPacked(polygon, cgl::As<cgl::PackedList>(value), transform) && !polygon.empty())
				{
					currentHoles.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "polygons" && IsType<PackedList>(value))
			{
				const PackedList& polygons = As<PackedList>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					const PackedVal& polygonVertices = polygonAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(polygonVertices), transform) && !polygon.empty())
					{
						currentPolygons.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "holes" && IsType<PackedList>(value))
			{
				const PackedList& holes = As<PackedList>(value);
				for (const auto& holeAddress : holes.data)
				{
					const PackedVal& hole = holeAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(hole), transform) && !polygon.empty())
					{
						currentHoles.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "line" && IsType<PackedList>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygonPacked(polygon, cgl::As<cgl::PackedList>(value), transform) && !polygon.empty())
				{
					currentLines.push_back(ToLineString(polygon));
				}
			}
			else if (cgl::IsType<cgl::PackedRecord>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromListPacked(cgl::As<cgl::PackedList>(value), transform));
			}
		}

		auto factory = gg::GeometryFactory::create();

		if (currentPolygons.empty())
		{
			return currentLines;
		}
		else if (currentHoles.empty())
		{
			currentPolygons.insert(currentPolygons.end(), currentLines.begin(), currentLines.end());
			return currentPolygons;
		}
		else
		{
			for (int s = 0; s < currentPolygons.size(); ++s)
			{
				gg::Geometry* erodeGeometry = currentPolygons[s];

				for (int d = 0; d < currentHoles.size(); ++d)
				{
					erodeGeometry = erodeGeometry->difference(currentHoles[d]);
					
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

				currentPolygons[s] = erodeGeometry;
			}

			currentPolygons.insert(currentPolygons.end(), currentLines.begin(), currentLines.end());
			return currentPolygons;
		}
	}

	std::vector<gg::Geometry*> GeosFromListPacked(const cgl::PackedList& list, const cgl::TransformPacked& transform)
	{
		std::vector<gg::Geometry*> currentPolygons;
		for (const auto& val : list.data)
		{
			const PackedVal& value = val.value;
			if (cgl::IsType<cgl::PackedRecord>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromListPacked(cgl::As<cgl::PackedList>(value), transform));
			}
		}
		return currentPolygons;
	}

	std::vector<gg::Geometry*> GeosFromRecordPacked(const PackedVal& value, const cgl::TransformPacked& transform)
	{
		if (cgl::IsType<cgl::PackedRecord>(value))
		{
			return GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), transform);
		}
		if (cgl::IsType<cgl::PackedList>(value))
		{
			return GeosFromListPacked(cgl::As<cgl::PackedList>(value), transform);
		}

		return{};
	}

	void BoundingRectRecordPackedImpl(BoundingRect& output, const cgl::PackedRecord& record, const cgl::TransformPacked& parent)
	{
		const cgl::TransformPacked current(record);

		const cgl::TransformPacked transform = parent * current;

		for (const auto& member : record.values)
		{
			const cgl::PackedVal& value = member.second.value;

			if ((member.first == "polygon" || member.first == "line") && cgl::IsType<cgl::PackedList>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygonPacked(polygon, cgl::As<cgl::PackedList>(value), transform) && !polygon.empty())
				{
					output.add(polygon);
				}
			}
			else if (member.first == "polygons" && IsType<PackedList>(value))
			{
				const PackedList& polygons = As<PackedList>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					const PackedVal& polygonVertices = polygonAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(polygonVertices), transform) && !polygon.empty())
					{
						output.add(polygon);
					}
				}
			}
			else if (cgl::IsType<cgl::PackedRecord>(value))
			{
				BoundingRectRecordPackedImpl(output, cgl::As<cgl::PackedRecord>(value), transform);
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				BoundingRectListPacked(output, cgl::As<cgl::PackedList>(value), transform);
			}
		}
	}

	void BoundingRectListPacked(BoundingRect& output, const cgl::PackedList& list, const cgl::TransformPacked& transform)
	{
		for (const auto& val : list.data)
		{
			const PackedVal& value = val.value;
			if (cgl::IsType<cgl::PackedRecord>(value))
			{
				BoundingRectRecordPackedImpl(output, cgl::As<cgl::PackedRecord>(value), transform);
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				BoundingRectListPacked(output, cgl::As<cgl::PackedList>(value), transform);
			}
		}
	}

	BoundingRect BoundingRectRecordPacked(const PackedVal& value, const cgl::TransformPacked& transform)
	{
		BoundingRect result;

		if (cgl::IsType<cgl::PackedRecord>(value))
		{
			BoundingRectRecordPackedImpl(result, cgl::As<cgl::PackedRecord>(value), transform);
		}
		if (cgl::IsType<cgl::PackedList>(value))
		{
			BoundingRectListPacked(result, cgl::As<cgl::PackedList>(value), transform);
		}

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

			for (int i = 1; i < static_cast<int>(line->getNumPoints()); ++i)//始点と終点は同じ座標なので最後だけ飛ばす
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

	void OutputPolygonsStream(PolygonsStream& ps, const gg::Polygon* polygon)
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

	void OutputLineStringStream(PolygonsStream& ps, const gg::LineString* lineString)
	{
		std::stringstream ss;

		const double area = lineString->getLength();

		ss << "<polyline stroke=\"rgb(0, 255, 255)\" fill=\"none\" points=\"";
		for (int i = 0; i < lineString->getNumPoints(); ++i)
		{
			const gg::Point* p = lineString->getPointN(i);
			ss << p->getX() << "," << p->getY() << " ";
		}
		ss << "\"/>\n";

		ps.emplace(area, ss.str());
	}

	void OutputPolygonsStreamDrawable(PolygonsStream& ps, const gg::Polygon* polygon, const Color& color)
	{
		{
			std::stringstream ss;

			const gg::LineString* outer = polygon->getExteriorRing();
			const double area = polygon->getEnvelope()->getArea();

			ss << "<polygon fill=\"" << color.toString() << "\" points=\"";
			//ss << "<polygon fill=\"black\" points=\"";
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

	void OutputLineStringStreamDrawable(PolygonsStream& ps, const gg::LineString* lineString, const Color& color)
	{
		std::stringstream ss;

		const double area = lineString->getLength();

		ss << "<polyline stroke=\"" << color.toString() << "\" fill=\"none\" points=\"";
		//ss << "<polyline stroke=\"rgb(0, 255, 255)\" fill=\"none\" points=\"";
		for (int i = 0; i < lineString->getNumPoints(); ++i)
		{
			const gg::Point* p = lineString->getPointN(i);
			ss << p->getX() << "," << p->getY() << " ";
		}
		ss << "\"/>\n";

		ps.emplace(area, ss.str());
	}

	bool OutputSVG(std::ostream& os, const PackedVal& value)
	{
		auto boundingBoxOpt = GetBoundingBoxPacked(value);
		if (IsType<PackedRecord>(value) && boundingBoxOpt)
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
			std::vector<PitaGeometry> geometries = GeosFromRecordDrawablePacked(value);
			for (const auto& geometry : geometries)
			{
				if (geometry.shape->getGeometryTypeId() == gg::GeometryTypeId::GEOS_POLYGON)
				{
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry.shape);
					OutputPolygonsStreamDrawable(ps, polygon, geometry.color);
				}
				else if (geometry.shape->getGeometryTypeId() == gg::GeometryTypeId::GEOS_MULTIPOLYGON)
				{
					const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(geometry.shape);
					for (int i = 0; i < polygons->getNumGeometries(); ++i)
					{
						const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(polygons->getGeometryN(i));
						OutputPolygonsStreamDrawable(ps, polygon, geometry.color);
					}
				}
				else if (geometry.shape->getGeometryTypeId() == gg::GeometryTypeId::GEOS_LINESTRING)
				{
					const gg::LineString* lineString = dynamic_cast<const gg::LineString*>(geometry.shape);
					OutputLineStringStreamDrawable(ps, lineString, geometry.color);
				}
			}

			/*
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
				else if (geometry->getGeometryTypeId() == gg::GeometryTypeId::GEOS_LINESTRING)
				{
					const gg::LineString* lineString = dynamic_cast<const gg::LineString*>(geometry);
					OutputLineStringStream(ps, lineString);
				}
			}
			*/

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

	std::string getIndent(int depth)
	{
		std::string indent;
		const std::string indentStr = "  ";
		for (int i = 0; i < depth; ++i)
		{
			indent += indentStr;
		}
		return indent;
	}

	bool GeosFromList2Packed(std::ostream& os, const PackedList& list, const std::string& name, int depth, const cgl::TransformPacked& transform);

	bool GeosFromRecord2Packed(std::ostream& os, const PackedVal& value, const std::string& name, int depth, const cgl::TransformPacked& transform = cgl::TransformPacked());

	bool GeosFromRecordImpl2Packed(std::ostream& os, const PackedRecord& record, const std::string& name, int depth, const cgl::TransformPacked& parent)
	{
		const TransformPacked current(record);
		const TransformPacked transform = parent * current;

		std::vector<PitaGeometry> wholePolygons;

		std::vector<gg::Geometry*> currentPolygons;
		std::vector<gg::Geometry*> currentHoles;

		gg::Geometry* currentLine = nullptr;

		//現時点では実際に描画されるデータを持っているかどうかわからないため、一旦別のストリームに保存しておく
		std::stringstream currentStream;
		currentStream << getIndent(depth) << "<g id=\"" << name << "\" ";

		std::stringstream currentChildStream;

		//Color currentColor;

		bool hasShape = false;

		for (const auto& member : record.values)
		{
			//const cgl::Val value = pEnv->expand(member.second);
			const auto& value = member.second.value;

			if (member.first == "polygon" && IsType<PackedList>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygonPacked(polygon, As<PackedList>(value), transform) && !polygon.empty())
				{
					currentPolygons.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "hole" && IsType<PackedList>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygonPacked(polygon, As<PackedList>(value), transform) && !polygon.empty())
				{
					currentHoles.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "polygons" && IsType<PackedList>(value))
			{
				const PackedList& polygons = As<PackedList>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					const auto& polygonVertices = polygonAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(polygonVertices), transform) && !polygon.empty())
					{
						currentPolygons.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "holes" && IsType<PackedList>(value))
			{
				const PackedList& holes = As<PackedList>(value);
				for (const auto& holeAddress : holes.data)
				{
					const PackedVal& hole = holeAddress.value;

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygonPacked(polygon, As<PackedList>(hole), transform) && !polygon.empty())
					{
						currentHoles.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "line" && IsType<PackedList>(value))
			{
				Vector<Eigen::Vector2d> polygon;
				if (ReadPolygonPacked(polygon, As<PackedList>(value), transform) && !polygon.empty())
				{
					currentLine = ToLineString(polygon);
				}
			}
			else if (member.first == "fill" && IsType<PackedRecord>(value))
			{
				Color currentColor;
				ReadColorPacked(currentColor, As<PackedRecord>(value), transform);
				currentStream << "fill=\"" << currentColor.toString() << "\" ";
			}
			else if (member.first == "stroke" && IsType<PackedRecord>(value))
			{
				Color currentColor;
				ReadColorPacked(currentColor, As<PackedRecord>(value), transform);
				currentStream << "stroke=\"" << currentColor.toString() << "\" ";
			}
			else if (member.first == "stroke_width")
			{
				if (!IsNum(value))
				{
					CGL_Error("stroke_widthに数値以外の値が指定されました");
				}
				currentStream << "stroke-width=\"" << AsDouble(value) << "\" ";
			}
			else if (IsType<PackedRecord>(value))
			{
				hasShape = GeosFromRecordImpl2Packed(currentChildStream, As<PackedRecord>(value), member.first, depth + 1, transform) || hasShape;
			}
			else if (IsType<PackedList>(value))
			{
				hasShape = GeosFromList2Packed(currentChildStream, As<PackedList>(value), member.first, depth + 1, transform) || hasShape;
			}
		}

		currentStream << ">\n";

		const auto writePolygon = [&depth](std::ostream& os, const gg::Polygon* polygon)
		{
			//穴がない場合ー＞Polygonで描画
			if (polygon->getNumInteriorRing() == 0)
			{
				const gg::LineString* outer = polygon->getExteriorRing();

				os << getIndent(depth + 1) << "<polygon " << "points=\"";
				for (int i = 0; i < outer->getNumPoints(); ++i)
				{
					const gg::Coordinate& p = outer->getCoordinateN(i);
					os << p.x << "," << p.y << " ";
				}
				os << "\"/>\n";
			}
			//穴がある場合ー＞Pathで描画
			else
			{
				os << getIndent(depth + 1) << "<path " << "d=\"";

				{
					const gg::LineString* outer = polygon->getExteriorRing();

					if (outer->getNumPoints() != 0)
					{
						const gg::Coordinate& p = outer->getCoordinateN(0);
						os << "M" << p.x << "," << p.y << " ";
					}

					for (int i = 1; i < outer->getNumPoints(); ++i)
					{
						const gg::Coordinate& p = outer->getCoordinateN(i);
						os << "L" << p.x << "," << p.y << " ";
					}
					os << "z ";
				}

				for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
				{
					const gg::LineString* hole = polygon->getInteriorRingN(i);

					if (hole->getNumPoints() != 0)
					{
						const gg::Coordinate& p = hole->getCoordinateN(0);
						os << "M" << p.x << "," << p.y << " ";
					}

					for (int n = 1; n < hole->getNumPoints(); ++n)
					{
						gg::Point* p = hole->getPointN(n);
						os << "L" << p->getX() << "," << p->getY() << " ";
					}
					os << "z ";
				}

				os << "\"/>\n";
			}
		};

		const auto writeLine = [&depth](std::ostream& os, const gg::LineString* lineString)
		{
			os << getIndent(depth + 1) << "<polyline " << "fill=\"none\" points=\"";
			//os << getIndent(depth + 1) << "<polyline " << "points=\"";
			for (int i = 0; i < lineString->getNumPoints(); ++i)
			{
				const gg::Point* p = lineString->getPointN(i);
				os << p->getX() << "," << p->getY() << " ";
			}
			os << "\"/>\n";
		};

		const auto writePolygons = [&wholePolygons, &writePolygon, &writeLine](std::ostream& os)->bool
		{
			bool hasShape = false;
			for (const auto& geometry : wholePolygons)
			{
				if (geometry.shape->getGeometryTypeId() == gg::GeometryTypeId::GEOS_POLYGON)
				{
					hasShape = true;
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry.shape);
					writePolygon(os, polygon);
				}
				else if (geometry.shape->getGeometryTypeId() == gg::GeometryTypeId::GEOS_MULTIPOLYGON)
				{
					const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(geometry.shape);
					for (int i = 0; i < polygons->getNumGeometries(); ++i)
					{
						hasShape = true;
						const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(polygons->getGeometryN(i));
						writePolygon(os, polygon);
					}
				}
				else if (geometry.shape->getGeometryTypeId() == gg::GeometryTypeId::GEOS_LINESTRING)
				{
					hasShape = true;
					const gg::LineString* lineString = dynamic_cast<const gg::LineString*>(geometry.shape);
					writeLine(os, lineString);
				}
			}
			return hasShape;
		};

		const auto writeWholeData = [&]()
		{
			hasShape = writePolygons(currentStream) || hasShape;
			currentStream << currentChildStream.str();
			currentStream << getIndent(depth) << "</g>\n";

			if (hasShape)
			{
				os << currentStream.str();
			}
		};

		auto factory = gg::GeometryFactory::create();

		if (currentPolygons.empty() && currentLine == nullptr)
		{
			writeWholeData();

			return hasShape;
		}
		else if (currentPolygons.empty())
		{
			//パスの色はどうするか　別で指定する必要がある？
			//図形の境界線も考慮すると、塗りつぶしの色と線の色は別の名前で指定できるようにすべき
			wholePolygons.emplace_back(currentLine, Color());
			writeWholeData();

			return true;
		}
		else if (currentHoles.empty())
		{
			for (gg::Geometry* geometry : currentPolygons)
			{
				wholePolygons.emplace_back(geometry, Color());
			}

			if (currentLine)
			{
				wholePolygons.emplace_back(currentLine, Color());
			}

			writeWholeData();

			return true;
		}
		else
		{
			for (int s = 0; s < currentPolygons.size(); ++s)
			{
				gg::Geometry* erodeGeometry = currentPolygons[s];

				for (int d = 0; d < currentHoles.size(); ++d)
				{
					gg::Geometry* testGeometry;
					try
					{
						testGeometry = erodeGeometry->difference(currentHoles[d]);
						erodeGeometry = testGeometry;
					}
					catch (const std::exception& e)
					{
						//std::cout << "error: " << e.what() << "\n";
						continue;
					}

					if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						currentPolygons.erase(currentPolygons.begin() + s);

						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);
						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							currentPolygons.insert(currentPolygons.begin() + s, polygons->getGeometryN(i)->clone());
						}

						erodeGeometry = currentPolygons[s];
					}
					else if (erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_POLYGON
						&& erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_GEOMETRYCOLLECTION)
					{
						std::cout << __FUNCTION__ << " Differenceの結果が予期せぬデータ形式" << __LINE__ << std::endl;
					}
				}

				currentPolygons[s] = erodeGeometry;
			}

			for (gg::Geometry* geometry : currentPolygons)
			{
				wholePolygons.emplace_back(geometry, Color());
			}

			if (currentLine)
			{
				wholePolygons.emplace_back(currentLine, Color());
			}

			writeWholeData();

			return true;
		}
	}

	bool GeosFromList2Packed(std::ostream& os, const PackedList& list, const std::string& name, int depth, const TransformPacked& transform)
	{
		std::vector<PitaGeometry> currentPolygons;

		bool hasShape = false;
		int i = 0;
		for (const auto& member : list.data)
		{
			const auto& value = member.value;

			std::stringstream currentName;
			currentName << name << "[" << i << "]";

			if (IsType<PackedRecord>(value))
			{
				hasShape = GeosFromRecordImpl2Packed(os, As<PackedRecord>(value), currentName.str(), depth + 1, transform) || hasShape;
			}
			else if (IsType<PackedList>(value))
			{
				hasShape = GeosFromList2Packed(os, As<PackedList>(value), currentName.str(), depth + 1, transform) || hasShape;
			}

			++i;
		}

		return hasShape;
	}

	bool GeosFromRecord2Packed(std::ostream& os, const PackedVal& value, const std::string& name, int depth, const TransformPacked& transform)
	{
		if (IsType<PackedRecord>(value))
		{
			const PackedRecord& record = As<PackedRecord>(value);
			return GeosFromRecordImpl2Packed(os, record, name, depth, transform);
		}
		if (IsType<PackedList>(value))
		{
			const PackedList& list = As<PackedList>(value);
			return GeosFromList2Packed(os, list, name, depth, transform);
		}

		return{};
	}

	bool OutputSVG2(std::ostream& os, const PackedVal& value, const std::string& name)
	{
		auto boundingBoxOpt = GetBoundingBoxPacked(value);
		if (IsType<PackedRecord>(value) && boundingBoxOpt)
		{
			const BoundingRect& rect = boundingBoxOpt.value();
			//const auto pos = rect.pos();
			const auto widthXY = rect.width();
			const auto center = rect.center();

			const double margin = 5.0;
			//const double width = std::max(widthXY.x(), widthXY.y());
			//const double halfWidth = width * 0.5;
			const double width = widthXY.x() + margin;
			const double height = widthXY.y() + margin;

			const Eigen::Vector2d pos = center - Eigen::Vector2d(width*0.5, height*0.5);

			/*os << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.2" baseProfile="tiny" width=")" << width << R"(" height=")" << height << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << height
				<< R"(" viewport-fill="black" viewport-fill-opacity="0.1)"  << R"(">)" << "\n";*/
			os << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << width << R"(" height=")" << height << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << height << R"(">)" << "\n";
			GeosFromRecord2Packed(os, value, name, 0);
			os << "</svg>" << "\n";

			return true;
		}

		return false;
	}
	
}
