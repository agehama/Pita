#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <Eigen/Core>

#include <geos/geom.h>
#include <geos/opBuffer.h>
#include <geos/opDistance.h>

#include <Pita/Node.hpp>
#include <Pita/Context.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/Printer.hpp>
#include <Pita/Evaluator.hpp>

namespace cgl
{
	class BoundingRectMaker : public gg::CoordinateFilter
	{
	public:
		BoundingRect rect;

		void filter_ro(const Coordinate* coord) override
		{
			rect.add(EigenVec2(coord->x, coord->y));
		}
	};

	BoundingRect BoundingRectRecordPacked(const PackedVal& value, std::shared_ptr<Context> pContext)
	{
		auto geometries = GeosFromRecordPacked(value, pContext);

		BoundingRectMaker maker;
		for (size_t i = 0; i < geometries.size(); ++i)
		{
			geometries.refer(i)->apply_ro(&maker);
		}

		return maker.rect;
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

	struct GradientStop
	{
		double x;
		double y;
		Color color;
		GradientStop() = default;
		GradientStop(double x, double y, const Color& color)
			:x(x), y(y), color(color)
		{}
	};

	bool WriteDefinitionsList(std::ostream& os, const PackedList& list, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform);

	bool WriteDefinitionsRecord(std::ostream& os, const PackedVal& value, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform = cgl::TransformPacked());

	bool WriteDefinitionsRecordImpl(std::ostream& os, const PackedRecord& record, std::shared_ptr<Context> pContext, const cgl::TransformPacked& parent)
	{
		const cgl::TransformPacked current(record);
		const cgl::TransformPacked transform = parent * current;

		bool hasDefinitions = false;

		enum class GradientType { Linear, Radial };
		GradientType gradientType;
		std::string gradientName;
		std::vector<GradientStop> gradientStops;

		for (const auto& member : record.values)
		{
			const cgl::PackedVal& value = member.second.value;

			if (member.first == "gradient")
			{
				if (auto opt = AsOpt<CharString>(member.second.value))
				{
					const std::string gradientTypeStr = AsUtf8(opt.get().toString());
					if (gradientTypeStr == "linear")
					{
						gradientType = GradientType::Linear;
					}
					else if (gradientTypeStr == "radial")
					{
						gradientType = GradientType::Radial;
					}
				}
			}
			else if (member.first == "gradient_id")
			{
				if (auto opt = AsOpt<CharString>(member.second.value))
				{
					gradientName = AsUtf8(opt.get().toString());
				}
			}
			else if (member.first == "stop")
			{
				if (auto stopListOpt = AsOpt<PackedList>(member.second.value))
				{
					for (const auto& stop : stopListOpt.get().data)
					{
						if (auto recordOpt = AsOpt<PackedRecord>(stop.value))
						{
							const auto& record = recordOpt.get().values;

							const auto posIt = record.find("pos");
							const auto colorIt = record.find("color");
							if (posIt != record.end() && IsType<PackedRecord>(posIt->second.value) && 
								colorIt != record.end() && (IsType<PackedRecord>(colorIt->second.value) || IsType<CharString>(colorIt->second.value)))
							{
								const auto resultPos = ReadVec2Packed(As<PackedRecord>(posIt->second.value), transform);
								Color resultColor;

								if (
									(IsType<PackedRecord>(colorIt->second.value) && ReadColorPacked(resultColor, As<PackedRecord>(colorIt->second.value))) ||
									(IsType<CharString>(colorIt->second.value) && ReadColorPacked(resultColor, As<CharString>(colorIt->second.value)))
									)
								{
									gradientStops.emplace_back(
										std::get<0>(resultPos),
										std::get<1>(resultPos),
										resultColor);
								}
							}
						}
					}
				}
				else
				{
					CGL_Error("stopに指定されたデータの形式が不正です。");
				}
			}
			else if (IsType<PackedRecord>(value))
			{
				hasDefinitions = WriteDefinitionsRecordImpl(os, As<PackedRecord>(value), pContext, transform) || hasDefinitions;
			}
			else if (IsType<PackedList>(value))
			{
				hasDefinitions = WriteDefinitionsList(os, As<PackedList>(value), pContext, transform) || hasDefinitions;
			}
		}

		if (!gradientName.empty() && 2 <= gradientStops.size())
		{
			hasDefinitions = true;
			const Eigen::Vector2d p0(gradientStops.front().x, gradientStops.front().y);
			const Eigen::Vector2d p1(gradientStops.back().x, gradientStops.back().y);
			const Eigen::Vector2d v = (p1 - p0).normalized();
			const double length = (p1 - p0).dot(v);
			
			os << getIndent(2) << "<linearGradient gradientUnits=\"userSpaceOnUse\" id=\"" << gradientName << "\" ";
			os << "x1=\"" << p0.x() << "\" y1=\"" << p0.y() << "\" x2=\"" << p1.x() << "\" y2=\"" << p1.y() << "\">\n";

			for (const auto& stop : gradientStops)
			{
				const Eigen::Vector2d p(stop.x, stop.y);
				const double t = (p - p0).dot(v) / length;
				os << getIndent(3) << "<stop stop-color=\"" << stop.color.toString() << "\" offset=\"" << t << "\"/>\n";
			}

			os << getIndent(2) << "</linearGradient>\n";
		}

		return hasDefinitions;
	}

	bool WriteDefinitionsList(std::ostream& os, const PackedList& list, std::shared_ptr<Context> pContext, const TransformPacked& transform)
	{
		bool hasDefinitions = false;
		for (const auto& member : list.data)
		{
			const auto& value = member.value;

			if (IsType<PackedRecord>(value))
			{
				hasDefinitions = WriteDefinitionsRecordImpl(os, As<PackedRecord>(value), pContext, transform) || hasDefinitions;
			}
			else if (IsType<PackedList>(value))
			{
				hasDefinitions = WriteDefinitionsList(os, As<PackedList>(value), pContext, transform) || hasDefinitions;
			}
		}

		return hasDefinitions;
	}

	bool WriteDefinitionsRecord(std::ostream& os, const PackedVal& value,  std::shared_ptr<Context> pContext, const TransformPacked& transform)
	{
		if (IsType<PackedRecord>(value))
		{
			const PackedRecord& record = As<PackedRecord>(value);
			return WriteDefinitionsRecordImpl(os, record, pContext, transform);
		}
		if (IsType<PackedList>(value))
		{
			const PackedList& list = As<PackedList>(value);
			return WriteDefinitionsList(os, list, pContext, transform);
		}

		return false;
	}

	bool GeosFromList2Packed(std::ostream& os, const PackedList& list, const std::string& name, int depth, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform);

	bool GeosFromRecord2Packed(std::ostream& os, const PackedVal& value, const std::string& name, int depth, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform = cgl::TransformPacked());

	bool GeosFromRecordImpl2Packed(std::ostream& os, const PackedRecord& record, const std::string& name, int depth, std::shared_ptr<Context> pContext, const cgl::TransformPacked& parent)
	{
		const TransformPacked current(record);
		const TransformPacked transform = parent * current;

		std::vector<PitaGeometry> wholePolygons;

		Geometries resultPolygons;
		Geometries currentLines;

		os << getIndent(depth) << "<g id=\"" << name << "\" ";

		bool hasShape = false;

		for (const auto& member : record.values)
		{
			const auto& value = member.second.value;

			if (member.first == "pos" || member.first == "scale")
			{
				continue;
			}
			else if (member.first == "polygon")
			{
				std::vector<OutputPolygon> outputPolygons;

				if (IsType<PackedList>(value))
				{
					if (!ReadPackedPolyData(cgl::As<cgl::PackedList>(value), outputPolygons, transform))
					{
						CGL_Error("polygonに指定されたデータの形式が不正です。");
					}
				}
				else if (IsType<FuncVal>(value))
				{
					Eval evaluator(pContext);
					const cgl::PackedVal evaluated = Packed(pContext->expand(evaluator.callFunction(LocationInfo(), cgl::As<cgl::FuncVal>(value), {}), LocationInfo()), *pContext);

					if (!cgl::IsType<cgl::PackedList>(evaluated))
					{
						CGL_Error("polygon()の評価結果の型が不正です。");
					}

					if (!ReadPackedPolyData(cgl::As<cgl::PackedList>(evaluated), outputPolygons, transform))
					{
						CGL_Error("polygonに指定されたデータの形式が不正です。");
					}
				}

				while(!outputPolygons.empty())
				{
					auto& currentPolygons = outputPolygons[0].polygon;
					const auto& currentHoles = outputPolygons[0].hole;

					if (!currentPolygons.empty())
					{
						if (!currentHoles.empty())
						{
							for (int s = 0; s < currentPolygons.size();)
							{
								GeometryPtr pErodeGeometry(currentPolygons.takeOut(s));

								for (int d = 0; d < currentHoles.size(); ++d)
								{
									GeometryPtr pTemporaryGeometry(ToUnique<GeometryDeleter>(pErodeGeometry->difference(currentHoles.refer(d))));

									if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
									{
										//->次のpErodeGeometryには現在のポリゴンがそのまま入っている
										pErodeGeometry = std::move(pTemporaryGeometry);
										continue;
									}
									else if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
									{
										const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(pTemporaryGeometry.get());
										for (int i = 0; i < polygons->getNumGeometries(); ++i)
										{
											currentPolygons.insert(s, ToUnique<GeometryDeleter>(polygons->getGeometryN(i)->clone()));
										}
										pErodeGeometry = currentPolygons.takeOut(s);

										//currentPolygonsに挿入したのはcloneなのでdifferenceの結果はここで削除する

										//->次のpErodeGeometryには分割された最初のポリゴンが入っている
									}
									else if (pErodeGeometry->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION)
									{
										//結果が空であれば、ポリゴンを削除する
										pErodeGeometry.reset();
										break;
										//->次のpErodeGeometryには何も入っていないのでループを抜ける
									}
									else
									{
										CGL_Error("Differenceの評価結果の型が不正です。");
									}
								}

								if (pErodeGeometry)
								{
									currentPolygons.insert(s, std::move(pErodeGeometry));

									//最初のcurrentPolygons.takeOutで要素が減っているので、ここで復活したときのみsを増やす
									++s;
								}
							}
						}

						resultPolygons.append(std::move(currentPolygons));
					}

					outputPolygons.erase(outputPolygons.begin());
				}
			}
			else if (member.first == "line")
			{
				if (IsType<PackedList>(value))
				{
					if (!ReadPackedLineData(As<PackedList>(value), currentLines, transform))
					{
						CGL_Error("lineに指定されたデータの形式が不正です。");
					}
				}
				else if (IsType<FuncVal>(value))
				{
					Eval evaluator(pContext);
					const cgl::PackedVal evaluated = Packed(pContext->expand(evaluator.callFunction(LocationInfo(), cgl::As<cgl::FuncVal>(value), {}), LocationInfo()), *pContext);

					if (!cgl::IsType<cgl::PackedList>(evaluated))
					{
						CGL_Error("line()の評価結果の型が不正です。");
					}

					if (!ReadPackedLineData(As<PackedList>(evaluated), currentLines, transform))
					{
						CGL_Error("lineに指定されたデータの形式が不正です。");
					}
				}
			}
			else if (member.first == "fill")
			{
				if (IsType<PackedRecord>(value))
				{
					const PackedRecord& record = As<PackedRecord>(value);

					Color currentColor;
					if (ReadColorPacked(currentColor, record))
					{
						os << "fill=\"" << currentColor.toString() << "\" ";
					}
					else
					{
						auto it = record.values.find("gradient_id");
						if (it != record.values.end() && IsType<CharString>(it->second.value))
						{
							const std::string id = AsUtf8(As<CharString>(it->second.value).toString());
							os << "fill=\"url(#" << id << ")\" ";
						}
					}
				}
				else if (auto strOpt = AsOpt<CharString>(value))
				{
					const std::string str = AsUtf8(strOpt.get().toString());
					os << "fill=\"" << str << "\" ";
				}
				else
				{
					os << "fill=\"none\" ";
				}
			}
			else if (member.first == "stroke")
			{
				if (IsType<PackedRecord>(value))
				{
					Color currentColor;
					ReadColorPacked(currentColor, As<PackedRecord>(value));
					os << "stroke=\"" << currentColor.toString() << "\" ";
				}
				else
				{
					os << "stroke=\"none\" ";
				}
			}
			else if (member.first == "stroke_width")
			{
				if (!IsNum(value))
				{
					CGL_Error("stroke_widthに数値以外の値が指定されました");
				}
				os << "stroke-width=\"" << AsDouble(value) << "\" ";
			}
		}

		os << ">\n";

		for (const auto& member : record.values)
		{
			const auto& value = member.second.value;

			if (member.first == "pos"
				|| member.first == "scale"
				|| member.first == "polygon"
				|| member.first == "line"
				|| member.first == "fill"
				|| member.first == "stroke"
				|| member.first == "stroke_width"
				)
			{
				continue;
			}
			else if (IsType<PackedRecord>(value))
			{
				hasShape = GeosFromRecordImpl2Packed(os, As<PackedRecord>(value), member.first, depth + 1, pContext, transform) || hasShape;
			}
			else if (IsType<PackedList>(value))
			{
				hasShape = GeosFromList2Packed(os, As<PackedList>(value), member.first, depth + 1, pContext, transform) || hasShape;
			}
		}

		const auto writePolygon = [&depth](std::ostream& os, const gg::Polygon* polygon)
		{
			//穴がない場合ー＞Polygonで描画
			if (polygon->getNumInteriorRing() == 0)
			{
				const gg::LineString* outer = polygon->getExteriorRing();

				os << getIndent(depth + 1) << "<polygon " << "points=\"";
				if (IsClockWise(outer))
				{
					for (int i = 0; i < outer->getNumPoints(); ++i)
					{
						const auto& p = outer->getCoordinateN(i);
						os << p.x << "," << p.y << " ";
					}
				}
				else
				{
					for (int i = outer->getNumPoints() - 1; 0 <= i; --i)
					{
						const auto& p = outer->getCoordinateN(i);
						os << p.x << "," << p.y << " ";
					}
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
						if (IsClockWise(outer))
						{
							const auto&  p = outer->getCoordinateN(0);
							os << "M" << p.x << "," << p.y << " ";

							for (int i = 1; i < outer->getNumPoints(); ++i)
							{
								const auto& p = outer->getCoordinateN(i);
								os << "L" << p.x << "," << p.y << " ";
							}
						}
						else
						{
							const auto&  p = outer->getCoordinateN(outer->getNumPoints() - 1);
							os << "M" << p.x << "," << p.y << " ";

							for (int i = outer->getNumPoints() - 2; 0 < i; --i)
							{
								const auto& p = outer->getCoordinateN(i);
								os << "L" << p.x << "," << p.y << " ";
							}
						}

						os << "z ";
					}
				}

				for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
				{
					const gg::LineString* hole = polygon->getInteriorRingN(i);

					if (hole->getNumPoints() != 0)
					{
						if (IsClockWise(hole))
						{
							const auto& p = hole->getCoordinateN(hole->getNumPoints() - 1);
							os << "M" << p.x << "," << p.y << " ";

							for (int n = hole->getNumPoints() - 2; 0 < n; --n)
							{
								const auto& p = hole->getCoordinateN(n);
								os << "L" << p.x << "," << p.y << " ";
							}
						}
						else
						{
							const auto& p = hole->getCoordinateN(0);
							os << "M" << p.x << "," << p.y << " ";

							for (int n = 1; n < hole->getNumPoints(); ++n)
							{
								const auto& p = hole->getCoordinateN(n);
								os << "L" << p.x << "," << p.y << " ";
							}
						}

						os << "z ";
					}
				}

				os << "\"/>\n";
			}
		};

		const auto writeLine = [&depth](std::ostream& os, const gg::LineString* lineString)
		{
			os << getIndent(depth + 1) << "<polyline " << "fill=\"none\" points=\"";
			for (int i = 0; i < lineString->getNumPoints(); ++i)
			{
				const auto& p = lineString->getCoordinateN(i);
				os << p.x << "," << p.y << " ";
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
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry.shape.get());
					writePolygon(os, polygon);
				}
				else if (geometry.shape->getGeometryTypeId() == gg::GeometryTypeId::GEOS_MULTIPOLYGON)
				{
					const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(geometry.shape.get());
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
					const gg::LineString* lineString = dynamic_cast<const gg::LineString*>(geometry.shape.get());
					writeLine(os, lineString);
				}
			}
			return hasShape;
		};

		const auto writeWholeData = [&]()
		{
			hasShape = writePolygons(os) || hasShape;
			os << getIndent(depth) << "</g>\n";

			if (os.bad())
				std::cout << "I/O error while reading" << std::endl;
			else if (os.eof())
				std::cout << "End of file reached successfully" << std::endl;
			else if (os.fail())
				std::cout << "Non-integer data encountered" << std::endl;
		};

		auto factory = gg::GeometryFactory::create();

		if (resultPolygons.empty() && currentLines.empty())
		{
			writeWholeData();
			return hasShape;
		}
		else if (resultPolygons.empty())
		{
			//パスの色はどうするか　別で指定する必要がある？
			//図形の境界線も考慮すると、塗りつぶしの色と線の色は別の名前で指定できるようにすべき
			/*for (gg::Geometry* line : currentLines)
			{
				wholePolygons.emplace_back(line, Color());
			}*/
			while (!currentLines.empty())
			{
				wholePolygons.emplace_back(currentLines.takeOut(0), Color());
			}

			writeWholeData();

			return true;
		}
		else
		{
			while (!resultPolygons.empty())
			{
				wholePolygons.emplace_back(resultPolygons.takeOut(0), Color());
			}

			while (!currentLines.empty())
			{
				wholePolygons.emplace_back(currentLines.takeOut(0), Color());
			}
			/*for (gg::Geometry* geometry : resultPolygons)
			{
				wholePolygons.emplace_back(geometry, Color());
			}

			for (gg::Geometry* line : currentLines)
			{
				wholePolygons.emplace_back(line, Color());
			}*/

			writeWholeData();

			return true;
		}
	}

	bool GeosFromList2Packed(std::ostream& os, const PackedList& list, const std::string& name, int depth, std::shared_ptr<Context> pContext, const TransformPacked& transform)
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
				hasShape = GeosFromRecordImpl2Packed(os, As<PackedRecord>(value), currentName.str(), depth + 1, pContext, transform) || hasShape;
			}
			else if (IsType<PackedList>(value))
			{
				hasShape = GeosFromList2Packed(os, As<PackedList>(value), currentName.str(), depth + 1, pContext, transform) || hasShape;
			}

			++i;
		}

		return hasShape;
	}

	bool GeosFromRecord2Packed(std::ostream& os, const PackedVal& value, const std::string& name, int depth, std::shared_ptr<Context> pContext, const TransformPacked& transform)
	{
		if (IsType<PackedRecord>(value))
		{
			const PackedRecord& record = As<PackedRecord>(value);
			return GeosFromRecordImpl2Packed(os, record, name, depth, pContext, transform);
		}
		if (IsType<PackedList>(value))
		{
			const PackedList& list = As<PackedList>(value);
			return GeosFromList2Packed(os, list, name, depth, pContext, transform);
		}

		return{};
	}

	bool OutputSVG2(std::ostream& os, const PackedVal& value, const std::string& name, std::shared_ptr<Context> pContext)
	{
		boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(value)
			? boost::none
			: boost::optional<PackedRecord>(MakeRecord(
				"inner", value,
				"pos", MakeRecord("x", 0, "y", 0),
				"scale", MakeRecord("x", 1.0, "y", 1.0),
				"angle", 0
			));
		
		//auto boundingBoxOpt = IsType<PackedRecord>(value) ? boost::optional<BoundingRect>(BoundingRectRecordPacked(value, pContext)) : boost::none;
		auto boundingBox = wrapped ? BoundingRectRecordPacked(wrapped.get(), pContext) : BoundingRectRecordPacked(value, pContext);
		//if (IsType<PackedRecord>(value) && boundingBoxOpt)
		if(!boundingBox.isEmpty())
		{
			os.exceptions(std::ostream::failbit | std::ostream::badbit);

			//const BoundingRect& rect = boundingBoxOpt.get();
			const BoundingRect& rect = boundingBox;
			//const auto pos = rect.pos();
			const auto widthXY = rect.width();
			const auto center = rect.center();

			//const double margin = 5.0;
			const double margin = 1.e-5;
			//const double width = std::max(widthXY.x(), widthXY.y());
			//const double halfWidth = width * 0.5;
			const double width = widthXY.x() + margin;
			const double height = widthXY.y() + margin;

			const Eigen::Vector2d pos = center - EigenVec2(width*0.5, height*0.5);

			/*os << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.2" baseProfile="tiny" width=")" << width << R"(" height=")" << height << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << height
				<< R"(" viewport-fill="black" viewport-fill-opacity="0.1)"  << R"(">)" << "\n";*/
			os << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << width << R"(" height=")" << height << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << height << R"(">)" << "\n";
			{
				std::stringstream ss;
				if (WriteDefinitionsRecord(ss, wrapped ? wrapped.get() : value, pContext))
				{
					os << getIndent(1) << "<defs>" << "\n";
					os << ss.str();
					os << getIndent(1) << "</defs>" << "\n";
				}
			}
			GeosFromRecord2Packed(os, wrapped ? wrapped.get() : value, name, 0, pContext);
			os << "</svg>" << "\n";

			return true;
		}

		return false;
	}
}
