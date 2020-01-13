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

		Geometries FFD(const Geometries& originalShape)
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

			Geometries result;

			const int num = 20;

			//for (const gg::Geometry* geometry : originalShape)
			for (int shapei = 0; shapei < originalShape.size(); ++shapei)
			{
				bool debugPrint = false;
				if (shapei + 18 == originalShape.size())
				{
					//continue;
					debugPrint = true;
				}

				//int pointsCount = 0;
				auto pGeometry = originalShape.refer(shapei);
				//CGL_DBG1(geometry->getGeometryType());
				if (pGeometry->getGeometryTypeId() == gg::GEOS_POLYGON)
				{
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(pGeometry);

					const gg::LineString* exterior = polygon->getExteriorRing();

					gg::CoordinateArraySequence newExterior;
					for (size_t p = 0; p + 1 < exterior->getNumPoints(); ++p)
					{
						const gg::Coordinate& p0 = exterior->getCoordinateN(p);
						const gg::Coordinate& p1 = exterior->getCoordinateN(p + 1);

						for (int sub = 0; sub < num; ++sub)
						{
							const double progress = 1.0 * sub / num;
							const double x = p0.x + (p1.x - p0.x)*progress;
							const double y = p0.y + (p1.y - p0.y)*progress;

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

					Geometries* holes = new Geometries;
					for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
					{
						const gg::LineString* hole = polygon->getInteriorRingN(i);

						gg::CoordinateArraySequence newInterior;
						//for (size_t p = 0; p < hole->getNumPoints(); ++p)
						/*for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p; --p)
						{
							const gg::Coordinate& point = hole->getCoordinateN(p);

							const auto uv = getUV(point->getX(), point->getY());

							const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

							newInterior.add(gg::Coordinate(newPos.x(), newPos.y()));
						}*/
						for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p - 1; --p)
						{
							const gg::Coordinate& p0 = hole->getCoordinateN(p);
							const gg::Coordinate& p1 = hole->getCoordinateN(p - 1);

							for (int sub = 0; sub < num; ++sub)
							{
								const double progress = 1.0 * sub / num;
								const double x = p0.x + (p1.x - p0.x)*progress;
								const double y = p0.y + (p1.y - p0.y)*progress;

								const auto uv = getUV(x, y);
								const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

								newInterior.add(gg::Coordinate(newPos.x(), newPos.y()));
							}
						}

						if (!newInterior.empty())
						{
							newInterior.add(newInterior.front());
						}

						holes->push_back_raw(factory->createLinearRing(newInterior));
						
						/*
						PackedRecord pathRecord;
						PackedList polygonList;

						if (IsClockWise(hole))
						{
							for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p; --p)
							{
								const gg::Coordinate& point = hole->getCoordinateN(p);
								appendCoord(polygonList, point->getX(), point->getY());
							}
						}
						else
						{
							for (size_t p = 0; p < hole->getNumPoints(); ++p)
							{
								const gg::Coordinate& point = hole->getCoordinateN(p);
								appendCoord(polygonList, point->getX(), point->getY());
							}
						}

						pathRecord.add("line", polygonList);
						pathList.add(pathRecord);
						*/
					}

					result.push_back_raw(factory->createPolygon(factory->createLinearRing(newExterior), {}));
				}
				else if (pGeometry->getGeometryTypeId() == gg::GEOS_LINESTRING)
				{
					const gg::LineString* lineString = dynamic_cast<const gg::LineString*>(pGeometry);

					gg::CoordinateArraySequence newPoints;
					for (size_t p = 0; p + 1 < lineString->getNumPoints(); ++p)
					{
						const gg::Coordinate& p0 = lineString->getCoordinateN(p);
						const gg::Coordinate& p1 = lineString->getCoordinateN(p + 1);

						for (int sub = 0; sub < num; ++sub)
						{
							//debugPrint = shapei == 0 && p == 0 && sub == 0;
							const double progress = 1.0 * sub / (num - 1);
							const double x = p0.x + (p1.x - p0.x)*progress;
							const double y = p0.y + (p1.y - p0.y)*progress;

							const auto uv = getUV(x, y);
							const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);
							newPoints.add(gg::Coordinate(newPos.x(), newPos.y()));
						}
					}

					result.push_back_raw(factory->createLineString(newPoints));
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

	PackedList GetPolygon(const Geometries& originalPolygons)
	{
		//ポリゴンが1つ未満
		try
		{
			if (originalPolygons.empty())
			{
				//polygon: []
				return PackedList();
			}
			else if (originalPolygons.size() == 1 && originalPolygons.refer(0)->getGeometryTypeId() == GEOS_POLYGON)
			{
				const auto polygonList = GetPolygonVertices(dynamic_cast<const gg::Polygon*>(originalPolygons.refer(0)));

				//1つのポリゴンのみからなるケース
				if (polygonList.size() == 1)
				{
					//polygon: [p0, p1, ... , pn]
					return polygonList.front();
				}

				//1つのポリゴンと複数の穴からなるケース
				//polygon: [[p00, p01, ... , p0n], [p10, p11, ... , p1n], ... , [pm0, pm1, ... , pmn]]
				return VectorToPackedList(polygonList);
			}
		}
		catch (std::exception& e)
		{
			std::cout << "GetPolygon 1: " << e.what() << std::endl;
			throw;
		}

		//ポリゴンが2つ以上
		std::vector<PackedList> polygons;
		try
		{
			for (size_t geometryIndex = 0; geometryIndex < originalPolygons.size(); ++geometryIndex)
			{
				auto pGeometry = originalPolygons.refer(geometryIndex);
				if (pGeometry->getGeometryTypeId() == GEOS_POLYGON)
				{
					const auto polygonList = GetPolygonVertices(dynamic_cast<const gg::Polygon*>(pGeometry));
					polygons.insert(polygons.end(), polygonList.begin(), polygonList.end());
				}
				else if (pGeometry->getGeometryTypeId() == GEOS_MULTIPOLYGON)
				{
					const gg::MultiPolygon* pMultiPolygon = dynamic_cast<const gg::MultiPolygon*>(pGeometry);
					for (size_t i = 0; i < pMultiPolygon->getNumGeometries(); ++i)
					{
						const auto polygonList = GetPolygonVertices(dynamic_cast<const gg::Polygon*>(pMultiPolygon->getGeometryN(i)));
						polygons.insert(polygons.end(), polygonList.begin(), polygonList.end());
					}
				}
			}
		}
		catch (std::exception& e)
		{
			std::cout << "GetPolygon 2: " << e.what() << std::endl;
			throw;
		}

		try
		{
			//polygon: [[p00, p01, ... , p0n], [p10, p11, ... , p1n], ... , [pm0, pm1, ... , pmn]]
			return VectorToPackedList(polygons);
		}
		catch (std::exception& e)
		{
			std::cout << "GetPolygon 3: " << e.what() << std::endl;
			throw;
		}
	}

	PackedList GetPolygon(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return GetPolygon(GeosFromRecordPacked(shape, pContext));
	}

	PackedList GetLine(const Geometries& originalLines)
	{
		try
		{
			if (originalLines.empty())
			{
				//line: []
				return PackedList();
			}
			//1本の折れ線からなるケース
			else if (originalLines.size() == 1 && originalLines.refer(0)->getGeometryTypeId() == GEOS_LINESTRING)
			{
				return GetLineStringVertices(dynamic_cast<const gg::LineString*>(originalLines.refer(0)));
			}
		}
		catch (std::exception & e)
		{
			std::cout << "GetLine 1: " << e.what() << std::endl;
			throw;
		}

		std::vector<PackedList> lines;
		try
		{
			for (size_t geometryIndex = 0; geometryIndex < originalLines.size(); ++geometryIndex)
			{
				auto pGeometry = originalLines.refer(geometryIndex);
				if (pGeometry->getGeometryTypeId() == GEOS_LINESTRING)
				{
					lines.push_back(GetLineStringVertices(dynamic_cast<const gg::LineString*>(pGeometry)));
				}
				else if (pGeometry->getGeometryTypeId() == GEOS_MULTILINESTRING)
				{
					const gg::MultiLineString* pLineString = dynamic_cast<const gg::MultiLineString*>(pGeometry);
					for (size_t i = 0; i < pLineString->getNumGeometries(); ++i)
					{
						lines.push_back(GetLineStringVertices(dynamic_cast<const gg::LineString*>(pLineString->getGeometryN(i))));
					}
				}
			}
		}
		catch (std::exception & e)
		{
			std::cout << "GetLine 2: " << e.what() << std::endl;
			throw;
		}

		try
		{
			//line: [[p00, p01, ... , p0n], [p10, p11, ... , p1n], ... , [pm0, pm1, ... , pmn]]
			return VectorToPackedList(lines);
		}
		catch (std::exception & e)
		{
			std::cout << "GetLine 3: " << e.what() << std::endl;
			throw;
		}
	}

	PackedRecord MakePolygonResult(const PackedVal& polygon)
	{
		return MakeRecord(
			"polygon", polygon,
			"pos", MakeRecord("x", 0, "y", 0),
			"scale", MakeRecord("x", 1.0, "y", 1.0),
			"angle", 0
		);
	}

	PackedRecord MakePathResult(const PackedVal& path)
	{
		return MakeRecord(
			"line", path,
			"pos", MakeRecord("x", 0, "y", 0),
			"scale", MakeRecord("x", 1.0, "y", 1.0),
			"angle", 0
		);
	}

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

		return MakePathResult(polygonList);
	}

	double ShapeTouch(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		if (lhsPolygon.size() != 1 || rhsPolygon.size() != 1)
		{
			CGL_Error("未対応の形状です");
		}

		const auto toEigen = [](const gg::Coordinate& p)
		{
			return EigenVec2(p.x, p.y);
		};

		const auto dist = [](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1)
		{
			return sqrt((v1 - v0).dot(v1 - v0));
		};

		const auto distSq = [](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1)
		{
			return (v1 - v0).dot(v1 - v0);
		};

		const auto calcHeightSq = [&](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1, const Eigen::Vector2d& crossPoint)
		{
			const auto rel0 = v1 - v0;
			const auto rel0_n = rel0.normalized();
			const auto rel1 = crossPoint - v0;
			const auto foot = rel0_n * rel0_n.dot(rel1);
			return distSq(foot, rel1);
		};

		const auto minimumSubLineLength = [&](const gg::LineString* l1, const gg::LineString* l2)
		{
			GeometryPtr crossPoint = ToUnique<GeometryDeleter>(l1->intersection(l2));
			if (crossPoint->getGeometryTypeId() != gg::GEOS_POINT)
			{
				CGL_Error("不正な式です");
			}

			const gg::Point* point1 = dynamic_cast<const gg::Point*>(crossPoint.get());

			const auto crossV = EigenVec2(point1->getX(), point1->getY());

			double prevDistance1 = 0;
			double postDistance1 = 0;
			double prevDistance2 = 0;
			double postDistance2 = 0;

			{
				bool isPrevCrossPoint = true;
				for (int i = 0; i + 1 < l1->getNumPoints(); ++i)
				{
					const auto v0 = toEigen(l1->getCoordinateN(i));
					const auto v1 = toEigen(l1->getCoordinateN(i + 1));

					if (!isPrevCrossPoint)
					{
						postDistance1 += dist(v0, v1);
					}
					else
					{
						if (calcHeightSq(v0, v1, crossV) < 1.e-4)
						{
							isPrevCrossPoint = false;
							prevDistance1 += dist(v0, crossV);
							postDistance1 += dist(crossV, v1);
						}
						else
						{
							prevDistance1 += dist(v0, v1);
						}
					}
				}
			}
			{
				bool isPrevCrossPoint = true;
				for (int i = 0; i + 1 < l2->getNumPoints(); ++i)
				{
					const auto v0 = toEigen(l2->getCoordinateN(i));
					const auto v1 = toEigen(l2->getCoordinateN(i + 1));

					if (!isPrevCrossPoint)
					{
						postDistance2 += dist(v0, v1);
					}
					else
					{
						if (calcHeightSq(v0, v1, crossV) < 1.e-4)
						{
							isPrevCrossPoint = false;
							prevDistance2 += dist(v0, crossV);
							postDistance2 += dist(crossV, v1);
						}
						else
						{
							prevDistance2 += dist(v0, v1);
						}
					}
				}
			}

			std::vector<double> ds({ prevDistance1,postDistance1,prevDistance2,postDistance2 });
			return *std::min_element(ds.begin(), ds.end());
		};

		const auto touchPointAndPoint = [](const gg::Geometry* point1, const gg::Geometry* point2)->double
		{
			return point1->distance(point2);
		};

		const auto touchPointAndLine = [](const gg::Geometry* point, const gg::Geometry* line)->double
		{
			return point->distance(line);
		};

		const auto touchPointAndPolygon = [](const gg::Geometry* point, const gg::Geometry* polygon)->double
		{
			return point->distance(polygon);
		};

		const auto touchLineAndLine = [&](const gg::Geometry* line1, const gg::Geometry* line2)->double
		{
			const gg::LineString* l1 = dynamic_cast<const gg::LineString*>(line1);
			const gg::LineString* l2 = dynamic_cast<const gg::LineString*>(line2);
			//交点を持たなければ距離を返す
			if (!l1->intersects(l2))
			{
				return l1->distance(l2);
			}
			//交点を持っていれば、それぞれの線分を交点で切断したときの4本の部分線のうち、最も短い部分線の長さを返す
			return minimumSubLineLength(l1, l2);
		};

		const auto touchLineAndPolygon = [](const gg::Geometry* line, const gg::Geometry* polygon)->double
		{
			try
			{
				const gg::LineString* l1 = dynamic_cast<const gg::LineString*>(line);
				const gg::Polygon* p2 = dynamic_cast<const gg::Polygon*>(polygon);
				//交点を持たなければ距離を返す
				if (!l1->intersects(p2))
				{
					return l1->distance(p2);
				}
				gg::Geometry* result = l1->intersection(p2);
				double resultLength = 0;
				if (result->getGeometryTypeId() == gg::GEOS_LINESTRING)
				{
					const gg::LineString* resultLine = dynamic_cast<const gg::LineString*>(result);
					resultLength = resultLine->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
				{
					const gg::MultiLineString* resultLineString = dynamic_cast<const gg::MultiLineString*>(result);
					resultLength = resultLineString->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_POINT)
				{
					resultLength = 0;
				}
				else
				{
					CGL_DBG1(result->getGeometryType());
					CGL_Error("不正な式です");
				}

				delete result;
				return resultLength;
			}
			catch (std::exception& e)
			{
				std::cout << "touchLineAndPolygon: " << e.what() << std::endl;
				throw;
			}
		};

		const auto touchPolygonAndPolygon = [](const gg::Geometry* polygon1, const gg::Geometry* polygon2)->double
		{
			CGL_Error("未対応の形状です");
		};

		enum TouchType { Point = 0, Line = 1, Polygon = 2 };
		const auto getTouchType = [](const gg::Geometry* geometry)
		{
			switch (geometry->getGeometryTypeId())
			{
			case gg::GEOS_POINT: return Point;
			case gg::GEOS_LINESTRING: return Line;
			case gg::GEOS_POLYGON: return Polygon;
			default:
				CGL_Error("未対応の形状です");
			}
		};

		struct TypeGeometry
		{
			TouchType type;
			const gg::Geometry* geometry;
			TypeGeometry(TouchType type, const gg::Geometry* geometry)
				:type(type), geometry(geometry)
			{}
		};

		const TouchType type1 = getTouchType(lhsPolygon.refer(0));
		const TouchType type2 = getTouchType(rhsPolygon.refer(0));

		const TypeGeometry smallerTypePoly = (type1 <= type2 ? TypeGeometry(type1, lhsPolygon.refer(0)) : TypeGeometry(type2, rhsPolygon.refer(0)));
		const TypeGeometry largerTypePoly = (type1 <= type2 ? TypeGeometry(type2, rhsPolygon.refer(0)) : TypeGeometry(type1, lhsPolygon.refer(0)));

		try
		{
			if (smallerTypePoly.type == TouchType::Point)
			{
				switch (largerTypePoly.type)
				{
				case TouchType::Point:
					return touchPointAndPoint(smallerTypePoly.geometry, largerTypePoly.geometry);
				case TouchType::Line:
					return touchPointAndLine(smallerTypePoly.geometry, largerTypePoly.geometry);
				default:
					return touchPointAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
			}
			else if (smallerTypePoly.type == TouchType::Line)
			{
				if (largerTypePoly.type == TouchType::Line)
				{
					return touchLineAndLine(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
				else
				{
					return touchLineAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
			}
			else
			{
				return touchPolygonAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
			}
		}
		catch (std::exception& e)
		{
			std::cout << "Touch End: " << e.what() << std::endl;
			throw;
		}
	}

	double ShapeNear(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		const double margin = 5.0;

		if (lhsPolygon.size() != 1 || rhsPolygon.size() != 1)
		{
			CGL_Error("未対応の形状です");
		}

		const auto toEigen = [](const gg::Coordinate& p)
		{
			return EigenVec2(p.x, p.y);
		};

		const auto dist = [](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1)
		{
			return sqrt((v1 - v0).dot(v1 - v0));
		};

		const auto distSq = [](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1)
		{
			return (v1 - v0).dot(v1 - v0);
		};

		const auto calcHeightSq = [&](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1, const Eigen::Vector2d& crossPoint)
		{
			const auto rel0 = v1 - v0;
			const auto rel0_n = rel0.normalized();
			const auto rel1 = crossPoint - v0;
			const auto foot = rel0_n * rel0_n.dot(rel1);
			return distSq(foot, rel1);
		};

		const auto minimumSubLineLength = [&](const gg::LineString* l1, const gg::LineString* l2)
		{
			GeometryPtr crossPoint = ToUnique<GeometryDeleter>(l1->intersection(l2));
			if (crossPoint->getGeometryTypeId() != gg::GEOS_POINT)
			{
				CGL_Error("不正な式です");
			}

			const gg::Point* point1 = dynamic_cast<const gg::Point*>(crossPoint.get());

			const auto crossV = EigenVec2(point1->getX(), point1->getY());

			double prevDistance1 = 0;
			double postDistance1 = 0;
			double prevDistance2 = 0;
			double postDistance2 = 0;

			{
				bool isPrevCrossPoint = true;
				for (int i = 0; i + 1 < l1->getNumPoints(); ++i)
				{
					const auto v0 = toEigen(l1->getCoordinateN(i));
					const auto v1 = toEigen(l1->getCoordinateN(i + 1));

					if (!isPrevCrossPoint)
					{
						postDistance1 += dist(v0, v1);
					}
					else
					{
						if (calcHeightSq(v0, v1, crossV) < 1.e-4)
						{
							isPrevCrossPoint = false;
							prevDistance1 += dist(v0, crossV);
							postDistance1 += dist(crossV, v1);
						}
						else
						{
							prevDistance1 += dist(v0, v1);
						}
					}
				}
			}
			{
				bool isPrevCrossPoint = true;
				for (int i = 0; i + 1 < l2->getNumPoints(); ++i)
				{
					const auto v0 = toEigen(l2->getCoordinateN(i));
					const auto v1 = toEigen(l2->getCoordinateN(i + 1));

					if (!isPrevCrossPoint)
					{
						postDistance2 += dist(v0, v1);
					}
					else
					{
						if (calcHeightSq(v0, v1, crossV) < 1.e-4)
						{
							isPrevCrossPoint = false;
							prevDistance2 += dist(v0, crossV);
							postDistance2 += dist(crossV, v1);
						}
						else
						{
							prevDistance2 += dist(v0, v1);
						}
					}
				}
			}

			std::vector<double> ds({ prevDistance1,postDistance1,prevDistance2,postDistance2 });
			return *std::min_element(ds.begin(), ds.end());
		};

		const auto nearPointAndPoint = [](const gg::Geometry* point1, const gg::Geometry* point2)->double
		{
			return point1->distance(point2);
		};

		const auto nearPointAndLine = [](const gg::Geometry* point, const gg::Geometry* line)->double
		{
			return point->distance(line);
		};

		const auto nearPointAndPolygon = [&](const gg::Geometry* point, const gg::Geometry* polygon)->double
		{
			return std::max(polygon->distance(point) - margin, 0.0);
		};

		const auto nearLineAndLine = [&](const gg::Geometry* line1, const gg::Geometry* line2)->double
		{
			const gg::LineString* l1 = dynamic_cast<const gg::LineString*>(line1);
			const gg::LineString* l2 = dynamic_cast<const gg::LineString*>(line2);
			//交点を持たなければ距離を返す
			if (!l1->intersects(l2))
			{
				return l1->distance(l2);
			}
			//交点を持っていれば、それぞれの線分を交点で切断したときの4本の部分線のうち、最も短い部分線の長さを返す
			return minimumSubLineLength(l1, l2);
		};

		const auto nearLineAndPolygon = [](const gg::Geometry* line, const gg::Geometry* polygon)->double
		{
			try
			{
				const gg::LineString* l1 = dynamic_cast<const gg::LineString*>(line);
				const gg::Polygon* p2 = dynamic_cast<const gg::Polygon*>(polygon);
				//交点を持たなければ距離を返す
				if (!l1->intersects(p2))
				{
					return l1->distance(p2);
				}
				gg::Geometry* result = l1->intersection(p2);
				double resultLength = 0;
				if (result->getGeometryTypeId() == gg::GEOS_LINESTRING)
				{
					const gg::LineString* resultLine = dynamic_cast<const gg::LineString*>(result);
					resultLength = resultLine->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
				{
					const gg::MultiLineString* resultLineString = dynamic_cast<const gg::MultiLineString*>(result);
					resultLength = resultLineString->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_POINT)
				{
					resultLength = 0;
				}
				else
				{
					CGL_DBG1(result->getGeometryType());
					CGL_Error("不正な式です");
				}

				delete result;
				return resultLength;
			}
			catch (std::exception& e)
			{
				std::cout << "nearLineAndPolygon: " << e.what() << std::endl;
				throw;
			}
		};

		const auto nearPolygonAndPolygon = [&](const gg::Geometry* polygon1, const gg::Geometry* polygon2)->double
		{
			try
			{
				const gg::Polygon* p1 = dynamic_cast<const gg::Polygon*>(polygon1);
				const gg::Polygon* p2 = dynamic_cast<const gg::Polygon*>(polygon2);

				if (!p1->intersects(p2))
				{
					return std::max(p1->distance(p2) - margin, 0.0);
				}
				gg::Geometry* result = p1->intersection(p2);
				double resultLength = 0;
				if (result->getGeometryTypeId() == gg::GEOS_LINESTRING)
				{
					const gg::LineString* resultLine = dynamic_cast<const gg::LineString*>(result);
					resultLength = resultLine->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
				{
					const gg::MultiLineString* resultLineString = dynamic_cast<const gg::MultiLineString*>(result);
					resultLength = resultLineString->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_POINT)
				{
					resultLength = 0;
				}
				else
				{
					CGL_DBG1(result->getGeometryType());
					CGL_Error("不正な式です");
				}

				delete result;
				return resultLength;
			}
			catch (std::exception& e)
			{
				std::cout << "nearLineAndPolygon: " << e.what() << std::endl;
				throw;
			}
		};

		enum GeoType { Point = 0, Line = 1, Polygon = 2 };
		const auto getGeoType = [](const gg::Geometry* geometry)
		{
			switch (geometry->getGeometryTypeId())
			{
			case gg::GEOS_POINT: return Point;
			case gg::GEOS_LINESTRING: return Line;
			case gg::GEOS_POLYGON: return Polygon;
			default:
				CGL_Error("未対応の形状です");
			}
		};

		struct TypeGeometry
		{
			GeoType type;
			const gg::Geometry* geometry;
			TypeGeometry(GeoType type, const gg::Geometry* geometry)
				:type(type), geometry(geometry)
			{}
		};

		const GeoType type1 = getGeoType(lhsPolygon.refer(0));
		const GeoType type2 = getGeoType(rhsPolygon.refer(0));

		const TypeGeometry smallerTypePoly = (type1 <= type2 ? TypeGeometry(type1, lhsPolygon.refer(0)) : TypeGeometry(type2, rhsPolygon.refer(0)));
		const TypeGeometry largerTypePoly = (type1 <= type2 ? TypeGeometry(type2, rhsPolygon.refer(0)) : TypeGeometry(type1, lhsPolygon.refer(0)));

		try
		{
			if (smallerTypePoly.type == GeoType::Point)
			{
				switch (largerTypePoly.type)
				{
				case GeoType::Point:
					return nearPointAndPoint(smallerTypePoly.geometry, largerTypePoly.geometry);
				case GeoType::Line:
					return nearPointAndLine(smallerTypePoly.geometry, largerTypePoly.geometry);
				default:
					return nearPointAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
			}
			else if (smallerTypePoly.type == GeoType::Line)
			{
				if (largerTypePoly.type == GeoType::Line)
				{
					return nearLineAndLine(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
				else
				{
					return nearLineAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
			}
			else
			{
				return nearPolygonAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
			}
		}
		catch (std::exception& e)
		{
			std::cout << "Touch End: " << e.what() << std::endl;
			throw;
		}
	}

	double ShapeAvoid(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		if (lhsPolygon.size() != 1 || rhsPolygon.size() != 1)
		{
			CGL_Error("未対応の形状です");
		}

		const auto toEigen = [](const gg::Coordinate& p)
		{
			return EigenVec2(p.x, p.y);
		};

		const auto dist = [](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1)
		{
			return sqrt((v1 - v0).dot(v1 - v0));
		};

		const auto distSq = [](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1)
		{
			return (v1 - v0).dot(v1 - v0);
		};

		const auto calcHeightSq = [&](const Eigen::Vector2d& v0, const Eigen::Vector2d& v1, const Eigen::Vector2d& crossPoint)
		{
			const auto rel0 = v1 - v0;
			const auto rel0_n = rel0.normalized();
			const auto rel1 = crossPoint - v0;
			const auto foot = rel0_n * rel0_n.dot(rel1);
			return distSq(foot, rel1);
		};

		const auto minimumSubLineLength = [&](const gg::LineString* l1, const gg::LineString* l2)
		{
			GeometryPtr crossPoint = ToUnique<GeometryDeleter>(l1->intersection(l2));
			if (crossPoint->getGeometryTypeId() != gg::GEOS_POINT)
			{
				CGL_Error("不正な式です");
			}

			const gg::Point* point1 = dynamic_cast<const gg::Point*>(crossPoint.get());

			const auto crossV = EigenVec2(point1->getX(), point1->getY());

			double prevDistance1 = 0;
			double postDistance1 = 0;
			double prevDistance2 = 0;
			double postDistance2 = 0;

			{
				bool isPrevCrossPoint = true;
				for (int i = 0; i + 1 < l1->getNumPoints(); ++i)
				{
					const auto v0 = toEigen(l1->getCoordinateN(i));
					const auto v1 = toEigen(l1->getCoordinateN(i + 1));

					if (!isPrevCrossPoint)
					{
						postDistance1 += dist(v0, v1);
					}
					else
					{
						if (calcHeightSq(v0, v1, crossV) < 1.e-4)
						{
							isPrevCrossPoint = false;
							prevDistance1 += dist(v0, crossV);
							postDistance1 += dist(crossV, v1);
						}
						else
						{
							prevDistance1 += dist(v0, v1);
						}
					}
				}
			}
			{
				bool isPrevCrossPoint = true;
				for (int i = 0; i + 1 < l2->getNumPoints(); ++i)
				{
					const auto v0 = toEigen(l2->getCoordinateN(i));
					const auto v1 = toEigen(l2->getCoordinateN(i + 1));

					if (!isPrevCrossPoint)
					{
						postDistance2 += dist(v0, v1);
					}
					else
					{
						if (calcHeightSq(v0, v1, crossV) < 1.e-4)
						{
							isPrevCrossPoint = false;
							prevDistance2 += dist(v0, crossV);
							postDistance2 += dist(crossV, v1);
						}
						else
						{
							prevDistance2 += dist(v0, v1);
						}
					}
				}
			}

			std::vector<double> ds({ prevDistance1,postDistance1,prevDistance2,postDistance2 });
			return *std::min_element(ds.begin(), ds.end());
		};

		const auto nearPointAndPoint = [](const gg::Geometry* point1, const gg::Geometry* point2)->double
		{
			return point1->distance(point2);
		};

		const auto nearPointAndLine = [](const gg::Geometry* point, const gg::Geometry* line)->double
		{
			return point->distance(line);
		};

		const auto nearPointAndPolygon = [](const gg::Geometry* point, const gg::Geometry* polygon)->double
		{
			CGL_Error("未対応の形状です");
		};

		const auto nearLineAndLine = [&](const gg::Geometry* line1, const gg::Geometry* line2)->double
		{
			const gg::LineString* l1 = dynamic_cast<const gg::LineString*>(line1);
			const gg::LineString* l2 = dynamic_cast<const gg::LineString*>(line2);
			//交点を持たなければ距離を返す
			if (!l1->intersects(l2))
			{
				return l1->distance(l2);
			}
			//交点を持っていれば、それぞれの線分を交点で切断したときの4本の部分線のうち、最も短い部分線の長さを返す
			return minimumSubLineLength(l1, l2);
		};

		const auto nearLineAndPolygon = [](const gg::Geometry* line, const gg::Geometry* polygon)->double
		{
			try
			{
				const gg::LineString* l1 = dynamic_cast<const gg::LineString*>(line);
				const gg::Polygon* p2 = dynamic_cast<const gg::Polygon*>(polygon);
				//交点を持たなければ距離を返す
				if (!l1->intersects(p2))
				{
					return l1->distance(p2);
				}
				gg::Geometry* result = l1->intersection(p2);
				double resultLength = 0;
				if (result->getGeometryTypeId() == gg::GEOS_LINESTRING)
				{
					const gg::LineString* resultLine = dynamic_cast<const gg::LineString*>(result);
					resultLength = resultLine->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
				{
					const gg::MultiLineString* resultLineString = dynamic_cast<const gg::MultiLineString*>(result);
					resultLength = resultLineString->getLength();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_POINT)
				{
					resultLength = 0;
				}
				else
				{
					CGL_DBG1(result->getGeometryType());
					CGL_Error("不正な式です");
				}

				delete result;
				return resultLength;
			}
			catch (std::exception& e)
			{
				std::cout << "nearLineAndPolygon: " << e.what() << std::endl;
				throw;
			}
		};

		const auto avoidPolygonAndPolygon = [](const gg::Geometry* polygon1, const gg::Geometry* polygon2)->double
		{
			try
			{
				const gg::Polygon* p1 = dynamic_cast<const gg::Polygon*>(polygon1);
				const gg::Polygon* p2 = dynamic_cast<const gg::Polygon*>(polygon2);

				const double margin = 5.0;

				if (!p1->intersects(p2))
				{
					return 0.0;
				}

				gg::Geometry* result = p1->intersection(p2);
				double cost = 0;
				if (result->getGeometryTypeId() == gg::GEOS_POLYGON)
				{
					const gg::Polygon* resultPoly = dynamic_cast<const gg::Polygon*>(result);
					cost = resultPoly->getArea();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_MULTIPOLYGON)
				{
					const gg::MultiPolygon* resultPoly = dynamic_cast<const gg::MultiPolygon*>(result);
					cost = resultPoly->getArea();
				}
				else if (result->getGeometryTypeId() == gg::GEOS_POINT)
				{
					cost = 0;
				}
				else
				{
					CGL_DBG1(result->getGeometryType());
					CGL_Error("不正な式です");
				}

				delete result;
				return cost;
			}
			catch (std::exception& e)
			{
				std::cout << "nearLineAndPolygon: " << e.what() << std::endl;
				throw;
			}
		};

		enum GeoType { Point = 0, Line = 1, Polygon = 2 };
		const auto getGeoType = [](const gg::Geometry* geometry)
		{
			switch (geometry->getGeometryTypeId())
			{
			case gg::GEOS_POINT: return Point;
			case gg::GEOS_LINESTRING: return Line;
			case gg::GEOS_POLYGON: return Polygon;
			default:
				CGL_Error("未対応の形状です");
			}
		};

		struct TypeGeometry
		{
			GeoType type;
			const gg::Geometry* geometry;
			TypeGeometry(GeoType type, const gg::Geometry* geometry)
				:type(type), geometry(geometry)
			{}
		};

		const GeoType type1 = getGeoType(lhsPolygon.refer(0));
		const GeoType type2 = getGeoType(rhsPolygon.refer(0));

		const TypeGeometry smallerTypePoly = (type1 <= type2 ? TypeGeometry(type1, lhsPolygon.refer(0)) : TypeGeometry(type2, rhsPolygon.refer(0)));
		const TypeGeometry largerTypePoly = (type1 <= type2 ? TypeGeometry(type2, rhsPolygon.refer(0)) : TypeGeometry(type1, lhsPolygon.refer(0)));

		try
		{
			if (smallerTypePoly.type == GeoType::Point)
			{
				switch (largerTypePoly.type)
				{
				case GeoType::Point:
					return nearPointAndPoint(smallerTypePoly.geometry, largerTypePoly.geometry);
				case GeoType::Line:
					return nearPointAndLine(smallerTypePoly.geometry, largerTypePoly.geometry);
				default:
					return nearPointAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
			}
			else if (smallerTypePoly.type == GeoType::Line)
			{
				if (largerTypePoly.type == GeoType::Line)
				{
					return nearLineAndLine(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
				else
				{
					return nearLineAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
				}
			}
			else
			{
				return avoidPolygonAndPolygon(smallerTypePoly.geometry, largerTypePoly.geometry);
			}
		}
		catch (std::exception& e)
		{
			std::cout << "Touch End: " << e.what() << std::endl;
			throw;
		}
	}

	PackedRecord ShapeDiff(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		auto factory = gg::GeometryFactory::create();

		if (lhsPolygon.empty())
		{
			return PackedRecord();
		}
		else if (rhsPolygon.empty())
		{
			return MakePolygonResult(GetPolygon(lhsPolygon));
		}
		else
		{
			for (int s = 0; s < lhsPolygon.size();)
			{
				GeometryPtr pErodeGeometry(lhsPolygon.takeOut(s));

				for (int d = 0; d < rhsPolygon.size(); ++d)
				{
					GeometryPtr pTemporaryGeometry(ToUnique<GeometryDeleter>(pErodeGeometry->difference(rhsPolygon.refer(d))));

					if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
					{
						pErodeGeometry = std::move(pTemporaryGeometry);
						continue;
						//->次のpErodeGeometryには現在のポリゴンがそのまま入っている
					}
					else if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_LINESTRING)
					{
						pErodeGeometry = std::move(pTemporaryGeometry);
						continue;
						//->次のpErodeGeometryには現在のポリゴンがそのまま入っている
					}
					else if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(pTemporaryGeometry.get());
						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							lhsPolygon.insert(s, ToUnique<GeometryDeleter>(polygons->getGeometryN(i)->clone()));
						}
						pErodeGeometry = lhsPolygon.takeOut(s);

						//currentPolygonsに挿入したのはcloneなのでdifferenceの結果はここで削除してよい
						//->次のpErodeGeometryには分割された最初のポリゴンが入っている
					}
					else if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTILINESTRING)
					{
						const gg::MultiLineString* lines = dynamic_cast<const gg::MultiLineString*>(pTemporaryGeometry.get());
						for (int i = 0; i < lines->getNumGeometries(); ++i)
						{
							lhsPolygon.insert(s, ToUnique<GeometryDeleter>(lines->getGeometryN(i)->clone()));
						}
						pErodeGeometry = lhsPolygon.takeOut(s);

						//currentPolygonsに挿入したのはcloneなのでdifferenceの結果はここで削除してよい
						//->次のpErodeGeometryには分割された最初のポリゴンが入っている
					}
					else if (pTemporaryGeometry->getNumGeometries() == 0 && pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION)
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
					lhsPolygon.insert(s, std::move(pErodeGeometry));

					//最初のcurrentPolygons.takeOutで要素が減っているので、ここで復活したときのみsを増やす
					++s;
				}
			}

			const auto resultPolygons = GetPolygon(lhsPolygon);
			const auto resultLines = GetLine(lhsPolygon);

			//両方ある
			if (!resultPolygons.data.empty() && !resultLines.data.empty())
			{
				return MakeRecord(
					"polygon", resultPolygons,
					"line", resultLines,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				);
			}
			//ポリゴンのみ
			else if (!resultPolygons.data.empty())
			{
				return MakePolygonResult(resultPolygons);
			}
			//線分のみ
			else if (!resultLines.data.empty())
			{
				return MakePathResult(resultLines);
			}

			return MakeRecord(
				"pos", MakeRecord("x", 0, "y", 0),
				"scale", MakeRecord("x", 1.0, "y", 1.0),
				"angle", 0
			);
		}
	}

	PackedRecord ShapeUnion(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		{
			Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));
			lhsPolygon.append(std::move(rhsPolygon));
		}

		std::vector<gg::Geometry*> ptrs = lhsPolygon.releaseAsRawPtrs();
		geos::operation::geounion::CascadedUnion unionCalc(&ptrs);

		Geometries result;
		result.push_back_raw(unionCalc.Union());

		while (!ptrs.empty())
		{
			delete ptrs.back();
			ptrs.pop_back();
		}

		return MakePolygonResult(GetPolygon(result));
	}

	PackedRecord ShapeIntersect(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		auto factory = gg::GeometryFactory::create();

		if (lhsPolygon.empty() || rhsPolygon.empty())
		{
			return PackedRecord();
		}
		else
		{
			Geometries resultGeometries;

			for (int s = 0; s < lhsPolygon.size();)
			{
				GeometryPtr pErodeGeometry(lhsPolygon.takeOut(s));

				for (int d = 0; d < rhsPolygon.size(); ++d)
				{
					GeometryPtr pTemporaryGeometry(ToUnique<GeometryDeleter>(pErodeGeometry->intersection(rhsPolygon.refer(d))));

					if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
					{
						pErodeGeometry = std::move(pTemporaryGeometry);
						continue;
					}
					else if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(pTemporaryGeometry.get());
						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							lhsPolygon.insert(s, ToUnique<GeometryDeleter>(polygons->getGeometryN(i)->clone()));
						}
						pErodeGeometry = lhsPolygon.takeOut(s);
					}
					else if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION)
					{
						pErodeGeometry.reset();
						break;
					}
					else if (pTemporaryGeometry->getGeometryTypeId() == geos::geom::GEOS_LINESTRING)
					{
						pErodeGeometry = std::move(pTemporaryGeometry);
						continue;
					}
					else
					{
						CGL_Error(std::string("Intersectの結果が予期せぬデータ形式: \"") + GetGeometryType(pTemporaryGeometry.get()) + "\"");
					}
				}

				if (pErodeGeometry)
				{
					lhsPolygon.insert(s, std::move(pErodeGeometry));
					++s;
				}
			}

			return MakePolygonResult(GetPolygon(lhsPolygon));
		}
	}

	PackedRecord ShapeSymDiff(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		std::vector<gg::Geometry*> lhsPtrs = lhsPolygon.releaseAsRawPtrs();
		geos::operation::geounion::CascadedUnion unionCalcLhs(&lhsPtrs);
		auto lhsResult = ToUnique<GeometryDeleter>(unionCalcLhs.Union());

		std::vector<gg::Geometry*> rhsPtrs = rhsPolygon.releaseAsRawPtrs();
		geos::operation::geounion::CascadedUnion unionCalcRhs(&rhsPtrs);
		auto rhsResult = ToUnique<GeometryDeleter>(unionCalcRhs.Union());

		Geometries result;
		result.push_back_raw(lhsResult->symDifference(rhsResult.get()));

		while (!lhsPtrs.empty())
		{
			delete lhsPtrs.back();
			lhsPtrs.pop_back();
		}

		while (!rhsPtrs.empty())
		{
			delete rhsPtrs.back();
			rhsPtrs.pop_back();
		}

		return MakePolygonResult(GetPolygon({ result }));
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
		Geometries polygons(GeosFromRecordPacked(shape, pContext));

		Geometries resultGeometries;
		for (int s = 0; s < polygons.size(); ++s)
		{
			auto pCurrentGeometry = polygons.refer(s);
			resultGeometries.push_back_raw(pCurrentGeometry->buffer(distance));
		}

		return MakePolygonResult(GetPolygon(resultGeometries));
	}

	PackedRecord ShapeSubDiv(const PackedVal& shape, int numSubDiv, std::shared_ptr<Context> pContext)
	{
		if (!IsType<PackedRecord>(shape))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(shape, pContext));
		Geometries resultGeometries;

		auto factory = gg::GeometryFactory::create();

		const auto subdividePoly = [&](const gg::Polygon* polygon)
		{
			const gg::LineString* exterior = polygon->getExteriorRing();

			gg::CoordinateArraySequence newExterior;
			for (size_t p = 0; p + 1 < exterior->getNumPoints(); ++p)
			{
				const gg::Coordinate& p0 = exterior->getCoordinateN(p);
				const gg::Coordinate& p1 = exterior->getCoordinateN(p + 1);

				for (int sub = 0; sub < numSubDiv; ++sub)
				{
					const double progress = 1.0 * sub / (numSubDiv - 1);
					const double x = p0.x + (p1.x - p0.x)*progress;
					const double y = p0.y + (p1.y - p0.y)*progress;
					newExterior.add(gg::Coordinate(x, y));
				}

				if (p + 2 == exterior->getNumPoints())
				{
					newExterior.add(gg::Coordinate(p1.x, p1.y));
				}
			}

			Geometries holes;
			for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
			{
				const gg::LineString* hole = polygon->getInteriorRingN(i);

				gg::CoordinateArraySequence newInterior;
				for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p - 1; --p)
				{
					const gg::Coordinate& p0 = hole->getCoordinateN(p);
					const gg::Coordinate& p1 = hole->getCoordinateN(p - 1);

					for (int sub = 0; sub < numSubDiv; ++sub)
					{
						const double progress = 1.0 * sub / (numSubDiv - 1);
						const double x = p0.x + (p1.x - p0.x)*progress;
						const double y = p0.y + (p1.y - p0.y)*progress;
						newInterior.add(gg::Coordinate(x, y));
					}

					if (p - 2 == 0)
					{
						newExterior.add(gg::Coordinate(p1.x, p1.y));
					}
				}

				holes.push_back_raw(factory->createLinearRing(newInterior));
			}
			auto holePtrs = holes.releaseAsRawPtrs();

			resultGeometries.push_back_raw(factory->createPolygon(factory->createLinearRing(newExterior), &holePtrs));
		};

		for (int s = 0; s < lhsPolygon.size(); ++s)
		{
			const gg::Geometry* geometry = lhsPolygon.refer(s);

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

		return MakePolygonResult(GetPolygon(resultGeometries));
	}

	double ShapeArea(const PackedVal& lhs, std::shared_ptr<Context> pContext)
	{
		if (!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs))
		{
			CGL_Error("不正な式です");
		}

		Geometries geometries(GeosFromRecordPacked(lhs, pContext));

		double area = 0.0;
		for (size_t i = 0; i < geometries.size(); ++i)
		{
			area += geometries.refer(i)->getArea();
		}

		return area;
	}

	double ShapeDistance(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		std::vector<gg::Geometry*> lhsPtrs = lhsPolygon.releaseAsRawPtrs();
		geos::operation::geounion::CascadedUnion unionCalcLhs(&lhsPtrs);
		auto lhsResult = ToUnique<GeometryDeleter>(unionCalcLhs.Union());

		std::vector<gg::Geometry*> rhsPtrs = rhsPolygon.releaseAsRawPtrs();
		geos::operation::geounion::CascadedUnion unionCalcRhs(&rhsPtrs);
		auto rhsResult = ToUnique<GeometryDeleter>(unionCalcRhs.Union());

		geos::operation::distance::DistanceOp distOp(lhsResult.get(), rhsResult.get());
		const double result = distOp.distance();

		while (!lhsPtrs.empty())
		{
			delete lhsPtrs.back();
			lhsPtrs.pop_back();
		}

		while (!rhsPtrs.empty())
		{
			delete rhsPtrs.back();
			rhsPtrs.pop_back();
		}

		return result;
	}

	PackedRecord ShapeClosestPoints(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext)
	{
		if ((!IsType<PackedRecord>(lhs) && !IsType<PackedList>(lhs)) || (!IsType<PackedRecord>(rhs) && !IsType<PackedList>(rhs)))
		{
			CGL_Error("不正な式です");
		}

		Geometries lhsPolygon(GeosFromRecordPacked(lhs, pContext));
		Geometries rhsPolygon(GeosFromRecordPacked(rhs, pContext));

		std::vector<gg::Geometry*> lhsPtrs = lhsPolygon.releaseAsRawPtrs();
		geos::operation::geounion::CascadedUnion unionCalcLhs(&lhsPtrs);
		auto lhsResult = ToUnique<GeometryDeleter>(unionCalcLhs.Union());

		std::vector<gg::Geometry*> rhsPtrs = rhsPolygon.releaseAsRawPtrs();
		geos::operation::geounion::CascadedUnion unionCalcRhs(&rhsPtrs);
		auto rhsResult = ToUnique<GeometryDeleter>(unionCalcRhs.Union());

		geos::operation::distance::DistanceOp distOp(lhsResult.get(), rhsResult.get());
		gg::CoordinateSequence* result = distOp.closestPoints();
		
		PackedList points;
		for (size_t i = 0; i < result->size(); ++i)
		{
			points.add(MakeRecord("x", result->getX(i), "y", result->getY(i)));
		}

		while (!lhsPtrs.empty())
		{
			delete lhsPtrs.back();
			lhsPtrs.pop_back();
		}

		while (!rhsPtrs.empty())
		{
			delete rhsPtrs.back();
			rhsPtrs.pop_back();
		}

		delete result;

		return MakePathResult(points);
	}

	PackedRecord GetDefaultFontString(const std::string& str)
	{
		if (str.empty() || str.front() == ' ')
		{
			return PackedRecord();
		}

		cgl::FontBuilder builder;
		const auto result = builder.textToPolygon(str, 5);
		//return MakePolygonResult(GetPolygon(result));

		CGL_Error("TODO:未対応");
	}

	PackedRecord BuildPath(const PackedList& passes, std::shared_ptr<Context> pContext, int numOfPoints, const PackedList& obstacleList)
	{
		auto factory = gg::GeometryFactory::create();

		//std::vector<gg::Coordinate> originalPoints;
		Geometries originalPoints;
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

					originalPoints.push_back_raw(factory->createPoint(gg::Coordinate(x, y)));
				}
				else
				{
					CGL_Error("passesが不正な式です");
				}
			}
		}

		std::vector<double> angles1(numOfPoints / 2);
		std::vector<double> angles2(numOfPoints / 2);

		Geometries obstacles(GeosFromRecordPacked(obstacleList, pContext));

		const gg::Coordinate beginPos(points.front().x(), points.front().y());
		const gg::Coordinate endPos(points.back().x(), points.back().y());

		points.erase(points.end() - 1);
		points.erase(points.begin());
		originalPoints.erase(originalPoints.size() - 1);
		originalPoints.erase(0);

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

			Vector<Eigen::Vector2d> cs3;
			cs3.push_back(EigenVec2(endPos.x, endPos.y));
			for (int i = 0; i < halfindex; ++i)
			{
				const int currentIndex = i + halfindex;
				const auto& lastPos = cs3.back();
				const double angle = x[currentIndex];
				const double dx = rodLength * cos(angle);
				const double dy = rodLength * sin(angle);

				const double lastX = lastPos.x();
				const double lastY = lastPos.y();
				cs3.push_back(EigenVec2(lastX + dx, lastY + dy));
			}
			const auto ik2Last = cs3.back();

			for (auto it = cs3.rbegin(); it != cs3.rend(); ++it)
			{
				cs2.add(gg::Coordinate(it->x(), it->y()));
			}

			auto ls2 = ToUnique<GeometryDeleter>(factory->createLineString(cs2));

			double pathLength = ls2->getLength();
			const double pathLengthSqrt = sqrt(pathLength);

			const double dx = ik1Last.x - ik2Last.x();
			const double dy = ik1Last.y - ik2Last.y();
			const double distanceSq = dx * dx + dy * dy;
			const double distance = sqrt(distanceSq);

			double penalty = 0;
			for (int i = 0; i < originalPoints.size(); ++i)
			{
				god::DistanceOp distanceOp(ls2.get(), originalPoints.refer(i));
				penalty += distanceOp.distance();
			}

			double penalty2 = 0;
			for (int i = 0; i < obstacles.size(); ++i)
			{
				try
				{
					auto g = ToUnique<GeometryDeleter>(obstacles.refer(i)->intersection(ls2.get()));
					if (g->getGeometryTypeId() == gg::GEOS_LINESTRING)
					{
						const gg::LineString* intersections = dynamic_cast<const gg::LineString*>(g.get());
						penalty2 += intersections->getLength();
					}
					else if (g->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
					{
						const gg::MultiLineString* intersections = dynamic_cast<const gg::MultiLineString*>(g.get());
						penalty2 += intersections->getLength();
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
			cs3.push_back(EigenVec2(endPos.x, endPos.y));
			for (int i = 0; i < halfindex; ++i)
			{
				const int currentIndex = i + halfindex;
				const auto& lastPos = cs3.back();
				const double angle = resultxs[currentIndex];
				const double dx = rodLength * cos(angle);
				const double dy = rodLength * sin(angle);

				const double lastX = lastPos.x();
				const double lastY = lastPos.y();
				cs3.push_back(EigenVec2(lastX + dx, lastY + dy));
			}

			for (auto it = cs3.rbegin(); it != cs3.rend(); ++it)
			{
				polygonList.add(MakeVec2(it->x(), it->y()));
			}
		}

		//*/

		/*result.add("line", polygonList);
		{
			PackedRecord record;
			record.add("r", 0);
			record.add("g", 255);
			record.add("b", 255);
			result.add("stroke", record);
		}
		result.type = RecordType::RecordTypePath;*/

		return MakePathResult(polygonList);
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
		GetCubicBezier(result, v_p0, v_p0 + v_n0, v_p1 - v_n1, v_p1, numOfPoints, true);

		PackedList polygonList;
		for (size_t i = 0; i < result.size(); ++i)
		{
			polygonList.add(MakeRecord("x", result[i].x(), "y", result[i].y()));
		}

		return MakePathResult(polygonList);
	}

	PackedRecord GetCubicBezier(const PackedRecord& p0Record, const PackedRecord& p1Record, const PackedRecord& p2Record, const PackedRecord& p3Record, int numOfPoints)
	{
		const auto p0 = ReadVec2Packed(p0Record);
		const auto p1 = ReadVec2Packed(p1Record);
		const auto p2 = ReadVec2Packed(p2Record);
		const auto p3 = ReadVec2Packed(p3Record);

		const Eigen::Vector2d v_p0(std::get<0>(p0), std::get<1>(p0));
		const Eigen::Vector2d v_p1(std::get<0>(p1), std::get<1>(p1));
		const Eigen::Vector2d v_p2(std::get<0>(p2), std::get<1>(p2));
		const Eigen::Vector2d v_p3(std::get<0>(p3), std::get<1>(p3));

		Vector<Eigen::Vector2d> result;
		GetCubicBezier(result, v_p0, v_p1, v_p2, v_p3, numOfPoints, true);

		PackedList polygonList;
		for (size_t i = 0; i < result.size(); ++i)
		{
			polygonList.add(MakeRecord("x", result[i].x(), "y", result[i].y()));
		}

		return MakePathResult(polygonList);
	}

	PackedRecord GetOffsetPath(const PackedRecord& packedPathRecord, double offset)
	{
		Path originalPath = std::move(ReadPathPacked(packedPathRecord));

		return GetOffsetPathImpl(originalPath, offset);
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

		return MakePathResult(points);
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

		return MakePathResult(polygonList);
	}

	PackedRecord BuildText(const CharString& str, double textHeight, const PackedRecord& packedPathRecord, const CharString& fontPath)
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
#ifdef USE_IMPORT
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
#else
			CGL_Error("Filesystem is disabled.");
#endif
		}

		std::u32string string = str.toString();
		double offsetHorizontal = 0;

		PackedList resultCharList;

		pFont->setScaledHeight(textHeight);

		if(!path.empty())
		{
			for (size_t i = 0; i < string.size(); ++i)
			{
				Geometries result;
				
				const int codePoint = static_cast<int>(string[i]);
				const double currentGlyphWidth = pFont->glyphWidth(codePoint);

				const auto offsetLeft = path.getOffset(offsetHorizontal);
				const auto offsetCenter = path.getOffset(offsetHorizontal + currentGlyphWidth * 0.5);

				{
					Geometries characterPolygon = pFont->makePolygon(codePoint, 5, 0, 0);
					result.append(std::move(characterPolygon));
				}

				offsetHorizontal += currentGlyphWidth;

				resultCharList.add(MakeRecord(
					"polygon", GetPolygon(result),
					"pos", MakeRecord("x", offsetLeft.x, "y", offsetLeft.y),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", offsetCenter.angle
				));
			}
		}
		else
		{
			double offsetVertical = 0;
			for (size_t i = 0; i < string.size(); ++i)
			{
				Geometries result;

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
				if (codePoint == '\t')
				{
					offsetHorizontal += pFont->glyphWidth('_') * 4;
					continue;
				}
				if (codePoint == ' ')
				{
					offsetHorizontal += pFont->glyphWidth('_');
					continue;
				}

				{
					Geometries characterPolygon = pFont->makePolygon(codePoint, 5, offsetHorizontal, offsetVertical);
					result.append(std::move(characterPolygon));
				}

				offsetHorizontal += pFont->glyphWidth(codePoint);

				resultCharList.add(MakeRecord(
					"polygon", GetPolygon(result),
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
			}
		}

		return MakeRecord(
			"polygon", PackedList(),
			"text", resultCharList,
			"str", str,
			"pos", MakeRecord("x", 0, "y", 0),
			"scale", MakeRecord("x", 1.0, "y", 1.0),
			"angle", 0
		);
	}

	PackedRecord GetShapeOuterPaths(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		Geometries geometries(GeosFromRecordPacked(shape, pContext));

		std::vector<PackedList> pathList;
		for (size_t g = 0; g < geometries.size(); ++g)
		{
			const gg::Geometry* geometry = geometries.refer(g);
			if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
			{
				const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);
				const gg::LineString* exterior = polygon->getExteriorRing();

				PackedList polygonList;

				if (IsClockWise(exterior))
				{
					for (size_t p = 0; p < exterior->getNumPoints(); ++p)
					{
						const gg::Coordinate& point = exterior->getCoordinateN(p);
						polygonList.add(MakeVec2(point.x, point.y));
					}
				}
				else
				{
					for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Coordinate& point = exterior->getCoordinateN(p);
						polygonList.add(MakeVec2(point.x, point.y));
					}
				}

				pathList.push_back(polygonList);
			}
		}

		return MakePathResult(VectorToPackedList(pathList));
	}

	PackedRecord GetShapeInnerPaths(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		Geometries geometries(GeosFromRecordPacked(shape, pContext));

		std::vector<PackedList> pathList;
		for (size_t g = 0; g < geometries.size(); ++g)
		{
			const gg::Geometry* geometry = geometries.refer(g);
			if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
			{
				const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);

				for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
				{
					const gg::LineString* exterior = polygon->getInteriorRingN(i);

					PackedList polygonList;

					if (IsClockWise(exterior))
					{
						for (size_t p = 0; p < exterior->getNumPoints(); ++p)
						{
							const gg::Coordinate& point = exterior->getCoordinateN(p);
							polygonList.add(MakeVec2(point.x, point.y));
						}
					}
					else
					{
						for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
						{
							const gg::Coordinate& point = exterior->getCoordinateN(p);
							polygonList.add(MakeVec2(point.x, point.y));
						}
					}

					pathList.push_back(polygonList);
				}
			}
		}

		return MakePathResult(VectorToPackedList(pathList));
	}

	PackedRecord GetShapePaths(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		Geometries geometries(GeosFromRecordPacked(shape, pContext));

		std::vector<PackedList> pathList;

		const auto appendPolygon = [&](const gg::Polygon* polygon)
		{
			{
				const gg::LineString* exterior = polygon->getExteriorRing();

				PackedList polygonList;

				if (IsClockWise(exterior))
				{
					for (size_t p = 0; p < exterior->getNumPoints(); ++p)
					{
						const gg::Coordinate& point = exterior->getCoordinateN(p);
						polygonList.add(MakeVec2(point.x, point.y));
					}
				}
				else
				{
					for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Coordinate& point = exterior->getCoordinateN(p);
						polygonList.add(MakeVec2(point.x, point.y));
					}
				}

				pathList.push_back(polygonList);
			}

			for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
			{
				const gg::LineString* hole = polygon->getInteriorRingN(i);

				PackedList polygonList;
				
				if (IsClockWise(hole))
				{
					for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Coordinate& point = hole->getCoordinateN(p);
						polygonList.add(MakeVec2(point.x, point.y));
					}
				}
				else
				{
					for (size_t p = 0; p < hole->getNumPoints(); ++p)
					{
						const gg::Coordinate& point = hole->getCoordinateN(p);
						polygonList.add(MakeVec2(point.x, point.y));
					}
				}

				pathList.push_back(polygonList);
			}
		};

		for (size_t g = 0; g < geometries.size(); ++g)
		{
			const gg::Geometry* geometry = geometries.refer(g);
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

		return MakePathResult(VectorToPackedList(pathList));
	}

	PackedRecord GetBoundingBox(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		BoundingRect boundingRect = BoundingRectRecordPacked(shape, pContext);

		if (boundingRect.isEmpty())
		{
			boundingRect.add(EigenVec2(0, 0));
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

		return resultRecord;
	}

#ifdef commentout
	PackedRecord GetMargin(const PackedRecord& lhs, const PackedRecord& rhs, const PackedRecord& directionRecord, std::shared_ptr<Context> pContext)
	{
		//const Geometries polygons1(GeosFromRecordPacked(lhs, pContext));
		//const Geometries polygons2(GeosFromRecordPacked(rhs, pContext));
		const std::array<Geometries, 2> bothPolygons({ GeosFromRecordPacked(lhs, pContext), GeosFromRecordPacked(rhs, pContext) });

		double dirx, diry;
		std::tie(dirx, diry) = ReadVec2Packed(directionRecord);
		const Eigen::Vector2d direction(dirx, diry);

		const auto asEigen = [](const gg::Coordinate& p)
		{
			return Eigen::Vector2d(p.x, p.y);
		};

		struct VertexData
		{
			size_t geometryIndex;
			size_t vertexIndex;
			gg::Coordinate position;
			Eigen::Vector2d normal;

			VertexData() = default;
			VertexData(size_t geometryIndex, size_t vertexIndex, const gg::Coordinate& position, const Eigen::Vector2d& normal) :
				geometryIndex(geometryIndex),
				vertexIndex(vertexIndex),
				position(position),
				normal(normal)
			{}
		};

		struct PartialVertices
		{
			std::vector<VertexData> vs;
			GeometryPtr pCollider;
			bool isForwardDirected;

			PartialVertices() = default;
			PartialVertices(const VertexData& vertex) :vs({ vertex }) {}

			void makeCollider(const Eigen::Vector2d& direction)
			{
				isForwardDirected = 0.0 < vs.front().normal.dot(direction);
				pCollider.reset();
				/*for ()
				{

				}*/

			}
		};

		const auto isValidNormal = [&](const Eigen::Vector2d& normal)
		{
			return 1.e-2 < std::abs(direction.dot(normal));
		};

		using PartialVerticesList = std::vector<PartialVertices>;
		const auto addVertex = [&](PartialVerticesList& partialVerticesList, const VertexData& vertex)
		{
			if (!isValidNormal(vertex.normal))
			{
				return;
			}

			if (partialVerticesList.empty())
			{
				partialVerticesList.push_back(PartialVertices(vertex));
			}
			else
			{
				//同符号：直前の頂点と同じ向きを向いている
				if (0.0 < direction.dot(partialVerticesList.back().vs.back().normal) * direction.dot(vertex.normal))
				{
					partialVerticesList.back().vs.push_back(vertex);
				}
				//異符号：向きが逆になる
				else
				{
					partialVerticesList.push_back(PartialVertices(vertex));
				}
			}
		};

		//同じ方向を向いているセグメントのリストを作る
		std::array<PartialVerticesList, 2> bothVertices;
		for (size_t shapeIndex = 0; shapeIndex < bothPolygons.size(); ++shapeIndex)
		{
			const auto& currentPolygons = bothPolygons[shapeIndex];
			auto& vertices = bothVertices[shapeIndex];

			for (size_t geometryIndex = 0; geometryIndex < currentPolygons.size(); ++geometryIndex)
			{
				const gg::Geometry* pCurrentGeometry = currentPolygons.refer(geometryIndex);
				if (pCurrentGeometry->getGeometryTypeId() == gg::GEOS_POLYGON)
				{
					const gg::Polygon* pCurrentPolygon = dynamic_cast<const gg::Polygon*>(pCurrentGeometry);
					const gg::LineString* pExteriorRing = pCurrentPolygon->getExteriorRing();

					const auto getNormal = [](const gg::Coordinate& p0, const gg::Coordinate& p1, const gg::Coordinate& p2)->Eigen::Vector2d
					{
						const auto v0 = Eigen::Vector2d(p1.x - p0.x, p1.y - p0.y).normalized();
						const auto v1 = Eigen::Vector2d(p2.x - p1.x, p2.y - p1.y).normalized();
						const auto n0 = Eigen::Vector2d(-v0.y, v0.x);
						const auto n1 = Eigen::Vector2d(-v1.y, v1.x);
						return (n0 + n1).normalized();
					};

					{
						const auto currentVertex = pExteriorRing->getCoordinateN(0);

						//先頭と末尾は同じ点を指すので、末尾から2番目の点が先頭から見た直前の頂点となる
						const auto normal = getNormal(
							pExteriorRing->getCoordinateN(pExteriorRing->getNumPoints() - 2),
							currentVertex,
							pExteriorRing->getCoordinateN(1));

						addVertex(vertices, VertexData(geometryIndex, 0, currentVertex, normal));
					}

					for (size_t vertexIndex = 1; vertexIndex < pExteriorRing->getNumPoints(); ++vertexIndex)
					{
						const auto currentVertex = pExteriorRing->getCoordinateN(vertexIndex);

						const bool isEndVertex = vertexIndex + 1 == pExteriorRing->getNumPoints();
						const Eigen::Vector2d normal = (isEndVertex
							? getNormal(
								pExteriorRing->getCoordinateN(vertexIndex - 1),
								currentVertex,
								pExteriorRing->getCoordinateN(1))
							: getNormal(
								pExteriorRing->getCoordinateN(vertexIndex - 1),
								currentVertex,
								pExteriorRing->getCoordinateN(vertexIndex + 1)));
						
						addVertex(vertices, VertexData(geometryIndex, vertexIndex, currentVertex, normal));
					}
				}
			}
		}

		//交差判定用に折れ線のコライダを作る
		for (size_t shapeIndex = 0; shapeIndex < bothPolygons.size(); ++shapeIndex)
		{
			auto& vertices = bothVertices[shapeIndex];

			for (size_t partialIndex = 0; partialIndex < vertices.size(); ++partialIndex)
			{
				vertices[partialIndex].makeCollider(direction);
			}
		}

		const auto getPointsFromLines = [&](const gg::Geometry* pGeometry)->Vector<Eigen::Vector2d>
		{
			if (pGeometry->getGeometryTypeId() == gg::GEOS_LINESTRING)
			{
				Vector<Eigen::Vector2d> result;

				const gg::LineString* pLine = dynamic_cast<const gg::LineString*>(pGeometry);
				for (size_t i = 0; i < pLine->getNumPoints(); ++i)
				{
					result.push_back(asEigen(pLine->getCoordinateN(i)));
				}

				return result;
			}
			else if (pGeometry->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
			{
				Vector<Eigen::Vector2d> result;

				const gg::MultiLineString* pLines = dynamic_cast<const gg::MultiLineString*>(pGeometry);
				for (size_t lineIndex = 0; lineIndex < pLines->getNumGeometries(); ++lineIndex)
				{
					const gg::LineString* pLine = dynamic_cast<const gg::LineString*>(pLines->getGeometryN(lineIndex));
					for (size_t i = 0; i < pLine->getNumPoints(); ++i)
					{
						result.push_back(asEigen(pLine->getCoordinateN(i)));
					}
				}

				return result;
			}

			return {};
		};

		const auto Sq = [](const Eigen::Vector2d& v)
		{
			return v.x()*v.x() + v.y()*v.y();
		};

		struct VertexInfo
		{
			VertexInfo() = default;
			VertexInfo(size_t shapeIndex, size_t polygonIndex, size_t segmentIndex, size_t vertexIndex) :
				shapeIndex(shapeIndex),
				polygonIndex(polygonIndex),
				segmentIndex(segmentIndex),
				vertexIndex(vertexIndex)
			{}

			size_t shapeIndex;//LHS or RHS
			size_t polygonIndex;
			size_t segmentIndex;
			size_t vertexIndex;
		};
		
		struct PointInfo
		{
			PointInfo() = default;
			PointInfo(size_t shapeIndex, size_t polygonIndex, size_t segmentIndex, double vertexIndex) :
				shapeIndex(shapeIndex),
				polygonIndex(polygonIndex),
				segmentIndex(segmentIndex),
				vertexIndex(vertexIndex)
			{}

			size_t shapeIndex;//LHS or RHS
			size_t polygonIndex;
			size_t segmentIndex;
			double vertexIndex;
		};

		struct EnclosedRegions
		{
			struct Region
			{
				std::vector<size_t> directedSegment_VerticesInfo;
				std::vector<size_t> antiDirectedSegment_VerticesInfo;
			};

			std::vector<Region> regions;
		};

		struct PartialRays
		{
			//流入レイ
			struct EnteringRay
			{
				EnteringRay() = default;
				EnteringRay(const VertexInfo& begin, double rayEndVertexIndex) :
					begin(begin), rayEndVertexIndex(rayEndVertexIndex)
				{}
				double index()const { return rayEndVertexIndex; }
				VertexInfo begin;
				double rayEndVertexIndex;
			};
			//流出レイ
			struct ExitingRay
			{
				ExitingRay() = default;
				ExitingRay(const PointInfo& end, int rayBeginVertexIndex) :
					end(end), rayBeginVertexIndex(rayBeginVertexIndex)
				{}
				double index()const { return rayBeginVertexIndex; }
				PointInfo end;
				int rayBeginVertexIndex;
			};
			void addEnteringRay(double endPointIndex, const VertexInfo& beginPoint)
			{
				rays.push_back(EnteringRay(beginPoint, endPointIndex));
			}
			void addExitingRay(int pointIndex, const PointInfo& endPoint)
			{
				rays.push_back(ExitingRay(endPoint, pointIndex));
			}
			void sortRays()
			{
				const auto getIndex = [](const boost::variant<EnteringRay, ExitingRay>& rayInfo)
				{
					if (IsType<EnteringRay>(rayInfo))
					{
						return As<EnteringRay>(rayInfo).index();
					}
					return As<ExitingRay>(rayInfo).index();
				};
				std::sort(rays.begin(), rays.end(), [&](const boost::variant<EnteringRay, ExitingRay>& a, const boost::variant<EnteringRay, ExitingRay>& b) {
					return getIndex(a) < getIndex(b);
				});
			}
			boost::optional<size_t> findRayByVertexIndex(double vertexIndex)const
			{
				for (size_t i = 0; i < rays.size(); ++i)
				{
					if (IsType<EnteringRay>(rays[i]))
					{
						if (As<EnteringRay>(rays[i]).rayEndVertexIndex == vertexIndex)
						{
							return i;
						}
					}
				}
				return boost::none;
			}
			boost::optional<size_t> findRayByVertexIndex(int vertexIndex)const
			{
				for (size_t i = 0; i < rays.size(); ++i)
				{
					if (IsType<ExitingRay>(rays[i]))
					{
						if (As<ExitingRay>(rays[i]).rayBeginVertexIndex == vertexIndex)
						{
							return i;
						}
					}
				}
				return boost::none;
			}
			bool isExitingRay(size_t index)const
			{
				return IsType<ExitingRay>(rays[index]);
			}
			bool isEnteringRay(size_t index)const
			{
				return IsType<EnteringRay>(rays[index]);
			}
			std::vector<boost::variant<EnteringRay, ExitingRay>> rays;
		};
		using PartialRaysList = std::vector<PartialRays>;
		
		
		std::array<PartialRaysList, 2> bothRays;
		for (size_t shapeIndex = 0; shapeIndex < bothPolygons.size(); ++shapeIndex)
		{
			const auto& vertices = bothVertices[shapeIndex];
			auto& segments = bothRays[shapeIndex];
			segments.resize(vertices.size());

			/*for (size_t lineSegmentIndex = 0; lineSegmentIndex < vertices.size(); ++lineSegmentIndex)
			{
				vertices[lineSegmentIndex];
				segments[lineSegmentIndex];
			}*/
		}

		//
		const auto getHeight = [](const gg::Coordinate& a, const gg::Coordinate& b)
		{
			return abs((a.x*b.y - b.x*a.y) / sqrt(a.x*a.x + a.y*a.y));
		};

		const auto getProgress = [](const gg::Coordinate& a, const gg::Coordinate& b)
		{
			return (a.x*b.x + a.y*b.y) / (a.x*a.x + a.y*a.y);
		};

		const auto getPointIndex = [&getHeight, &getProgress](const std::vector<VertexData>& vs, const Eigen::Vector2d& pos)
		{
			int minIndex = 0;
			double minDistanceSq = DBL_MAX;
			double minProgress = 0;
			for (size_t i = 0; i + 1 < vs.size(); ++i)
			{
				const double currentDistance = getHeight(vs[i].position, vs[i + 1].position);
				if (currentDistance < minDistanceSq)
				{
					minDistanceSq = currentDistance;
					minIndex = i;
					minProgress = getProgress(vs[i].position, vs[i + 1].position);
				}
			}
			if (2 <= vs.size())
			{
				const double currentDistance = getHeight(vs.back().position, vs.front().position);
				if (currentDistance < minDistanceSq)
				{
					minDistanceSq = currentDistance;
					minIndex = vs.size() - 1;
					minProgress = getProgress(vs.back().position, vs.front().position);
				}
			}

			return minIndex + minProgress;
		};

		//それぞれのセグメントについて法線方向に飛ばしたレイとセグメントの交差判定を取り、
		//交差したレイの情報を互いに保存する
		std::unordered_map<unsigned, EnclosedRegions> enclosedRegions;
		for (size_t shapeIndex = 0; shapeIndex < bothPolygons.size(); ++shapeIndex)
		{
			/*const auto& selfPolygons = bothPolygons[shapeIndex];
			const auto& otherPolygons = bothPolygons[(shapeIndex + 1) % bothPolygons.size()];*/

			const size_t otherShapeIndex = (shapeIndex + 1) % bothVertices.size();
			const auto& selfVertices = bothVertices[shapeIndex];
			const auto& otherVertices = bothVertices[otherShapeIndex];
			auto& vertices = bothVertices[shapeIndex];
			auto& rays = bothRays[shapeIndex];

			for (size_t lineSegmentIndex = 0; lineSegmentIndex < vertices.size(); ++lineSegmentIndex)
			{
				PartialVertices& currentLine = vertices[lineSegmentIndex];
				PartialRays& currentInfo = rays[lineSegmentIndex];
				std::vector<VertexData>& currentVertices = currentLine.vs;
				const bool currentSegmentDirection = currentLine.isForwardDirected;
				const Eigen::Vector2d rayDirection = 0.0 < currentLine.isForwardDirected ? direction : -direction;

				for (size_t partialPointIndex = 0; partialPointIndex < currentVertices.size(); ++partialPointIndex)
				{
					//const gg::Polygon* pCurrentSelfPolygon = dynamic_cast<const gg::Polygon*>(selfPolygons.refer(currentVertices[partialPointIndex].geometryIndex));
					//const gg::LineString* pCurrentSelfRing = pCurrentSelfPolygon->getExteriorRing();
					const Eigen::Vector2d rayBegin = asEigen(currentVertices[partialPointIndex].position);
					const Eigen::Vector2d rayEnd = rayBegin + rayDirection * 1.e+6;

					GeometryPtr rayLine = MakeLine(rayBegin, rayEnd);

					const VertexInfo rayBeginInfo(shapeIndex, 0, lineSegmentIndex, partialPointIndex);

					double self_minDistanceSq = 1.e+20;
					size_t self_minDistanceIndex;
					double self_minDistanceVertexIndex;
					for (size_t targetLineSegmentIndex = 0; targetLineSegmentIndex < selfVertices.size(); ++targetLineSegmentIndex)
					{
						if (lineSegmentIndex == targetLineSegmentIndex ||
							currentSegmentDirection == selfVertices[targetLineSegmentIndex].isForwardDirected)
						{
							continue;
						}

						GeometryPtr rayCast = ToUnique<GeometryDeleter>(rayLine->intersection(selfVertices[targetLineSegmentIndex].pCollider.get()));
						for (const auto& p : getPointsFromLines(rayCast.get()))
						{
							const double currentDistanceSq = Sq(p - rayBegin);
							if (currentDistanceSq < self_minDistanceSq)
							{
								self_minDistanceSq = currentDistanceSq;
								self_minDistanceIndex = targetLineSegmentIndex;
								self_minDistanceVertexIndex = getPointIndex(selfVertices[targetLineSegmentIndex].vs, p);
							}
						}
					}

					double other_minDistanceSq = 1.e+30;//self_minDistanceSq < other_minDistanceSq
					Eigen::Vector2d other_minDistancePos(0, 0);
					size_t other_minDistanceIndex;
					double other_minDistanceVertexIndex;
					for (size_t targetLineSegmentIndex = 0; targetLineSegmentIndex < otherVertices.size(); ++targetLineSegmentIndex)
					{
						if (currentSegmentDirection == otherVertices[targetLineSegmentIndex].isForwardDirected)
						{
							continue;
						}

						GeometryPtr rayCast = ToUnique<GeometryDeleter>(rayLine->intersection(otherVertices[targetLineSegmentIndex].pCollider.get()));
						for (const auto& p : getPointsFromLines(rayCast.get()))
						{
							const double currentDistanceSq = Sq(p - rayBegin);
							if (currentDistanceSq < other_minDistanceSq)
							{
								other_minDistanceSq = currentDistanceSq;
								other_minDistancePos = p;
								other_minDistanceIndex = targetLineSegmentIndex;

								other_minDistanceVertexIndex = getPointIndex(otherVertices[targetLineSegmentIndex].vs, p);
							}
						}
					}

					if (other_minDistanceSq < self_minDistanceSq)
					{
						const PointInfo rayEndInfo(otherShapeIndex, 0, other_minDistanceIndex, other_minDistanceVertexIndex);
						currentInfo.addExitingRay(partialPointIndex, rayEndInfo);

						auto& otherRays = bothRays[otherShapeIndex];
						PartialRays& otherInfo = otherRays[other_minDistanceIndex];
						otherInfo.addEnteringRay(other_minDistanceVertexIndex, rayBeginInfo);
						//const unsigned h1 = std::hash<size_t>{}(lineSegmentIndex);
						//const unsigned h2 = std::hash<size_t>{}(other_minDistanceIndex);
						//const unsigned combined = h1 ^ h2;//commutative
						//
						//if (enclosedRegions[combined].regions.empty())
						//{
						//	enclosedRegions[combined].regions.emplace_back();
						//}
						// -> と <- の2ケースのみ考慮すればよい
						//
					}
					else
					{
						const PointInfo rayEndInfo(shapeIndex, 0, self_minDistanceIndex, self_minDistanceVertexIndex);
						currentInfo.addExitingRay(partialPointIndex, rayEndInfo);

						auto& otherRays = bothRays[shapeIndex];
						PartialRays& otherInfo = otherRays[self_minDistanceIndex];
						otherInfo.addEnteringRay(self_minDistanceVertexIndex, rayBeginInfo);
					}
				}
			}
		}

		{
			const size_t shapeIndex = 0;

			const auto& selfVertices = bothVertices[shapeIndex];
			auto& vertices = bothVertices[shapeIndex];
			auto& rays = bothRays[shapeIndex];

			for (size_t lineSegmentIndex = 0; lineSegmentIndex < vertices.size(); ++lineSegmentIndex)
			{
				PartialVertices& currentLine = vertices[lineSegmentIndex];
				PartialRays& currentInfo = rays[lineSegmentIndex];
				std::vector<VertexData>& currentVertices = currentLine.vs;
				const bool currentSegmentDirection = currentLine.isForwardDirected;
				const Eigen::Vector2d rayDirection = 0.0 < currentLine.isForwardDirected ? direction : -direction;

				currentInfo.sortRays();

				//rayについて
				//両端はExitingRay
				//連続するExitingRayは無視してよい
				//EnteringRayで区切られた範囲の閉路を探す
				std::vector<size_t> loopIndices;
				if (currentInfo.isExitingRay(0))
				{
					loopIndices.push_back(0);
				}
				for (size_t rayIndex = 0; rayIndex < currentInfo.rays.size(); ++rayIndex)
				{
					if (currentInfo.isEnteringRay(rayIndex))
					{
						loopIndices.push_back(rayIndex);
					}
				}
				if (currentInfo.isExitingRay(loopIndices.size() - 1))
				{
					loopIndices.push_back(loopIndices.size() - 1);
				}

				for (size_t loopIndex = 0; loopIndex + 1 < loopIndices.size(); ++loopIndex)
				{
					const size_t loopBeginIndex = loopIndices[loopIndex];
					const size_t loopEndIndex = loopIndices[loopIndex + 1];

					if (IsType<PartialRays::EnteringRay>(currentInfo.rays[loopBeginIndex]))
					{
						const PartialRays::EnteringRay& currentRay = As<PartialRays::EnteringRay>(currentInfo.rays[loopBeginIndex]);
						const auto& beginInfo = currentRay.begin;

						bothVertices[beginInfo.shapeIndex][beginInfo.segmentIndex].vs[beginInfo.vertexIndex];
					}
					else
					{
						const PartialRays::ExitingRay& currentRay = As<PartialRays::ExitingRay>(currentInfo.rays[loopBeginIndex]);
						const auto& endInfo = currentRay.end;

						const double currentEndIndex = endInfo.vertexIndex;

						const int vertexIndexFloor = static_cast<int>(floor(endInfo.vertexIndex));
						bothVertices[endInfo.shapeIndex][endInfo.segmentIndex].vs[vertexIndexFloor];

						const int oppositeRayIndex = bothRays[endInfo.shapeIndex][endInfo.segmentIndex].findRayByVertexIndex(currentEndIndex).value();

						//基準のセグメントについて頂点インデックスの昇順でレイを見ている
						//したがって、基準セグメントの向かい側のセグメントで閉路を作る方向は、必ず頂点インデックスの降順になる
						const int returnRayIndex = oppositeRayIndex - 1;

						/*if (returnRayIndex < 0)
						{
							continue;
						}*/


						//帰ってくる
						if (IsType<PartialRays::ExitingRay>(bothRays[endInfo.shapeIndex][endInfo.segmentIndex].rays[returnRayIndex]))
						{
							const auto& returnRay = As<PartialRays::ExitingRay>(bothRays[endInfo.shapeIndex][endInfo.segmentIndex].rays[returnRayIndex]);
							returnRay.end;
						}
						//別ルートへ進む
						else
						{
							const auto& moveRay = As<PartialRays::EnteringRay>(bothRays[endInfo.shapeIndex][endInfo.segmentIndex].rays[returnRayIndex]);
							moveRay.begin.shapeIndex;
							moveRay.begin.segmentIndex;
							moveRay.begin.vertexIndex;
						}
						;
					}
				}

			}
		}
	}
#endif

	PackedRecord GetConvexHull(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		Geometries polygons(GeosFromRecordPacked(shape, pContext));

		std::vector<gg::Coordinate> cs;
		for (int s = 0; s < polygons.size(); ++s)
		{
			auto pCurrentGeometry = polygons.refer(s);

			gg::CoordinateSequence* points = pCurrentGeometry->getCoordinates();
			for (size_t i = 0; i < points->getSize(); ++i)
			{
				cs.emplace_back(points->getX(i), points->getY(i));
			}
			delete points;
		}

		auto factory = gg::GeometryFactory::create();
		gg::MultiPoint* mp = factory->createMultiPoint(cs);

		Geometries resultGeometries;
		resultGeometries.push_back_raw(mp->convexHull());

		delete mp;

		return MakePolygonResult(GetPolygon(resultGeometries));
	}

	PackedRecord GetGlobalShape(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		try
		{
			Geometries lhsPolygon(GeosFromRecordPacked(shape, pContext));
			return MakePolygonResult(GetPolygon(lhsPolygon));
		}
		catch (std::exception& e)
		{
			std::cout << "GetGlobalShape: " << e.what() << std::endl;
			throw;
		}
	}

	PackedRecord GetTransformedShape(const PackedRecord& shape, const PackedRecord& posRecord, const PackedRecord& scaleRecord, double angle, std::shared_ptr<Context> pContext)
	{
		const auto pos = ReadVec2Packed(posRecord);
		const auto scale = ReadVec2Packed(scaleRecord);

		TransformPacked transform(std::get<0>(pos), std::get<1>(pos), std::get<0>(scale), std::get<1>(scale), angle);

		Geometries lhsPolygon = GeosFromRecordPacked(shape, pContext, transform);
		return MakePolygonResult(GetPolygon(lhsPolygon));
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

		return MakePolygonResult(GetPolygon(result));
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
		Geometries geometries(GeosFromRecordPacked(shape, pContext));
		const auto result = deformer.FFD(geometries);

		return MakePolygonResult(GetPolygon(result));
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

		return GetCenterLineDeformedShape(transformedShape, targetPath, pContext);
	}

	PackedList AlignH(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<double> currentLeftX;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentLeftX)
			{
				result.add(data.value);
				currentLeftX = bb.maxPos().x();
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", currentLeftX.get() - bb.minPos().x(), "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentLeftX = currentLeftX.get() + bb.width().x();
			}
		}

		return result;
	}

	struct Vec2
	{
		Vec2() = default;
		Vec2(double x, double y) :x(x), y(y) {}
		double x, y;
	};

	PackedList AlignHTop(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<Vec2> currentTopLeft;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentTopLeft)
			{
				result.add(data.value);
				currentTopLeft = Vec2(bb.maxPos().x(), bb.minPos().y());
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", currentTopLeft.get().x - bb.minPos().x(), "y", currentTopLeft.get().y - bb.minPos().y()),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentTopLeft.get().x = currentTopLeft.get().x + bb.width().x();
			}
		}

		return result;
	}

	PackedList AlignHBottom(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<Vec2> currentBottomLeft;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentBottomLeft)
			{
				result.add(data.value);
				currentBottomLeft = Vec2(bb.maxPos().x(), bb.maxPos().y());
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", currentBottomLeft.get().x - bb.minPos().x(), "y", currentBottomLeft.get().y - bb.maxPos().y()),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentBottomLeft.get().x = currentBottomLeft.get().x + bb.width().x();
			}
		}

		return result;
	}

	PackedList AlignHCenter(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<Vec2> currentCenterLeft;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentCenterLeft)
			{
				result.add(data.value);
				currentCenterLeft = Vec2(bb.maxPos().x(), bb.center().y());
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", currentCenterLeft.get().x - bb.minPos().x(), "y", currentCenterLeft.get().y - bb.center().y()),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentCenterLeft.get().x = currentCenterLeft.get().x + bb.width().x();
			}
		}

		return result;
	}

	PackedList AlignV(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<double> currentTopY;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentTopY)
			{
				result.add(data.value);
				currentTopY = bb.maxPos().y();
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", currentTopY.get() - bb.minPos().y()),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentTopY = currentTopY.get() + bb.width().y();
			}
		}

		return result;
	}

	PackedList AlignVLeft(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<Vec2> currentTopLeft;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentTopLeft)
			{
				result.add(data.value);
				currentTopLeft = Vec2(bb.minPos().x(), bb.maxPos().y());
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", currentTopLeft.get().x - bb.minPos().x(), "y", currentTopLeft.get().y - bb.minPos().y()),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentTopLeft.get().y = currentTopLeft.get().y + bb.width().y();
			}
		}

		return result;
	}

	PackedList AlignVRight(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<Vec2> currentTopRight;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentTopRight)
			{
				result.add(data.value);
				currentTopRight = Vec2(bb.maxPos().x(), bb.maxPos().y());
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", currentTopRight.get().x - bb.maxPos().x(), "y", currentTopRight.get().y - bb.minPos().y()),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentTopRight.get().y = currentTopRight.get().y + bb.width().y();
			}
		}

		return result;
	}

	PackedList AlignVCenter(const PackedList& shapes, std::shared_ptr<Context> pContext)
	{
		if (shapes.data.empty())
		{
			return shapes;
		}

		boost::optional<Vec2> currentTopCenter;
		PackedList result;
		for (const auto& data : shapes.data)
		{
			boost::optional<PackedRecord> wrapped = IsType<PackedRecord>(data.value)
				? boost::none
				: boost::optional<PackedRecord>(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", 0, "y", 0),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));

			const BoundingRect bb = BoundingRectRecordPacked(wrapped ? wrapped.get() : data.value, pContext);

			if (bb.isEmpty())
			{
				result.add(data.value);
			}
			else if (!currentTopCenter)
			{
				result.add(data.value);
				currentTopCenter = Vec2(bb.center().x(), bb.maxPos().y());
			}
			else
			{
				result.add(MakeRecord(
					"inner", data.value,
					"pos", MakeRecord("x", currentTopCenter.get().x - bb.center().x(), "y", currentTopCenter.get().y - bb.minPos().y()),
					"scale", MakeRecord("x", 1.0, "y", 1.0),
					"angle", 0
				));
				currentTopCenter.get().y = currentTopCenter.get().y + bb.width().y();
			}
		}

		return result;
	}

	std::vector<int> ReadIndices(const PackedList& alignIndices)
	{
		std::vector<int> indices;
		for (const auto& data : alignIndices.data)
		{
			if (!IsType<int>(data.value))
			{
				CGL_Error("インデックスに整数以外の型が渡されました");
			}
			else
			{
				indices.push_back(As<int>(data.value));
			}
		}
		return indices;
	}

	/*PackedList AlignHColumns(const PackedList& shapes, const PackedList& alignIndices, std::shared_ptr<Context> pContext)
	{
		const std::vector<int> indices = ReadIndices(alignIndices);
		if (indices.empty())
		{
			return shapes;
		}

		const size_t regionSize = indices.size() + 1;
	}

	PackedList AlignVRows(const PackedList& shapes, const PackedList& alignIndices, std::shared_ptr<Context> pContext)
	{
		const std::vector<int> indices = ReadIndices(alignIndices);
		if (indices.empty())
		{
			return shapes;
		}

		const size_t regionSize = indices.size() + 1;
		std::vector<double> eachRegionMaxWidths(regionSize, 0.0);
	}*/

	PackedRecord ShapeDirectedPoint(const PackedRecord& shape, const Eigen::Vector2d& v, std::shared_ptr<Context> pContext)
	{
		double maxDot = -1.0;
		Eigen::Vector2d maxDotPoint;

		const BoundingRect boundingRect = BoundingRectRecordPacked(shape, pContext);
		const auto center = boundingRect.center();

		Geometries shapePolygon(GeosFromRecordPacked(shape, pContext));

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

		//for (const gg::Geometry* polygon : shapePolygon)
		for (size_t g = 0; g < shapePolygon.size(); ++g)
		{
			const gg::Geometry* polygon = shapePolygon.refer(g);
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
		return ShapeDirectedPoint(shape, EigenVec2(-1, 0), pContext);
	}

	PackedRecord ShapeRight(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, EigenVec2(+1, 0), pContext);
	}

	PackedRecord ShapeTop(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, EigenVec2(0, -1), pContext);
	}

	PackedRecord ShapeBottom(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, EigenVec2(0, +1), pContext);
	}

	PackedRecord ShapeTopLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, EigenVec2(-1, -1), pContext);
	}

	PackedRecord ShapeTopRight(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, EigenVec2(+1, -1), pContext);
	}

	PackedRecord ShapeBottomLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, EigenVec2(-1, +1), pContext);
	}

	PackedRecord ShapeBottomRight(const PackedRecord& shape, std::shared_ptr<Context> pContext)
	{
		return ShapeDirectedPoint(shape, EigenVec2(+1, +1), pContext);
	}
}
