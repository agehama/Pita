#ifndef NOMINMAX
#define NOMINMAX
#endif

//#include <windows.h>
//#include <imagehlp.h>
//
//#pragma comment(lib, "imagehlp.lib")

//#include <StackWalker/StackWalker.h>
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

//void printStack()
//{
//	unsigned int   i;
//	void         * stack[100];
//	unsigned short frames;
//	SYMBOL_INFO  * symbol;
//	HANDLE         process;
//
//	process = GetCurrentProcess();
//
//	SymInitialize(process, NULL, TRUE);
//
//	const size_t trace_frames_max = 50;
//	frames = CaptureStackBackTrace(0, trace_frames_max, stack, NULL);
//	symbol = (SYMBOL_INFO *)calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1);
//	symbol->MaxNameLen = 255;
//	symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
//
//	for (i = 0; i < frames; i++)
//	{
//		SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
//
//		printf("%i: %s - 0x%0X\n", frames - i - 1, symbol->Name, symbol->Address);
//	}
//
//	free(symbol);
//}

//void printStack()
//{
//	StackWalker sw; sw.ShowCallstack();
//}

namespace cgl
{
	//Geometries::Geometries() = default;
	//Geometries::~Geometries() = default;

	//bool Geometries::empty()const
	//{
	//	return gs.empty();
	//}

	//size_t Geometries::size()const
	//{
	//	return gs.size();
	//}

	//std::unique_ptr<gg::Geometry> Geometries::takeOut(size_t index)
	//{
	//	std::unique_ptr<gg::Geometry> ptr(std::move(gs[index]));
	//	gs.erase(gs.begin() + index);
	//	return ptr;
	//}

	//const gg::Geometry* const Geometries::refer(size_t index)const
	//{
	//	return gs[index].get();
	//}

	///*Geometries::Type::iterator Geometries::begin()
	//{
	//return gs.begin();
	//}

	//Geometries::Type::const_iterator Geometries::begin()const
	//{
	//return gs.begin();
	//}

	//Geometries::Type::iterator Geometries::end()
	//{
	//return gs.end();
	//}

	//Geometries::Type::const_iterator Geometries::end()const
	//{
	//return gs.end();
	//}

	//void Geometries::insert(Type::iterator it, std::unique_ptr<gg::Geometry> g)
	//{
	//gs.insert(it, std::move(g));
	//}

	//void Geometries::erase(Type::iterator it)
	//{
	//gs.erase(it);
	//}*/

	//void Geometries::insert(size_t i, std::unique_ptr<gg::Geometry> g)
	//{
	//	gs.insert(gs.begin() + i, std::move(g));
	//}

	//void Geometries::erase(size_t i)
	//{
	//	gs.erase(gs.begin() + i);
	//}

	//void Geometries::push_back(std::unique_ptr<gg::Geometry> g)
	//{
	//	gs.push_back(std::move(g));
	//}

	//void Geometries::pop_back()
	//{
	//	gs.pop_back();
	//}

	//void Geometries::append(Geometries&& tail)
	//{
	//	gs.insert(gs.end(), std::make_move_iterator(tail.gs.begin()), std::make_move_iterator(tail.gs.end()));
	//	tail.gs.clear();
	//}

	Geometries::Geometries() = default;
	Geometries::~Geometries() = default;

	bool Geometries::empty()const
	{
		return gs.empty();
	}

	size_t Geometries::size()const
	{
		return gs.size();
	}

	std::shared_ptr<gg::Geometry> Geometries::takeOut(size_t index)
	{
		std::shared_ptr<gg::Geometry> ptr(std::move(gs[index]));
		gs.erase(gs.begin() + index);
		return ptr;
	}

	const gg::Geometry* const Geometries::refer(size_t index)const
	{
		return gs[index].get();
	}

	void Geometries::insert(size_t i, std::shared_ptr<gg::Geometry> g)
	{
		gs.insert(gs.begin() + i, std::move(g));
	}

	void Geometries::erase(size_t i)
	{
		gs.erase(gs.begin() + i);
	}

	void Geometries::push_back(std::shared_ptr<gg::Geometry> g)
	{
		gs.push_back(std::move(g));
	}

