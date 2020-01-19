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

extern bool printAddressInsertion;

namespace cgl
{
	void GeometryDeleter::operator()(gg::Geometry* pGeometry)const
	{
		delete pGeometry;
	}

	Geometries::Geometries() = default;
	Geometries::~Geometries() = default;
	Geometries::Geometries(Geometries&&) = default;
	Geometries& Geometries::operator=(Geometries&&) = default;

	bool ReadPackedPolyData(const PackedList& polygons, std::vector<OutputPolygon>& outputPolygonDatas, const TransformPacked& transform)
	{
		const auto type = GetPackedListType(polygons);

		const auto readPolygon = [&](const std::vector<PackedList::Data>& vertices)->bool
		{
			gg::CoordinateSequence::Ptr pPoints = std::make_unique<gg::CoordinateArraySequence>();

			for (const auto& vertexData : vertices)
			{
				if (!IsType<PackedRecord>(vertexData.value))
				{
					return false;
				}

				const auto& cooedinateData = As<PackedRecord>(vertexData.value).values;
				auto itX = cooedinateData.find("x");
				auto itY = cooedinateData.find("y");
				if (itX == cooedinateData.end() || itY == cooedinateData.end() || !IsNum(itX->second.value) || !IsNum(itY->second.value))
				{
					return false;
				}

				auto pos = transform.product(EigenVec2(AsDouble(itX->second.value), AsDouble(itY->second.value)));
				pPoints->add(gg::Coordinate(pos.x(), pos.y()));
			}

			if (!pPoints->isEmpty())
			{
				pPoints->add(pPoints->front());

				auto pFactory = gg::GeometryFactory::create();
				std::unique_ptr<gg::Geometry> pLinearRing = pFactory->createLinearRing(std::move(pPoints));

				bool isClockWise;
				std::tie(isClockWise, pLinearRing) = IsClockWise(std::move(pLinearRing));
				if (isClockWise)
				{
					outputPolygonDatas.emplace_back();
					auto& outputPolygons = outputPolygonDatas.back().polygon;
					outputPolygons.push_back_raw(pFactory->createPolygon(dynamic_cast<LinearRing*>(pLinearRing.release()), {}));
				}
				//穴がデータに追加されるのは、既存のポリゴンが存在するときのみ
				else if(!outputPolygonDatas.empty())
				{
					auto& outputHoles = outputPolygonDatas.back().hole;
					outputHoles.push_back_raw(pFactory->createPolygon(dynamic_cast<LinearRing*>(pLinearRing.release()), {}));
				}
			}

			return true;
		};

		if (type == PackedPolyDataType::POLYGON)
		{
			return readPolygon(polygons.data);
		}
		else if (type == PackedPolyDataType::MULTIPOLYGON)
		{
			for (const auto& polygonData : polygons.data)
			{
				const auto& currentPolygonData = polygonData.value;

				if (!IsType<PackedList>(currentPolygonData) || !readPolygon(As<PackedList>(currentPolygonData).data))
				{
					return false;
				}
			}

			return true;
		}

		//PackedPolyDataType::?
		return false;
	}

	bool ReadPackedLineData(const PackedList& lines, Geometries& outputLineDatas, const TransformPacked& transform)
	{
		const auto type = GetPackedListType(lines);

		const auto readLine = [&](const std::vector<PackedList::Data>& vertices)->bool
		{
			gg::CoordinateSequence::Ptr pPoints = std::make_unique<gg::CoordinateArraySequence>();

			for (const auto& vertexData : vertices)
			{
				if (!IsType<PackedRecord>(vertexData.value))
				{
					return false;
				}

				const auto& cooedinateData = As<PackedRecord>(vertexData.value).values;
				auto itX = cooedinateData.find("x");
				auto itY = cooedinateData.find("y");
				if (itX == cooedinateData.end() || itY == cooedinateData.end() || !IsNum(itX->second.value) || !IsNum(itY->second.value))
				{
					return false;
				}

				auto pos = transform.product(EigenVec2(AsDouble(itX->second.value), AsDouble(itY->second.value)));
				pPoints->add(gg::Coordinate(pos.x(), pos.y()));
			}

			if (!pPoints->isEmpty())
			{
				auto pFactory = gg::GeometryFactory::create();
				outputLineDatas.push_back_raw(pFactory->createLineString(std::move(pPoints)).release());
			}

			return true;
		};

		if (type == PackedPolyDataType::POLYGON)
		{
			return readLine(lines.data);
		}
		else if (type == PackedPolyDataType::MULTIPOLYGON)
		{
			for (const auto& lineData : lines.data)
			{
				const auto& currentLineData = lineData.value;

				if (!IsType<PackedList>(currentLineData) || !readLine(As<PackedList>(currentLineData).data))
				{
					return false;
				}
			}

			return true;
		}

		//PackedPolyDataType::?
		return false;
	}

	Geometries GeosFromListPacked(const cgl::PackedList& list, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform);

