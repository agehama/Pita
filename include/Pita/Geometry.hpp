#pragma once

#include "Node.hpp"
#include "Environment.hpp"
#include "Vectorizer.hpp"
#include "FontShape.hpp"

namespace cgl
{
	List ShapeDifference(const Evaluated& lhs, const Evaluated& rhs, std::shared_ptr<cgl::Environment> pEnv);

	double ShapeArea(const Evaluated& lhs, std::shared_ptr<cgl::Environment> pEnv);

	List GetDefaultFontString(const std::string& str, std::shared_ptr<cgl::Environment> pEnv);
}
