#include <cmath>
#include <algorithm>
#include <Pita/Node.hpp>
#include <Pita/BinaryEvaluator.hpp>
#include <Pita/IntrinsicGeometricFunctions.hpp>
#include <Pita/Printer.hpp>

namespace cgl
{
	Val NotFunc(const Val& lhs, Context& env)
	{
		if (IsType<bool>(lhs))
		{
			return !As<bool>(lhs);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val PlusFunc(const Val& lhs, Context& env)
	{
		if (IsType<int>(lhs) || IsType<double>(lhs))
		{
			return lhs;
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val MinusFunc(const Val& lhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			return -As<int>(lhs);
		}
		else if (IsType<double>(lhs))
		{
			return -As<double>(lhs);
		}
		else if (IsVec2(lhs))
		{
			Eigen::Vector2d v = AsVec2(lhs, env);
			return MakeRecord("x", -v.x(), "y", -v.y()).unpacked(env);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val AndFunc(const Val& lhs, const Val& rhs, Context& context)
	{
		if ((IsType<bool>(lhs) || IsNum(lhs)) && (IsType<bool>(rhs) || IsNum(rhs)))
		{
			if (IsType<bool>(lhs) && IsType<bool>(rhs))
			{
				return As<bool>(lhs) && As<bool>(rhs);
			}
			else if (IsNum(lhs) && IsNum(rhs))
			{
				const double eps = 1.e-4;
				return AsDouble(lhs) < eps && AsDouble(rhs) < eps;
			}
			else if (IsType<bool>(lhs) && IsNum(rhs))
			{
				const double eps = 1.e-4;
				return As<bool>(lhs) && AsDouble(rhs) < eps;
			}
			else if (IsNum(lhs) && IsType<bool>(rhs))
			{
				const double eps = 1.e-4;
				return AsDouble(lhs) < eps && As<bool>(rhs);
			}
		}
		else if (IsShape(lhs) && IsShape(rhs))
		{
			return Unpacked(ShapeIntersect(
				Packed(lhs, context),
				Packed(rhs, context), context.m_weakThis.lock()), context);
		}

		std::cout << "lhs: \n";
		printVal2(lhs, nullptr, std::cout);
		std::cout << std::endl;

		std::cout << "rhs: \n";
		printVal2(rhs, nullptr, std::cout);
		std::cout << std::endl;

		CGL_Error("不正な式です");
		return 0;
	}

	Val OrFunc(const Val& lhs, const Val& rhs, Context& context)
	{
		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) || As<bool>(rhs);
		}
		else if (IsShape(lhs) && IsShape(rhs))
		{
			return Unpacked(ShapeUnion(
				Packed(lhs, context),
				Packed(rhs, context), context.m_weakThis.lock()), context);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	bool EqualFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		const double eps = 0.001;

		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) == As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return std::abs(static_cast<double>(As<int>(lhs)) - As<double>(rhs)) < eps;
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::abs(As<double>(lhs) - static_cast<double>(As<int>(rhs))) < eps;
			}
			else if (IsType<double>(rhs))
			{
				return std::abs(As<double>(lhs) - As<double>(rhs)) < eps;
			}
		}

		if (IsType<bool>(lhs) && IsType<bool>(rhs))
		{
			return As<bool>(lhs) == As<bool>(rhs);
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool NotEqualFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		return !EqualFunc(lhs, rhs, env);
	}

	bool LessThanFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) < As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) < As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) < As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) < As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool LessEqualFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) <= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) <= As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) <= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) <= As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool GreaterThanFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) > As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) > As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) > As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) > As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	bool GreaterEqualFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<int>(lhs) >= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<int>(lhs) >= As<double>(rhs);
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return As<double>(lhs) >= As<int>(rhs);
			}
			else if (IsType<double>(rhs))
			{
				return As<double>(lhs) >= As<double>(rhs);
			}
		}

		CGL_Error("不正な式です");
		return false;
	}

	Val MaxFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::max(As<int>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::max<double>(As<int>(lhs), As<double>(rhs));
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::max<double>(As<double>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::max(As<double>(lhs), As<double>(rhs));
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val MinFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::min(As<int>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::min<double>(As<int>(lhs), As<double>(rhs));
			}
		}
		else if (IsType<double>(lhs))
		{
			if (IsType<int>(rhs))
			{
				return std::min<double>(As<double>(lhs), As<int>(rhs));
			}
			else if (IsType<double>(rhs))
			{
				return std::min(As<double>(lhs), As<double>(rhs));
			}
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val AbsFunc(const Val& lhs, Context& env)
	{
		if (IsType<int>(lhs))
		{
			return std::abs(As<int>(lhs));
		}
		else if (IsType<double>(lhs))
		{
			return std::abs(As<double>(lhs));
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val SinFunc(const Val& lhs)
	{
		if (IsNum(lhs))
		{
			return std::sin(AsDouble(lhs));
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val CosFunc(const Val& lhs)
	{
		if (IsNum(lhs))
		{
			return std::cos(AsDouble(lhs));
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val AddFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsNum(lhs) && IsNum(rhs))
		{
			if (IsType<int>(lhs) && IsType<int>(rhs))
			{
				return As<int>(lhs) + As<int>(rhs);
			}
			else
			{
				return AsDouble(lhs) + AsDouble(rhs);
			}
		}
		else if(IsVec2(lhs) && IsVec2(rhs))
		{
			Eigen::Vector2d v = AsVec2(lhs, env) + AsVec2(rhs, env);
			return MakeRecord("x", v.x(), "y", v.y()).unpacked(env);
		}
		else if (IsType<CharString>(lhs))
		{
			if (IsType<CharString>(rhs))
			{
				CharString str = As<CharString>(lhs);
				str.str.append(As<CharString>(rhs).str);
				return str;
			}
			std::stringstream ss;
			ValuePrinter2 printer(nullptr, ss, 0);
			boost::apply_visitor(printer, rhs);
			CharString lhsStr = As<CharString>(lhs);
			lhsStr.str.append(AsUtf32(ss.str()));
			return lhsStr;
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val SubFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsNum(lhs) && IsNum(rhs))
		{
			if (IsType<int>(lhs) && IsType<int>(rhs))
			{
				return As<int>(lhs) - As<int>(rhs);
			}
			else
			{
				return AsDouble(lhs) - AsDouble(rhs);
			}
		}
		else if (IsVec2(lhs) && IsVec2(rhs))
		{
			Eigen::Vector2d v = AsVec2(lhs, env) - AsVec2(rhs, env);
			return MakeRecord("x", v.x(), "y", v.y()).unpacked(env);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val MulFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsNum(lhs) && IsNum(rhs))
		{
			if (IsType<int>(lhs) && IsType<int>(rhs))
			{
				return As<int>(lhs) * As<int>(rhs);
			}
			else
			{
				return AsDouble(lhs) * AsDouble(rhs);
			}
		}
		else if (IsVec2(lhs) && IsNum(rhs))
		{
			Eigen::Vector2d v = AsVec2(lhs, env) * AsDouble(rhs);
			return MakeRecord("x", v.x(), "y", v.y()).unpacked(env);
		}
		else if (IsNum(lhs) && IsVec2(rhs))
		{
			Eigen::Vector2d v = AsVec2(rhs, env) * AsDouble(lhs);
			return MakeRecord("x", v.x(), "y", v.y()).unpacked(env);
		}

		std::cout << "lhs: \n";
		printVal2(lhs, nullptr, std::cout);
		std::cout << std::endl;

		std::cout << "rhs: \n";
		printVal2(rhs, nullptr, std::cout);
		std::cout << std::endl;

		CGL_Error("不正な式です");
		return 0;
	}

	Val DivFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (IsNum(lhs) && IsNum(rhs))
		{
			if (IsType<int>(lhs) && IsType<int>(rhs))
			{
				int a = As<int>(lhs);
				int b = As<int>(rhs);
				if (a % b == 0)
				{
					return a / b;
				}
				return 1.0*a / b;
			}
			else
			{
				return AsDouble(lhs) / AsDouble(rhs);
			}
		}
		else if (IsVec2(lhs) && IsNum(rhs))
		{
			Eigen::Vector2d v = AsVec2(lhs, env) / AsDouble(rhs);
			return MakeRecord("x", v.x(), "y", v.y()).unpacked(env);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val PowFunc(const Val& lhs, const Val& rhs, Context& context)
	{
		if (IsNum(lhs) && IsNum(rhs))
		{
			if (IsType<int>(lhs) && IsType<int>(rhs))
			{
				const int x = As<int>(lhs);
				const int y = As<int>(rhs);
				if (y == 0)
				{
					return 1;
				}
				else if (1 <= y)
				{
					int result = 1;
					for (int i = 0; i < y; ++i)
					{
						result *= x;
					}
					return result;
				}
				else
				{
					return std::pow(AsDouble(lhs), AsDouble(rhs));
				}
			}
			else
			{
				return std::pow(AsDouble(lhs), AsDouble(rhs));
			}
		}
		else if (IsShape(lhs) && IsShape(rhs))
		{
			return Unpacked(ShapeSymDiff(
				Packed(lhs, context),
				Packed(rhs, context), context.m_weakThis.lock()), context);
		}

		CGL_Error("不正な式です");
		return 0;
	}

	Val ConcatFunc(const Val& lhs, const Val& rhs, Context& env)
	{
		if (!IsType<List>(lhs) || !IsType<List>(rhs))
		{
			CGL_Error("リスト結合演算子がリスト以外の式に使われています");
		}

		return List::Concat(As<List>(lhs), As<List>(rhs));
	}

	Val SetDiffFunc(const Val& lhs, const Val& rhs, Context& context)
	{
		if (!IsShape(lhs) || !IsShape(rhs))
		{
			CGL_Error("差集合演算子がシェイプ以外の式に使われています");
		}

		return Unpacked(ShapeDiff(
			Packed(lhs, context),
			Packed(rhs, context), context.m_weakThis.lock()), context);
	}
}
