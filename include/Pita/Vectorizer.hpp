#pragma once
#include <tuple>
#include <cfloat>
#include <numeric>
#include <Eigen/Core>

#include "Node.hpp"
#include "Context.hpp"

namespace geos
{
	namespace geom
	{
		class Geometry;
		class Polygon;
		class LineString;
		class CoordinateArraySequence;
	}
	namespace operation
	{
		namespace buffer {}
		namespace distance {}
	}
}

namespace cgl
{
	namespace gg = geos::geom;
	namespace gob = geos::operation::buffer;
	namespace god = geos::operation::distance;

	template<class DeleterT, class PointerT>
	inline std::unique_ptr<PointerT, DeleterT> ToUnique(PointerT* p)
	{
		return std::unique_ptr<PointerT, DeleterT>(p);
	}

	struct GeometryDeleter
	{
		void operator()(gg::Geometry* pGeometry) const;
	};
	using GeometryPtr = std::unique_ptr<gg::Geometry, GeometryDeleter>;

	class Geometries
	{
	public:
		Geometries();
		~Geometries();
		Geometries(Geometries&&);

		bool empty()const
		{
			return gs.empty();
		}

		size_t size()const
		{
			return gs.size();
		}

		GeometryPtr takeOut(size_t index)
		{
			GeometryPtr ptr(std::move(gs[index]));
			gs.erase(gs.begin() + index);
			return ptr;
		}

		const gg::Geometry* const refer(size_t index)const
		{
			return gs[index].get();
		}

		void insert(size_t index, GeometryPtr g)
		{
			gs.insert(gs.begin() + index, std::move(g));
		}

		void erase(size_t index)
		{
			gs.erase(gs.begin() + index);
		}

		void push_back(GeometryPtr g)
		{
			gs.push_back(std::move(g));
		}

		void push_back_raw(gg::Geometry* g)
		{
			gs.push_back(ToUnique<GeometryDeleter>(g));
		}

		void pop_back()
		{
			gs.pop_back();
		}

		void append(Geometries&& tail)
		{
			gs.insert(gs.end(), std::make_move_iterator(tail.gs.begin()), std::make_move_iterator(tail.gs.end()));
			tail.gs.clear();
		}

		std::vector<gg::Geometry*> releaseAsRawPtrs()
		{
			std::vector<gg::Geometry*> ps;
			for (size_t i = 0; i < gs.size(); ++i)
			{
				ps.push_back(gs[i].release());
			}
			gs.clear();
			return ps;
		}

		Geometries& operator=(Geometries&&);

	private:
		std::vector<GeometryPtr> gs;
	};

