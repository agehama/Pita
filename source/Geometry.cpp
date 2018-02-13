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

namespace cgl
{
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

	List ShapeDifference(const Evaluated& lhs, const Evaluated& rhs, std::shared_ptr<cgl::Context> pEnv)
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

	List ShapeBuffer(const Evaluated & shape, const Evaluated & amount, std::shared_ptr<cgl::Context> pEnv)
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
			const Evaluated evaluated = pEnv->expand(itPoints->second);
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

			const Evaluated evaluated = pEnv->expand(itPasses->second);
			if (!IsType<List>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			List passShapes = As<List>(evaluated);

			for (Address address : passShapes.data)
			{
				const Evaluated pointEvaluated = pEnv->expand(address);
				if (!IsType<Record>(pointEvaluated))
				{
					CGL_Error("不正な式です");
					return;
				}
				const auto& pos = As<Record>(pointEvaluated);
				const Evaluated xval = pEnv->expand(pos.values.find("x")->second);
				const Evaluated yval = pEnv->expand(pos.values.find("y")->second);
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
			const Evaluated evaluated = pEnv->expand(itCircumvents->second);
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
		const auto& values = pathRule.values;

		auto itPoints = values.find("points");
		auto itPasses = values.find("passes");
		auto itCircumvents = values.find("circumvents");

		int num = 10;
		if (itPoints != values.end())
		{
			const Evaluated evaluated = pEnv->expand(itPoints->second);
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

			const Evaluated evaluated = pEnv->expand(itPasses->second);
			if (!IsType<List>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			List passShapes = As<List>(evaluated);

			for (Address address : passShapes.data)
			{
				const Evaluated pointEvaluated = pEnv->expand(address);
				if (!IsType<Record>(pointEvaluated))
				{
					CGL_Error("不正な式です");
					return;
				}
				const auto& pos = As<Record>(pointEvaluated);
				const Evaluated xval = pEnv->expand(pos.values.find("x")->second);
				const Evaluated yval = pEnv->expand(pos.values.find("y")->second);
				const double x = IsType<int>(xval) ? static_cast<double>(As<int>(xval)) : As<double>(xval);
				const double y = IsType<int>(yval) ? static_cast<double>(As<int>(yval)) : As<double>(yval);

				Eigen::Vector2d v;
				v << x, y;
				points.push_back(v);

				originalPoints.push_back(factory->createPoint(gg::Coordinate(x, y)));
			}
		}

		//double rodLength = 50;
		std::vector<double> angles1(num / 2);
		std::vector<double> angles2(num / 2);

		std::vector<gg::Geometry*> obstacles;
		//List circumvents;
		if (itCircumvents != values.end())
		{
			{
				const Evaluated evaluated = pEnv->expand(itCircumvents->second);
				if (!IsType<List>(evaluated))
				{
					CGL_Error("不正な式です");
					return;
				}

				obstacles = GeosFromRecord(evaluated, pEnv);
			}
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
			const int halfindex = N / 2;

			const double rodLength = abs(x[N - 1]) + 1.0;

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
			//cs3.add(endPos);
			cs3.push_back(Eigen::Vector2d(endPos.x, endPos.y));
			for (int i = 0; i < halfindex; ++i)
			{
				const int currentIndex = i + halfindex;
				const auto& lastPos = cs3.back();
				const double angle = x[currentIndex];
				const double dx = rodLength * cos(angle);
				const double dy = rodLength * sin(angle);
				//cs3.add(gg::Coordinate(lastPos.x + dx, lastPos.y + dy));

				const double lastX = lastPos.x();
				const double lastY = lastPos.y();
				cs3.push_back(Eigen::Vector2d(lastX + dx, lastY + dy));
			}
			const auto ik2Last = cs3.back();
			
			for (auto it = cs3.rbegin(); it != cs3.rend(); ++it)
			{
				cs2.add(gg::Coordinate(it->x(), it->y()));
			}

			gg::LineString* ls2 = factory->createLineString(cs2);

			double pathLength = ls2->getLength();

			const double dx = ik1Last.x - ik2Last.x();
			const double dy = ik1Last.y - ik2Last.y();
			const double distanceSq = dx*dx + dy*dy;

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
				else if (g->getGeometryTypeId() == gg::GEOS_MULTILINESTRING)
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

			const double totalCost = pathLength + distanceSq + penalty*penalty + penalty2*penalty2;
			//std::cout << std::string("path cost: ") << ToS(pathLength, 10) << ", " << ToS(penalty, 10) << ", " << ToS(penalty2, 10) << " => " << ToS(totalCost, 15) << "\n";

			//const double totalCost = penalty2;
			//std::cout << std::string("path cost: ") << ToS(penalty2, 17) << "\n";
			return totalCost;
		};

		std::vector<double> x0(num + 1, 0.0);
		x0.back() = 10;
		
		const double sigma = 0.1;

		const int lambda = 100;

		libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda, 1);
		libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
		auto resultxs = cmasols.best_candidate().get_x();

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
				//cs2.add(gg::Coordinate(it->x(), it->y()));
				appendCoord(polygonList, it->x(), it->y());
			}
		}

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