	void Geometries::pop_back()
	{
		gs.pop_back();
	}

	void Geometries::append(Geometries&& tail)
	{
		gs.insert(gs.end(), std::make_move_iterator(tail.gs.begin()), std::make_move_iterator(tail.gs.end()));
		tail.gs.clear();
	}

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
		/*for (auto geometry : geometries)
		{
			geometry->apply_ro(&maker);
		}*/
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

	/*struct OutputPolygon
	{
		Geometries polygon;
		Geometries hole;

		~OutputPolygon()
		{
			for (gg::Geometry* g : polygon)
			{
				delete g;
			}
			for (gg::Geometry* g : hole)
			{
				delete g;
			}
		}
	};*/

	struct OutputPolygon
	{
		Geometries polygon;
		Geometries hole;
	};

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
					outputPolygons.push_back(
						//ToUnique(pFactory->createPolygon(dynamic_cast<LinearRing*>(pLinearRing.release()), {}))
						ToShared(pFactory->createPolygon(dynamic_cast<LinearRing*>(pLinearRing.release()), {}))
					);
				}
				//穴がデータに追加されるのは、既存のポリゴンが存在するときのみ
				else if(!outputPolygonDatas.empty())
				{
					auto& outputHoles = outputPolygonDatas.back().hole;
					outputHoles.push_back(
						//ToUnique(pFactory->createPolygon(dynamic_cast<LinearRing*>(pLinearRing.release()), {}))
						ToShared(pFactory->createPolygon(dynamic_cast<LinearRing*>(pLinearRing.release()), {}))
					);
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

	//bool ReadPackedLineData(const PackedList& lines, Geometries& outputLineDatas, const TransformPacked& transform)
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
				outputLineDatas.push_back(
					//ToUnique(pFactory->createLineString(std::move(pPoints)).release())
					ToShared(pFactory->createLineString(std::move(pPoints)).release())
				);
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
									//std::unique_ptr<gg::Geometry> pErodeGeometry(currentPolygons.takeOut(s));
									std::shared_ptr<gg::Geometry> pErodeGeometry(currentPolygons.takeOut(s));

									for (int d = 0; d < currentHoles.size(); ++d)
									{
										gg::Geometry* temporaryGeometry = pErodeGeometry->difference(currentHoles.refer(d));

										if (temporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
										{
											delete temporaryGeometry;
											//->次のpErodeGeometryには現在のポリゴンがそのまま入っている
										}
										else if (temporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
										{
											gg::MultiPolygon* polygons = dynamic_cast<gg::MultiPolygon*>(temporaryGeometry);
											for (int i = 0; i < polygons->getNumGeometries(); ++i)
											{
												//currentPolygons.insert(s, ToUnique(polygons->getGeometryN(i)->clone()));
												currentPolygons.insert(s, ToShared(polygons->getGeometryN(i)->clone()));
											}
											pErodeGeometry = currentPolygons.takeOut(s);

											//currentPolygonsに挿入したのはcloneなのでdifferenceの結果はここで削除する
											delete temporaryGeometry;

											//->次のpErodeGeometryには分割された最初のポリゴンが入っている
										}
										else if (pErodeGeometry->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION)
										{
											//結果が空であれば、ポリゴンを削除する
											pErodeGeometry.reset();
											delete temporaryGeometry;
											break;
											//->次のpErodeGeometryには何も入っていないのでループを抜ける
										}
										else
										{
											delete temporaryGeometry;
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
				//GeosPolygonsConcat(resultPolygons, GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pContext, transform));
				resultPolygons.append(GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pContext, transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				//GeosPolygonsConcat(resultPolygons, GeosFromListPacked(cgl::As<cgl::PackedList>(value), pContext, transform));
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
				//resultPolygons.insert(resultPolygons.end(), currentLines.begin(), currentLines.end());
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
				//GeosPolygonsConcat(currentPolygons, GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pContext, transform));
				currentPolygons.append(GeosFromRecordPackedImpl(cgl::As<cgl::PackedRecord>(value), pContext, transform));
			}
			else if (cgl::IsType<cgl::PackedList>(value))
			{
				//GeosPolygonsConcat(currentPolygons, GeosFromListPacked(cgl::As<cgl::PackedList>(value), pContext, transform));
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
				//currentPolygons.push_back(factory->createPoint(gg::Coordinate(std::get<0>(v), std::get<1>(v))));
				currentPolygons.push_back(
					//ToUnique(static_cast<gg::Geometry*>(factory->createPoint(gg::Coordinate(std::get<0>(v), std::get<1>(v)))))
					ToShared(static_cast<gg::Geometry*>(factory->createPoint(gg::Coordinate(std::get<0>(v), std::get<1>(v)))))
				);

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

	bool GeosFromList2Packed(std::ostream& os, const PackedList& list, const std::string& name, int depth, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform);

	bool GeosFromRecord2Packed(std::ostream& os, const PackedVal& value, const std::string& name, int depth, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform = cgl::TransformPacked());

	bool GeosFromRecordImpl2Packed(std::ostream& os, const PackedRecord& record, const std::string& name, int depth, std::shared_ptr<Context> pContext, const cgl::TransformPacked& parent)
	{
		const TransformPacked current(record);
		const TransformPacked transform = parent * current;

		std::vector<PitaGeometry> wholePolygons;

		Geometries resultPolygons;
		//gg::Geometry* currentLine = nullptr;
		Geometries currentLines;

		//現時点では実際に描画されるデータを持っているかどうかわからないため、一旦別のストリームに保存しておく
		std::stringstream currentStream;
		currentStream << getIndent(depth) << "<g id=\"" << name << "\" ";

		std::stringstream currentChildStream;

		//Color currentColor;

		bool hasShape = false;

		for (const auto& member : record.values)
		{
			const auto& value = member.second.value;

			if (member.first == "polygon")
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

				for (size_t i = 0; i < outputPolygons.size();)
				{
					auto& currentPolygons = outputPolygons[i].polygon;
					const auto& currentHoles = outputPolygons[i].hole;

					if (!currentPolygons.empty())
					{
						if (!currentHoles.empty())
						{
							for (int s = 0; s < currentPolygons.size();)
							{
								//std::unique_ptr<gg::Geometry> pErodeGeometry(currentPolygons.takeOut(s));
								std::shared_ptr<gg::Geometry> pErodeGeometry(currentPolygons.takeOut(s));

								for (int d = 0; d < currentHoles.size(); ++d)
								{
									gg::Geometry* temporaryGeometry = pErodeGeometry->difference(currentHoles.refer(d));

									if (temporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
									{
										delete temporaryGeometry;
										//->次のpErodeGeometryには現在のポリゴンがそのまま入っている
									}
									else if (temporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
									{
										gg::MultiPolygon* polygons = dynamic_cast<gg::MultiPolygon*>(temporaryGeometry);
										for (int i = 0; i < polygons->getNumGeometries(); ++i)
										{
											//currentPolygons.insert(s, ToUnique(polygons->getGeometryN(i)->clone()));
											currentPolygons.insert(s, ToShared(polygons->getGeometryN(i)->clone()));
										}
										pErodeGeometry = currentPolygons.takeOut(s);

										//currentPolygonsに挿入したのはcloneなのでdifferenceの結果はここで削除する
										delete temporaryGeometry;

										//->次のpErodeGeometryには分割された最初のポリゴンが入っている
									}
									else if (pErodeGeometry->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION)
									{
										//結果が空であれば、ポリゴンを削除する
										pErodeGeometry.reset();
										delete temporaryGeometry;
										break;
										//->次のpErodeGeometryには何も入っていないのでループを抜ける
									}
									else
									{
										delete temporaryGeometry;
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
					Color currentColor;
					ReadColorPacked(currentColor, As<PackedRecord>(value), transform);
					currentStream << "fill=\"" << currentColor.toString() << "\" ";
				}
				else
				{
					currentStream << "fill=\"none\" ";
				}
			}
			else if (member.first == "stroke")
			{
				if (IsType<PackedRecord>(value))
				{
					Color currentColor;
					ReadColorPacked(currentColor, As<PackedRecord>(value), transform);
					currentStream << "stroke=\"" << currentColor.toString() << "\" ";
				}
				else
				{
					currentStream << "stroke=\"none\" ";
				}
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
				hasShape = GeosFromRecordImpl2Packed(currentChildStream, As<PackedRecord>(value), member.first, depth + 1, pContext, transform) || hasShape;
			}
			else if (IsType<PackedList>(value))
			{
				hasShape = GeosFromList2Packed(currentChildStream, As<PackedList>(value), member.first, depth + 1, pContext, transform) || hasShape;
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
				if (IsClockWise(outer))
				{
					for (int i = 0; i < outer->getNumPoints(); ++i)
					{
						auto p = outer->getPointN(i);
						os << p->getX() << "," << p->getY() << " ";
					}
				}
				else
				{
					for (int i = outer->getNumPoints() - 1; 0 <= i; --i)
					{
						auto p = outer->getPointN(i);
						os << p->getX() << "," << p->getY() << " ";
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
							auto p = outer->getPointN(0);
							os << "M" << p->getX() << "," << p->getY() << " ";

							for (int i = 1; i < outer->getNumPoints(); ++i)
							{
								auto p = outer->getPointN(i);
								os << "L" << p->getX() << "," << p->getY() << " ";
							}
						}
						else
						{
							auto p = outer->getPointN(outer->getNumPoints() - 1);
							os << "M" << p->getX() << "," << p->getY() << " ";

							for (int i = outer->getNumPoints() - 2; 0 < i; --i)
							{
								auto p = outer->getPointN(i);
								os << "L" << p->getX() << "," << p->getY() << " ";
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
							auto p = hole->getPointN(hole->getNumPoints() - 1);
							os << "M" << p->getX() << "," << p->getY() << " ";

							for (int n = hole->getNumPoints() - 2; 0 < n; --n)
							{
								auto p = hole->getPointN(n);
								os << "L" << p->getX() << "," << p->getY() << " ";
							}
						}
						else
						{
							auto p = hole->getPointN(0);
							os << "M" << p->getX() << "," << p->getY() << " ";

							for (int n = 1; n < hole->getNumPoints(); ++n)
							{
								auto p = hole->getPointN(n);
								os << "L" << p->getX() << "," << p->getY() << " ";
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
				wholePolygons.emplace_back(currentLines.takeOut(0).get(), Color());
			}

			writeWholeData();

			return true;
		}
		else
		{
			while (!resultPolygons.empty())
			{
				wholePolygons.emplace_back(resultPolygons.takeOut(0).get(), Color());
			}

			while (!currentLines.empty())
			{
				wholePolygons.emplace_back(currentLines.takeOut(0).get(), Color());
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
		auto boundingBoxOpt = IsType<PackedRecord>(value) ? boost::optional<BoundingRect>(BoundingRectRecordPacked(value, pContext)) : boost::none;
		if (IsType<PackedRecord>(value) && boundingBoxOpt)
		{
			const BoundingRect& rect = boundingBoxOpt.get();
			//const auto pos = rect.pos();
			const auto widthXY = rect.width();
			const auto center = rect.center();

			const double margin = 5.0;
			//const double width = std::max(widthXY.x(), widthXY.y());
			//const double halfWidth = width * 0.5;
			const double width = widthXY.x() + margin;
			const double height = widthXY.y() + margin;

			const Eigen::Vector2d pos = center - EigenVec2(width*0.5, height*0.5);

			/*os << R"(<svg xmlns="http://www.w3.org/2000/svg" version="1.2" baseProfile="tiny" width=")" << width << R"(" height=")" << height << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << height
				<< R"(" viewport-fill="black" viewport-fill-opacity="0.1)"  << R"(">)" << "\n";*/
			os << R"(<svg xmlns="http://www.w3.org/2000/svg" width=")" << width << R"(" height=")" << height << R"(" viewBox=")" << pos.x() << " " << pos.y() << " " << width << " " << height << R"(">)" << "\n";
			GeosFromRecord2Packed(os, value, name, 0, pContext);
			os << "</svg>" << "\n";

			return true;
		}

		return false;
	}
}
