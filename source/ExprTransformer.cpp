#pragma warning(disable:4996)
#include <iostream>

#include <Pita/ExprTransformer.hpp>
#include <Pita/Printer.hpp>

namespace cgl
{
	Expr ExprTransformer::operator()(const LRValue& node) { return node; }

	Expr ExprTransformer::operator()(const Identifier& node) { return node; }

	Expr ExprTransformer::operator()(const UnaryExpr& node)
	{
		return UnaryExpr(boost::apply_visitor(*this, node.lhs), node.op).setLocation(node);
	}

	Expr ExprTransformer::operator()(const BinaryExpr& node)
	{
		return BinaryExpr(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs),
			node.op).setLocation(node);
	}

	Expr ExprTransformer::operator()(const Lines& node)
	{
		Lines result;
		for (size_t i = 0; i < node.size(); ++i)
		{
			result.add(boost::apply_visitor(*this, node.exprs[i]));
		}
		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const DefFunc& node)
	{
		return DefFunc(node.arguments, boost::apply_visitor(*this, node.expr)).setLocation(node);
	}

	Expr ExprTransformer::operator()(const If& node)
	{
		If result(boost::apply_visitor(*this, node.cond_expr));
		result.then_expr = boost::apply_visitor(*this, node.then_expr);
		if (node.else_expr)
		{
			result.else_expr = boost::apply_visitor(*this, node.else_expr.get());
		}

		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const For& node)
	{
		For result;
		result.rangeStart = boost::apply_visitor(*this, node.rangeStart);
		result.rangeEnd = boost::apply_visitor(*this, node.rangeEnd);
		result.loopCounter = node.loopCounter;
		result.doExpr = boost::apply_visitor(*this, node.doExpr);
		result.asList = node.asList;
		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const ListConstractor& node)
	{
		ListConstractor result;
		for (const auto& expr : node.data)
		{
			result.data.push_back(boost::apply_visitor(*this, expr));
		}
		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const KeyExpr& node)
	{
		KeyExpr result(node.name);
		result.expr = boost::apply_visitor(*this, node.expr);
		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const RecordConstractor& node)
	{
		RecordConstractor result;
		for (const auto& expr : node.exprs)
		{
			result.exprs.push_back(boost::apply_visitor(*this, expr));
		}
		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const Accessor& node)
	{
		Accessor result;
		result.head = boost::apply_visitor(*this, node.head);

		for (const auto& access : node.accesses)
		{
			if (auto listAccess = AsOpt<ListAccess>(access))
			{
				ListAccess newListAccess(boost::apply_visitor(*this, listAccess.get().index));
				newListAccess.isArbitrary = listAccess.get().isArbitrary;
				result.add(newListAccess);
			}
			else if (auto recordAccess = AsOpt<RecordAccess>(access))
			{
				result.add(recordAccess.get());
			}
			else if (auto functionAccess = AsOpt<FunctionAccess>(access))
			{
				FunctionAccess access;
				for (const auto& argument : functionAccess.get().actualArguments)
				{
					access.add(boost::apply_visitor(*this, argument));
				}
				result.add(access);
			}
			else if (auto inheritAccess = AsOpt<InheritAccess>(access))
			{
				Expr originalAdder = inheritAccess.get().adder;
				Expr replacedAdder = boost::apply_visitor(*this, originalAdder);
				if (auto opt = AsOpt<RecordConstractor>(replacedAdder))
				{
					InheritAccess newInheritAccess(opt.get());
					result.add(newInheritAccess);
				}
				else
				{
					CGL_Error("node.adderの置き換え結果がRecordConstractorでない");
				}
			}
			else
			{
				CGL_Error("aaa");
			}
		}

		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const DeclSat& node)
	{
		return DeclSat(boost::apply_visitor(*this, node.expr)).setLocation(node);
	}

	Expr ExprTransformer::operator()(const DeclFree& node)
	{
		DeclFree result;
		for (const auto& freeVar : node.accessors)
		{
			Expr expr = freeVar.accessor;
			result.addAccessor(boost::apply_visitor(*this, expr));
			if (freeVar.range)
			{
				result.addRange(boost::apply_visitor(*this, freeVar.range.get()));
			}
		}
		return result.setLocation(node);
	}

	Expr ExprTransformer::operator()(const Import& node) { return node; }

	Expr ExprTransformer::operator()(const Range& node)
	{
		return Range(
			boost::apply_visitor(*this, node.lhs),
			boost::apply_visitor(*this, node.rhs)).setLocation(node);
	}

	Expr ExprTransformer::operator()(const Return& node)
	{
		return Return(boost::apply_visitor(*this, node.expr)).setLocation(node);
	}
}
