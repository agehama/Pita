#include <cppoptlib/meta.h>
#include <cppoptlib/problem.h>
#include <cppoptlib/solver/bfgssolver.h>

#include <geos/geom.h>
#include <geos/opBuffer.h>
#include <geos/opDistance.h>
#include <geos/operation/union/CascadedUnion.h>

#include <cmaes.h>

#include <Unicode.hpp>

#include <Pita/IntrinsicGeometricFunctions.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/FontShape.hpp>
#include <Pita/Printer.hpp>
#include <Pita/Evaluator.hpp>
#include <Pita/Parser.hpp>

namespace
{
	inline double Combination(int n, int k)
	{
		if (k > n) return 0;
		if (k * 2 > n) k = n - k;
		if (k == 0) return 1;

		double result = n;
		for (int i = 2; i <= k; ++i)
		{
			result *= (n - i + 1);
			result /= i;
		}

		return result;
	}

	inline double BernsteinBasis(int i, int n, double t)
	{
		const double coef = Combination(n, i);
		return coef * pow(t, i)*pow(1 - t, n - i);
	}

	inline double Lerp(double x0, double x1, double t)
	{
		return x0 + (x1 - x0)*t;
	}

	class LinearBernstein
	{
	public:
		using LinearFunction = std::vector<double>;

		LinearBernstein() = default;

		void initialize(int n, int quality = 100)
		{
			m_n = n;
			m_quality = quality;

			m_functions.resize(n + 1);
			for (int i = 0; i <= n; ++i)
			{
				auto& function = m_functions[i];
				function.resize(quality);

				for (int d = 0; d < quality; ++d)
				{
					const double t = 1.0*d / (quality - 1);
					function[d] = BernsteinBasis(i, n, t);
				}
			}
		}

		double operator()(int i, double t)const
		{
			const double p = t * m_quality;
			const int index = static_cast<int>(p);
			const double remainder = p - index;

			return Lerp(m_functions[i][index], m_functions[i][index + 1], remainder);
		}

	private:
		std::vector<LinearFunction> m_functions;
		int m_n;
		int m_quality;
	};
}

namespace cgl
{
	bool IsClockWise(const gg::LineString* closedPath)
	{
		double sum = 0;

		for (size_t p = 0; p + 1 < closedPath->getNumPoints(); ++p)
		{
			const gg::Point* p1 = closedPath->getPointN(p);
			const gg::Point* p2 = closedPath->getPointN(p+1);

			sum += (p2->getX() - p1->getX())*(p2->getY() + p1->getY());
		}
		{
			const gg::Point*  p1 = closedPath->getPointN(closedPath->getNumPoints() - 1);
			const gg::Point*  p2 = closedPath->getPointN(0);

			sum += (p2->getX() - p1->getX())*(p2->getY() + p1->getY());
		}

		return sum < 0.0;
	}

	class Deformer
	{
	public:
		Deformer() = default;

		void initialize(const BoundingRect& boundingRect, const std::vector<std::vector<double>>& dataX, const std::vector<std::vector<double>>& dataY)
		{
			m_boundingRect = boundingRect;

			xs = dataX;
			ys = dataY;

			const int yNum = dataX.size();
			const int xNum = dataX.front().size();

			m_bernsteinX.initialize(xNum - 1);
			m_bernsteinY.initialize(yNum - 1);
		}

		std::vector<gg::Geometry*> FFD(const std::vector<gg::Geometry*>& originalShape)
		{
			const auto getUV = [&](double x, double y)->Eigen::Vector2d
			{
				const auto pos = m_boundingRect.pos();
				const auto size = m_boundingRect.width();

				const double dx = x - pos.x();
				const double dy = y - pos.y();

				Eigen::Vector2d uv;
				uv << (dx / size.x()), 1.0 - (dy / size.y());
				if (!(0.0 < uv.x() && uv.x() < 1.0 && 0.0 < uv.y() && uv.y() < 1.0))
				{
					std::cout << "m_boundingRect.pos(): (" << pos.x() << ", " << pos.y() << ")\n";
					std::cout << "m_boundingRect.width(): (" << size.x() << ", " << size.y() << ")\n";
					std::cout << "(dx, dy): (" << dx << ", " << dy << ")\n";
					std::cout << "(u, v): (" << (dx / size.x()) << ", " << 1.0 - (dy / size.y()) << ")\n";
					CGL_Error("UV is out of range in FFD.");
				}
				return uv;
			};

			const auto get = [&](double tX, double tY, const std::vector<std::vector<double>>& dataX, const std::vector<std::vector<double>>& dataY,bool debug)
			{
				const int nX = dataX.front().size() - 1;
				const int nY = dataX.size() - 1;

				/*if (debug)
				{
					std::cout << "dataSizeX: (" << dataX.front().size() << ", " << dataX.size() << ")" << std::endl;
					std::cout << "dataSizeY: (" << dataY.front().size() << ", " << dataY.size() << ")" << std::endl;
					std::cout << "(u, v): (" << tX << ", " << tY << ")" << std::endl;
				}*/

				double sumX = 0.0;
				double sumY = 0.0;

				for (int yi = 0; yi <= nY; ++yi)
				{
					//const double bY = BernsteinBasis(yi, nY, tY);
					const double bY = m_bernsteinY(yi, tY);
					/*if (debug)
					{
						std::cout << "(yi): (" << yi << ")" << std::endl;
					}*/
					for (int xi = 0; xi <= nX; ++xi)
					{
						//const double bX = BernsteinBasis(xi, nX, tX);
						const double bX = m_bernsteinX(xi, tX);
						const double coef = bX * bY;

						const double dY = dataY[yi][xi];
						const double dX = dataX[yi][xi];

						sumX += coef * dX;
						sumY += coef * dY;
					}
				}

				//itv sumX(0.0);
				//itv sumY(0.0);

				//for (int yi = 0; yi <= nY; ++yi)
				//{
				//	const itv bY(BernsteinBasis2(yi, nY, itv(tY)));
				//	//const double bY = m_bernsteinY(yi, tY);
				//	for (int xi = 0; xi <= nX; ++xi)
				//	{
				//		const itv bX(BernsteinBasis2(xi, nX, itv(tX)));
				//		//const double bX = m_bernsteinX(xi, tX);
				//		const itv coef = bX * bY;

				//		const itv dY(dataY[yi][xi]);
				//		const itv dX(dataX[yi][xi]);

				//		sumX += coef * dX;
				//		sumY += coef * dY;
				//	}
				//}

				/*if (debug)
				{
					std::cout<<"width: " << std::max(width(sumX), width(sumY)) << "\n";
				}*/

				Eigen::Vector2d pos;
				//pos << mid(sumX), mid(sumY);
				pos << sumX, sumY;
				return pos;
			};

			auto factory = gg::GeometryFactory::create();

			std::vector<gg::Geometry*> result;

			const int num = 20;

			//for (const gg::Geometry* geometry : originalShape)
			for (int shapei=0;shapei<originalShape.size();++shapei)
			{
				bool debugPrint = false;
				if (shapei + 18 == originalShape.size())
				{
					//continue;
					debugPrint = true;
				}

				//int pointsCount = 0;
				const gg::Geometry* geometry = originalShape[shapei];
				//CGL_DBG1(geometry->getGeometryType());
				if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
				{
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);

					const gg::LineString* exterior = polygon->getExteriorRing();

					gg::CoordinateArraySequence newExterior;
					for (size_t p = 0; p + 1 < exterior->getNumPoints(); ++p)
					{
						const gg::Point* p0 = exterior->getPointN(p);
						const gg::Point* p1 = exterior->getPointN(p + 1);

						for (int sub = 0; sub < num; ++sub)
						{
							const double progress = 1.0 * sub / num;
							const double x = p0->getX() + (p1->getX() - p0->getX())*progress;
							const double y = p0->getY() + (p1->getY() - p0->getY())*progress;

							const auto uv = getUV(x, y);
							const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

							//std::cout <<"sub" << sub << ": (" << x << ", " << y << ")\n";
							//++pointsCount;
							//std::cout << pointsCount << " | " << "sub" << sub << ": (" << newPos.x() << ", " << newPos.y() << ")\n";

							newExterior.add(gg::Coordinate(newPos.x(), newPos.y()));
						}
					}

					if (!newExterior.empty())
					{
						newExterior.add(newExterior.front());
					}

					std::vector<gg::Geometry*>* holes = new std::vector<gg::Geometry*>;
					for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
					{
						const gg::LineString* hole = polygon->getInteriorRingN(i);

						gg::CoordinateArraySequence newInterior;
						//for (size_t p = 0; p < hole->getNumPoints(); ++p)
						/*for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p; --p)
						{
							const gg::Point* point = hole->getPointN(p);

							const auto uv = getUV(point->getX(), point->getY());

							const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

							newInterior.add(gg::Coordinate(newPos.x(), newPos.y()));
						}*/
						for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p - 1; --p)
						{
							const gg::Point* p0 = hole->getPointN(p);
							const gg::Point* p1 = hole->getPointN(p - 1);

							for (int sub = 0; sub < num; ++sub)
							{
								const double progress = 1.0 * sub / num;
								const double x = p0->getX() + (p1->getX() - p0->getX())*progress;
								const double y = p0->getY() + (p1->getY() - p0->getY())*progress;

								const auto uv = getUV(x, y);
								const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

								newInterior.add(gg::Coordinate(newPos.x(), newPos.y()));
							}
						}

						if (!newInterior.empty())
						{
							newInterior.add(newInterior.front());
						}

						holes->push_back(factory->createLinearRing(newInterior));

						/*
						PackedRecord pathRecord;
						PackedList polygonList;

						if (IsClockWise(hole))
						{
							for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p; --p)
							{
								const gg::Point* point = hole->getPointN(p);
								appendCoord(polygonList, point->getX(), point->getY());
							}
						}
						else
						{
							for (size_t p = 0; p < hole->getNumPoints(); ++p)
							{
								const gg::Point* point = hole->getPointN(p);
								appendCoord(polygonList, point->getX(), point->getY());
							}
						}

						pathRecord.add("line", polygonList);
						pathList.add(pathRecord);
						*/
					}
					
					//result.push_back(factory->createPolygon(factory->createLinearRing(newExterior), {}));
					result.push_back(factory->createPolygon(factory->createLinearRing(newExterior), holes));
				}
				else if(geometry->getGeometryTypeId() == gg::GEOS_LINESTRING)
				{
					const gg::LineString* lineString = dynamic_cast<const gg::LineString*>(geometry);
					
					gg::CoordinateArraySequence newPoints;
					for (size_t p = 0; p + 1 < lineString->getNumPoints(); ++p)
					{
						const gg::Point* p0 = lineString->getPointN(p);
						const gg::Point* p1 = lineString->getPointN(p + 1);

						for (int sub = 0; sub < num; ++sub)
						{
							//debugPrint = shapei == 0 && p == 0 && sub == 0;
							const double progress = 1.0 * sub / (num - 1);
							const double x = p0->getX() + (p1->getX() - p0->getX())*progress;
							const double y = p0->getY() + (p1->getY() - p0->getY())*progress;

							const auto uv = getUV(x, y);
							const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);
							newPoints.add(gg::Coordinate(newPos.x(), newPos.y()));
						}
					}

