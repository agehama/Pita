#pragma once

#include "Node.hpp"
#include "Environment.hpp"
#include "Vectorizer.hpp"

namespace cgl
{
	inline List ShapeDifference(const Evaluated& lhs, const Evaluated& rhs, std::shared_ptr<cgl::Environment> pEnv)
	{
		std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
		if (!IsType<Record>(lhs) || !IsType<Record>(rhs))
		{
			CGL_Error("不正な式です");
			return{};
		}

		std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

		CGL_READ_VERTS_REV = false;
		std::vector<gg::Geometry*> lhsPolygon = GeosFromRecord(lhs, pEnv);
		std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

		//CGL_READ_VERTS_REV = true;
		std::vector<gg::Geometry*> rhsPolygon = GeosFromRecord(rhs, pEnv);
		std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

		CGL_READ_VERTS_REV = false;

		gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();

		std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

		if (lhsPolygon.empty())
		{
			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
			return {};
		}
		else if (rhsPolygon.empty())
		{
			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
			return GetShapesFromGeos(lhsPolygon, pEnv);
		}
		else
		{
			gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();

			std::vector<gg::Geometry*> resultGeometries;
			//gg::Geometry* resultGeometry = factory->createEmptyGeometry();

			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
			for (int s = 0; s < lhsPolygon.size(); ++s)
			{
				//std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
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
					//std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

					if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_POLYGON)
					{
						std::cout << "ok" << std::endl;
					}
					else if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
					{
						lhsPolygon.erase(lhsPolygon.begin() + s);

						const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);
						std::cout << "erodeGeometry is MultiPolygon(size: " << polygons->getNumGeometries() << ")" << std::endl;

						for (int i = 0; i < polygons->getNumGeometries(); ++i)
						{
							std::cout << "MultiPolygon[" << i << "]: " << polygons->getGeometryN(i)->getGeometryType() << std::endl;
							lhsPolygon.insert(lhsPolygon.begin() + s, polygons->getGeometryN(i)->clone());
						}

						erodeGeometry = lhsPolygon[s];
						//std::cout << __LINE__ << lhsPolygon[s]->getGeometryType() << std::endl;
					}
					else if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_GEOMETRYCOLLECTION)
					{
						const gg::GeometryCollection* geometries = dynamic_cast<const gg::GeometryCollection*>(erodeGeometry);
						std::cout << "erodeGeometry is GeometryCollection(size: " << geometries->getNumGeometries() << ")" << std::endl;
						for (int i = 0; i < geometries->getNumGeometries(); ++i)
						{
							std::cout << "GeometryCollection[" << i << "]: " << geometries->getGeometryN(i)->getGeometryType() << std::endl;
							//lhsPolygon.insert(lhsPolygon.begin() + s, polygons->getGeometryN(i)->clone());
						}
					}
					else
					{
						//std::cout << __FUNCTION__ << " Differenceの結果が予期せぬデータ形式" << __LINE__ << std::endl;
						std::cout << " Differenceの結果が予期せぬデータ形式 : " << erodeGeometry->getGeometryType() << std::endl;
					}
				}
				std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;

				resultGeometries.push_back(erodeGeometry);
				//resultGeometry->Union(erodeGeometry);
			}

			std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
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
}