	void GetQuadraticBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, int n, bool includesEndPoint);
	void GetCubicBezier(Vector<Eigen::Vector2d>& output, const Eigen::Vector2d& p0, const Eigen::Vector2d& p1, const Eigen::Vector2d& p2, const Eigen::Vector2d& p3, int n, bool includesEndPoint);

	bool IsClockWise(const Vector<Eigen::Vector2d>& closedPath);
	bool IsClockWise(const gg::LineString* closedPath);
	std::tuple<bool, std::unique_ptr<gg::Geometry>> IsClockWise(std::unique_ptr<gg::Geometry> pLineString);

	std::string GetGeometryType(gg::Geometry* geometry);
	gg::Polygon* ToPolygon(const Vector<Eigen::Vector2d>& exterior);
	gg::LineString* ToLineString(const Vector<Eigen::Vector2d>& exterior);

	void DebugPrint(const gg::Geometry* geometry);

	inline Eigen::Vector2d EigenVec2(double x, double y)
	{
		Eigen::Vector2d v;
		v << x, y;
		return v;
	}

	struct Color
	{
		int r = 0, g = 0, b = 0;

		std::string toString()const
		{
			std::stringstream ss;
			const auto clamp = [](int x) {return std::max(std::min(x, 255), 0); };
			ss << "rgb(" << clamp(r) << ", " << clamp(g) << ", " << clamp(b) << ")";
			return ss.str();
		}
	};

	class PitaGeometry
	{
	public:
		PitaGeometry() = default;

		PitaGeometry(std::shared_ptr<gg::Geometry> shape, const Color& color) :
			shape(shape),
			color(color)
		{}

		std::shared_ptr<gg::Geometry> shape;
		Color color;
	};

	struct BaseLineOffset
	{
		double x = 0, y = 0;
		double nx = 0, ny = 0;
		double angle = 0;
	};

	struct Path
	{
		std::unique_ptr<gg::CoordinateArraySequence> cs;
		std::vector<double> distances;

		double length()const
		{
			return distances.back();
			//return std::accumulate(distances.begin(), distances.end(), 0.0);
		}

		bool empty()const
		{
			return distances.empty();
		}

		Path clone()const;

		BaseLineOffset getOffset(double offset)const;
	};

	class BoundingRect
	{
	public:
		BoundingRect() = default;

		BoundingRect(double min_x, double min_y, double max_x, double max_y) :
			m_min(min_x, min_y),
			m_max(max_x, max_y)
		{}

		BoundingRect(const Vector<Eigen::Vector2d>& vs)
		{
			add(vs);
		}

		void add(const Eigen::Vector2d& v);

		void add(const Vector<Eigen::Vector2d>& vs);

		bool intersects(const BoundingRect& other)const
		{
			return std::max(m_min.x(), other.m_min.x()) < std::min(m_max.x(), other.m_max.x())
				&& std::max(m_min.y(), other.m_min.y()) < std::min(m_max.y(), other.m_max.y());
		}

		bool includes(const Eigen::Vector2d& point)const
		{
			return m_min.x() < point.x() && point.x() < m_max.x()
				&& m_min.y() < point.y() && point.y() < m_max.y();
		}

		Eigen::Vector2d pos()const
		{
			return m_min;
		}

		Eigen::Vector2d center()const
		{
			return (m_min + m_max)*0.5;
		}

		Eigen::Vector2d width()const
		{
			return m_max - m_min;
		}

		double area()const
		{
			const auto wh = width();
			return wh.x()*wh.y();
		}

		Eigen::Vector2d minPos()const
		{
			return m_min;
		}

		Eigen::Vector2d maxPos()const
		{
			return m_max;
		}

		bool isEmpty()const
		{
			return m_min.x() == DBL_MAX && m_min.y() == DBL_MAX
				&& m_max.x() == -DBL_MAX && m_max.y() == -DBL_MAX;
		}

	private:
		Eigen::Vector2d m_min = EigenVec2(DBL_MAX, DBL_MAX);
		Eigen::Vector2d m_max = EigenVec2(-DBL_MAX, -DBL_MAX);
	};

	struct TransformPacked
	{
		using Mat3x3 = Eigen::Matrix<double, 3, 3, 0, 3, 3>;

		TransformPacked()
		{
			init();
		}

		TransformPacked(double px, double py, double sx = 1, double sy = 1, double angle = 0)
		{
			init(px, py, sx, sy, angle);
		}

		TransformPacked(const Mat3x3& mat) :mat(mat) {}

		TransformPacked(const PackedRecord& record);

		void init(double px = 0, double py = 0, double sx = 1, double sy = 1, double angle = 0);

		TransformPacked operator*(const TransformPacked& other)const
		{
			return static_cast<Mat3x3>(mat * other.mat);
		}

		Eigen::Vector2d product(const Eigen::Vector2d& v)const;

		void printMat()const;

	private:
		Mat3x3 mat;
	};

	GeometryPtr MakeLine(const Eigen::Vector2d& p0, const Eigen::Vector2d& p1);

	//Compress to ShapeRecord
	PackedRecord WritePathPacked(const Path& path);
	//PackedRecord GetPolygonPacked(const gg::Polygon* poly);

	using PackedPolyData = std::vector<PackedList>;
	//最終的にShapeに変換する個所以外では、再利用性を考えPackedListは内部に頂点のみを持つものとし、
	//複数のポリゴンはstd::vector<PackedList>で表現する
	PackedPolyData GetPolygonVertices(const gg::Polygon* poly);
	//PackedList GetShapesFromGeosPacked(const std::vector<gg::Geometry*>& polygons);

	enum class PackedPolyDataType {
		POLYGON, MULTIPOLYGON
	};

	PackedList AsPackedListPolygons(const PackedPolyData& polygons);
	PackedPolyDataType GetPackedListType(const PackedList& packedList);

	//Convert ShapeRecord
	bool ReadDoublePacked(double& output, const std::string& name, const PackedRecord& record);
	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record);
	std::tuple<double, double> ReadVec2Packed(const PackedRecord& record, const TransformPacked& transform);
	Path ReadPathPacked(const PackedRecord& record);
	bool ReadColorPacked(Color& output, const PackedRecord& record, const TransformPacked& transform);
	bool ReadPolygonPacked(Vector<Eigen::Vector2d>& output, const PackedList& vertices, const TransformPacked& transform);

	//Interpret ShapeRecord
	/*BoundingRect BoundingRectRecordPacked(const PackedVal& value);
	std::vector<gg::Geometry*> GeosFromRecordPacked(const PackedVal& value, const cgl::TransformPacked& transform = cgl::TransformPacked());
	bool OutputSVG2(std::ostream& os, const PackedVal& value, const std::string& name);*/
	BoundingRect BoundingRectRecordPacked(const PackedVal& value, std::shared_ptr<Context> pContext);
	Geometries GeosFromRecordPacked(const PackedVal& value, std::shared_ptr<Context> pContext, const cgl::TransformPacked& transform = cgl::TransformPacked());
	bool OutputSVG2(std::ostream& os, const PackedVal& value, const std::string& name, std::shared_ptr<Context> pContext);
}
