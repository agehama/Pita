#pragma once
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	PackedList ShapeDiff(const PackedVal& lhs, const PackedVal& rhs);

	PackedList ShapeUnion(const PackedVal& lhs, const PackedVal& rhs);

	PackedList ShapeIntersect(const PackedVal& lhs, const PackedVal& rhs);

	PackedList ShapeSymDiff(const PackedVal& lhs, const PackedVal& rhs);


	PackedList ShapeBuffer(const PackedVal& shape, const PackedVal& amount);

	double ShapeArea(const PackedVal& lhs);

	double ShapeDistance(const PackedVal& lhs, const PackedVal& rhs);

	PackedRecord ShapeClosestPoints(const PackedVal& lhs, const PackedVal& rhs);

	PackedList GetDefaultFontString(const std::string& str);


	PackedRecord BuildPath(const PackedList& passes, int numOfPoints = 10, const PackedList& obstacleList = PackedList());

	PackedRecord GetOffsetPath(const PackedRecord& pathRule, double offset);

	PackedRecord GetSubPath(const PackedRecord& pathRule, double offsetBegin, double offsetEnd);

	PackedRecord GetFunctionPath(std::shared_ptr<Context> pContext, const LocationInfo& info, const FuncVal& function, double beginValue, double endValue, int numOfPoints);

	PackedRecord BuildText(const CharString& str, const PackedRecord& basePath = PackedRecord(), const CharString& fontPath = CharString());

	PackedList GetShapeOuterPaths(const PackedRecord& shape);

	PackedList GetShapePaths(const PackedRecord& shape);

	PackedRecord GetBoundingBox(const PackedRecord& shape);

	PackedList GetBaseLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPath);

	PackedList GetCenterLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPath);

	PackedList GetDeformedPathShape(const PackedRecord& shape, const PackedRecord& p0, const PackedRecord& p1, const PackedRecord& targetPath);
}
