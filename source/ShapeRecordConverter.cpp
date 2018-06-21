#include <Eigen/Core>

#include <geos/geom.h>
#include <geos/opBuffer.h>
#include <geos/opDistance.h>

#include <Pita/Node.hpp>
#include <Pita/Context.hpp>
#include <Pita/Vectorizer.hpp>
#include <Pita/Printer.hpp>

namespace cgl
{
	bool ReadDoublePacked(double& output, const std::string& name, const PackedRecord& record)
	{
		const auto& values = record.values;

		auto it = values.find(name);
		if (it == values.end())
		{
			return false;
		}
		const PackedVal& value = it->second.value;
		if (!IsType<int>(value) && !IsType<double>(value))
		{
			return false;
		}
		output = IsType<int>(value) ? static_cast<double>(As<int>(value)) : As<double>(value);
		return true;
	}

	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record)
	{
		const auto& values = record.values;

		auto itX = values.find("x");
		auto itY = values.find("y");

		Eigen::Vector2d xy(AsDouble(itX->second.value), AsDouble(itY->second.value));

		return std::tuple<double, double>(xy.x(), xy.y());
	}

	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record, const TransformPacked& transform)
	{
		const auto& values = record.values;

		auto itX = values.find("x");
		auto itY = values.find("y");

		Eigen::Vector2d xy;
		xy << AsDouble(itX->second.value), AsDouble(itY->second.value);
		const auto xy_ = transform.product(xy);

		//return std::tuple<double, double>(AsDouble(itX->second.value), AsDouble(itY->second.value));
		return std::tuple<double, double>(xy_.x(), xy_.y());
	}

	Path ReadPathPacked(const PackedRecord& pathRecord)
	{
		Path resultPath;

		resultPath.cs = std::make_unique<gg::CoordinateArraySequence>();
		auto& cs = resultPath.cs;
		auto& distances = resultPath.distances;

		const TransformPacked transform(pathRecord);

		auto itLine = pathRecord.values.find("line");
		if (itLine == pathRecord.values.end())
			CGL_Error("不正な式です");

		const PackedVal lineVal = itLine->second.value;
		if (auto pointListOpt = AsOpt<PackedList>(lineVal))
		{
			for (const auto pointVal : pointListOpt.get().data)
			{
				if (auto posRecordOpt = AsOpt<PackedRecord>(pointVal.value))
				{
					const auto v = ReadVec2Packed(posRecordOpt.get());
					const double x = std::get<0>(v);
					const double y = std::get<1>(v);
					//cs->add(gg::Coordinate(x, y));
					Eigen::Vector2d xy(x, y);
					const auto xy_ = transform.product(xy);

					cs->add(gg::Coordinate(xy_.x(), xy_.y()));
				}
				else CGL_Error("不正な式です");
			}

			distances.push_back(0);
			for (int i = 1; i < cs->size(); ++i)
			{
				const double newDistance = cs->getAt(i - 1).distance(cs->getAt(i));
				distances.push_back(distances.back() + newDistance);
			}
		}
		else CGL_Error("不正な式です");

		return std::move(resultPath);
	}

	bool ReadPolygonPacked(Vector<Eigen::Vector2d>& output, const PackedList& vertices, const TransformPacked& transform)
	{
		output.clear();

		for (const auto& val: vertices.data)
		{
			const PackedVal& value = val.value;

			if (IsType<PackedRecord>(value))
			{
				double x = 0, y = 0;
				const PackedRecord& pos = As<PackedRecord>(value);
				if (!ReadDoublePacked(x, "x", pos) || !ReadDoublePacked(y, "y", pos))
				{
					return false;
				}
				Eigen::Vector2d v;
				v << x, y;
				output.push_back(transform.product(v));
			}
			else
			{
				return false;
			}
		}

		return true;
	}

	bool ReadColorPacked(Color& output, const PackedRecord& record, const TransformPacked& transform)
	{
		double r = 0, g = 0, b = 0;

		if (!(
			ReadDoublePacked(r, "r", record) &&
			ReadDoublePacked(g, "g", record) &&
			ReadDoublePacked(b, "b", record)
			))
		{
			return false;
		}

		output.r = r;
		output.g = g;
		output.b = b;

		return true;
	}
}
