#pragma once
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	PackedRecord ShapeDiff(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext);

	PackedRecord ShapeUnion(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext);

	PackedRecord ShapeIntersect(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext);

	PackedRecord ShapeSymDiff(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext);


	PackedRecord ShapeBuffer(const PackedVal& shape, const PackedVal& amount, std::shared_ptr<Context> pContext);

	PackedRecord ShapeSubDiv(const PackedVal& shape, int numSubDiv, std::shared_ptr<Context> pContext);

	double ShapeArea(const PackedVal& lhs, std::shared_ptr<Context> pContext);

	double ShapeDistance(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext);

	PackedRecord ShapeClosestPoints(const PackedVal& lhs, const PackedVal& rhs, std::shared_ptr<Context> pContext);

	PackedRecord GetDefaultFontString(const std::string& str);

	PackedRecord BuildPath(const PackedList& passes, std::shared_ptr<Context> pContext, int numOfPoints = 10, const PackedList& obstacleList = PackedList());

	PackedRecord GetBezierPath(const PackedRecord& p0, const PackedRecord& n0, const PackedRecord& p1, const PackedRecord& n1, int numOfPoints);

	PackedRecord GetOffsetPath(const PackedRecord& pathRule, double offset);

	PackedRecord GetSubPath(const PackedRecord& pathRule, double offsetBegin, double offsetEnd);

	PackedRecord GetFunctionPath(std::shared_ptr<Context> pContext, const LocationInfo& info, const FuncVal& function, double beginValue, double endValue, int numOfPoints);

	PackedRecord BuildText(const CharString& str, const PackedRecord& basePath = PackedRecord(), const CharString& fontPath = CharString());

	PackedRecord GetShapeOuterPaths(const PackedRecord& shape, std::shared_ptr<Context> pContext);

	PackedRecord GetShapeInnerPaths(const PackedRecord& shape, std::shared_ptr<Context> pContext);

	PackedRecord GetShapePaths(const PackedRecord& shape, std::shared_ptr<Context> pContext);

	PackedRecord GetBoundingBox(const PackedRecord& shape, std::shared_ptr<Context> pContext);

	PackedRecord GetGlobalShape(const PackedRecord& shape, std::shared_ptr<Context> pContext);

	PackedRecord GetTransformedShape(const PackedRecord& shape, const PackedRecord& pos, const PackedRecord& scale, double angle, std::shared_ptr<Context> pContext);

	PackedRecord GetBaseLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPath, std::shared_ptr<Context> pContext);

	PackedRecord GetCenterLineDeformedShape(const PackedRecord& shape, const PackedRecord& targetPath, std::shared_ptr<Context> pContext);

	PackedRecord GetDeformedPathShape(const PackedRecord& shape, const PackedRecord& p0, const PackedRecord& p1, const PackedRecord& targetPath, std::shared_ptr<Context> pContext);

	PackedRecord ShapeLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext);
	PackedRecord ShapeRight(const PackedRecord& shape, std::shared_ptr<Context> pContext);
	PackedRecord ShapeTop(const PackedRecord& shape, std::shared_ptr<Context> pContext);
	PackedRecord ShapeBottom(const PackedRecord& shape, std::shared_ptr<Context> pContext);

	PackedRecord ShapeTopLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext);
	PackedRecord ShapeTopRight(const PackedRecord& shape, std::shared_ptr<Context> pContext);
	PackedRecord ShapeBottomLeft(const PackedRecord& shape, std::shared_ptr<Context> pContext);
	PackedRecord ShapeBottomRight(const PackedRecord& shape, std::shared_ptr<Context> pContext);
}
