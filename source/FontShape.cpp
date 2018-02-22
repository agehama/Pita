#pragma warning(disable:4996)
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>

#include <geos/geom.h>
#include <geos/opBuffer.h>
#include <geos/opDistance.h>

#include <Pita/FontShape.hpp>

namespace cgl
{
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

	double FontSizeToReal(int fontSize)
	{
		const double size = 0.05;
		return fontSize*size;
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

	FontBuilder::FontBuilder()
	{
		//mplus-1m-medium-sub.ttf
		const std::string fontDataEN = "AAEAAAAQAQAABAAARkZUTWXqEdoAACJYAAAAHE9TLzI4xNOTAAABiAAAAFZQZkVkSCRE2AAAInQAAAGsY21hcAAMALEAAAKkAAAALGN2dCABTQVTAAAFaAAAAAxmcGdtD7QvpwAAAtAAAAJlZ2FzcP//AAMAACJQAAAACGdseWYJ4MDgAAAG9AAAF+BoZWFkAY3KvAAAAQwAAAA2aGhlYQh3ALcAAAFEAAAAJGhtdHgPlgtDAAAB4AAAAMJsb2NhAAR+LAAABXQAAAGAbWF4cAGYAVEAAAFoAAAAIG5hbWUQceV4AAAe1AAAA1lwb3N0/4YAMgAAIjAAAAAgcHJlcLDyKxQAAAU4AAAALgABAAAAAQ7Z088n9l8PPPUAKwPoAAAAAM8zPAUAAAAAzzM8BQAA/sAD6AQzAAAACAACAAEAAAAAAAEAAAQz/sAAWgH0AAAAAAPoAAEAAAAAAAAAAAAAAAAAAAACAAEAAABfALwAFQBiAAsAAgABAAIAFgAAAQAALgABAAEAAQH0AfQABQAAAooCvAAAAIwCigK8AAAB4AAxAQIICQILBgkCAgMCAgfgAAL/akf96wAAABIAAAAATSsgIABAACD//wNc/3QAWgQzAUBAEgGf39cAAAAAAWwAIQH0ALYAYgAeAEEACgAUALIAcABcAB4ANwCHAGQAqgApADIASgBGAEYAHgBOADoAQQAoADAArwB9ADIANwA8AD4AGQAMAD4AKgA0AEsAVQAgADIAUAAyAD4AXwAgADcAHgA+AB4AOgA8AC0AMgAWABQAJgAWAEEAggApAGQAFgAtAG4AMgA3AEYAIwAwAEQAKAA8AFgAUwBBADwADwA8ACgAMgAoAGIARAA3ADcAKAAPAC0AIwBIAD4AyABIAC0AAAAAAAEAAwABAAAADAAEACAAAAAEAAQAAQAAAH7//wAAACH////gAAEAAAAAsAAssAATS7AqUFiwSnZZsAAjPxiwBitYPVlLsCpQWH1ZINSwARMuGC2wASwg2rAMKy2wAixLUlhFI1khLbADLGkYILBAUFghsEBZLbAELLAGK1ghIyF6WN0bzVkbS1JYWP0b7VkbIyGwBStYsEZ2WVjdG81ZWVkYLbAFLA1cWi2wBiyxIgGIUFiwIIhcXBuwAFktsAcssSQBiFBYsECIXFwbsABZLbAILBIRIDkvLbAJLCB9sAYrWMQbzVkgsAMlSSMgsAQmSrAAUFiKZYphILAAUFg4GyEhWRuKimEgsABSWDgbISFZWRgtsAossAYrWCEQGxAhWS2wCywg0rAMKy2wDCwgL7AHK1xYICBHI0ZhaiBYIGRiOBshIVkbIVktsA0sEhEgIDkvIIogR4pGYSOKIIojSrAAUFgjsABSWLBAOBshWRsjsABQWLBAZTgbIVlZLbAOLLAGK1g91hghIRsg1opLUlggiiNJILAAVVg4GyEhWRshIVlZLbAPLCMg1iAvsAcrXFgjIFhLUxshsAFZWIqwBCZJI4ojIIpJiiNhOBshISEhWRshISEhIVktsBAsINqwEistsBEsINKwEistsBIsIC+wBytcWCAgRyNGYWqKIEcjRiNhamAgWCBkYjgbISFZGyEhWS2wEywgiiCKhyCwAyVKZCOKB7AgUFg8G8BZLbAULLMAQAFAQkIBS7gQAGMAS7gQAGMgiiCKVVggiiCKUlgjYiCwACNCG2IgsAEjQlkgsEBSWLIAIABDY0KyASABQ2NCsCBjsBllHCFZGyEhWS2wFSywAUNjI7AAQ2MjLQAAALgB/4WwAY0AS7AIUFixAQGOWbFGBitYIbAQWUuwFFJYIbCAWR2wBitcWFmwFCsAAP8kAAACCALaACECeQAAAAAAAABUAAAAfAAAAKgAAAEIAAABgAAAAeQAAAJ0AAACkAAAArwAAALoAAADJAAAA1AAAANsAAADhAAAA5wAAAO8AAAEJAAABEgAAASIAAAE3AAABRgAAAVkAAAF0AAABgAAAAaEAAAG8AAABxQAAAc8AAAHaAAAB5AAAAe4AAAIIAAACKQAAAjYAAAJQAAACYgAAAnUAAAKAAAACigAAAp4AAAKqAAACtQAAAsQAAALRAAAC2QAAAugAAAL0AAADBgAAAxoAAAMyAAADSQAAA2AAAANpAAADdwAAA4EAAAORAAADoAAAA6sAAAO3AAADwAAAA8cAAAPQAAAD2gAAA+AAAAPnAAAEAwAABBkAAAQoAAAEPgAABFEAAARiAAAEfgAABI4AAASbAAAErQAABLkAAATHAAAE4AAABPEAAAT+AAAFEwAABSgAAAU3AAAFUAAABWAAAAVwAAAFegAABYoAAAWYAAAFowAABa8AAAXIAAAFzwAABegAAAX4AACACEAAAEqApoAAwAHAC6xAQAvPLIHBATtMrEGBdw8sgMCBO0yALEDAC88sgUEBO0ysgcGBfw8sgECBO0yMxEhESczESMhAQnox8cCmv1mIQJYAAACALYAAAE+AtoAAwAHAAATMwMjBzUzFbaIEGgQiALa/gfhjIwAAAAAAgBiAeABkgMMAAMABwAAATMDIwMzAyMBEIIYUMiCGlADDP7UASz+1AAAAAACAB4AAAHWAtoAGwAfAAA3NTM3IzUzNzMHMzczBzMVIwczFSMHIzcjByM3EwczNx5GGj1JFGwTPBRsEzZCGzpGF2wWPBZtFpQbPRqxX89gm5ubm2DPX7GxsbEBLs/PAAADAEH/qwG9Ay8AHAAhACYAAAEVHgEVFAcVIzUmJzUWFzUuATU0Njc1MxUWFxUmJwYVFBcTNjU0JwFTOjCyYDQvKTo5MV1VYC4rJpNDQxhCQgJc1B9UQ7cLZWwMIXUlFuAgVT9VYQNkbQoYcB0cCU5GHP7DDVJFHQAFAAr/9gHqAuQAAwALABMAFwAbAAATJRUFEjIWFAYiJjQAMhYUBiImNAIyNCISMjQiKAGk/lwyjFBQjFABBIxQUIxQRExMtExMATbcbtwCHFKcUlKc/qRSnFJSnAEifP3WfAAAAwAU//YB7wLkAAsAEgAvAAATIgYVFBYXPgE1NCYDBhUUMzI3FwYjIiY1NDcnLgE1NDYyFhUUBgcXPQEzFQYHFyPnHiQUIS8hJEY2VCwjNUZUVV9yAy8hYaBkPkhXZwwWQXcCgCMeFzI4KTchHST+nzs5URtSLVxWaWgFTE8mSVxcSTloOo0BsO8VHGgAAAAAAQCyAeABQgMMAAMAABMzAyOykB5UAwz+1AAAAAEAcP9lAZgDAgAJAAATEDczBhEQFyMmcMNlxMRlwwE0ARS6vP7u/u28ugAAAQBc/2UBhAMCAAkAAAEQByM2ERAnMxYBhMNlxMRlwwE0/uu6vAETARK8ugABAB4BTAHWAu4ADgAAEzMHNxcHFwcnByc3JzcXy10DlB2WX0xZWUxflh2UAu6cM1gufDeAgDd8LlgzAAABADcAMgG9AiYACwAAARUzFSMVIzUjNTM1AS+OjmqOjgImzVrNzVrNAAAAAAEAh/9+AV4AqgADAAA3MwMjw5t4X6r+1AAAAAABAGQA+AGQAVMAAwAANzUhFWQBLPhbWwABAKoAAAFKAKoAAwAAMzUzFaqgqqoAAAABACn/2AHLAtoAAwAACQEjAQHL/r9hAUEC2vz+AwIAAAMAMv/2AcIC5AALABQAHQAAEjIeARAOASIuARA2ExYzMj4BNTQnBxMmIyIOARUUtYpULy9UilQvL0kVOyAoGAO7qhU3ICgYAuRBp/7ip0FBpwEep/4EVS5/bjksuAEmSC5/biEAAAAAAQBKAAABaALaAAcAADMRJwc1NzMR+QKtr28CawFgaGb9JgAAAQBGAAABrgLkABQAADcVMxUhNT4BNTQjIgYHNTYzMhUUBsbo/piAc1ceWSVOZLFpYgJgYJDWVmcsI2xEw1/TAAEARv/2AbgC2gAbAAATIRUHFTMyFhUUIyImJzUWMzI2NTQmKwE1NzUjRgFtrwpVVdAvPSxOPjc1M1I3qvAC2mDLA2Zs5A0TbSw+RUouX8cCAAAAAAIAHgAAAdYC2gAKAA8AACUVIzUjNRMzETMVJxEjAxUBhG357XlSvwKXoqKiZQHT/ihgYAEs/tcDAAAAAAEATv/2AbYC2gAYAAATMzYzMhUUIyInNRYzMjU0JiMiByMTIRUjvgIjMqHVTEdQPGsnJyggXQoBRt4Buhnr8iBtLJFKQCYBjmAAAAACADr/9gHEAuQAFwAiAAABMhcVJiMiBgczNjMyFhUUBiMiJjU0PgETMjY1NCYiBhUUFgEaQjw2Oz42BwMxOFpSYWRpXDJfNC8oJVotKQLkFGUcVHYrboKKeI+7oLpK/XFEX1c/RU1fSAAAAAABAEEAAAGzAtoACQAAATUhNSEVAgMjEgFK/vcBcndWdVcCeAJgYP7o/p4BUwAAAAADACj/9gHMAuQACQAgACoAAAEiFRQWFz4BNCYSIiY1NDc1LgE1NDYyFhUUBgcVHgEVFAcyNjU0JwYVFBYBAF4wKisvLjbIbnMtN2a6ZjgxOT/TMTZxYDoCiGAuQA4PQVg0/W5nYYA6AhtkN1RgX1U0WRwCGmhFYQs7OWciImc3PQAAAAACADD/9gG6AuQAGAAjAAAXIic1FjMyNjcjBiMiJjU0NjMyHgEVFA4BAyIGFRQWMzI1NCbaQjw2Oz81BwMxOFZWYWRKUikyXzQvKCcrWikKFGkcUHYrdYWBdziPg6C6SgKPQldaRpxWRwAAAAACAK8AAAFFAjAAAwAHAAATNTMVAzUzFa+WlpYBhqqq/nqqqgACAH3/fgFUAjAAAwAHAAA3MwMjEzUzFbmbeF88lqr+1AIIqqoAAAAAAQAyAAoBuAJOAAcAAAEFFQUVJTUlAbj+uwFF/noBhgHqvQK9ZOtu6wAAAAACADcAkQG9AccAAwAHAAATNSEVBTUhFTcBhv56AYYBcFdX31tbAAAAAQA8AAoBwgJOAAcAABM1BRUFNSU1PAGG/noBRQHqZOtu62S9AgAAAAIAPgAAAcQC5AAbAB8AAAEUDgEHDgIHIzQ+ATc+AjU0JiMiBzU2MzIWATUzFQHEGxobGh0bAmoiHx4YExYzMVJiXWdcZv7khgJCJkYlHx4nRSc0XCsjHBkwGCInOW4qVv1yjIwAAAAAAgAZ/40B0QJ7AAoALgAAJTU0JiMiBhQWMzInNDYzMhczNTQmIyIGFRQWMzI3FQYjIiY1NDYzMhYVEQYjIiYBbRYXHRkZHiC5QkImIAIzJ05MUVE8QUBMfH58e11kQVpIRp6dKSI1mjWCcWYZCiYxhZqYhCJiG7HGxLNmYv67QWIAAAAAAgAMAAAB6ALaAAMACwAAEyMDMxcjByMTMxMj+QJFjBKwJW+tgq10AoD+mly+Atr9JgAAAAMAPv/2Ac4C5AAIABEAIQAAEzMyNjU0IyIHERUWMzI1NCYjFxQjIicRNjMyFRQGBxUeAa4eRj10GBUeJXFBSfboVlJTWs8+NDpMAbYxNWsH/tn/B4tAO4fcDwLQD7Y9Vg0BC2gAAAAAAQAq//YBsALkABUAAAEiBhUUFjMyNxUGIyImNTQ2MzIXFSYBMU1OUU03RUNJfH5+eks/QQKHg5eWhCNkHLHGxLMcXh0AAAAAAgA0//YB2ALkAAsAFgAANxE2MzIWFRQOASMiEzQmIyIHERYzMjY0SFeLejpvXFfvSU8eEhIeUEgFAtAPqM+OpkMBd5t8Bv3eBncAAAEASwAAAa4C2gALAAATFTMVIxUzFSERIRW66ur0/p0BYwJ6xF35YALaYAAAAQBVAAABrgLaAAkAABMRIxEhFSMVMxXEbwFZ6uABWf6nAtpgxF0AAAEAIP/2AcoC5AAYAAABIgYVFBYzMjc1IzUzEQYjIiYQNjMyFxUmAS5UT0tHIyB/6VFfeoCFiUJASAKHf5uYghH1Xf5oKLMBiLMZYBwAAAAAAQAyAAABwgLaAAsAAAEjESMRMxEzETMRIwFRsm1tsnFxAVn+pwLa/t8BIf0mAAAAAQBQAAABpALaAAsAACkBNTMRIzUhFSMRMwGk/qxwcAFUcHBcAiJcXP3eAAABADL/9gGVAtoAEAAAAREUBiMiJzUeATMyNjURIzUBlVxvVEQeVBwzMJwC2v3wdl4ebBIZOUUBq1wAAAABAD4AAAHYAtoADAAAEyMRIxEzETMTMwMTI7ACcHACqnrBxXoBWf6nAtr+xQE7/qT+ggAAAQBfAAABrgLaAAUAABMRMxUhEc7g/rEC2v2GYALaAAABACAAAAHUAtoADwAAMxEzEzMTMxEjESMDIwMjESCCWQJZfm4CP1o/AgLa/lcBqf0mAgj+wAFA/fgAAAABADcAAAG9AtoACwAAEyMRIxEzEzMRMxEjqQJwdKQCbHAB+f4HAtr+BwH5/SYAAAACAB7/9gHWAuQABwATAAASMhYQBiImEBIyPgE0LgEiDgEUFojkamrkarVOMBsbME4wGxsC5Kn+ZKmpAZz+Gi1+2n4tLX7afgACAD4AAAHOAuQACgAXAAABNCYjIgcRFjMyNjcUBiMiJxEjETYzMhYBYz5AHxgZHkE9a21yGyZwU1p2bQIAR0AH/vkFP017bwX+5QLVD2wAAAIAHv9qAeUC5AAQABwAABIyFhUQBxUeARcjLgEjIiYQEjI+ATQuASIOARQWiORqYCc+CnQSNi9yarVOMBsbME4wGxsC5KnO/vZIAhZhOE89qQGc/hotftp+LS1+2n4AAAIAOgAAAd4C5AATABwAAAEUBxUWHwEjJy4BKwERIxE2MzIWBTMyNjU0IyIHAcpoHSA/dDoPJSsob1NZdm7+3y1MPH4eGQIPkjICEWzM0jYi/tYC1Q9m9jpNeAcAAQA8//YBwgLkAB4AAAEmIyIGFRQXHgEVFCMiJzUWMzI1NCYnLgE1NDYzMhcBrk9TLDdRcVfUY0pSWWgsM2VUbFpdTwJQNTQpUiQwbVfINHRJZy8+FyxrT1VpJgABAC0AAAHHAtoABwAAAREjESM1IRUBMW6WAZoCdv2KAnZkZAABADL/9gHCAtoADwAABCImNREzERQWMjY1ETMRFAFi0GBxKWApbQpndQII/hZbQEBbAer9+HUAAAEAFgAAAd4C2gAHAAA3EzMDIwMzE/1yb6OCo3RxWgKA/SYC2v2AAAABABQAAAHgAtoADwAAGwEzEzMTMxMzAyMDIwMjA4EeAi5fLgIeZDSGLAItgzQC2v3BAdv+JQI//SYB1v4qAtoAAQAmAAABzgLaAA0AABMzEzMDEyMDIwMjEwMz+wJdcoyOeF0CXXSOjHYBxwET/pr+jAEg/uABdAFmAAAAAQAWAAAB3gLaAAkAABMzEzMDESMRAzP7Amx1rW6teQGLAU/+QP7mARoBwAABAEEAAAGzAtoACwAAATUjNSEVAxUzFSE1ATj3AXL39/6OAnsDXFz94QNcXAAAAAABAIL/ZQGQAwIABwAAExEzFSERIRXirv7yAQ4CtPz+TQOdTgABACn/2AHLAtoAAwAAEzMBIylhAUFhAtr8/gAAAQBk/2UBcgMCAAcAAAEjNSERITUzARKuAQ7+8q4CtE78Y00AAQAWARgB3gLaAAcAABsBMxMjAyMDFp6MnmKBAoEBGAHC/j4BhP58AAEALf9lAcf/rQADAAAXNSEVLQGam0hIAAEAbgH+AW0DKgADAAATMxMjbptkXwMq/tQAAAACADL/9gG4AhIAGgAlAAATPgEzMhYVESMnIwYjIiY1NDY7ATU0JiMiBgcFIyIGFRQWMzI2NUslaSNqUmQCAi9eQk99eyQlLyFpJQEDJElJJiA0PAHvDhVVcf60RlBTS1liETEnFw+ONzImKU5OAAAAAgA3//YB0QLuAA8AGgAAExEzNjMyERQGIyInIwcjEQE0IyIGHQEUFjI2owIvTq9gT0w0AgJnAS5hLDY2WDUC7v7UUP7yhohSSALu/hasVVIKU1RUAAAAAAEARv/2Aa4CEgASAAA2EDYzMhcVJiMiFRQWMjcVBiMiRnNrQkA9OHxFeDxEQG19AQ6HGV8ds1xaIF8ZAAIAI//2Ab0C7gAPABoAAAERIycjBiMiJjUQMzIXMxEDFBYyNj0BNCYjIgG9ZwICNExPYK9OLwLCNVg2NixhAu79EkhSiIYBDlABLP4WWFRUUwpSVQAAAAACADD/9gHAAhIAEAAWAAA3HgEzMjcVBiMiEDMyFhUUByUzJiMiBqAGOzw2UU9F4NJeYAL+4rMBUDAu3E5AIF8ZAhyBiwogVYk7AAAAAQBEAAABsALkABUAABM1MzU0NjMyFxUmIyIGHQEzFSMRIxFEbkhSMjImJyobkpJsAZtZNGVXD1sRLUogWf5lAZsAAAACACj/GgHCAhIAGQAmAAAXFjMyNj0BIwYjIiY1EDMyFzM3MxEUBiMiJxMUFjMyNj0BNCYjIgZSSkU6OwIsUU9gr0w0AgJnbnBPQ0I1LC01NiwwMWwiSlNBUIV/AQ5SSP4CfHQZAdFRUU1QClJVUQAAAQA8AAABwgLuABMAABMRMzYzMhYVESMRNCYjIgYdASMRqAIsWE9FaiAsKjpsAu7+1FBddf7AAS1SNWRc9ALuAAIAWAAAAbYC+AAJAA0AABMzETMVITUzESM3NTMVdtdp/qKGaFSDAgj+UFhYAVjLfX0AAAACAFP/GgFcAvgAEgAWAAAXESM1MxEUDgIjNTI+BQM1MxXtc+ITPWJXIigmEw8GAhSDGwHLWP3dQkowD1gCBAwOHB4Cr319AAABAEEAAAHWAu4ACgAAAQcTIycVIxEzETcB1tDQgahsbKoCCPr+8vHxAu7+O98AAAABADz/9gG4AtoADwAAEzMRFBYzMjcVBiMiJjURIzzrEzMoIzUnZT59Atr90EMZBVgFPWMB7AAAAAEADwAAAeUCEgAhAAAzETMXMz4BMzIWFzM2MzIWFREjES4BIyIVESMRLgEjIh0BD18CAhAyGh0lDgIqPDMsZwEQEytnAQ8RLQIIRCQqIytORVv+jgFeKByK/ugBXicdqPoAAAABADwAAAHCAhIAFAAAASIGHQEjETMXMz4BMzIWFREjETQmAQwqOmxnAgIVSCpPRWogAbRkXPQCCEgmLF11/sABLVI1AAIAKP/2AcwCEgADAAsAABIgECA2MjY0JiIGFCgBpP5cnmgvL2gvAhL95FhPzk9PzgAAAAACADL/JAHMAhIACgAaAAABIgYdARQWMjY1NBcUBiMiJyMRIxEzFzM2MzIA/yw2Nlg1bGBPUSwCbGcCAjRMrwGwVVIKU1RUWKyshohS/twC5EhSAAACACj/JAHCAhIADwAaAAATEDMyFzM3MxEjESMGIyImNxQWMjY9ATQmIyIor0w0AgJnbAIsUU9gbDVYNjYsYQEEAQ5SSP0cASRSiIZYVFRTClJVAAABAGIAAAG2AhIAEAAAASIGHQEjETMVMz4BMzIXFSYBcEdbbGkCIk00JCIfAbZxX+YCCFw3LwpcCgAAAAABAET/9gG2AhIAIAAAASIVFBYXHgEVFAYjIic1FjMyNjU0JicuATU0NjMyFxUmAQRUIyxrTGJaYlBUTDQtJDNfT1xaXU1VAbg6HCELGUlIR08mZDAdIR4hDRhNQkVMIGAmAAAAAQA3//YBswKyABQAAAEVIxUUFjI3FQYjIiY1ESM1MzUzFQGzqRpgJTU0W0dnZ2wB9FnxOx8RXA9EXAEFWb6+AAEAN//2AbMCCAATAAATERQWMzI2PQEzESMnIwYjIiY1EaEdKik2bGcCAi5UTUICCP7JTTBhX/T9+EhSWHABSgABACgAAAHMAggABwAANzMTMwMjAzP7AmFuipCKcksBvf34AggAAAAAAQAPAAAB5QIIAA8AACUzEzMDIwMjAyMDMxMzEzMBWwInYUt2LQIpcktnJwIsZGQBpP34AY/+cQII/lwBpAAAAAEALQAAAccCCAANAAATMzczAxMjJyMHIxMDM/sCV3OHh3tTAlN3h4d4AUDI/vz+/MbGAQQBBAAAAQAj/yQB2wIIAAgAACUzEzMDIzcDMwEAAmZz725SrXORAXf9HOYB/gAAAAABAEgAAAGsAggACwAAEyEVAxUzFSE1EzUjSAFk5eX+nOXlAghZ/qwCWVkBVAIAAAABAD7/ZQGsAwIAJAAAExcWHQEUFjsBFSMiJj0BNCYrATUzMjY9ATQ2OwEVIyIGHQEUBr8CYR0+L1ZNRywtKystLEdNVi8/HC8BNAEqcntJIU1IU5Q/Ok06P5RTSE4gSHw+SAAAAQDI/yQBLAMqAAMAABcRMxHIZNwEBvv6AAAAAAEASP9lAbYDAgAkAAABLgE9ATQmKwE1MzIWHQEUFjsBFSMiBh0BFAYrATUzMjY9ATQ3ATU0Lxw/L1ZNRywtKystLEdNVi8+HWEBNBZIPnxIIE5IU5Q/Ok06P5RTSE0hSXtyKgABAC0A5QHHAXMAEwAAJSInJiMiFSM0NjMyFxYzMjUzFAYBXCpJNxogS0ArKkk3GiBLQOUmHThGPSYdOEY9AAAAAAAWAQ4AAQAAAAAAAAAiAEYAAQAAAAAAAQAMAIMAAQAAAAAAAgAHAKAAAQAAAAAAAwAoAPoAAQAAAAAABAAMAT0AAQAAAAAABQANAWYAAQAAAAAABgAPAZQAAQAAAAAACwAhAegAAQAAAAAAEAAFAhYAAQAAAAAAEQAGAioAAwABBAkAAABEAAAAAwABBAkAAQAYAGkAAwABBAkAAgAOAJAAAwABBAkAAwBQAKgAAwABBAkABAAYASMAAwABBAkABQAaAUoAAwABBAkABgAeAXQAAwABBAkACwBCAaQAAwABBAkAEAAKAgoAAwABBAkAEQAMAhwAAwABBBEAEAAKAjEAAwABBBEAEQAMAj0AQwBvAHAAeQByAGkAZwBoAHQAKABjACkAIAAyADAAMQA0ACAATQArACAARgBPAE4AVABTACAAUABSAE8ASgBFAEMAVAAAQ29weXJpZ2h0KGMpIDIwMTQgTSsgRk9OVFMgUFJPSkVDVAAATQArACAAMQBtACAAbQBlAGQAaQB1AG0AAE0rIDFtIG1lZGl1bQAAUgBlAGcAdQBsAGEAcgAAUmVndWxhcgAARgBvAG4AdABGAG8AcgBnAGUAIAAyAC4AMAAgADoAIABNACsAIAAxAG0AIABtAGUAZABpAHUAbQAgADoAIAAyADYALQAyAC0AMgAwADEANAAARm9udEZvcmdlIDIuMCA6IE0rIDFtIG1lZGl1bSA6IDI2LTItMjAxNAAATQArACAAMQBtACAAbQBlAGQAaQB1AG0AAE0rIDFtIG1lZGl1bQAAVgBlAHIAcwBpAG8AbgAgADEALgAwADUAOAAAVmVyc2lvbiAxLjA1OAAAbQBwAGwAdQBzAC0AMQBtAC0AbQBlAGQAaQB1AG0AAG1wbHVzLTFtLW1lZGl1bQAAaAB0AHQAcAA6AC8ALwBtAHAAbAB1AHMALQBmAG8AbgB0AHMALgBzAG8AdQByAGMAZQBmAG8AcgBnAGUALgBqAHAAAGh0dHA6Ly9tcGx1cy1mb250cy5zb3VyY2Vmb3JnZS5qcAAATQArACAAMQBtAABNKyAxbQAAbQBlAGQAaQB1AG0AAG1lZGl1bQAATQArACAAMQBtAAAAbQBlAGQAaQB1AG0AAAAAAAADAAAAAAAA/4MAMgAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAf//AAIAAAABAAAAAMw9os8AAAAAynkzBgAAAADPMzwEAAEAAAAAAAJHU1VCAAAAGEdQT1MAAAGoAAAABwBSACAAagAmAHMALAB7ADIAiAA4AJUAPgC1AEQAAQDCAAAAAQDZAAAAAQDgAAAAAQDqAAAAAQEAAAAAAQEWAAAAAwE/AAABWQAAAXMAAGthbmEgc2VtaS12b2ljZWQgbG9va3VwAGdzdWJ2ZXJ0AGppczIwMDQAY2NtcGxvb2t1cDAxAGNjbXBsb29rdXAwMgBTaW5nbGVTdWJzdGl0dXRpb25sb29rdXBEb3RsZXNzAGNjbXBsb29rdXAwMwBrYW5hIHNlbWktdm9pY2VkIHRhYmxlAGotdmVydABqcDA0dGFibGUAY2NtcGxvb2t1cDAxIHN1YnRhYmxlAGNjbXBsb29rdXAwMiBzdWJ0YWJsZQBTaW5nbGVTdWJzdGl0dXRpb25sb29rdXBEb3RsZXNzIHN1YnRhYmxlAGNjbXBsb29rdXAwMyBjb250ZXh0dWFsIDAAY2NtcGxvb2t1cDAzIGNvbnRleHR1YWwgMQBjY21wbG9va3VwMDMgY29udGV4dHVhbCAyAAAAAAAAAAA=";

		//mplus-1-medium-sub.ttf
		std::vector<std::string> fontDataJPOriginal({
#include<Pita/FontDataJP>
		});

		std::string fontDataJP;
		fontDataJP.reserve(671775);
		for (const auto& str : fontDataJPOriginal)
		{
			fontDataJP.append(str);
		}

		using namespace boost::archive::iterators;
		using DecodeIt = transform_width<binary_from_base64<std::string::const_iterator>, 8, 6, char>;

		{
			fontDataRawEN = std::string(DecodeIt(fontDataEN.begin()), DecodeIt(fontDataEN.end()));
			const unsigned char* pc = reinterpret_cast<const unsigned char*>(fontDataRawEN.c_str());

			fontInfo1 = new stbtt_fontinfo;
			stbtt_InitFont(fontInfo1, pc, stbtt_GetFontOffsetForIndex(pc, 0));
			stbtt_GetFontVMetrics(fontInfo1, &ascent1, &descent1, &lineGap1);
		}
		{
			fontDataRawJP = std::string(DecodeIt(fontDataJP.begin()), DecodeIt(fontDataJP.end()));
			const unsigned char* pc = reinterpret_cast<const unsigned char*>(fontDataRawJP.c_str());

			fontInfo2 = new stbtt_fontinfo;
			stbtt_InitFont(fontInfo2, pc, stbtt_GetFontOffsetForIndex(pc, 0));
			stbtt_GetFontVMetrics(fontInfo2, &ascent2, &descent2, &lineGap2);
		}
	}

	FontBuilder::FontBuilder(const std::string& fontPath)
	{
		fontInfo1 = new stbtt_fontinfo;
		fread(current_buffer, 1, 1 << 25, fopen(fontPath.c_str(), "rb"));
		stbtt_InitFont(fontInfo1, current_buffer, stbtt_GetFontOffsetForIndex(current_buffer, 0));
		stbtt_GetFontVMetrics(fontInfo1, &ascent1, &descent1, &lineGap1);
	}

	FontBuilder::~FontBuilder()
	{
		delete fontInfo1;
		delete fontInfo2;
	}

	std::vector<gg::Geometry*> FontBuilder::makePolygon(int codePoint, int quality, double offsetX, double offsetY)
	{
		const auto vec2 = [&](short x, short y)
		{
			//return Eigen::Vector2d(0.1*(offsetX + x), 0.1*(offsetY - y));
			
			//const double size = 0.025;
			//return Eigen::Vector2d(size * (offsetX + x), size * (offsetY - y));
			return Eigen::Vector2d(offsetX + FontSizeToReal(x), offsetY + FontSizeToReal(-y));
		};

		bool isFont1 = true;
		int glyphIndex = stbtt_FindGlyphIndex(fontInfo1, codePoint);
		if (glyphIndex == 0)
		{
			isFont1 = false;
			glyphIndex = stbtt_FindGlyphIndex(fontInfo2, codePoint);
			if (glyphIndex == 0)
			{
				return{};
			}
		}
		
		stbtt_vertex* pv;
		const int verticesNum = isFont1 ? stbtt_GetGlyphShape(fontInfo1, glyphIndex, &pv) : stbtt_GetGlyphShape(fontInfo2, glyphIndex, &pv);

		using Vertices = Vector<Eigen::Vector2d>;

		std::vector<gg::Geometry*> currentPolygons;
		std::vector<gg::Geometry*> currentHoles;

		int polygonBeginIndex = 0;
		while (polygonBeginIndex < verticesNum)
		{
			stbtt_vertex* nextPolygonBegin = std::find_if(pv + polygonBeginIndex + 1, pv + verticesNum, [](const stbtt_vertex& p) {return p.type == STBTT_vmove; });
			const int nextPolygonFirstIndex = std::distance(pv, nextPolygonBegin);
			const int currentPolygonLastIndex = nextPolygonFirstIndex - 1;

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

		auto factory = gg::GeometryFactory::create();

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
					//穴に含まれるポリゴンについては、引かないようにする
					//本当は包含判定をすべきだがフォントならそんなに複雑な構造にならないはず
					if (erodeGeometry->getArea() < currentHoles[d]->getArea())
					{
						continue;
					}

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

	std::vector<gg::Geometry*> FontBuilder::textToPolygon(const std::string& str, int quality)
	{
		std::vector<gg::Geometry*> result;
		int offsetX = 0;
		for (int i = 0; i < str.size(); ++i)
		{
			//const int glyphIndex = stbtt_FindGlyphIndex(fontInfo, static_cast<int>(str[i]));
			const int codePoint = static_cast<int>(str[i]);
			//int x0, x1, y0, y1;
			//stbtt_GetGlyphBox(fontInfo, glyphIndex, &x0, &y0, &x1, &y1);
			const auto characterPolygon = makePolygon(codePoint, quality, offsetX, 0);
			result.insert(result.end(), characterPolygon.begin(), characterPolygon.end());
			//offsetX += (x1 - x0);

			offsetX += glyphWidth(codePoint);
		}
		return result;
	}

	double FontBuilder::glyphWidth(int codePoint)
	{
		/*
		const int glyphIndex = stbtt_FindGlyphIndex(fontInfo, codePoint);
		int advanceWidth;
		int leftSideBearing;
		stbtt_GetCodepointHMetrics(fontInfo, codePoint, &advanceWidth, &leftSideBearing);
		return FontSizeToReal(advanceWidth - leftSideBearing);
		*/

		bool isFont1 = true;
		int glyphIndex = stbtt_FindGlyphIndex(fontInfo1, codePoint);
		if (glyphIndex == 0)
		{
			isFont1 = false;
			glyphIndex = stbtt_FindGlyphIndex(fontInfo2, codePoint);
			if (glyphIndex == 0)
			{
				return{};
			}
		}

		int advanceWidth;
		int leftSideBearing;
		isFont1 
			? stbtt_GetCodepointHMetrics(fontInfo1, codePoint, &advanceWidth, &leftSideBearing)
			: stbtt_GetCodepointHMetrics(fontInfo2, codePoint, &advanceWidth, &leftSideBearing);

		return FontSizeToReal(advanceWidth - leftSideBearing);
	}
}