	void GetText(Record& textRule, std::shared_ptr<cgl::Context> pEnv)
	{
		const auto& values = textRule.values;

		auto itStr = values.find("str");
		auto itBaseLines = values.find("base");

		CharString str;
		if (itStr != values.end())
		{
			const Evaluated evaluated = pEnv->expand(itStr->second);
			if (!IsType<CharString>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			str = As<CharString>(evaluated);
		}

		auto factory = gg::GeometryFactory::create();
		gg::LineString* ls;
		gg::CoordinateArraySequence cs;
		std::vector<double> distances;

		boost::optional<Record> baseLineRecord;
		if (itBaseLines != values.end())
		{
			const Evaluated evaluated = pEnv->expand(itBaseLines->second);
			if (!IsType<Record>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			baseLineRecord = As<Record>(evaluated);

			if (baseLineRecord.value().type != Record::Path)
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
			const Evaluated pathEvaluated = pEnv->expand(itPath->second);
			if (!IsType<Record>(pathEvaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			const Record& pathRecord= As<Record>(pathEvaluated);

			auto itLine = pathRecord.values.find("line");
			if (itLine == pathRecord.values.end())
			{
				CGL_Error("不正な式です");
				return;
			}
			const Evaluated lineEvaluated = pEnv->expand(itLine->second);
			if (!IsType<List>(lineEvaluated))
			{
				CGL_Error("不正な式です");
				return;
			}

			const auto vs = As<List>(lineEvaluated).data;
			for (const auto v : vs)
			{
				const Evaluated vertex = pEnv->expand(v);
				const auto& pos = As<Record>(vertex);
				const Evaluated xval = pEnv->expand(pos.values.find("x")->second);
				const Evaluated yval = pEnv->expand(pos.values.find("y")->second);
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

		FontBuilder font;
		//cgl::FontBuilder font("c:/windows/fonts/font_1_kokumr_1.00_rls.ttf");
		
		std::u32string string = str.toString();
		double offsetHorizontal = 0;

		List resultCharList;
		if (baseLineRecord)
		{
			for (size_t i = 0; i < string.size(); ++i)
			{
				Record currentCharRecord;

				std::vector<gg::Geometry*> result;

				const int codePoint = static_cast<int>(string[i]);

				double offsetX = 0;
				double offsetY = 0;

				double angle = 0;

				auto it = std::upper_bound(distances.begin(), distances.end(), offsetHorizontal);
				if (it == distances.end())
				{
					const double innerDistance = offsetHorizontal - distances[distances.size() - 2];

					Eigen::Vector2d p0(cs[cs.size() - 2].x, cs[cs.size() - 2].y);
					Eigen::Vector2d p1(cs[cs.size() - 1].x, cs[cs.size() - 1].y);

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

					Eigen::Vector2d p0(cs[lineIndex].x, cs[lineIndex].y);
					Eigen::Vector2d p1(cs[lineIndex + 1].x, cs[lineIndex + 1].y);

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

				Record posRecord;
				posRecord.append("x", pEnv->makeTemporaryValue(offsetX));
				posRecord.append("y", pEnv->makeTemporaryValue(offsetY));

				currentCharRecord.append("char", pEnv->makeTemporaryValue(GetShapesFromGeos(result, pEnv)));
				currentCharRecord.append("pos", pEnv->makeTemporaryValue(posRecord));

				currentCharRecord.append("angle", pEnv->makeTemporaryValue(angle));

				resultCharList.append(pEnv->makeTemporaryValue(currentCharRecord));
			}
		}
		else
		{
			for (size_t i = 0; i < string.size(); ++i)
			{
				std::vector<gg::Geometry*> result;

				const int codePoint = static_cast<int>(string[i]);
				const auto characterPolygon = font.makePolygon(codePoint, 1, offsetHorizontal);
				result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());

				offsetHorizontal += font.glyphWidth(codePoint);

				resultCharList.append(pEnv->makeTemporaryValue(GetShapesFromGeos(result, pEnv)));
			}
		}

		textRule.append("text", pEnv->makeTemporaryValue(resultCharList));
	}

	void GetShapePath(Record & pathRule, std::shared_ptr<cgl::Context> pEnv)
	{
		const auto& values = pathRule.values;

		auto itStr = values.find("shapes");
		auto itBaseLines = values.find("base");

		List shapeList;
		if (itStr != values.end())
		{
			const Evaluated evaluated = pEnv->expand(itStr->second);
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
			const Evaluated evaluated = pEnv->expand(itBaseLines->second);
			if (!IsType<Record>(evaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			baseLineRecord = As<Record>(evaluated);

			if (baseLineRecord.value().type != Record::Path)
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
			const Evaluated pathEvaluated = pEnv->expand(itPath->second);
			if (!IsType<Record>(pathEvaluated))
			{
				CGL_Error("不正な式です");
				return;
			}
			const Record& pathRecord = As<Record>(pathEvaluated);

			auto itLine = pathRecord.values.find("line");
			if (itLine == pathRecord.values.end())
			{
				CGL_Error("不正な式です");
				return;
			}
			const Evaluated lineEvaluated = pEnv->expand(itLine->second);
			if (!IsType<List>(lineEvaluated))
			{
				CGL_Error("不正な式です");
				return;
			}

			const auto vs = As<List>(lineEvaluated).data;
			for (const auto v : vs)
			{
				const Evaluated vertex = pEnv->expand(v);
				const auto& pos = As<Record>(vertex);
				const Evaluated xval = pEnv->expand(pos.values.find("x")->second);
				const Evaluated yval = pEnv->expand(pos.values.find("y")->second);
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

	double ShapeArea(const Evaluated& lhs, std::shared_ptr<cgl::Context> pEnv)
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
}
