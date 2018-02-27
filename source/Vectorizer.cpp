#include <Eigen/Core>

#include <geos/geom.h>
#include <geos/opBuffer.h>
#include <geos/opDistance.h>

#include <Pita/Node.hpp>
#include <Pita/Context.hpp>
#include <Pita/Vectorizer.hpp>

namespace cgl
{
	struct Color
	{
		int r = 0, g = 0, b = 0;
	};

	class PitaGeometry
	{
		gg::Geometry* shape;
		Color color;
	};

	std::vector<gg::Geometry*> GeosFromList(const cgl::List& list, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform);

	std::vector<gg::Geometry*> GeosFromRecordImpl(const cgl::Record& record, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& parent = cgl::Transform());

	std::vector<gg::Geometry*> GeosFromListPacked(const cgl::PackedList& list, std::shared_ptr<cgl::Context> pEnv, const cgl::TransformPacked& transform);

	std::vector<gg::Geometry*> GeosFromRecordPackedImpl(const cgl::PackedRecord& record, std::shared_ptr<cgl::Context> pEnv, const cgl::TransformPacked& parent = cgl::TransformPacked());

	bool ReadDouble(double& output, const std::string& name, const Record& record, std::shared_ptr<Context> environment)
	{
		const auto& values = record.values;
		
		auto it = values.find(name);
		if (it == values.end())
		{
			return false;
		}
		auto opt = environment->expandOpt(it->second);
		if (!opt)
		{
			return false;
		}
		const Val& value = opt.value();
		if (!IsType<int>(value) && !IsType<double>(value))
		{
			return false;
		}
		output = IsType<int>(value) ? static_cast<double>(As<int>(value)) : As<double>(value);
		return true;
	}

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
					cs->add(gg::Coordinate(x, y));
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

	bool ReadDoublePacked(double& output, const std::string& name, const PackedRecord& record, std::shared_ptr<Context> environment)
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

	Transform::Transform(const Record& record, std::shared_ptr<Context> pEnv)
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

	void Transform::init(double px, double py, double sx, double sy, double angle)
	{
		const double pi = 3.1415926535;
		const double cosTheta = std::cos(pi*angle / 180.0);
		const double sinTheta = std::sin(pi*angle / 180.0);

		mat <<
			sx*cosTheta, -sy*sinTheta, px,
			sx*sinTheta, sy*cosTheta, py,
			0, 0, 1;
	}

	Eigen::Vector2d Transform::product(const Eigen::Vector2d& v)const
	{
		Eigen::Vector3d xs;
		xs << v.x(), v.y(), 1;
		Eigen::Vector3d result = mat * xs;
		Eigen::Vector2d result2d;
		result2d << result.x(), result.y();
		return result2d;
	}

	void Transform::printMat()const
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

