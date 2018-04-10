#pragma once
#pragma warning(disable:4996)
#include "Vectorizer.hpp"

struct stbtt_fontinfo;

namespace cgl
{
	static unsigned char current_buffer[1 << 25];

	bool IsClockWise(const Vector<Eigen::Vector2d>& closedPath);

	class FontBuilder
	{
	public:
		FontBuilder();
		FontBuilder(const std::string& fontPath);
		~FontBuilder();

		std::vector<gg::Geometry*> makePolygon(int codePoint, int quality = 1, double offsetX = 0, double offsetY = 0);

		std::vector<gg::Geometry*> textToPolygon(const std::string& str, int quality = 1);

		double glyphWidth(int codePoint);

		int ascent()const
		{
			return std::max(ascent1, ascent2);
		}

		int descent()const
		{
			return std::min(descent1, descent2);
		}

		int lineGap()const
		{
			return std::max(lineGap1, lineGap2);
		}

	private:
		void checkClockWise();

		std::string fontDataRawEN, fontDataRawJP;
		stbtt_fontinfo *fontInfo1 = nullptr, *fontInfo2 = nullptr;
		int ascent1 = 0, descent1 = 0, lineGap1 = 0;
		int ascent2 = 0, descent2 = 0, lineGap2 = 0;
		bool clockWisePolygons;
	};
}
