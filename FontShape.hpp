#pragma once

#pragma warning(disable:4996)
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include "Vectorizer.hpp"

#include "Node.hpp"
#include "Environment.hpp"
#include "Vectorizer.hpp"

namespace cgl
{
	unsigned char current_buffer[1 << 25];

	bool IsClockWise(const Vector<Eigen::Vector2d>& closedPath)
	{
		double sum = 0;

		for (int i = 0; i < closedPath.size(); ++i)
		{
			const auto& p1 = closedPath[i];
			const auto& p2 = closedPath[(i + 1) % closedPath.size()];

			sum += (p2.x() - p1.x())*(p2.y() + p1.y());
		}

		return sum < 0.0;
	}

	//最後の点は含めない
	void GetQuadraticBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, int n)
	{
		for (int i = 0; i < n; ++i)
		{
			const double t = 1.0*i / n;
			output.push_back(p0*(1.0 - t)*(1.0 - t) + p1*2.0*(1.0 - t)*t + p2*t*t);
		}
	}

	class FontBuilder
	{
	public:
		FontBuilder(const std::string& fontPath)
		{
			fread(current_buffer, 1, 1 << 25, fopen(fontPath.c_str(), "rb"));
			stbtt_InitFont(&fontInfo, current_buffer, stbtt_GetFontOffsetForIndex(current_buffer, 0));
		}

		std::vector<gg::Geometry*> makePolygon(int character, int quality = 1)
		{
			const auto vec2 = [](short x, short y) {return Eigen::Vector2d(0.1*x, -0.1*y); };

			const int glyphIndex = stbtt_FindGlyphIndex(&fontInfo, character);

			stbtt_vertex* pv;
			const int verticesNum = stbtt_GetGlyphShape(&fontInfo, glyphIndex, &pv);

			using Vertices = Vector<Eigen::Vector2d>;

			std::vector<gg::Geometry*> currentPolygons;
			std::vector<gg::Geometry*> currentHoles;

			int polygonBeginIndex = 0;
			while (polygonBeginIndex < verticesNum)
			{
				stbtt_vertex* nextPolygonBegin = std::find_if(pv + polygonBeginIndex + 1, pv + verticesNum, [](const stbtt_vertex& p) {return p.type == STBTT_vmove; });
				const int nextPolygonFirstIndex = std::distance(pv, nextPolygonBegin);
				const int currentPolygonLastIndex = nextPolygonFirstIndex - 1;

				stbtt_vertex* vertex = pv + polygonBeginIndex;

				const auto vec2 = [](short x, short y) {return Eigen::Vector2d(0.1*x, -0.1*y); };
				const auto isEq = [](const stbtt_vertex& p1, const stbtt_vertex& p2) {return p1.x == p2.x && p1.y == p2.y; };

				Vector<Eigen::Vector2d> points;

				for (int vi = polygonBeginIndex; vi < currentPolygonLastIndex; ++vi)
				{
					stbtt_vertex* v1 = pv + vi;
					stbtt_vertex* v2 = pv + vi + 1;

					const Eigen::Vector2d p1 = vec2(v1->x, v1->y);
					const Eigen::Vector2d p2 = vec2(v2->x, v2->y);
					const Eigen::Vector2d pc = vec2(v2->cx, v2->cy);

					//Line
					if (v2->type == STBTT_vline)
					{
						points.push_back(p1);
					}
					//Quadratic Bezier
					else if (v2->type == STBTT_vcurve)
					{
						GetQuadraticBezier(points, p1, pc, p2, quality);
					}
				}

				if (IsClockWise(points))
				{
					currentPolygons.push_back(ToPolygon(points));
				}
				else
				{
					currentHoles.push_back(ToPolygon(points));
				}

				polygonBeginIndex = nextPolygonFirstIndex;
			}

			gg::GeometryFactory::unique_ptr factory = gg::GeometryFactory::create();

			if (currentPolygons.empty())
			{
				return{};
			}
			else if (currentHoles.empty())
			{
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
						
						if (erodeGeometry->getGeometryTypeId() == geos::geom::GEOS_MULTIPOLYGON)
						{
							currentPolygons.erase(currentPolygons.begin() + s);

							const gg::MultiPolygon* polygons = dynamic_cast<const gg::MultiPolygon*>(erodeGeometry);
							for (int i = 0; i < polygons->getNumGeometries(); ++i)
							{
								currentPolygons.insert(currentPolygons.begin() + s, polygons->getGeometryN(i)->clone());
							}

							erodeGeometry = currentPolygons[s];
						}
					}

					currentPolygons[s] = erodeGeometry;
				}

				return currentPolygons;
			}
		}

	private:
		stbtt_fontinfo fontInfo;
	};
}