	TransformPacked::TransformPacked(const PackedRecord& record, std::shared_ptr<Context> pEnv)
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
					ReadDoublePacked(px, "x", childRecord, pEnv);
					ReadDoublePacked(py, "y", childRecord, pEnv);
				}
				else if (member.first == "scale")
				{
					ReadDoublePacked(sx, "x", childRecord, pEnv);
					ReadDoublePacked(sy, "y", childRecord, pEnv);
				}
			}
			else if (member.first == "angle")
			{
				ReadDoublePacked(angle, "angle", record, pEnv);
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

	bool ReadPolygon(Vector<Eigen::Vector2d>& output, const List& vertices, std::shared_ptr<Context> pEnv, const Transform& transform)
	{
		output.clear();

		for (const Address vertex : vertices.data)
		{
			const Val value = pEnv->expand(vertex);

			if (IsType<Record>(value))
			{
				double x = 0, y = 0;
				const Record& pos = As<Record>(value);
				if (!ReadDouble(x, "x", pos, pEnv) || !ReadDouble(y, "y", pos, pEnv))
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

	bool ReadColor(Color& output, const Record& record, std::shared_ptr<Context> pEnv, const Transform& transform)
	{
		double r = 0, g = 0, b = 0;

		if (!(
			ReadDouble(r, "r", record, pEnv) &&
			ReadDouble(g, "g", record, pEnv) &&
			ReadDouble(b, "b", record, pEnv)
			))
		{
			return false;
		}

		output.r = r;
		output.g = g;
		output.b = b;

		return true;
	}

	bool ReadPolygonPacked(Vector<Eigen::Vector2d>& output, const PackedList& vertices, std::shared_ptr<Context> pEnv, const TransformPacked& transform)
	{
		output.clear();

		for (const auto& val: vertices.data)
		{
			const PackedVal& value = val.value;

			if (IsType<PackedRecord>(value))
			{
				double x = 0, y = 0;
				const PackedRecord& pos = As<PackedRecord>(value);
				if (!ReadDoublePacked(x, "x", pos, pEnv) || !ReadDoublePacked(y, "y", pos, pEnv))
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

	void GetBoundingBoxImpl(BoundingRect& output, const Record& record, std::shared_ptr<Context> pEnv, const Transform& parent)
	{
		const Transform current(record, pEnv);
		const Transform transform = parent * current;

		for (const auto& member : record.values)
		{
			const Val value = pEnv->expand(member.second);

			if ((member.first == "polygon" || member.first == "line") && IsType<List>(value))
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
					const Val& polygonVertices = pEnv->expand(polygonAddress);

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

	void GetBoundingBoxImpl(BoundingRect& output, const List& list, std::shared_ptr<Context> pEnv, const Transform& transform)
	{
		for (const Address member : list.data)
		{
			const Val value = pEnv->expand(member);

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

	boost::optional<BoundingRect> GetBoundingBox(const Val& value, std::shared_ptr<Context> pEnv)
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

	std::vector<gg::Geometry*> GeosFromRecordImpl(const cgl::Record& record, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& parent)
	{
		const cgl::Transform current(record, pEnv);

		const cgl::Transform transform = parent * current;

		std::vector<gg::Geometry*> currentPolygons;
		std::vector<gg::Geometry*> currentHoles;

		std::vector<gg::Geometry*> currentLines;

		for (const auto& member : record.values)
		{
			const cgl::Val value = pEnv->expand(member.second);

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
					const Val& polygonVertices = pEnv->expand(polygonAddress);

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
					const Val& hole = pEnv->expand(holeAddress);

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygon(polygon, As<List>(hole), pEnv, transform) && !polygon.empty())
					{
						currentHoles.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "line" && IsType<List>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygon(polygon, cgl::As<cgl::List>(value), pEnv, transform) && !polygon.empty())
				{
					currentLines.push_back(ToLineString(polygon));
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

		//gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();
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

			currentPolygons.insert(currentPolygons.end(), currentLines.begin(), currentLines.end());
			return currentPolygons;

			/*gg::MultiPolygon* accumlatedPolygons = factory->createMultiPolygon(currentPolygons);
			gg::MultiPolygon* accumlatedHoles = factory->createMultiPolygon(currentHoles);

			return{ accumlatedPolygons->difference(accumlatedHoles) };*/
		}
	}

	std::vector<gg::Geometry*> GeosFromList(const cgl::List& list, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform)
	{
		std::vector<gg::Geometry*> currentPolygons;
		for (const cgl::Address member : list.data)
		{
			const cgl::Val value = pEnv->expand(member);

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

	std::vector<gg::Geometry*> GeosFromRecord(const Val& value, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform)
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

#ifdef comment
	std::vector<PitaGeometry> GeosFromRecordImplDrawable(const cgl::Record& record, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& parent)
	{
		const cgl::Transform current(record, pEnv);

		const cgl::Transform transform = parent * current;

		std::vector<gg::Geometry*> currentPolygons;
		std::vector<gg::Geometry*> currentHoles;

		std::vector<gg::Geometry*> currentLines;

		Color color;

		for (const auto& member : record.values)
		{
			const cgl::Val value = pEnv->expand(member.second);

			if (member.first == "polygon" && cgl::IsType<cgl::List>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygon(polygon, cgl::As<cgl::List>(value), pEnv, transform) && !polygon.empty())
				{
					currentPolygons.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "hole" && cgl::IsType<cgl::List>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygon(polygon, cgl::As<cgl::List>(value), pEnv, transform) && !polygon.empty())
				{
					currentHoles.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "polygons" && IsType<List>(value))
			{
				const List& polygons = As<List>(value);
				for (const auto& polygonAddress : polygons.data)
				{
					const Val& polygonVertices = pEnv->expand(polygonAddress);

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
					const Val& hole = pEnv->expand(holeAddress);

					Vector<Eigen::Vector2d> polygon;
					if (ReadPolygon(polygon, As<List>(hole), pEnv, transform) && !polygon.empty())
					{
						currentHoles.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "line" && IsType<List>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygon(polygon, cgl::As<cgl::List>(value), pEnv, transform) && !polygon.empty())
				{
					currentLines.push_back(ToLineString(polygon));
				}
			}
			else if (member.first == "color" && IsType<Record>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				//cgl::ReadColor(color,);
				if (cgl::ReadPolygon(polygon, cgl::As<cgl::List>(value), pEnv, transform) && !polygon.empty())
				{
					currentLines.push_back(ToLineString(polygon));
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

		//gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();
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
				currentPolygons[s] = erodeGeometry;
			}

			currentPolygons.insert(currentPolygons.end(), currentLines.begin(), currentLines.end());
			return currentPolygons;

		}
	}

	std::vector<PitaGeometry> GeosFromListDrawable(const cgl::List& list, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform)
	{

	}
	std::vector<PitaGeometry> GeosFromRecordDrawable(const Val& value, std::shared_ptr<cgl::Context> pEnv, const cgl::Transform& transform)
	{

	}
#endif

	std::vector<gg::Geometry*> GeosFromListPacked(const cgl::PackedList& list, std::shared_ptr<cgl::Context> pEnv, const cgl::TransformPacked& transform)
	{
		std::vector<gg::Geometry*> currentPolygons;
		for (const auto& val : list.data)
		{
			const PackedVal& value = val.value;
			if (cgl::IsType<cgl::PackedRecord>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pEnv, transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromListPacked(cgl::As<cgl::PackedList>(value), pEnv, transform));
			}
		}
		return currentPolygons;
	}

	std::vector<gg::Geometry*> GeosFromRecordPackedImpl(const cgl::PackedRecord& record, std::shared_ptr<cgl::Context> pEnv, const cgl::TransformPacked& parent)
	{
		const cgl::TransformPacked current(record, pEnv);

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
				if (cgl::ReadPolygonPacked(polygon, cgl::As<cgl::PackedList>(value), pEnv, transform) && !polygon.empty())
				{
					currentPolygons.push_back(ToPolygon(polygon));
				}
			}
			else if (member.first == "hole" && cgl::IsType<cgl::PackedList>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygonPacked(polygon, cgl::As<cgl::PackedList>(value), pEnv, transform) && !polygon.empty())
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
					if (ReadPolygonPacked(polygon, As<PackedList>(polygonVertices), pEnv, transform) && !polygon.empty())
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
					if (ReadPolygonPacked(polygon, As<PackedList>(holes), pEnv, transform) && !polygon.empty())
					{
						currentHoles.push_back(ToPolygon(polygon));
					}
				}
			}
			else if (member.first == "line" && IsType<PackedList>(value))
			{
				cgl::Vector<Eigen::Vector2d> polygon;
				if (cgl::ReadPolygonPacked(polygon, cgl::As<cgl::PackedList>(value), pEnv, transform) && !polygon.empty())
				{
					currentLines.push_back(ToLineString(polygon));
				}
			}
			else if (cgl::IsType<cgl::PackedRecord>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pEnv, transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				GeosPolygonsConcat(currentPolygons, GeosFromListPacked(cgl::As<cgl::PackedList>(value), pEnv, transform));
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

	std::vector<gg::Geometry*> GeosFromRecordPacked(const PackedVal& value, std::shared_ptr<cgl::Context> pEnv, const cgl::TransformPacked& transform)
	{
		if (cgl::IsType<cgl::PackedRecord>(value))
		{
			return GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pEnv, transform);
		}
		if (cgl::IsType<cgl::PackedList>(value))
		{
			return GeosFromListPacked(cgl::As<cgl::PackedList>(value), pEnv, transform);
		}

		return{};
	}

	Record GetPolygon(const gg::Polygon* poly, std::shared_ptr<cgl::Context> pEnv)
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
			result.append("holes", pEnv->makeTemporaryValue(List(holeList)));
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

			for (int i = 1; i < static_cast<int>(outer->getNumPoints()); ++i)//始点と終点は同じ座標なので最後だけ飛ばす
			{
				const gg::Coordinate& p = outer->getCoordinateN(i);
				appendCoord(polygonList, p.x, p.y);
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

	List GetShapesFromGeos(const std::vector<gg::Geometry*>& polygons, std::shared_ptr<cgl::Context> pEnv)
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

	PackedList GetPackedShapesFromGeos(const std::vector<gg::Geometry*>& polygons)
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
				break;
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

	bool OutputSVG(std::ostream& os, const Val& value, std::shared_ptr<Context> pEnv)
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
			//std::vector<PitaGeometry> geometries = GeosFromRecord(value, pEnv);
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
