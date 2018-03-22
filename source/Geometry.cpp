#include <cppoptlib/meta.h>
#include <cppoptlib/problem.h>
#include <cppoptlib/solver/bfgssolver.h>

#include <geos/geom.h>
#include <geos/opBuffer.h>
#include <geos/opDistance.h>

#include <cmaes.h>

#include <Pita/Geometry.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/FontShape.hpp>
#include <Pita/Printer.hpp>
#include <Pita/Evaluator.hpp>

namespace
{
	/*inline long long Combination(int n, int k)
	{
		if (k > n) return 0;
		if (k * 2 > n) k = n - k;
		if (k == 0) return 1;

		long long result = n;
		for (int i = 2; i <= k; ++i)
		{
			result *= (n - i + 1);
			result /= i;
		}

		return result;
	}*/

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
			/*initialScope = scope;
			yNum = ynum;
			xNum = xnum;
			xs = std::vector<std::vector<double>>(ynum, std::vector<double>(xnum, 0));
			ys = std::vector<std::vector<double>>(ynum, std::vector<double>(xnum, 0));

			const Vec2 pos = scope.pos;
			const Vec2 size = scope.size;

			for (int y = 0; y < ynum; ++y)
			{
			const double progressY = 1.0*y / (ynum - 1);
			for (int x = 0; x < xnum; ++x)
			{
			const double progressX = 1.0*x / (xnum - 1);
			ys[y][x] = pos.y + size.y*progressY;
			xs[y][x] = pos.x + size.x*progressX;
			}
			}*/

			m_boundingRect = boundingRect;

			xs = dataX;
			ys = dataY;

			const int yNum = dataX.size();
			const int xNum = dataX.front().size();

