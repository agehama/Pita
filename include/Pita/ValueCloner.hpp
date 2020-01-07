#pragma once
#pragma warning(disable:4996)

#include "Node.hpp"
#include "Context.hpp"

namespace cgl
{
	Val Clone(std::shared_ptr<Context> pEnv, const Val& value, const LocationInfo& info);
}
