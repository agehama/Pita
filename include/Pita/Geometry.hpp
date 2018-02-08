#pragma once

#include "Node.hpp"
#include "Environment.hpp"
#include "Vectorizer.hpp"
#include "FontShape.hpp"

namespace cgl
{
	List ShapeDifference(const Evaluated& lhs, const Evaluated& rhs, std::shared_ptr<cgl::Environment> pEnv);

	List ShapeBuffer(const Evaluated& shape, const Evaluated& amount, std::shared_ptr<cgl::Environment> pEnv);

	//std::vector<Eigen::Vector2d> GetPath(const Record& pathRule, std::shared_ptr<cgl::Environment> pEnv);
	void GetPath(Record& pathRule, std::shared_ptr<cgl::Environment> pEnv);

	void GetText(Record& pathRule, std::shared_ptr<cgl::Environment> pEnv);

	double ShapeArea(const Evaluated& lhs, std::shared_ptr<cgl::Environment> pEnv);

	List GetDefaultFontString(const std::string& str, std::shared_ptr<cgl::Environment> pEnv);
}