			m_bernsteinX.initialize(xNum - 1);
			m_bernsteinY.initialize(yNum - 1);
		}

		/*void update()
		{
		if (Input::MouseL.pressed)
		{
		const Vec2 p = Mouse::PosF();
		const Vec2 d = Mouse::DeltaF();

		const double unitW = initialScope.w / (static_cast<int>(xNum) - 1);
		const double unitH = initialScope.h / (static_cast<int>(yNum) - 1);
		const double unitDist = Sqrt(unitW*unitW + unitH * unitH);

		for (int y = 0; y < yNum; ++y)
		{
		for (int x = 0; x < xNum; ++x)
		{
		const double distY = ys[y][x] - p.y;
		const double distX = xs[y][x] - p.x;
		const double dist = Sqrt(distY*distY + distX * distX);
		const double weight = NormalPDF(dist / unitDist, 0.4);

		ys[y][x] += d.y*weight;
		xs[y][x] += d.x*weight;
		}
		}
		}
		}*/

		/*void draw()
		{
		const Color color = Color(64, 64, 64);
		for (int y = 0; y < yNum; ++y)
		{
		for (int x = 0; x + 1 < xNum; ++x)
		{
		const double py0 = ys[y][x];
		const double py1 = ys[y][x + 1];
		const double px0 = xs[y][x];
		const double px1 = xs[y][x + 1];

		Line(px0, py0, px1, py1).draw(1.0, color);
		}
		}
		for (int y = 0; y + 1 < yNum; ++y)
		{
		for (int x = 0; x < xNum; ++x)
		{
		const double py0 = ys[y][x];
		const double py1 = ys[y + 1][x];
		const double px0 = xs[y][x];
		const double px1 = xs[y + 1][x];

		Line(px0, py0, px1, py1).draw(1.0, color);
		}
		}
		}*/

		/*
		Image FFD(const Image& image, const Vec2& pos)
		{
			Image result(Window::Size(), Palette::White);

			for (int y = 0; y < image.height; ++y)
			{
				const double v = 1.0*(pos.y + y) / (static_cast<int>(result.height) - 1);

				for (int x = 0; x < image.width; ++x)
				{
					const double u = 1.0*(pos.x + x) / (static_cast<int>(result.width) - 1);

					const auto get = [&](double tX, double tY, const std::vector<std::vector<double>>& dataX, const std::vector<std::vector<double>>& dataY)
					{
						const int nX = dataX.front().size() - 1;
						const int nY = dataX.size() - 1;

						double sumX = 0.0;
						double sumY = 0.0;

						for (int yi = 0; yi <= nY; ++yi)
						{
							//const double bY = BernsteinBasis(yi, nY, tY);
							const double bY = m_bernsteinY(yi, tY);
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

						return Vec2(sumX, sumY);
					};

					const Vec2 pos_ = get(u, v, xs, ys);
					const Point p = pos_.asPoint();

					if (Rect(0, 0, result.width, result.height).contains(p))
					{
						const auto c = image[y][x];
						result[p.y][p.x] = result[p.y + 1][p.x] = result[p.y][p.x + 1] = result[p.y + 1][p.x + 1] = c;
					}
				}
			}

			return result;
		}*/

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

			//for (const gg::Geometry* geometry : originalShape)
			for (int shapei=0;shapei<originalShape.size();++shapei)
			{
				bool debugPrint = false;
				if (shapei + 18 == originalShape.size())
				{
					//continue;
					debugPrint = true;
				}

				const gg::Geometry* geometry = originalShape[shapei];
				if (geometry->getGeometryTypeId() == gg::GEOS_POLYGON)
				{
					
					const gg::Polygon* polygon = dynamic_cast<const gg::Polygon*>(geometry);
					
					const gg::LineString* exterior = polygon->getExteriorRing();

					gg::CoordinateArraySequence newExterior;
					/*for (size_t p = 0; p < exterior->getNumPoints(); ++p)
					{
						const gg::Point* point = exterior->getPointN(p);

						const auto uv = getUV(point->getX(), point->getY());

						const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

						if (debugPrint)
						{
							//std::cout << "uv(" << uv.x() << ", " << uv.y() << "), pos(" << newPos.x() << ", " << newPos.y() << ")\n";
						}

						newExterior.add(gg::Coordinate(newPos.x(), newPos.y()));
						//newExterior.add(gg::Coordinate(point->getX(), point->getY()));
					}
					*/
					for (size_t p = 0; p + 1 < exterior->getNumPoints(); ++p)
					{
						const gg::Point* p0 = exterior->getPointN(p);
						const gg::Point* p1 = exterior->getPointN(p + 1);

						const int num = 5;
						for (int sub = 0; sub < num; ++sub)
						{
							const double progress = 1.0 * sub / (num - 1);
							const double x = p0->getX() + (p1->getX() - p0->getX())*progress;
							const double y = p0->getY() + (p1->getY() - p0->getY())*progress;

							const auto uv = getUV(x, y);

							const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

							newExterior.add(gg::Coordinate(newPos.x(), newPos.y()));
						}
					}

					std::vector<gg::Geometry*>* holes = new std::vector<gg::Geometry*>;
					for (size_t i = 0; i < polygon->getNumInteriorRing(); ++i)
					{
						const gg::LineString* hole = polygon->getInteriorRingN(i);

						gg::CoordinateArraySequence newInterior;
						//for (size_t p = 0; p < hole->getNumPoints(); ++p)
						for (int p = static_cast<int>(hole->getNumPoints()) - 1; 0 <= p; --p)
						{
							const gg::Point* point = hole->getPointN(p);

							const auto uv = getUV(point->getX(), point->getY());

							const auto newPos = get(uv.x(), uv.y(), xs, ys, debugPrint);

							newInterior.add(gg::Coordinate(newPos.x(), newPos.y()));
							//newInterior.add(gg::Coordinate(point->getX(), point->getY()));
						}

						//holes.push_back(factory->createLinearRing(newInterior));
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
						

						const int num = 5;
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

	List ShapeDifference(const Val& lhs, const Val& rhs, std::shared_ptr<cgl::Context> pEnv)
	{
		if ((!IsType<Record>(lhs) && !IsType<List>(lhs)) || (!IsType<Record>(rhs) && !IsType<List>(rhs)))
		{
			CGL_Error("不正な式です");
			return{};
		}

		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecord(lhs, pEnv);
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecord(rhs, pEnv);

		auto factory = gg::GeometryFactory::create();

		if (lhsPolygon.empty())
		{
			return {};
		}
		else if (rhsPolygon.empty())
		{
			return GetShapesFromGeos(lhsPolygon, pEnv);
		}
		else
		{
			auto factory = gg::GeometryFactory::create();

			std::vector<gg::Geometry*> resultGeometries;
			//gg::Geometry* resultGeometry = factory->createEmptyGeometry();

			for (int s = 0; s < lhsPolygon.size(); ++s)
			{
				gg::Geometry* erodeGeometry = lhsPolygon[s];
				/*std::cout << "accumlatedPolygons : " << std::endl;
				{
					gg::CoordinateSequence* cs = erodeGeometry->getCoordinates();
					for (int i = 0; i < cs->size(); ++i)
					{
						std::cout << "    " << i << ": (" << cs->getX(i) << ", " << cs->getY(i) << ")" << std::endl;
					}
				}*/
				for (int d = 0; d < rhsPolygon.size(); ++d)
				{
					/*std::cout << "accumlatedHoles : " << std::endl;
					{
						gg::CoordinateSequence* cs = rhsPolygon[d]->getCoordinates();
						for (int i = 0; i < cs->size(); ++i)
						{
							std::cout << "    " << i << ": (" << cs->getX(i) << ", " << cs->getY(i) << ")" << std::endl;
						}
					}*/
					erodeGeometry = erodeGeometry->difference(rhsPolygon[d]);

					if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
					{
						//std::cout << "ok" << std::endl;
					}
					else if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						lhsPolygon.erase(lhsPolygon.begin() + s);

						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);
						//std::cout << "erodeGeometry is MultiPolygon(size: " << polygons->getNumGeometries() << ")" << std::endl;

						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							//std::cout << "MultiPolygon[" << i << "]: " << polygons->getGeometryN(i)->getGeometryType() << std::endl;
							lhsPolygon.insert(lhsPolygon.begin() + s, polygons->getGeometryN(i)->clone());
						}

						erodeGeometry = lhsPolygon[s];
					}
					else if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION)
					{
						const gg::GeometryCollection* geometries = dynamic_cast<const gg::GeometryCollection*>(erodeGeometry);
						//std::cout << "erodeGeometry is GeometryCollection(size: " << geometries->getNumGeometries() << ")" << std::endl;
						for (int i = 0; i < geometries->getNumGeometries(); ++i)
						{
							//std::cout << "GeometryCollection[" << i << "]: " << geometries->getGeometryN(i)->getGeometryType() << std::endl;
							//lhsPolygon.insert(lhsPolygon.begin() + s, polygons->getGeometryN(i)->clone());
						}
					}
					else
					{
						//std::cout << __FUNCTION__ << " Differenceの結果が予期せぬデータ形式" << __LINE__ << std::endl;
						std::cout << " Differenceの結果が予期せぬデータ形式 : " << GetGeometryType(erodeGeometry) << std::endl;
					}
				}

				resultGeometries.push_back(erodeGeometry);
				//resultGeometry->Union(erodeGeometry);
			}

			return GetShapesFromGeos(resultGeometries, pEnv);
			/*
			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
			gg::MultiPolygon* accumlatedPolygons = factory->createMultiPolygon(lhsPolygon);
			
			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
			gg::MultiPolygon* accumlatedHoles = factory->createMultiPolygon(rhsPolygon);
			
			std::cout << "accumlatedPolygons : " << std::endl;
			{
				gg::CoordinateSequence* cs = accumlatedPolygons->getCoordinates();
				for (int i = 0; i < cs->size(); ++i)
				{
					std::cout << "    " << i << ": (" << cs->getX(i) << ", " << cs->getY(i) << ")" << std::endl;
				}
			}
			
			std::cout << "accumlatedHoles : " << std::endl;
			{
				gg::CoordinateSequence* cs = accumlatedHoles->getCoordinates();
				for (int i = 0; i < cs->size(); ++i)
				{
					std::cout << "    " << i << ": (" << cs->getX(i) << ", " << cs->getY(i) << ")" << std::endl;
				}
			}

			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

			gg::Geometry* resultg =  accumlatedPolygons->difference(accumlatedHoles);

			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

			std::vector<gg::Geometry*> diffResult({ resultg });
			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
			return GetShapesFromGeos(diffResult, pEnv);
			*/
		}
	}

	List ShapeBuffer(const Val & shape, const Val & amount, std::shared_ptr<cgl::Context> pEnv)
	{
		if (!IsType<Record>(shape) && !IsType<List>(shape))
		{
			CGL_Error("不正な式です");
			return{};
		}

		if (!IsType<int>(amount) && !IsType<double>(amount))
		{
			CGL_Error("不正な式です");
			return{};
		}

		const double distance = IsType<int>(amount) ? static_cast<double>(As<int>(amount)) : As<double>(amount);
		std::vector<gg::Geometry*> polygons = GeosFromRecord(shape, pEnv);

		std::vector<gg::Geometry*> resultGeometries;
		
		for (int s = 0; s < polygons.size(); ++s)
		{
			gg::Geometry* currentGeometry = polygons[s];
			resultGeometries.push_back(currentGeometry->buffer(distance));
		}

		return GetShapesFromGeos(resultGeometries, pEnv);
	}

