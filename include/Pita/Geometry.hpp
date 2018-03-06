#pragma once
#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	List ShapeDifference(const Val& lhs, const Val& rhs, std::shared_ptr<cgl::Context> pEnv);

	List ShapeBuffer(const Val& shape, const Val& amount, std::shared_ptr<cgl::Context> pEnv);

	//std::vector<Eigen::Vector2d> GetPath(const Record& pathRule, std::shared_ptr<cgl::Context> pEnv);
	void GetPath(Record& pathRule, std::shared_ptr<cgl::Context> pEnv);

	void GetText(Record& pathRule, std::shared_ptr<cgl::Context> pEnv);

	void GetShapePath(Record& pathRule, std::shared_ptr<cgl::Context> pEnv);

	double ShapeArea(const Val& lhs, std::shared_ptr<cgl::Context> pEnv);

	List GetDefaultFontString(const std::string& str, std::shared_ptr<cgl::Context> pEnv);


	PackedRecord BuildPath(const PackedList& passes, int numOfPoints = 10, const PackedList& obstacleList = PackedList());

	PackedRecord GetOffsetPath(const PackedRecord& pathRule, double offset);

	PackedRecord BuildText(const CharString& str, const PackedRecord& basePath = PackedRecord());
}