	Geometries GeosFromRecordPackedImpl(const cgl::PackedRecord& record, std::shared_ptr<Context> pContext, const cgl::TransformPacked& parent)
	{
		const cgl::TransformPacked current(record);
		const cgl::TransformPacked transform = parent * current;

		Geometries resultPolygons;
		Geometries currentLines;

		for (const auto& member : record.values)
		{
			const cgl::PackedVal& value = member.second.value;

			if (member.first == "polygon")
			{
				std::vector<OutputPolygon> outputPolygons;

				if (cgl::IsType<cgl::PackedList>(value))
				{
					try
					{
						if (!ReadPackedPolyData(cgl::As<cgl::PackedList>(value), outputPolygons, transform))
						{
							CGL_Error("polygonに指定されたデータの形式が不正です。");
						}
					}
					catch (std::exception& e)
					{
						std::cout << "Packed Record ReadList: " << e.what() << std::endl;
						throw;
					}
				}
				else if (cgl::IsType<cgl::FuncVal>(value))
				{
					Eval evaluator(pContext);
					LRValue result;
					try
					{
						result = evaluator.callFunction(LocationInfo(), cgl::As<cgl::FuncVal>(value), {});
					}
					catch (std::exception& e)
					{
						std::cout << "Packed Record ReadFunc: " << e.what() << std::endl;
						throw;
					}
					const cgl::PackedVal evaluated = Packed(pContext->expand(result, LocationInfo()), *pContext);

					if (!cgl::IsType<cgl::PackedList>(evaluated))
					{
						CGL_Error("polygon()の評価結果の型が不正です。");
					}

					if (!ReadPackedPolyData(cgl::As<cgl::PackedList>(evaluated), outputPolygons, transform))
					{
						CGL_Error("polygonに指定されたデータの形式が不正です。");
					}
				}

				try
				{
					for(size_t i = 0; i < outputPolygons.size();)
					{
						auto& currentPolygons = outputPolygons[i].polygon;
						const auto& currentHoles = outputPolygons[i].hole;

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

						outputPolygons.erase(outputPolygons.begin() + i);
					}
				}
				catch (std::exception& e)
				{
					std::cout << "Packed Record2: " << e.what() << std::endl;
					throw;
				}
			}
			else if (member.first == "line")
			{
				if (cgl::IsType<cgl::PackedList>(value))
				{
					if (!ReadPackedLineData(cgl::As<cgl::PackedList>(value), currentLines, transform))
					{
						CGL_Error("lineに指定されたデータの形式が不正です。");
					}
				}
				else if (cgl::IsType<cgl::FuncVal>(value))
				{
					Eval evaluator(pContext);
					const cgl::PackedVal evaluated = Packed(pContext->expand(evaluator.callFunction(LocationInfo(), cgl::As<cgl::FuncVal>(value), {}), LocationInfo()), *pContext);

					if (!cgl::IsType<cgl::PackedList>(evaluated))
					{
						CGL_Error("line()の評価結果の型が不正です。");
					}

					if (!ReadPackedLineData(cgl::As<cgl::PackedList>(evaluated), currentLines, transform))
					{
						CGL_Error("lineに指定されたデータの形式が不正です。");
					}
				}
			}
			else if (cgl::IsType<cgl::PackedRecord>(value))
			{
				resultPolygons.append(GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pContext, transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				resultPolygons.append(GeosFromListPacked(cgl::As<cgl::PackedList>(value), pContext, transform));
			}
		}

		try
		{
			auto factory = gg::GeometryFactory::create();

			if (resultPolygons.empty())
			{
				return currentLines;
			}
			else
			{
				resultPolygons.append(std::move(currentLines));
				return resultPolygons;
			}
		}
		catch (std::exception& e)
		{
			std::cout << "Packed Record End: " << e.what() << std::endl;
			throw;
		}
	}

	Geometries GeosFromListPacked(const cgl::PackedList& list, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform)
	{
		Geometries currentPolygons;
		for (const auto& val : list.data)
		{
			const PackedVal& value = val.value;
			if (cgl::IsType<cgl::PackedRecord>(value))
			{
				currentPolygons.append(GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pContext, transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				currentPolygons.append(GeosFromListPacked(cgl::As<cgl::PackedList>(value), pContext, transform));
			}
		}

		return currentPolygons;
	}

	Geometries GeosFromRecordPacked(const PackedVal& value, std::shared_ptr<Context> pContext, const cgl::TransformPacked& parent)
	{
		if (printAddressInsertion)
		{
			//std::cout << "GeosFromRecordPacked begin" << std::endl;
		}
		if (cgl::IsType<cgl::PackedRecord>(value))
		{
			const auto& record = cgl::As<cgl::PackedRecord>(value);

			//valueが直下にxとyを持つ構造{x: _, y: _, ...}だった場合は単一のベクトル値として解釈する
			if (record.values.find("x") != record.values.end() &&
				record.values.find("y") != record.values.end())
			{
				const cgl::TransformPacked current(record);
				const cgl::TransformPacked transform = parent * current;

				Geometries currentPolygons;

				const auto v = ReadVec2Packed(record, transform);

				auto factory = gg::GeometryFactory::create();
				currentPolygons.push_back_raw(factory->createPoint(gg::Coordinate(std::get<0>(v), std::get<1>(v))));

				if (printAddressInsertion)
				{
					//std::cout << "GeosFromRecordPacked end" << std::endl;
				}
				return currentPolygons;
			}

			auto result = GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pContext, parent);
			if (printAddressInsertion)
			{
				//std::cout << "GeosFromRecordPacked end" << std::endl;
			}
			return result;
		}
		if (cgl::IsType<cgl::PackedList>(value))
		{
			auto result = GeosFromListPacked(cgl::As<cgl::PackedList>(value), pContext, parent);
			if (printAddressInsertion)
			{
				//std::cout << "GeosFromRecordPacked end" << std::endl;
			}
			return result;
		}

		if (printAddressInsertion)
		{
			//std::cout << "GeosFromRecordPacked end" << std::endl;
		}
		return{};
	}
}