					result.push_back(factory->createLineString(newPoints));
				}
			}

			return result;
		}

	private:
		BoundingRect m_boundingRect;
		std::vector<std::vector<double>> xs;
		std::vector<std::vector<double>> ys;
		int yNum, xNum;

		LinearBernstein m_bernsteinX, m_bernsteinY;
	};

	class PathConstraintProblem : public cppoptlib::Problem<double>
	{
	public:
		using typename cppoptlib::Problem<double>::TVector;

		std::function<double(const TVector&)> evaluator;

		double value(const TVector &x)
		{
			return evaluator(x);
		}
	};

	PackedRecord GetOffsetPathImpl(const Path& originalPath, double offset)
	{
		auto factory = gg::GeometryFactory::create();
		gg::PrecisionModel model(gg::PrecisionModel::Type::FLOATING);

		//gob::BufferParameters param(10, gob::BufferParameters::CAP_ROUND, gob::BufferParameters::JOIN_BEVEL, 10.0);
		gob::BufferParameters param(10, gob::BufferParameters::CAP_FLAT, gob::BufferParameters::JOIN_BEVEL, 10.0);

		gob::OffsetCurveBuilder builder(&model, param);

		std::vector<gg::CoordinateSequence*> resultLines;
		bool isClosedPath = false;
		if (originalPath.empty())
		{
			CGL_Error("Original Path is Empty");
		}

		if (2 <= originalPath.cs->size())
		{
			const auto front = originalPath.cs->front();
			const auto back = originalPath.cs->back();

			isClosedPath = (front.x == back.x && front.y == back.y);
		}

		const bool isLeftSide = 0.0 <= offset;
		//builder.getLineCurve(&points, 15.0, result);
		builder.getSingleSidedLineCurve(originalPath.cs.get(), std::abs(offset), resultLines, isLeftSide, !isLeftSide);

		PackedList polygonList;
		for (size_t i = 0; i < resultLines.size(); ++i)
		{
			const auto line = resultLines[i];
			for (size_t p = 0; p < line->getSize(); ++p)
			{
				polygonList.add(MakeVec2(line->getX(p), line->getY(p)));
			}
		}

		//getSingleSidedLineCurveがClosedPathを返すので最後の点を消す
		if (!isClosedPath)
		{
			polygonList.data.pop_back();
		}

		//getSingleSidedLineCurveに左と右を指定したとき方向が逆になるので合わせる
		if (!isLeftSide)
		{
			std::reverse(polygonList.data.begin(), polygonList.data.end());
		}

		PackedRecord result;
		result.add("line", polygonList);
		return result;
	}

	PackedRecord ShapeResult(const PackedVal& lhs = PackedList())
	{
		return MakeRecord(
			"result", lhs,
			"pos", MakeRecord("x", 0, "y", 0),
			"scale", MakeRecord("x", 1.0, "y", 1.0),
			"angle", 0
		);
	}

	PackedRecord ShapeDiff(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(lhs, pContext);
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecordPacked(rhs, pContext);

		auto factory = gg::GeometryFactory::create();

		if (lhsPolygon.empty())
		{
			return ShapeResult();
		}
		else if (rhsPolygon.empty())
		{
			return ShapeResult(GetShapesFromGeosPacked(lhsPolygon));
		}
		else
		{
			std::vector<gg::Geometry*> resultGeometries;

			for (int s = 0; s < lhsPolygon.size(); ++s)
			{
				gg::Geometry* erodeGeometry = lhsPolygon[s];

				for (int d = 0; d < rhsPolygon.size(); ++d)
				{
					erodeGeometry = erodeGeometry->difference(rhsPolygon[d]);

					if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						lhsPolygon.erase(lhsPolygon.begin() + s);

						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);

						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							lhsPolygon.insert(lhsPolygon.begin() + s, polygons->getGeometryN(i)->clone());
						}

						erodeGeometry = lhsPolygon[s];
					}
					else if(erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_POLYGON && 
						erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_GEOMETRYCOLLECTION)
					{
						CGL_Error(std::string("Diffの結果が予期せぬデータ形式: \"") + GetGeometryType(erodeGeometry) + "\"");
					}
				}

				resultGeometries.push_back(erodeGeometry);
			}

			return ShapeResult(GetShapesFromGeosPacked(resultGeometries));
		}
	}

	PackedRecord ShapeUnion(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(lhs, pContext);
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecordPacked(rhs, pContext);

		lhsPolygon.insert(lhsPolygon.end(), rhsPolygon.begin(), rhsPolygon.end());

		geos::operation::geounion::CascadedUnion unionCalc(&lhsPolygon);
		gg::Geometry* result = unionCalc.Union();

		return ShapeResult(GetShapesFromGeosPacked({ result }));
	}

	PackedRecord ShapeIntersect(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(lhs, pContext);
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecordPacked(rhs, pContext);

		auto factory = gg::GeometryFactory::create();

		if (lhsPolygon.empty() || rhsPolygon.empty())
		{
			return ShapeResult();
		}
		else
		{
			std::vector<gg::Geometry*> resultGeometries;

			for (int s = 0; s < lhsPolygon.size(); ++s)
			{
				gg::Geometry* erodeGeometry = lhsPolygon[s];

				for (int d = 0; d < rhsPolygon.size(); ++d)
				{
					erodeGeometry = erodeGeometry->intersection(rhsPolygon[d]);

					if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						lhsPolygon.erase(lhsPolygon.begin() + s);

						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);

						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							lhsPolygon.insert(lhsPolygon.begin() + s, polygons->getGeometryN(i)->clone());
						}

						erodeGeometry = lhsPolygon[s];
					}
					else if (erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_POLYGON &&
						erodeGeometry->getGeometryTypeId() != geos::geom::GEOS_GEOMETRYCOLLECTION)
					{
						CGL_Error(std::string("Intersectの結果が予期せぬデータ形式: \"") + GetGeometryType(erodeGeometry) + "\"");
					}
				}

				resultGeometries.push_back(erodeGeometry);
			}

			return ShapeResult(GetShapesFromGeosPacked(resultGeometries));
		}
	}

	PackedRecord ShapeSymDiff(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(lhs, pContext);
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecordPacked(rhs, pContext);

		geos::operation::geounion::CascadedUnion unionCalcLhs(&lhsPolygon);
		gg::Geometry* lhsUnion = unionCalcLhs.Union();

		geos::operation::geounion::CascadedUnion unionCalcRhs(&rhsPolygon);
		gg::Geometry* rhsUnion = unionCalcRhs.Union();

		gg::Geometry* resultGeometry = lhsUnion->symDifference(rhsUnion);

		return ShapeResult(GetShapesFromGeosPacked({ resultGeometry }));
	}

	PackedRecord ShapeBuffer(const PackedVal& shape, const PackedVal& amount, std::shared_ptr<Context> pContext)
	{
		if (!IsType<PackedRecord>(shape) && !IsType<PackedList>(shape))
		{
			CGL_Error("不正な式です");
		}

		if (!IsNum(amount))
		{
			CGL_Error("不正な式です");
		}

		const double distance = AsDouble(amount);
		std::vector<gg::Geometry*> polygons = GeosFromRecordPacked(shape, pContext);

		std::vector<gg::Geometry*> resultGeometries;
		for (int s = 0; s < polygons.size(); ++s)
		{
			gg::Geometry* currentGeometry = polygons[s];
			resultGeometries.push_back(currentGeometry->buffer(distance));
		}

		return ShapeResult(GetShapesFromGeosPacked(resultGeometries));
	}

	PackedRecord ShapeSubDiv(const PackedVal& shape, int numSubDiv, std::shared_ptr<Context> pContext)
	{
		if (!IsType<PackedRecord>(shape))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(shape, pContext);
		std::vector<gg::Geometry*> resultGeometries;

		auto factory = gg::GeometryFactory::create();

		const auto subdividePoly = [&](const gg::Polygon* polygon)
		{
			const gg::LineString* exterior = polygon->getExteriorRing();

			gg::CoordinateArraySequence newExterior;
			for (size_t p = 0; p + 1 < exterior->getNumPoints(); ++p)
			{
				const gg::Point* p0 = exterior->getPointN(p);
				const gg::Point* p1 = exterior->getPointN(p + 1);

				for (int sub = 0; sub < numSubDiv; ++sub)
				{
					const double progress = 1.0 * sub / (numSubDiv - 1);
					const double x = p0->getX() + (p1->getX() - p0->getX())*progress;
					const double y = p0->getY() + (p1->getY() - p0->getY())*progress;
					newExterior.add(gg::Coordinate(x, y));
				}

				if (p + 2 == exterior->getNumPoints())
				{
					newExterior.add(gg::Coordinate(p1->getX(), p1->getY()));
				}
			}

			std::vector<gg::Geometry*>* holes = new std::vector<gg::Geometry*>;
			for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
			{
				const gg::LineString* hole = polygon->getInteriorRingN(i);

				gg::CoordinateArraySequence newInterior;
				//for (size_t p = 0; p < hole->getNumPoints(); ++p)
				for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p - 1; --p)
				{
					const gg::Point* p0 = hole->getPointN(p);
					const gg::Point* p1 = hole->getPointN(p - 1);

					for (int sub = 0; sub < numSubDiv; ++sub)
					{
						const double progress = 1.0 * sub / (numSubDiv - 1);
						const double x = p0->getX() + (p1->getX() - p0->getX())*progress;
						const double y = p0->getY() + (p1->getY() - p0->getY())*progress;
						newInterior.add(gg::Coordinate(x, y));
					}

					if (p - 2 == 0)
					{
						newExterior.add(gg::Coordinate(p1->getX(), p1->getY()));
					}
				}

				holes->push_back(factory->createLinearRing(newInterior));
			}

			resultGeometries.push_back(factory->createPolygon(factory->createLinearRing(newExterior), holes));
		};

		for (int s = 0; s < lhsPolygon.size(); ++s)
		{
			const gg::Geometry* geometry = lhsPolygon[s];

			if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
			{
				const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);
				subdividePoly(polygon);
			}
			if (geometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
			{
				const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(geometry);

				for (int i = 0; i < polygons->getNumGeometries(); ++i)
				{
					subdividePoly(dynamic_cast<const gg::Polygon*>(polygons->getGeometryN(i)));
				}
			}
		}

		return ShapeResult(GetShapesFromGeosPacked(resultGeometries));
	}

	double ShapeArea(const PackedVal& lhs, std::shared_ptr<Context> pContext)
	{
		if (!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> geometries = GeosFromRecordPacked(lhs, pContext);

		double area = 0.0;
		for (gg::Geometry* geometry : geometries)
		{
			area += geometry->getArea();
		}

		return area;
	}

	double ShapeDistance(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(lhs, pContext);
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecordPacked(rhs, pContext);

		geos::operation::geounion::CascadedUnion unionCalcLhs(&lhsPolygon);
		gg::Geometry* lhsUnion = unionCalcLhs.Union();

		geos::operation::geounion::CascadedUnion unionCalcRhs(&rhsPolygon);
		gg::Geometry* rhsUnion = unionCalcRhs.Union();

		geos::operation::distance::DistanceOp distOp(lhsUnion, rhsUnion);
		const auto result = distOp.distance();

		return result;
	}

	PackedRecord ShapeClosestPoints(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(lhs, pContext);
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecordPacked(rhs, pContext);

		geos::operation::geounion::CascadedUnion unionCalcLhs(&lhsPolygon);
		gg::Geometry* lhsUnion = unionCalcLhs.Union();

		geos::operation::geounion::CascadedUnion unionCalcRhs(&rhsPolygon);
		gg::Geometry* rhsUnion = unionCalcRhs.Union();

		geos::operation::distance::DistanceOp distOp(lhsUnion, rhsUnion);
		const auto result = distOp.closestPoints();
		
		PackedList points;
		for (size_t i = 0; i < result->size(); ++i)
		{
			points.add(MakeRecord("x", result->getX(i), "y", result->getY(i)));
		}

		return ShapeResult(MakeRecord("line", points));
	}

	PackedRecord GetDefaultFontString(const std::string& str)
	{
		if (str.empty() || str.front() == ' ')
		{
			return ShapeResult();
		}

		cgl::FontBuilder builder;
		const auto result = builder.textToPolygon(str, 5);
		return ShapeResult(GetShapesFromGeosPacked(result));
	}

	PackedRecord BuildPath(const PackedList& passes, std::shared_ptr<Context> pContext, int numOfPoints, const PackedList& obstacleList)
	{
		auto factory = gg::GeometryFactory::create();

		std::vector<gg::Point*> originalPoints;
		Vector<Eigen::Vector2d> points;
		{
			if (passes.data.size() < 2)
			{
				CGL_Error("buildPathの第一引数には通る点を二点以上指定する必要があります");
			}

			for (const auto& pointVal : passes.data)
			{
				if (auto posRecordOpt = AsOpt<PackedRecord>(pointVal.value))
				{
					const auto v = ReadVec2Packed(posRecordOpt.get());
					const double x = std::get<0>(v);
					const double y = std::get<1>(v);

					Eigen::Vector2d v2d;
					v2d << x, y;
					points.push_back(v2d);

					originalPoints.push_back(factory->createPoint(gg::Coordinate(x, y)));
				}
				else
				{
					CGL_Error("passesが不正な式です");
				}
			}
		}

		std::vector<double> angles1(numOfPoints / 2);
		std::vector<double> angles2(numOfPoints / 2);

		std::vector<gg::Geometry*> obstacles = GeosFromRecordPacked(obstacleList, pContext);

		const gg::Coordinate beginPos(points.front().x(), points.front().y());
		const gg::Coordinate endPos(points.back().x(), points.back().y());

		points.erase(points.end() - 1);
		points.erase(points.begin());
		originalPoints.erase(originalPoints.end() - 1);
		originalPoints.erase(originalPoints.begin());

		PackedRecord result;
		PackedList polygonList;

		/*
		PathConstraintProblem  constraintProblem;
		constraintProblem.evaluator = [&](const PathConstraintProblem::TVector& v)->double
		{
		gg::CoordinateArraySequence cs2;
		cs2.add(beginPos);
		for (int i = 0; i * 2 < v.size(); ++i)
		{
		cs2.add(gg::Coordinate(v[i * 2 + 0], v[i * 2 + 1]));
		}
		cs2.add(endPos);

		gg::LineString* ls2 = factory->createLineString(cs2);

		double pathLength = ls2->getLength();

		double penalty = 0;
		for (int i = 0; i < originalPoints.size(); ++i)
		{
		god::DistanceOp distanceOp(ls2, originalPoints[i]);
		penalty += distanceOp.distance();
		}

		double penalty2 = 0;
		for (int i = 0; i < obstacles.size(); ++i)
		{
		gg::Geometry* g = obstacles[i]->intersection(ls2);
		if (g->getGeometryTypeId() == gg::GEOS_LINESTRING)
		{
		const gg::LineString* intersections = dynamic_cast<const gg::LineString*>(g);
		penalty2 += intersections->getLength();
		}
		else
		{
		//std::cout << "Result type: " << g->getGeometryType();
		}
		}

		penalty = 100 * penalty*penalty;
		penalty2 = 100 * penalty2*penalty2;

		const double totalCost = pathLength + penalty + penalty2;

		//std::cout << std::string("path cost: ") << ToS(totalCost, 17) << "\n";

		std::cout << std::string("path cost: ") << ToS(pathLength, 10) << ", " << ToS(penalty, 10) << ", " << ToS(penalty2, 10) << " => " << ToS(totalCost, 15) << "\n";
		return totalCost;
		};

		Eigen::VectorXd x0s(points.size() * 2);
		for (int i = 0; i < points.size(); ++i)
		{
		x0s[i * 2 + 0] = points[i].x();
		x0s[i * 2 + 1] = points[i].y();
		}

		cppoptlib::BfgsSolver<PathConstraintProblem> solver;
		solver.minimize(constraintProblem, x0s);

		for (int i = 0; i * 2 < x0s.size(); ++i)
		{
			polygonList.add(MakeVec2(x0s[i * 2 + 0], x0s[i * 2 + 1]));
		}
		//*/

		//*
		libcmaes::FitFunc func = [&](const double *x, const int N)->double
		{
			//std::cout << "A";

			const int halfindex = N / 2;

			const double rodLength = std::abs(x[N - 1]) + 1.0;

			gg::CoordinateArraySequence cs2;
			cs2.add(beginPos);
			for (int i = 0; i < halfindex; ++i)
			{
				const auto& lastPos = cs2.back();
				const double angle = x[i];
				const double dx = rodLength * cos(angle);
				const double dy = rodLength * sin(angle);
				cs2.add(gg::Coordinate(lastPos.x + dx, lastPos.y + dy));
			}
			const auto ik1Last = cs2.back();

			//std::cout << "B";
			Vector<Eigen::Vector2d> cs3;
			cs3.push_back(Eigen::Vector2d(endPos.x, endPos.y));
			for (int i = 0; i < halfindex; ++i)
			{
				const int currentIndex = i + halfindex;
				const auto& lastPos = cs3.back();
				const double angle = x[currentIndex];
				const double dx = rodLength * cos(angle);
				const double dy = rodLength * sin(angle);

				const double lastX = lastPos.x();
				const double lastY = lastPos.y();
				cs3.push_back(Eigen::Vector2d(lastX + dx, lastY + dy));
			}
			const auto ik2Last = cs3.back();

			for (auto it = cs3.rbegin(); it != cs3.rend(); ++it)
			{
				cs2.add(gg::Coordinate(it->x(), it->y()));
			}

			//std::cout << "C";
			gg::LineString* ls2 = factory->createLineString(cs2);

			double pathLength = ls2->getLength();
			const double pathLengthSqrt = sqrt(pathLength);

			const double dx = ik1Last.x - ik2Last.x();
			const double dy = ik1Last.y - ik2Last.y();
			const double distanceSq = dx * dx + dy * dy;
			const double distance = sqrt(distanceSq);

			double penalty = 0;
			for (int i = 0; i < originalPoints.size(); ++i)
			{
				god::DistanceOp distanceOp(ls2, originalPoints[i]);
				penalty += distanceOp.distance();
			}

			//std::cout << "D";
			double penalty2 = 0;
			for (int i = 0; i < obstacles.size(); ++i)
			{
				try
				{
					gg::Geometry* g = obstacles[i]->intersection(ls2);
					//std::cout << "E";
					if (g->getGeometryTypeId() == gg::GEOS_LINESTRING)
					{
						//std::cout << "F";
						const gg::LineString* intersections = dynamic_cast<const gg::LineString*>(g);
						penalty2 += intersections->getLength();
					}
					else if (g->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
					{
						//std::cout << "G";
						const gg::MultiLineString* intersections = dynamic_cast<const gg::MultiLineString*>(g);
						penalty2 += intersections->getLength();
					}
					else
					{
						//std::cout << "H";
					}
				}
				catch (const std::exception& e)
				{
					penalty2 += pathLength;
				}
			}

			penalty2 /= pathLength;
			penalty2 *= 100.0;

			const double penaltySq = penalty * penalty;
			const double penalty2Sq = penalty2 * penalty2;

			//penalty = 100 * penalty*penalty;
			//penalty2 = 100 * penalty2*penalty2;

			const double totalCost = pathLength + distance + penaltySq + penalty2Sq;
			/*std::cout << std::string("cost: ") <<
			ToS(pathLength, 10) << ", " <<
			ToS(distance, 10) << ", " <<
			ToS(penaltySq, 10) << ", " <<
			ToS(penalty2Sq, 10) << " => " << ToS(totalCost, 15) << "\n";*/

			//const double totalCost = penalty2;
			//std::cout << std::string("cost: ") << ToS(totalCost, 10) << "\n";
			//std::cout << "H " << ToS(totalCost, 10) << "\n";

			return totalCost;
		};

		std::vector<double> x0(numOfPoints + 1, 0.0);
		x0.back() = 10;

		const double sigma = 0.1;

		const int lambda = 100;

		/*
		std::vector<double> lbounds(x0.size()), ubounds(x0.size());
		for (size_t i = 0; i < num; ++i)
		{
		lbounds[i] = -3.1415926535*0.5;
		ubounds[i] = 3.1415926535*0.5;
		}
		lbounds.back() = 0.0;
		ubounds.back() = 100.0;

		libcmaes::GenoPheno<libcmaes::pwqBoundStrategy, libcmaes::linScalingStrategy> gp(lbounds.data(), ubounds.data(), x0.size());
		libcmaes::CMAParameters<libcmaes::GenoPheno<libcmaes::pwqBoundStrategy, libcmaes::linScalingStrategy>> cmaparams(x0.size(), x0.data(), sigma, -1, 0, gp);
		//cmaparams.set_algo(aCMAES);
		cmaparams.set_algo(IPOP_CMAES);
		libcmaes::CMASolutions cmasols = libcmaes::cmaes<libcmaes::GenoPheno<libcmaes::pwqBoundStrategy, libcmaes::linScalingStrategy>>(func, cmaparams);
		Eigen::VectorXd bestparameters = gp.pheno(cmasols.get_best_seen_candidate().get_x_dvec());
		auto resultxs = x0;
		for (size_t i = 0; i < resultxs.size(); ++i)
		{
		resultxs[i] = bestparameters[i];
		}
		//*/

		//*
		libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda, 1);
		libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
		auto resultxs = cmasols.best_candidate().get_x();
		//*/

		{
			const double rodLength = resultxs.back();

			const int halfindex = numOfPoints / 2;
			{
				double lastX = beginPos.x;
				double lastY = beginPos.y;

				polygonList.add(MakeVec2(beginPos.x, beginPos.y));
				for (int i = 0; i < halfindex; ++i)
				{
					const double angle = resultxs[i];
					const double dx = rodLength * cos(angle);
					const double dy = rodLength * sin(angle);
					lastX += dx;
					lastY += dy;
					polygonList.add(MakeVec2(lastX, lastY));
				}
			}

			Vector<Eigen::Vector2d> cs3;
			cs3.push_back(Eigen::Vector2d(endPos.x, endPos.y));
			for (int i = 0; i < halfindex; ++i)
			{
				const int currentIndex = i + halfindex;
				const auto& lastPos = cs3.back();
				const double angle = resultxs[currentIndex];
				const double dx = rodLength * cos(angle);
				const double dy = rodLength * sin(angle);

				const double lastX = lastPos.x();
				const double lastY = lastPos.y();
				cs3.push_back(Eigen::Vector2d(lastX + dx, lastY + dy));
			}

			for (auto it = cs3.rbegin(); it != cs3.rend(); ++it)
			{
				polygonList.add(MakeVec2(it->x(), it->y()));
			}
		}

		//*/

		result.add("line", polygonList);
		{
			PackedRecord record;
			record.add("r", 0);
			record.add("g", 255);
			record.add("b", 255);
			result.add("stroke", record);
		}
		result.type = RecordType::RecordTypePath;

		return ShapeResult(result);
	}

	PackedRecord GetBezierPath(const PackedRecord& p0Record, const PackedRecord& n0Record, const PackedRecord& p1Record, const PackedRecord& n1Record, int numOfPoints)
	{
		const auto p0 = ReadVec2Packed(p0Record);
		const auto n0 = ReadVec2Packed(n0Record);
		const auto p1 = ReadVec2Packed(p1Record);
		const auto n1 = ReadVec2Packed(n1Record);

		const Eigen::Vector2d v_p0(std::get<0>(p0), std::get<1>(p0));
		const Eigen::Vector2d v_n0(std::get<0>(n0), std::get<1>(n0));
		const Eigen::Vector2d v_p1(std::get<0>(p1), std::get<1>(p1));
		const Eigen::Vector2d v_n1(std::get<0>(n1), std::get<1>(n1));

		Vector<Eigen::Vector2d> result;
		GetCubicBezier(result, v_p0, v_p0 + v_n0, v_p1 + v_n1, v_p1, numOfPoints, true);

		PackedList polygonList;
		for (size_t i = 0; i < result.size(); ++i)
		{
			polygonList.add(MakeRecord("x", result[i].x(), "y", result[i].y()));
		}

		return ShapeResult(MakeRecord("line", polygonList));
	}

	PackedRecord GetOffsetPath(const PackedRecord& packedPathRecord, double offset)
	{
		Path originalPath = std::move(ReadPathPacked(packedPathRecord));

		return ShapeResult(GetOffsetPathImpl(originalPath, offset));
	}

	PackedRecord GetSubPath(const PackedRecord& packedPathRecord, double offsetBegin, double offsetEnd)
	{
		Path originalPath = std::move(ReadPathPacked(packedPathRecord));

		const int beginCycle = static_cast<int>(std::floor(offsetBegin));
		const int endCycle = static_cast<int>(std::floor(offsetEnd));
		const double beginT = offsetBegin - beginCycle;
		const double endT = offsetEnd - endCycle;

		const auto& cs = originalPath.cs;
		const auto& distances = originalPath.distances;
		const double length = distances.back();

		const auto lerp = [](const gg::Coordinate& p0, const gg::Coordinate& p1, double t)
		{
			return gg::Coordinate(p0.x + (p1.x - p0.x)*t, p0.y + (p1.y - p0.y)*t);
		};

		//t in [0.0, 1.0)
		const auto round = [&](const auto it)
		{
			return (distances.end() == it || distances.end() - 1 == it) ? distances.end() - 2 : it;
		};

		const auto coord = [&](const gg::Coordinate& p)
		{
			return MakeRecord("x", p.x, "y", p.y);
		};

		PackedList points;
		for (int i = beginCycle; i <= endCycle; ++i)
		{
			const double currentBeginT = (i == beginCycle ? beginT : 0.0);
			const double currentEndT = (i == endCycle ? endT : 1.0);

			const double currentOffsetBegin = currentBeginT * length;
			auto itBegin = round(std::upper_bound(distances.begin(), distances.end(), currentOffsetBegin));

			const int lineIndexBegin = std::distance(distances.begin(), itBegin) - 1;
			const double innerTBegin = (currentOffsetBegin - distances[lineIndexBegin]) / (distances[lineIndexBegin + 1] - distances[lineIndexBegin]);

			const double currentOffsetEnd = currentEndT * length;
			auto itEnd = round(std::upper_bound(distances.begin(), distances.end(), currentOffsetEnd));

			const int lineIndexEnd = std::distance(distances.begin(), itEnd) - 1;
			const double innerTEnd = (currentOffsetEnd - distances[lineIndexEnd]) / (distances[lineIndexEnd + 1] - distances[lineIndexEnd]);

			points.add(coord(lerp(cs->getAt(lineIndexBegin), cs->getAt(lineIndexBegin + 1), innerTBegin)));
			for (int line = lineIndexBegin + 1; line < lineIndexEnd; ++line)
			{
				points.add(coord(cs->getAt(line)));
			}
			points.add(coord(lerp(cs->getAt(lineIndexEnd), cs->getAt((lineIndexEnd + 1) % cs->size()), innerTEnd)));
		}

		return ShapeResult(MakeRecord("line", points));
	}

	PackedRecord GetFunctionPath(std::shared_ptr<Context> pContext, const LocationInfo& info, const FuncVal& function, double beginValue, double endValue, int numOfPoints)
	{
		Eval evaluator(pContext);

		PackedRecord pathRecord;
		PackedList polygonList;

		const double unitX = (endValue - beginValue) / (numOfPoints - 1);
		for (int i = 0; i < numOfPoints; ++i)
		{
			const double x = beginValue + unitX * i;
			const Val value = pContext->expand(evaluator.callFunction(info, function, { pContext->makeTemporaryValue(x) }), info);
			if (!IsType<double>(value))
			{
				CGL_Error("Error");
			}
			polygonList.add(MakeVec2(x, As<double>(value)));
		}

		pathRecord.add("line", polygonList);

		return ShapeResult(pathRecord);
	}

	PackedRecord BuildText(const CharString& str, const PackedRecord& packedPathRecord, const CharString& fontPath)
	{
		Path path = packedPathRecord.values.empty() ? Path() : std::move(ReadPathPacked(packedPathRecord));

		auto factory = gg::GeometryFactory::create();

		std::unique_ptr<FontBuilder> pFont;

		if (fontPath.empty())
		{
			pFont = std::make_unique<FontBuilder>();
		}
		else
		{
			const std::string u8FilePath = Unicode::UTF32ToUTF8(fontPath.toString());
			const auto filepath = cgl::filesystem::path(u8FilePath);

			if (filepath.is_absolute())
			{
				const std::string pathStr = filesystem::canonical(filepath).string();
				pFont = std::make_unique<FontBuilder>(pathStr);
			}
			else
			{
				CGL_Error("Fontのパスは絶対パスで指定してください。");
			}
		}
		//FontBuilder font;
		//cgl::FontBuilder font("c:/windows/fonts/font_1_kokumr_1.00_rls.ttf");

		std::u32string string = str.toString();
		double offsetHorizontal = 0;

		PackedList resultCharList;
		
		if(!path.empty())
		{
			for (size_t i = 0; i < string.size(); ++i)
			{
				std::vector<gg::Geometry*> result;
				
				const int codePoint = static_cast<int>(string[i]);
				const double currentGlyphWidth = pFont->glyphWidth(codePoint);

				const auto offsetLeft = path.getOffset(offsetHorizontal);
				const auto offsetCenter = path.getOffset(offsetHorizontal + currentGlyphWidth * 0.5);

				const auto characterPolygon = pFont->makePolygon(codePoint, 5, 0, 0);
				result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());

				offsetHorizontal += currentGlyphWidth;

				/*resultCharList.add(MakeRecord(
					"char", GetPackedShapesFromGeos(result),
					"pos", MakeRecord("x", offsetX, "y", offsetY),
					"angle", angle
				));*/
				resultCharList.add(MakeRecord(
					"char", GetShapesFromGeosPacked(result),
					"pos", MakeRecord("x", offsetLeft.x, "y", offsetLeft.y),
					"angle", offsetCenter.angle
				));
			}
		}
		else
		{
			double offsetVertical = 0;
			for (size_t i = 0; i < string.size(); ++i)
			{
				std::vector<gg::Geometry*> result;

				const int codePoint = static_cast<int>(string[i]);

				if (codePoint == '\r') continue;
				if (codePoint == '\n')
				{
					//offsetVertical += pFont->ascent() - pFont->descent() + pFont->lineGap();
					//offsetVertical += pFont->lineGap()*1.2;
					//offsetVertical += (pFont->ascent() - pFont->descent())*1.2;
					offsetVertical += (pFont->ascent() - pFont->descent())*1.0;
					offsetHorizontal = 0;
					continue;
				}

				const auto characterPolygon = pFont->makePolygon(codePoint, 5, offsetHorizontal, offsetVertical);
				result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());

				offsetHorizontal += pFont->glyphWidth(codePoint);

				resultCharList.add(GetShapesFromGeosPacked(result));
			}
		}

		PackedRecord result;
		result.add("shapes", resultCharList);
		result.add("str", str);
		//result.add("base", WritePathPacked(path));

		return ShapeResult(result);
	}

	PackedRecord GetShapeOuterPaths(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		auto geometries = GeosFromRecordPacked(shape, pContext);
		
		PackedList pathList;
		for (size_t g = 0; g < geometries.size(); ++g)
		{
			const gg::Geometry* geometry = geometries[g];
			if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
			{
				const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);
				const gg::LineString* exterior = polygon->getExteriorRing();

				PackedRecord pathRecord;
				PackedList polygonList;

				if (IsClockWise(exterior))
				{
					for (size_t p = 0; p < exterior->getNumPoints(); ++p)
					{
						const gg::Point* point = exterior->getPointN(p);
						polygonList.add(MakeVec2(point->getX(), point->getY()));
					}
				}
				else
				{
					for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Point* point = exterior->getPointN(p);
						polygonList.add(MakeVec2(point->getX(), point->getY()));
					}
				}

				pathRecord.add("line", polygonList);
				pathList.add(pathRecord);
			}
		}

		return ShapeResult(pathList);
	}

	PackedRecord GetShapeInnerPaths(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		auto geometries = GeosFromRecordPacked(shape, pContext);

		PackedList pathList;
		for (size_t g = 0; g < geometries.size(); ++g)
		{
			const gg::Geometry* geometry = geometries[g];
			if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
			{
				const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);

				for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
				{
					const gg::LineString* exterior = polygon->getInteriorRingN(i);

					PackedRecord pathRecord;
					PackedList polygonList;

					if (IsClockWise(exterior))
					{
						for (size_t p = 0; p < exterior->getNumPoints(); ++p)
						{
							const gg::Point* point = exterior->getPointN(p);
							polygonList.add(MakeVec2(point->getX(), point->getY()));
						}
					}
					else
					{
						for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
						{
							const gg::Point* point = exterior->getPointN(p);
							polygonList.add(MakeVec2(point->getX(), point->getY()));
						}
					}

					pathRecord.add("line", polygonList);
					pathList.add(pathRecord);
				}
			}
		}

		return ShapeResult(pathList);
	}

	PackedRecord GetShapePaths(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		auto geometries = GeosFromRecordPacked(shape, pContext);

		PackedList pathList;

		const auto appendPolygon = [&](const gg::Polygon* polygon)
		{
			{
				const gg::LineString* exterior = polygon->getExteriorRing();

				PackedRecord pathRecord;
				PackedList polygonList;

				if (IsClockWise(exterior))
				{
					for (size_t p = 0; p < exterior->getNumPoints(); ++p)
					{
						const gg::Point* point = exterior->getPointN(p);
						polygonList.add(MakeVec2(point->getX(), point->getY()));
					}
				}
				else
				{
					for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Point* point = exterior->getPointN(p);
						polygonList.add(MakeVec2(point->getX(), point->getY()));
					}
				}

				pathRecord.add("line", polygonList);
				pathList.add(pathRecord);
			}

			for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
			{
				const gg::LineString* hole = polygon->getInteriorRingN(i);

				PackedRecord pathRecord;
				PackedList polygonList;
				
				if (IsClockWise(hole))
				{
					for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Point* point = hole->getPointN(p);
						polygonList.add(MakeVec2(point->getX(), point->getY()));
					}
				}
				else
				{
					for (size_t p = 0; p < hole->getNumPoints(); ++p)
					{
						const gg::Point* point = hole->getPointN(p);
						polygonList.add(MakeVec2(point->getX(), point->getY()));
					}
				}

				pathRecord.add("line", polygonList);
				pathList.add(pathRecord);
			}
		};

		for (size_t g = 0; g < geometries.size(); ++g)
		{
			const gg::Geometry* geometry = geometries[g];
			if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
			{
				const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);
				appendPolygon(polygon);
			}
			else if (geometry->getGeometryTypeId() == gg::GEOS_MULTIPOLYGON)
			{
				const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(geometry);
				for (size_t poly = 0; poly < polygons->getNumGeometries(); ++poly)
				{
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(polygons->getGeometryN(poly));
					appendPolygon(polygon);
				}
			}
		}

		return ShapeResult(pathList);
	}

	PackedRecord GetBoundingBox(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		BoundingRect boundingRect = BoundingRectRecordPacked(shape, pContext);

		if (boundingRect.isEmpty())
		{
			boundingRect.add(Eigen::Vector2d(0, 0));
		}

		const double minX = boundingRect.minPos().x();
		const double minY = boundingRect.minPos().y();
		const double maxX = boundingRect.maxPos().x();
		const double maxY = boundingRect.maxPos().y();

		PackedRecord resultRecord;
		/*resultRecord.adds(
			"min", MakeRecord("x", minX, "y", minY),
			"max", MakeRecord("x", maxX, "y", maxY),
			"top", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY))),
			"bottom", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", maxY), MakeRecord("x", maxX, "y", maxY)))
		);*/
		resultRecord.adds(
			"polygon", MakeList(
				MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY), MakeRecord("x", maxX, "y", maxY), MakeRecord("x", minX, "y", maxY)
			),
			"min", MakeRecord("x", minX, "y", minY),
			"max", MakeRecord("x", maxX, "y", maxY),
			//"topEdge", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY))),
			//"bottomEdge", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", maxY), MakeRecord("x", maxX, "y", maxY))),
			"topEdge", FuncVal({},
				MakeRecordConstructor(
					Identifier("line"), MakeListConstractor(
						MakeRecordConstructor(Identifier("x"), Expr(LRValue(minX)), Identifier("y"), Expr(LRValue(minY))),
						MakeRecordConstructor(Identifier("x"), Expr(LRValue(maxX)), Identifier("y"), Expr(LRValue(minY)))
					)
				)
			),
			"bottomEdge", FuncVal({},
				MakeRecordConstructor(
					Identifier("line"), MakeListConstractor(
						MakeRecordConstructor(Identifier("x"), Expr(LRValue(minX)), Identifier("y"), Expr(LRValue(maxY))),
						MakeRecordConstructor(Identifier("x"), Expr(LRValue(maxX)), Identifier("y"), Expr(LRValue(maxY)))
					)
				)
			)/*,
			"height", FuncVal({},
				Lines({
					BinaryExpr(Identifier("_"), DefFunc(
						BinaryExpr(
							Expr(Accessor(Identifier("polygon")).add(ListAccess(LRValue(2))).add(RecordAccess(Identifier("y")))),
							Expr(Accessor(Identifier("polygon")).add(ListAccess(LRValue(0))).add(RecordAccess(Identifier("y")))),
							BinaryOp::Sub
						)
					), BinaryOp::Assign),
					Accessor(Identifier("_")).add(FunctionAccess())
					}
				)
			),*/
			,
			"height", FuncVal({},
				BinaryExpr(LRValue(maxY), LRValue(minY), BinaryOp::Sub)
			),
			"width", FuncVal({},
				BinaryExpr(LRValue(maxX), LRValue(minX), BinaryOp::Sub)
			),
			"topLeft", MakeRecord("x", minX, "y", minY),
			"topRight", MakeRecord("x", maxX, "y", minY),
			"bottomLeft", MakeRecord("x", minX, "y", maxY),
			"bottomRight", MakeRecord("x", maxX, "y", maxY),
			"center", MakeRecord("x", (minX + maxX)*0.5, "y", (minY + maxY)*0.5),
			"pos", MakeRecord("x", 0, "y", 0),
			"scale", MakeRecord("x", 1.0, "y", 1.0),
			"angle", 0
		);

		//return ShapeResult(resultRecord);
		return resultRecord;
	}

	PackedRecord GetGlobalShape(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(shape, pContext);
		return ShapeResult(GetShapesFromGeosPacked(lhsPolygon));
	}

	PackedRecord GetTransformedShape(const PackedRecord& shape, const PackedRecord& posRecord, const PackedRecord& scaleRecord, double angle, std::shared_ptr<Context> pContext)
	{
		const auto pos = ReadVec2Packed(posRecord);
		const auto scale = ReadVec2Packed(scaleRecord);

		TransformPacked transform(std::get<0>(pos), std::get<1>(pos), std::get<0>(scale), std::get<1>(scale), angle);

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecordPacked(shape, pContext, transform);
		return ShapeResult(GetShapesFromGeosPacked(lhsPolygon));
	}

	PackedRecord GetBaseLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPathRecord, std::shared_ptr<Context> pContext)
	{
		const bool debugDraw = false;
		const BoundingRect originalBoundingRect = BoundingRectRecordPacked(shape, pContext);

		const double eps = std::max(originalBoundingRect.width().x(), originalBoundingRect.width().y())*0.02;
		//const double eps = std::max(originalBoundingRect.width().x(), originalBoundingRect.width().y())*0.05;
		//const double eps = std::max(originalBoundingRect.width().x(), originalBoundingRect.width().y())*0;
		const double minX = originalBoundingRect.minPos().x() - eps;
		const double minY = originalBoundingRect.minPos().y() - eps;
		const double maxX = originalBoundingRect.maxPos().x() + eps;
		const double maxY = originalBoundingRect.maxPos().y() + eps;
		//const double maxY = originalBoundingRect.maxPos().y();

		const BoundingRect boundingRect(minX, minY, maxX, maxY);
		const double aspect = (maxY - minY) / (maxX - minX);

		/*const int xNum = 15;
		const int yNum = static_cast<int>(xNum * aspect + 0.5);
		std::cout << "yNum: " << yNum << "\n";*/
		/*const int xNum = 10;
		const int yNum = 4;*/
		const int xNum = 40;
		const int yNum = 5;

		std::vector<Path> targetPaths;
		targetPaths.push_back(std::move(ReadPathPacked(targetPathRecord)));
		const double maxWidth = targetPaths.front().length();
		const double maxHeight = maxWidth * aspect;

		//std::cout << "yNum: " << yNum << "\n";
		//std::cout << "originalWidth: " << maxX - minX << "\n";
		//std::cout << "originalHeight: " << maxY - minY << "\n";

		//std::cout << "maxWidth: " << maxWidth << "\n";
		//std::cout << "maxHeight: " << maxHeight << "\n";
		
		const double unitY = maxHeight / (yNum - 1);
		for (int i = 1; i < yNum; ++i)
		{
			const double currentHeight = i * unitY;
			const auto offsetPath = GetOffsetPathImpl(targetPaths.front(), -currentHeight);
			targetPaths.push_back(std::move(ReadPathPacked(offsetPath)));
		}

		PackedList packedList;
		if (debugDraw)
		{
			for (const auto& path : targetPaths)
			{
				packedList.add(WritePathPacked(path));
			}
		}

		const auto makeSquare = [](double x, double y)
		{
			return MakeRecord(
				"polygon", MakeList(
					MakeRecord("x", -0.5, "y", -0.5), MakeRecord("x", +0.5, "y", -0.5), MakeRecord("x", +0.5, "y", +0.5), MakeRecord("x", -0.5, "y", +0.5)
				),
				"pos", MakeRecord("x", x, "y", y),
				"scale", MakeRecord("x", 3, "y", 3),
				"fill", MakeRecord("r", 0, "g", 0, "b", 0)
			);
		};

		const auto makeLine = [](double x0, double y0, double x1, double y1)
		{
			return MakeRecord(
				"line", MakeList(
					MakeRecord("x", x0, "y", y0), MakeRecord("x", x1, "y", y1)
				),
				"stroke", MakeRecord("r", 0, "g", 0, "b", 0)
			);
		};
		
		if (debugDraw)
		{
			packedList.add(
				MakeRecord(
					"polygon", MakeList(
						MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY), MakeRecord("x", maxX, "y", maxY), MakeRecord("x", minX, "y", maxY)
					),
					"min", MakeRecord("x", minX, "y", minY),
					"max", MakeRecord("x", maxX, "y", maxY),
					"top", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY))),
					"bottom", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", maxY), MakeRecord("x", maxX, "y", maxY)))
				)
			);
		}

		//Path targetPath = std::move(ReadPathPacked(targetPathRecord));
		//const double height = targetPath.length()*aspect;

		//Path targetPathTop = std::move(ReadPathPacked(GetOffsetPathImpl(targetPath, -height)));

		Deformer deformer;

		std::vector<std::vector<double>> xs;
		std::vector<std::vector<double>> ys;

		xs = std::vector<std::vector<double>>(yNum, std::vector<double>(xNum, 0));
		ys = std::vector<std::vector<double>>(yNum, std::vector<double>(xNum, 0));
		
		//
		for (int y = 0; y < yNum; ++y)
		{
			const auto frontPos = targetPaths[y].cs->front();
			const auto backPos = targetPaths[y].cs->back();

			xs[y].front() = frontPos.x;
			xs[y].back() = backPos.x;
			ys[y].front() = frontPos.y;
			ys[y].back() = backPos.y;

			if (debugDraw)
			{
				packedList.add(makeSquare(frontPos.x, frontPos.y));
				packedList.add(makeSquare(backPos.x, backPos.y));
			}
		}
		
		//法線との交差判定により格子点の位置を求める
		const double dX = 1.0 / (xNum - 1);
		for (int x = 1; x + 1 < xNum; ++x)
		{
			const double currentProgress = x * dX;
			const double currentBaseOffset = targetPaths.front().length()*currentProgress;

			const auto basePos = targetPaths.front().getOffset(currentBaseOffset);
			xs[0][x] = basePos.x;
			ys[0][x] = basePos.y;
			if (debugDraw)
			{
				packedList.add(makeSquare(basePos.x, basePos.y));
			}

			for (int y = 1; y < yNum; ++y)
			{
				const double currentHeight = y * unitY;
				const double currentX = basePos.x + basePos.nx * currentHeight;
				const double currentY = basePos.y + basePos.ny * currentHeight;

				xs[y][x] = currentX;
				ys[y][x] = currentY;
				if (debugDraw)
				{
					packedList.add(makeSquare(currentX, currentY));
					packedList.add(makeLine(basePos.x, basePos.y, currentX, currentY));
				}
			}
		}

		deformer.initialize(boundingRect, xs, ys);
		auto geometries = GeosFromRecordPacked(shape, pContext);
		const auto result = deformer.FFD(geometries);
		//return GetPackedShapesFromGeos(result);
		packedList.add(GetShapesFromGeosPacked(result));

		return ShapeResult(packedList);
	}

	PackedRecord GetCenterLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPathRecord, std::shared_ptr<Context> pContext)
	{
		const bool debugDraw = false;
		const BoundingRect originalBoundingRect = BoundingRectRecordPacked(shape, pContext);

		const double eps = std::max(originalBoundingRect.width().x(), originalBoundingRect.width().y())*0.02;
		//const double eps = std::max(originalBoundingRect.width().x(), originalBoundingRect.width().y())*0.05;
		//const double eps = std::max(originalBoundingRect.width().x(), originalBoundingRect.width().y())*0;
		const double minX = originalBoundingRect.minPos().x() - eps;
		const double minY = originalBoundingRect.minPos().y() - eps;
		const double maxX = originalBoundingRect.maxPos().x() + eps;
		const double maxY = originalBoundingRect.maxPos().y() + eps;
		//const double maxY = originalBoundingRect.maxPos().y();

		const BoundingRect boundingRect(minX, minY, maxX, maxY);
		const double aspect = (maxY - minY) / (maxX - minX);

		/*const int xNum = 15;
		const int yNum = static_cast<int>(xNum * aspect + 0.5);
		std::cout << "yNum: " << yNum << "\n";*/
		/*const int xNum = 10;
		const int yNum = 4;*/
		const int xNum = 40;
		const int yNum = 5;//yNumは奇数（真ん中0, 上と下に(yNum-1)/2ずつ）

		std::vector<Path> targetPaths;
		const Path centerPath(ReadPathPacked(targetPathRecord));

		const double maxWidth = centerPath.length();
		const double maxHeight = maxWidth * aspect;

		//std::cout << "yNum: " << yNum << "\n";
		//std::cout << "originalWidth: " << maxX - minX << "\n";
		//std::cout << "originalHeight: " << maxY - minY << "\n";

		//std::cout << "maxWidth: " << maxWidth << "\n";
		//std::cout << "maxHeight: " << maxHeight << "\n";

		const int centerY = (yNum - 1) / 2;
		const double unitY = maxHeight / (yNum - 1);
		for (int i = 0; i < yNum; ++i)
		{
			const double currentHeight = (i - centerY) * unitY;
			if (i == centerY)
			{
				targetPaths.push_back(centerPath.clone());
				continue;
			}
			const auto offsetPath = GetOffsetPathImpl(centerPath, -currentHeight);
			targetPaths.push_back(ReadPathPacked(offsetPath));
		}

		PackedList packedList;
		if (debugDraw)
		{
			for (const auto& path : targetPaths)
			{
				packedList.add(WritePathPacked(path));
			}
		}

		const auto makeSquare = [](double x, double y)
		{
			return MakeRecord(
				"polygon", MakeList(
					MakeRecord("x", -0.5, "y", -0.5), MakeRecord("x", +0.5, "y", -0.5), MakeRecord("x", +0.5, "y", +0.5), MakeRecord("x", -0.5, "y", +0.5)
				),
				"pos", MakeRecord("x", x, "y", y),
				"scale", MakeRecord("x", 3, "y", 3),
				"fill", MakeRecord("r", 0, "g", 0, "b", 0)
			);
		};

		const auto makeLine = [](double x0, double y0, double x1, double y1)
		{
			return MakeRecord(
				"line", MakeList(
					MakeRecord("x", x0, "y", y0), MakeRecord("x", x1, "y", y1)
				),
				"stroke", MakeRecord("r", 0, "g", 0, "b", 0)
			);
		};

		if (debugDraw)
		{
			packedList.add(
				MakeRecord(
					"polygon", MakeList(
						MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY), MakeRecord("x", maxX, "y", maxY), MakeRecord("x", minX, "y", maxY)
					),
					"min", MakeRecord("x", minX, "y", minY),
					"max", MakeRecord("x", maxX, "y", maxY),
					"top", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY))),
					"bottom", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", maxY), MakeRecord("x", maxX, "y", maxY)))
				)
			);
		}

		Deformer deformer;

		std::vector<std::vector<double>> xs;
		std::vector<std::vector<double>> ys;

		xs = std::vector<std::vector<double>>(yNum, std::vector<double>(xNum, 0));
		ys = std::vector<std::vector<double>>(yNum, std::vector<double>(xNum, 0));

		for (int y = 0; y < yNum; ++y)
		{
			const auto frontPos = targetPaths[y].cs->front();
			const auto backPos = targetPaths[y].cs->back();

			xs[y].front() = frontPos.x;
			xs[y].back() = backPos.x;
			ys[y].front() = frontPos.y;
			ys[y].back() = backPos.y;

			if (debugDraw)
			{
				packedList.add(makeSquare(frontPos.x, frontPos.y));
				packedList.add(makeSquare(backPos.x, backPos.y));
			}
		}

		//法線との交差判定により格子点の位置を求める
		const double dX = 1.0 / (xNum - 1);
		for (int x = 1; x + 1 < xNum; ++x)
		{
			const double currentProgress = x * dX;
			const double currentBaseOffset = centerPath.length()*currentProgress;

			const auto basePos = centerPath.getOffset(currentBaseOffset);
			xs[centerY][x] = basePos.x;
			ys[centerY][x] = basePos.y;
			if (debugDraw)
			{
				packedList.add(makeSquare(basePos.x, basePos.y));
			}

			for (int y = 0; y < yNum; ++y)
			{
				if (y == centerY)continue;

				const double currentHeight = (y - centerY) * unitY;
				const double currentX = basePos.x + basePos.nx * currentHeight;
				const double currentY = basePos.y + basePos.ny * currentHeight;

				xs[y][x] = currentX;
				ys[y][x] = currentY;
				if (debugDraw)
				{
					packedList.add(makeSquare(currentX, currentY));
					packedList.add(makeLine(basePos.x, basePos.y, currentX, currentY));
				}
			}
		}

		deformer.initialize(boundingRect, xs, ys);
		auto geometries = GeosFromRecordPacked(shape, pContext);
		const auto result = deformer.FFD(geometries);
		packedList.add(GetShapesFromGeosPacked(result));

		return ShapeResult(packedList);
	}

	PackedRecord GetDeformedPathShape(const PackedRecord& shape, const PackedRecord& p0Record, const PackedRecord& p1Record, const PackedRecord& targetPath, std::shared_ptr<Context> pContext)
	{
		const auto p0 = ReadVec2Packed(p0Record);
		const auto p1 = ReadVec2Packed(p1Record);

		const double x0 = std::get<0>(p0);
		const double y0 = std::get<1>(p0);
		const double x1 = std::get<0>(p1);
		const double y1 = std::get<1>(p1);

		const double xm = (x0 + x1)*0.5;
		const double ym = (y0 + y1)*0.5;
		const double deg = rad2deg * std::atan2(y1 - y0, x1 - x0);

		auto transformedShape = MakeRecord(
			"s", shape,
			"pos", MakeRecord("x", -xm, "y", -ym),
			"angle", -deg
		);

		return ShapeResult(GetCenterLineDeformedShape(transformedShape, targetPath, pContext));
	}

	PackedRecord ShapeDirectedPoint(const PackedRecord& shape, const Eigen::Vector2d& v, std::shared_ptr<Context> pContext)
	{
		double maxDot = -1.0;
		Eigen::Vector2d maxDotPoint;

		const BoundingRect boundingRect = BoundingRectRecordPacked(shape, pContext);
		const auto center = boundingRect.center();

		std::vector<gg::Geometry*> shapePolygon = GeosFromRecordPacked(shape, pContext);

		/*for (const gg::Geometry* polygon : shapePolygon)
		{
			const gg::CoordinateSequence* points = polygon->getCoordinates();
			for (size_t i = 0; i < points->getSize(); ++i)
			{
				Eigen::Vector2d p;
				p << points->getX(i), points->getY(i);
				const double currentDot = p.dot(v);
				if (maxDot < currentDot)
				{
					maxDot = currentDot;
					maxDotPoint = p;
				}
			}
		}*/

		const double eps = 1.e-3;

		for (const gg::Geometry* polygon : shapePolygon)
		{
			const gg::CoordinateSequence* points = polygon->getCoordinates();

			if (points->isEmpty() || polygon->getGeometryTypeId() == gg::GeometryTypeId::GEOS_POINT)
			{
				continue;
			}

			Eigen::Vector2d prevPoint(points->getX(0), points->getY(0));
			{
				const double currentDot = (prevPoint - center).dot(v);
				if (maxDot < currentDot)
				{
					maxDot = currentDot;
					maxDotPoint = prevPoint;
				}
			}

			const auto update = [&](const Eigen::Vector2d& currentPoint)
			{
				const double currentDot = (currentPoint - center).dot(v);

				//std::string result= ToS(prevPoint) + " -> " + ToS(currentPoint);
				if (maxDot < currentDot + eps)
				{
					//result += ": true";

					//Top(Square{})のように該当頂点が連続して存在する場合はその中点を取る
					//TODO: 共線上に三点以上あるケースの考慮
					if ((currentPoint - prevPoint).dot(v) < eps)
					{
						//result += "(colinear)";
						maxDot = currentDot;
						maxDotPoint = (prevPoint + currentPoint)*0.5;
					}
					else if (maxDot + eps < currentDot)
					{
						maxDot = currentDot;
						maxDotPoint = currentPoint;
					}
				}
				else
				{
					//result += ": false";
				}

				//CGL_DBG1(result);
			};

			for (size_t i = 1; i < points->getSize(); ++i)
			{
				Eigen::Vector2d currentPoint(points->getX(i), points->getY(i));
				update(currentPoint);
				prevPoint = currentPoint;
			}
		}

		return MakeRecord("x", maxDotPoint.x(), "y", maxDotPoint.y());
	}

	PackedRecord ShapeLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(-1, 0), pContext);
	}

	PackedRecord ShapeRight(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(+1, 0), pContext);
	}

	PackedRecord ShapeTop(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(0, -1), pContext);
	}

	PackedRecord ShapeBottom(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(0, +1), pContext);
	}

	PackedRecord ShapeTopLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(-1, -1), pContext);
	}

	PackedRecord ShapeTopRight(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(+1, -1), pContext);
	}

	PackedRecord ShapeBottomLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(-1, +1), pContext);
	}

	PackedRecord ShapeBottomRight(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, Eigen::Vector2d(+1, +1), pContext);
	}
}