#ifdef commentout
	void GetPath(Record& pathRule, std::shared_ptr<cgl::Context> pEnv)
	{
		const auto& values = pathRule.values;

		auto itPoints = values.find("points");
		auto itPasses = values.find("passes");
		auto itCircumvents = values.find("circumvents");

		int num = 10;
		if (itPoints != values.end())
		{
			const Val evaluated = pEnv->expand(itPoints->second);
			if (!IsType<int>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			num = As<int>(evaluated);
		}

		auto factory = gg::GeometryFactory::create();

		std::vector<gg::Point*> originalPoints;
		Vector<Eigen::Vector2d> points;
		{
			if (itPasses == values.end())
			{
				CGL_Error("不正な式です");
				return;
			}

			const Val evaluated = pEnv->expand(itPasses->second);
			if (!IsType<List>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			List passShapes = As<List>(evaluated);

			for (Address address : passShapes.data)
			{
				const Val pointVal = pEnv->expand(address);
				if (!IsType<Record>(pointVal))
				{
					CGL_Error("不正な式です");
					return;
				}
				const auto& pos = As<Record>(pointVal);
				const Val xval = pEnv->expand(pos.values.find("x")->second);
				const Val yval = pEnv->expand(pos.values.find("y")->second);
				const double x = IsType<int>(xval) ? static_cast<double>(As<int>(xval)) : As<double>(xval);
				const double y = IsType<int>(yval) ? static_cast<double>(As<int>(yval)) : As<double>(yval);
				//points.add(gg::Coordinate(x, y));
				Eigen::Vector2d v;
				v << x, y;
				points.push_back(v);

				originalPoints.push_back(factory->createPoint(gg::Coordinate(x, y)));
			}
		}

		const auto mostDistantIndex = [](const Vector<Eigen::Vector2d>& points)
		{
			int index = 0;
			double maxDistanceSq = 0;
			for (int i = 0; i + 1 < points.size(); ++i)
			{
				const Eigen::Vector2d diff = points[i] - points[i + 1];
				const double currentDistanceSq = diff.dot(diff);
				if (maxDistanceSq < currentDistanceSq)
				{
					index = i;
					maxDistanceSq = currentDistanceSq;
				}
			}
			return index;
		};

		if (points.size() < num)
		{
			const int additionalPointsSize = num - points.size();
			for (int s = 0; s < additionalPointsSize; ++s)
			{
				const int insertIndex = mostDistantIndex(points);
				Eigen::Vector2d mid = (points[insertIndex] + points[insertIndex + 1])*0.5;
				points.insert(points.begin() + insertIndex + 1, mid);
			}
		}

		std::vector<gg::Geometry*> obstacles;
		List circumvents;
		{
			const Val evaluated = pEnv->expand(itCircumvents->second);
			if (!IsType<List>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}

			obstacles = GeosFromRecord(evaluated, pEnv);
		}

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

		const gg::Coordinate beginPos(points.front().x(), points.front().y());
		const gg::Coordinate endPos(points.back().x(), points.back().y());

		points.erase(points.end() - 1);
		points.erase(points.begin());
		originalPoints.erase(originalPoints.end() - 1);
		originalPoints.erase(originalPoints.begin());

		Record result;
		List polygonList;

		//*
		PathConstraintProblem  constraintProblem;
		constraintProblem.evaluator = [&](const PathConstraintProblem::TVector& v)->double
		{
			gg::CoordinateArraySequence cs2;
			cs2.add(beginPos);
			for (int i = 0; i*2 < v.size(); ++i)
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

		Eigen::VectorXd x0s(points.size()*2);
		for (int i = 0; i < points.size(); ++i)
		{
			x0s[i * 2 + 0] = points[i].x();
			x0s[i * 2 + 1] = points[i].y();
		}

		cppoptlib::BfgsSolver<PathConstraintProblem> solver;
		solver.minimize(constraintProblem, x0s);

		for (int i = 0; i*2 < x0s.size(); ++i)
		{
			appendCoord(polygonList, x0s[i * 2 + 0], x0s[i * 2 + 1]);
		}
		//*/

		//*
		libcmaes::FitFunc func = [&](const double *x, const int N)->double
		{
			gg::CoordinateArraySequence cs2;
			cs2.add(beginPos);
			for (int i = 0; i * 2 < N; ++i)
			{
				cs2.add(gg::Coordinate(x[i * 2 + 0], x[i * 2 + 1]));
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
				else if(g->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
				{
					const gg::MultiLineString* intersections = dynamic_cast<const gg::MultiLineString*>(g);
					penalty2 += intersections->getLength();
				}
				else
				{
					//std::cout << "Result type: " << g->getGeometryType();
				}
			}

			//penalty = 100 * penalty*penalty;
			//penalty2 = 100 * penalty2*penalty2;

			const double totalCost = pathLength + penalty*penalty + penalty2*penalty2;
			//std::cout << std::string("path cost: ") << ToS(pathLength, 10) << ", " << ToS(penalty, 10) << ", " << ToS(penalty2, 10) << " => " << ToS(totalCost, 15) << "\n";

			//const double totalCost = penalty2;
			//std::cout << std::string("path cost: ") << ToS(penalty2, 17) << "\n";
			return totalCost;
		};

		std::vector<double> x0(points.size() * 2);
		std::cout << "begin : (" << beginPos.x << ", " << beginPos.y << ")\n";
		for (int i = 0; i < points.size(); ++i)
		{
			x0[i * 2 + 0] = points[i].x();
			x0[i * 2 + 1] = points[i].y();

			std::cout << i << ": (" << points[i].x() << ", " << points[i].y() << ")\n";
		}
		std::cout << "end : (" << endPos.x << ", " << endPos.y << ")\n";

		const double sigma = 0.1;

		const int lambda = 100;

		libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda, 1);
		libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
		auto resultxs = cmasols.best_candidate().get_x();

		appendCoord(polygonList, beginPos.x, beginPos.y);
		for (int i = 0; i * 2 < resultxs.size(); ++i)
		{
			appendCoord(polygonList, resultxs[i * 2 + 0], resultxs[i * 2 + 1]);
		}
		appendCoord(polygonList, endPos.x, endPos.y);

		//*/

		result.append("line", pEnv->makeTemporaryValue(polygonList));
		{
			Record record;
			record.append("r", pEnv->makeTemporaryValue(0));
			record.append("g", pEnv->makeTemporaryValue(255));
			record.append("b", pEnv->makeTemporaryValue(255));
			result.append("color", pEnv->makeTemporaryValue(record));
		}

		pathRule.append("path", pEnv->makeTemporaryValue(result));
	}
#endif

	void GetPath(Record& pathRule, std::shared_ptr<cgl::Context> pEnv)
	{
		PackedRecord packedPathRule = As<PackedRecord>(pathRule.packed(*pEnv));
		
		const auto& values = packedPathRule.values;

		auto itPoints = values.find("points");
		auto itPasses = values.find("passes");
		auto itCircumvents = values.find("obstacles");

		int num = 10;
		if (itPoints != values.end())
		{
			const PackedVal& numVal = itPoints->second.value;
			if (auto numOpt = AsOpt<int>(numVal))
			{
				num = numOpt.value();	
			}
			else
			{
				CGL_Error("pointsが不正な式です");
			}
		}

		auto factory = gg::GeometryFactory::create();

		std::vector<gg::Point*> originalPoints;
		Vector<Eigen::Vector2d> points;
		{
			if (itPasses == values.end())
			{
				CGL_Error("passesが不正な式です");
			}

			const PackedVal& pathsVal = itPasses->second.value;
			if (auto pathListOpt = AsOpt<PackedList>(pathsVal))
			{
				for (const auto& pointVal : pathListOpt.value().data)
				{
					if (auto posRecordOpt = AsOpt<PackedRecord>(pointVal.value))
					{
						const auto v = ReadVec2Packed(posRecordOpt.value());
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
			else
			{
				CGL_Error("passesが不正な式です");
			}
		}

		//double rodLength = 50;
		std::vector<double> angles1(num / 2);
		std::vector<double> angles2(num / 2);

		std::vector<gg::Geometry*> obstacles;
		if (itCircumvents != values.end())
		{
			const PackedVal& obstacleListVal = itCircumvents->second.value;
			if (auto obstacleListOpt = AsOpt<PackedList>(obstacleListVal))
			{
				obstacles = GeosFromRecordPacked(obstacleListOpt.value());
			}
			else
			{
				CGL_Error("obstaclesが不正な式です");
			}
		}
		
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
		appendCoord(polygonList, x0s[i * 2 + 0], x0s[i * 2 + 1]);
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
			const double distanceSq = dx*dx + dy*dy;
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

			const double penaltySq = penalty*penalty;
			const double penalty2Sq = penalty2*penalty2;

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

		std::vector<double> x0(num + 1, 0.0);
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
		libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda, 0);
		libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
		auto resultxs = cmasols.best_candidate().get_x();
		//*/
		
		{
			const double rodLength = resultxs.back();

			const int halfindex = num / 2;
			{
				double lastX = beginPos.x;
				double lastY = beginPos.y;

				appendCoord(polygonList, beginPos.x, beginPos.y);
				for (int i = 0; i < halfindex; ++i)
				{
					const double angle = resultxs[i];
					const double dx = rodLength * cos(angle);
					const double dy = rodLength * sin(angle);
					lastX += dx;
					lastY += dy;
					appendCoord(polygonList, lastX, lastY);
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
				appendCoord(polygonList, it->x(), it->y());
			}
		}

		//*/

		result.add("line", polygonList);
		{
			PackedRecord record;
			record.add("r", 0);
			record.add("g", 255);
			record.add("b", 255);
			result.add("color", record);
		}
		
		packedPathRule.add("path", result);

		pathRule = As<Record>(packedPathRule.unpacked(*pEnv));
	}

	void GetText(Record& textRule, std::shared_ptr<cgl::Context> pEnv)
	{
		PackedRecord packedTextRule = As<PackedRecord>(textRule.packed(*pEnv));
		
		const auto& values = packedTextRule.values;
		
		auto itStr = values.find("str");
		auto itBaseLines = values.find("base");

		CharString str;
		if (itStr != values.end())
		{
			const PackedVal& strVal = itStr->second.value;
			if (auto strOpt = AsOpt<CharString>(strVal))
			{
				str = strOpt.value();
			}
			else CGL_Error("不正な式です");
		}

		auto factory = gg::GeometryFactory::create();
		gg::LineString* ls;
		Path path;

		boost::optional<PackedRecord> baseLineRecordOpt;
		if (itBaseLines != values.end())
		{
			const PackedVal& baseLineRecordVal = itBaseLines->second.value;
			if (auto baseLineRecordExistOpt = AsOpt<PackedRecord>(baseLineRecordVal))
			{
				baseLineRecordOpt = baseLineRecordExistOpt.value();
			}
			else CGL_Error("不正な式です");
			
			if (baseLineRecordOpt.value().type != RecordType::Path)
				CGL_Error("テキストのベースラインにはpath以外指定できません");

			const PackedRecord& baseLineRecord = baseLineRecordOpt.value();
			auto itPath = baseLineRecord.values.find("path");
			if (itPath == baseLineRecord.values.end())
				CGL_Error("不正な式です");

			const PackedVal& pathVal = itPath->second.value;
			if (auto pathRecordOpt = AsOpt<PackedRecord>(pathVal))
			{
				const PackedRecord& pathRecord = pathRecordOpt.value();

				path = std::move(ReadPathPacked(pathRecordOpt.value()));
			}
			else CGL_Error("不正な式です");

			ls = factory->createLineString(path.cs.get());
		}

		FontBuilder font;
		//cgl::FontBuilder font("c:/windows/fonts/font_1_kokumr_1.00_rls.ttf");
		
		std::u32string string = str.toString();
		double offsetHorizontal = 0;

		PackedList resultCharList;
		if (baseLineRecordOpt)
		{
			const auto& cs = path.cs;
			const auto& distances = path.distances;

			for (size_t i = 0; i < string.size(); ++i)
			{
				std::vector<gg::Geometry*> result;

				const int codePoint = static_cast<int>(string[i]);

				double offsetX = 0;
				double offsetY = 0;

				double angle = 0;

				auto it = std::upper_bound(distances.begin(), distances.end(), offsetHorizontal);
				if (it == distances.end())
				{
					const double innerDistance = offsetHorizontal - distances[distances.size() - 2];

					Eigen::Vector2d p0(cs->getAt(cs->size() - 2).x, cs->getAt(cs->size() - 2).y);
					Eigen::Vector2d p1(cs->getAt(cs->size() - 1).x, cs->getAt(cs->size() - 1).y);

					const Eigen::Vector2d v = (p1 - p0);
					const double currentLineLength = sqrt(v.dot(v));
					const double progress = innerDistance / currentLineLength;

					const Eigen::Vector2d targetPos = p0 + v * progress;
					offsetX = targetPos.x();
					offsetY = targetPos.y();

					const auto n = v.normalized();
					angle = rad2deg * atan2(n.y(), n.x());
				}
				else
				{
					const int lineIndex = std::distance(distances.begin(), it) - 1;
					const double innerDistance = offsetHorizontal - distances[lineIndex];

					Eigen::Vector2d p0(cs->getAt(lineIndex).x, cs->getAt(lineIndex).y);
					Eigen::Vector2d p1(cs->getAt(lineIndex + 1).x, cs->getAt(lineIndex + 1).y);

					const Eigen::Vector2d v = (p1 - p0);
					const double currentLineLength = sqrt(v.dot(v));
					const double progress = innerDistance / currentLineLength;

					const Eigen::Vector2d targetPos = p0 + v * progress;
					offsetX = targetPos.x();
					offsetY = targetPos.y();

					const auto n = v.normalized();
					angle = rad2deg * atan2(n.y(), n.x());
				}

				const auto characterPolygon = font.makePolygon(codePoint, 5, 0, 0);
				result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());

				offsetHorizontal += font.glyphWidth(codePoint);

				resultCharList.add(MakeRecord(
					"char", GetPackedShapesFromGeos(result),
					"pos", MakeRecord("x", offsetX, "y", offsetY),
					"angle", angle
				));
			}
		}
		else
		{
			for (size_t i = 0; i < string.size(); ++i)
			{
				std::vector<gg::Geometry*> result;

				const int codePoint = static_cast<int>(string[i]);
				const auto characterPolygon = font.makePolygon(codePoint, 5, offsetHorizontal);
				result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());

				offsetHorizontal += font.glyphWidth(codePoint);

				resultCharList.add(GetPackedShapesFromGeos(result));
			}
		}

		packedTextRule.add("text", resultCharList);

		textRule = As<Record>(packedTextRule.unpacked(*pEnv));
	}

	void GetShapePath(Record & pathRule, std::shared_ptr<cgl::Context> pEnv)
	{
		const auto& values = pathRule.values;
		
		auto itStr = values.find("shapes");
		auto itBaseLines = values.find("base");

		List shapeList;
		if (itStr != values.end())
		{
			const Val evaluated = pEnv->expand(itStr->second);
			if (!IsType<List>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			shapeList = As<List>(evaluated);
		}

		auto factory = gg::GeometryFactory::create();
		gg::LineString* ls;
		gg::CoordinateArraySequence cs;
		std::vector<double> distances;

		boost::optional<Record> baseLineRecord;
		if (itBaseLines != values.end())
		{
			const Val evaluated = pEnv->expand(itBaseLines->second);
			if (!IsType<Record>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			baseLineRecord = As<Record>(evaluated);

			if (baseLineRecord.value().type != RecordType::Path)
			{
				CGL_Error("不正な式です");
				return;
			}

			const Record& record = baseLineRecord.value();

			auto itPath = record.values.find("path");
			if (itPath == record.values.end())
			{
				CGL_Error("不正な式です");
				return;
			}
			const Val pathVal = pEnv->expand(itPath->second);
			if (!IsType<Record>(pathVal))
			{
				CGL_Error("不正な式です");
				return;
			}
			const Record& pathRecord = As<Record>(pathVal);

			auto itLine = pathRecord.values.find("line");
			if (itLine == pathRecord.values.end())
			{
				CGL_Error("不正な式です");
				return;
			}
			const Val lineVal = pEnv->expand(itLine->second);
			if (!IsType<List>(lineVal))
			{
				CGL_Error("不正な式です");
				return;
			}

			const auto vs = As<List>(lineVal).data;
			for (const auto v : vs)
			{
				const Val vertex = pEnv->expand(v);
				
				const auto& pos = As<Record>(vertex);
				const Val xval = pEnv->expand(pos.values.find("x")->second);
				const Val yval = pEnv->expand(pos.values.find("y")->second);
				const double x = IsType<int>(xval) ? static_cast<double>(As<int>(xval)) : As<double>(xval);
				const double y = IsType<int>(yval) ? static_cast<double>(As<int>(yval)) : As<double>(yval);

				cs.add(gg::Coordinate(x, y));
			}

			distances.push_back(0);
			for (int i = 1; i < cs.size(); ++i)
			{
				const double newDistance = cs[i - 1].distance(cs[i]);
				distances.push_back(distances.back() + newDistance);
			}

			ls = factory->createLineString(cs);
		}

		/*TODO: 実装する*/
	}

	double ShapeArea(const Val& lhs, std::shared_ptr<cgl::Context> pEnv)
	{
		if (!IsType<Record>(lhs) && !IsType<List>(lhs))
		{
			CGL_Error("不正な式です");
			return{};
		}

		std::vector<gg::Geometry*> geometries = GeosFromRecord(lhs, pEnv);

		double area = 0.0;
		for (gg::Geometry* geometry : geometries)
		{
			area += geometry->getArea();
		}

		return area;
	}

	List GetDefaultFontString(const std::string& str, std::shared_ptr<cgl::Context> pEnv)
	{
		if (str.empty() || str.front() == ' ')
		{
			return{};
		}

		cgl::FontBuilder builder;
		//cgl::FontBuilder builder("c:/windows/fonts/font_1_kokumr_1.00_rls.ttf");

		const auto result = builder.textToPolygon(str, 5);
		return GetShapesFromGeos(result, pEnv);
	}

	PackedRecord BuildPath(const PackedList& passes, int numOfPoints, const PackedList& obstacleList)
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
					const auto v = ReadVec2Packed(posRecordOpt.value());
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

		std::vector<gg::Geometry*> obstacles = GeosFromRecordPacked(obstacleList);

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

		const gg::Coordinate beginPos(points.front().x(), points.front().y());
		const gg::Coordinate endPos(points.back().x(), points.back().y());

		points.erase(points.end() - 1);
		points.erase(points.begin());
		originalPoints.erase(originalPoints.end() - 1);
		originalPoints.erase(originalPoints.begin());

		PackedRecord result;
		PackedList polygonList;


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

				appendCoord(polygonList, beginPos.x, beginPos.y);
				for (int i = 0; i < halfindex; ++i)
				{
					const double angle = resultxs[i];
					const double dx = rodLength * cos(angle);
					const double dy = rodLength * sin(angle);
					lastX += dx;
					lastY += dy;
					appendCoord(polygonList, lastX, lastY);
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
				appendCoord(polygonList, it->x(), it->y());
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
		result.type = RecordType::Path;

		return result;
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
		for (size_t i = 0; i < resultLines.size(); ++i)
		{
			const auto line = resultLines[i];
			for (size_t p = 0; p < line->getSize(); ++p)
			{
				appendCoord(polygonList, line->getX(p), line->getY(p));
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

	PackedRecord GetOffsetPath(const PackedRecord& packedPathRecord, double offset)
	{
		Path originalPath = std::move(ReadPathPacked(packedPathRecord));

		return GetOffsetPathImpl(originalPath, offset);
	}

	PackedRecord GetFunctionPath(std::shared_ptr<Context> pContext, const FuncVal& function, double beginValue, double endValue, int numOfPoints)
	{
		Eval evaluator(pContext);

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

		PackedRecord pathRecord;
		PackedList polygonList;

		const double unitX = (endValue - beginValue) / (numOfPoints - 1);
		for (int i = 0; i < numOfPoints; ++i)
		{
			const double x = beginValue + unitX * i;
			const Val value = pContext->expand(evaluator.callFunction(function, { pContext->makeTemporaryValue(x) }));
			if (!IsType<double>(value))
			{
				CGL_Error("Error");
			}
			appendCoord(polygonList, x, As<double>(value));
		}

		pathRecord.add("line", polygonList);

		return pathRecord;
	}

	PackedRecord BuildText(const CharString& str, const PackedRecord& packedPathRecord)
	{
		Path path = packedPathRecord.values.empty() ? Path() : std::move(ReadPathPacked(packedPathRecord));

		auto factory = gg::GeometryFactory::create();

		FontBuilder font;
		//cgl::FontBuilder font("c:/windows/fonts/font_1_kokumr_1.00_rls.ttf");

		std::u32string string = str.toString();
		double offsetHorizontal = 0;

		PackedList resultCharList;
		
		if(!path.empty())
		{
			const auto& cs = path.cs;
			const auto& distances = path.distances;
			
			for (size_t i = 0; i < string.size(); ++i)
			{
				std::vector<gg::Geometry*> result;
				
				const int codePoint = static_cast<int>(string[i]);
				const double currentGlyphWidth = font.glyphWidth(codePoint);

				const auto offsetLeft = path.getOffset(offsetHorizontal);
				const auto offsetCenter = path.getOffset(offsetHorizontal + currentGlyphWidth * 0.5);

				const auto characterPolygon = font.makePolygon(codePoint, 5, 0, 0);
				result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());

				offsetHorizontal += currentGlyphWidth;

				/*resultCharList.add(MakeRecord(
					"char", GetPackedShapesFromGeos(result),
					"pos", MakeRecord("x", offsetX, "y", offsetY),
					"angle", angle
				));*/
				resultCharList.add(MakeRecord(
					"char", GetPackedShapesFromGeos(result),
					"pos", MakeRecord("x", offsetLeft.x, "y", offsetLeft.y),
					"angle", offsetCenter.angle
				));
			}
		}
		else
		{
			for (size_t i = 0; i < string.size(); ++i)
			{
				std::vector<gg::Geometry*> result;

				const int codePoint = static_cast<int>(string[i]);
				const auto characterPolygon = font.makePolygon(codePoint, 5, offsetHorizontal);
				result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());

				offsetHorizontal += font.glyphWidth(codePoint);

				resultCharList.add(GetPackedShapesFromGeos(result));
			}
		}
		
		PackedRecord result;
		result.add("shapes", resultCharList);
		result.add("str", str);
		//result.add("base", WritePathPacked(path));

		return result;
	}

	PackedList GetShapeOuterPaths(const PackedRecord& shape)
	{
		auto geometries = GeosFromRecordPacked(shape);

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
						appendCoord(polygonList, point->getX(), point->getY());
					}
				}
				else
				{
					for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Point* point = exterior->getPointN(p);
						appendCoord(polygonList, point->getX(), point->getY());
					}
				}

				pathRecord.add("line", polygonList);
				pathList.add(pathRecord);
			}
		}

		return pathList;
	}

	PackedList GetShapePaths(const PackedRecord& shape)
	{
		auto geometries = GeosFromRecordPacked(shape);

		PackedList pathList;

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
						appendCoord(polygonList, point->getX(), point->getY());
					}
				}
				else
				{
					for (int p = static_cast<int>(exterior->getNumPoints()) - 1; 0 <= p; --p)
					{
						const gg::Point* point = exterior->getPointN(p);
						appendCoord(polygonList, point->getX(), point->getY());
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

		return pathList;
	}

	PackedRecord GetBoundingBox(const PackedRecord& shape)
	{
		BoundingRect boundingRect = BoundingRectRecordPacked(shape);

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
			"top", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", minY), MakeRecord("x", maxX, "y", minY))),
			"bottom", MakeRecord("line", MakeList(MakeRecord("x", minX, "y", maxY), MakeRecord("x", maxX, "y", maxY)))
		);

		return resultRecord;
	}

	PackedList GetDeformedShape(const PackedRecord& shape, const PackedRecord& targetPathRecord)
	{
		//return PackedList();
		
		const bool debugDraw = false;
		const BoundingRect originalBoundingRect = BoundingRectRecordPacked(shape);

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
		
		auto geometries = GeosFromRecordPacked(shape);
		
		const auto result = deformer.FFD(geometries);
		//return GetPackedShapesFromGeos(result);
		
		packedList.add(GetPackedShapesFromGeos(result));
		
		return packedList;
	}
}
