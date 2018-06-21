#pragma once
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	PackedRecord ShapeDiff(const PackedVal& lhs, const PackedVal& rhs);

	PackedRecord ShapeUnion(const PackedVal& lhs, const PackedVal& rhs);

	PackedRecord ShapeIntersect(const PackedVal& lhs, const PackedVal& rhs);

	PackedRecord ShapeSymDiff(const PackedVal& lhs, const PackedVal& rhs);


	PackedRecord ShapeBuffer(const PackedVal& shape, const PackedVal& amount);

	PackedRecord ShapeSubDiv(const PackedVal& shape, int numSubDiv);

	double ShapeArea(const PackedVal& lhs);

	double ShapeDistance(const PackedVal& lhs, const PackedVal& rhs);

	PackedRecord ShapeClosestPoints(const PackedVal& lhs, const PackedVal& rhs);

	PackedRecord GetDefaultFontString(const std::string& str);

	PackedRecord BuildPath(const PackedList& passes, int numOfPoints = 10, const PackedList& obstacleList = PackedList());

	PackedRecord GetBezierPath(const PackedRecord& p0, const PackedRecord& n0, const PackedRecord& p1, const PackedRecord& n1, int numOfPoints);

	PackedRecord GetOffsetPath(const PackedRecord& pathRule, double offset);

	PackedRecord GetSubPath(const PackedRecord& pathRule, double offsetBegin, double offsetEnd);

	PackedRecord GetFunctionPath(std::shared_ptr<Context> pContext, const LocationInfo& info, const FuncVal& function, double beginValue, double endValue, int numOfPoints);

	PackedRecord BuildText(const CharString& str, const PackedRecord& basePath = PackedRecord(), const CharString& fontPath = CharString());

	PackedRecord GetShapeOuterPaths(const PackedRecord& shape);

	PackedRecord GetShapeInnerPaths(const PackedRecord& shape);

	PackedRecord GetShapePaths(const PackedRecord& shape);

	PackedRecord GetBoundingBox(const PackedRecord& shape);

	PackedRecord GetGlobalShape(const PackedRecord& shape);

	PackedRecord GetTransformedShape(const PackedRecord& shape, const PackedRecord& pos, const PackedRecord& scale, double angle);

	PackedRecord GetBaseLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPath);

	PackedRecord GetCenterLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPath);

	PackedRecord GetDeformedPathShape(const PackedRecord& shape, const PackedRecord& p0, const PackedRecord& p1, const PackedRecord& targetPath);

	PackedRecord ShapeLeft(const PackedRecord& shape);
	PackedRecord ShapeRight(const PackedRecord& shape);
	PackedRecord ShapeTop(const PackedRecord& shape);
	PackedRecord ShapeBottom(const PackedRecord& shape);

	PackedRecord ShapeTopLeft(const PackedRecord& shape);
	PackedRecord ShapeTopRight(const PackedRecord& shape);
	PackedRecord ShapeBottomLeft(const PackedRecord& shape);
	PackedRecord ShapeBottomRight(const PackedRecord& shape);
}
