#pragma once
#include <string>
#include <boost/regex/pending/unicode_iterator.hpp>

namespace cgl
{
	const double pi = 3.1415926535;
	const double deg2rad = pi / 180.0;
	const double rad2deg = 180.0 / pi;

	inline std::string AsUtf8(const std::u32string& input) {
		return std::string(
			boost::u32_to_u8_iterator<std::u32string::const_iterator>(input.begin()),
			boost::u32_to_u8_iterator<std::u32string::const_iterator>(input.end()));
	}

	inline std::u32string AsUtf32(const std::string& input) {
		return std::u32string(
			boost::u8_to_u32_iterator<std::string::const_iterator>(input.begin()),
			boost::u8_to_u32_iterator<std::string::const_iterator>(input.end()));
	}
}
