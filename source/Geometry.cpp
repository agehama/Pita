#include <Pita/Node.hpp>
#include <Pita/Environment.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/FontShape.hpp>
#include <Pita/Geometry.hpp>
#include <Pita/Printer.hpp>

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

	List ShapeDifference(const Evaluated& lhs, const Evaluated& rhs, std::shared_ptr<cgl::Environment> pEnv)
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

	List ShapeBuffer(const Evaluated & shape, const Evaluated & amount, std::shared_ptr<cgl::Environment> pEnv)
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

	void GetPath(Record& pathRule, std::shared_ptr<cgl::Environment> pEnv)
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
				const Eigen::Vector2d diff = points[index] - points[index + 1];
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
				points.insert(points.begin() + insertIndex, mid);
			}
		}

		/*gg::CoordinateArraySequence cs;
		for (int i = 0; i + 1 < points.size(); ++i)
		{
			cs.add(gg::Coordinate(points[i].x(), points[i].y()));
		}

		gg::LineString* ls = factory->createLineString(cs);*/

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

		Record result;
		List polygonList;

		/*
		PathConstraintProblem  constraintProblem;
		constraintProblem.evaluator = [&](const PathConstraintProblem::TVector& v)->double
		{
			gg::CoordinateArraySequence cs2;
			for (int i = 0; i*2 < v.size(); ++i)
			{
				cs2.add(gg::Coordinate(v[i * 2 + 0], v[i * 2 + 1]));
			}

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
		for (int i = 0; i*2 < x0s.size(); ++i)
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
			for (int i = 0; i * 2 < N; ++i)
			{
				cs2.add(gg::Coordinate(x[i * 2 + 0], x[i * 2 + 1]));
			}

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
		for (int i = 0; i < points.size(); ++i)
		{
			x0[i * 2 + 0] = points[i].x();
			x0[i * 2 + 1] = points[i].y();

			std::cout << i << ": (" << points[i].x() << ", " << points[i].y() << ")\n";
		}

		const double sigma = 0.1;

		const int lambda = 100;

		libcmaes::CMAParameters<> cmaparams(x0, sigma, lambda);
		libcmaes::CMASolutions cmasols = libcmaes::cmaes<>(func, cmaparams);
		auto resultxs = cmasols.best_candidate().get_x();

		for (int i = 0; i * 2 < resultxs.size(); ++i)
		{
			appendCoord(polygonList, resultxs[i * 2 + 0], resultxs[i * 2 + 1]);
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

		/*
		points: 10
        passes: [rect.top(), {x: 300, y:400}, rect.bottom()]
        circumvents: [buffer(rect, 10)]
		*/
	}

	double ShapeArea(const Evaluated& lhs, std::shared_ptr<cgl::Environment> pEnv)
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

	List GetDefaultFontString(const std::string& str, std::shared_ptr<cgl::Environment> pEnv)
	{
		if (str.empty() || str.front() == ' ')
		{
			return{};
		}

		cgl::FontBuilder builder("c:/windows/fonts/font_1_kokumr_1.00_rls.ttf");
		const auto result = builder.textToPolygon(str, 5);
		return GetShapesFromGeos(result, pEnv);
	}
}
