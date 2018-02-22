#pragma warning(disable:4996)
#include <Unicode.hpp>

#include <Pita/Node.hpp>
#include <Pita/Context.hpp>
#include <Pita/OptimizationEvaluator.hpp>

namespace cgl
{
	Identifier Identifier::MakeIdentifier(const std::u32string& name_)
	{
		return Identifier(Unicode::UTF32ToUTF8(name_));
	}

	LRValue LRValue::Float(const std::u32string& str)
	{
		return LRValue(std::stod(Unicode::UTF32ToUTF8(str)));
	}

	bool LRValue::isValid() const
	{
		return IsType<Address>(value)
			? As<Address>(value).isValid()
			: true; //Reference と Val は常に有効であるものとする
	}

	std::string LRValue::toString() const
	{
		if (isAddress())
		{
			return std::string("Address(") + As<Address>(value).toString() + std::string(")");
		}
		else if (isReference())
		{
			return std::string("Reference(") + As<Reference>(value).toString() + std::string(")");
		}
		else
		{
			return std::string("Val(...)");
		}
	}

	Address LRValue::address(const Context & env) const
	{
		return IsType<Address>(value)
			? As<Address>(value)
			: env.getReference(As<Reference>(value));
	}

	Expr BuildString(const std::u32string& str32)
	{
		Expr expr;
		expr = LRValue(CharString(str32));

		return expr;
	}

	void OptimizationProblemSat::addConstraint(const Expr& logicExpr)
	{
		if (candidateExpr)
		{
			candidateExpr = BinaryExpr(candidateExpr.value(), logicExpr, BinaryOp::And);
		}
		else
		{
			candidateExpr = logicExpr;
		}
	}

	void OptimizationProblemSat::constructConstraint(std::shared_ptr<Context> pEnv, std::vector<Address>& freeVariables)
	{
		refs.clear();
		invRefs.clear();
		hasPlateausFunction = false;

		if (!candidateExpr || freeVariables.empty())
		{
			return;
		}

		/*Expr2SatExpr evaluator(0, pEnv, freeVariables);
		expr = boost::apply_visitor(evaluator, candidateExpr.value());
		refs.insert(refs.end(), evaluator.refs.begin(), evaluator.refs.end());

		{
			CGL_DebugLog("Print:");
			Printer printer;
			boost::apply_visitor(printer, expr.value());
			CGL_DebugLog("");
		}*/

		CGL_DebugLog("freeVariables:");
		for (const auto& val : freeVariables)
		{
			CGL_DebugLog(std::string("  Address(") + val.toString() + ")");
		}
		
		std::vector<char> usedInSat(freeVariables.size(), 0);
		//SatVariableBinder binder(pEnv, freeVariables);
		SatVariableBinder binder(pEnv, freeVariables, usedInSat, refs, invRefs, hasPlateausFunction);
		if (boost::apply_visitor(binder, candidateExpr.value()))
		{
			//refs = binder.refs;
			//invRefs = binder.invRefs;
			//hasPlateausFunction = binder.hasPlateausFunction;

			//satに出てこないfreeVariablesの削除
			for (int i = static_cast<int>(freeVariables.size()) - 1; 0 <= i; --i)
			{
				if (usedInSat[i] == 0)
				{
					freeVariables.erase(freeVariables.begin() + i);
				}
			}
		}
		else
		{
			refs.clear();
			invRefs.clear();
			freeVariables.clear();
			hasPlateausFunction = false;
		}

		/*{
			CGL_DebugLog("env:");
			pEnv->printContext(true);

			CGL_DebugLog("expr:");
			printExpr(candidateExpr.value());
		}*/
	}

	bool OptimizationProblemSat::initializeData(std::shared_ptr<Context> pEnv)
	{
		data.resize(refs.size());

		for (size_t i = 0; i < data.size(); ++i)
		{
			const Val val = pEnv->expand(refs[i]);
			if (auto opt = AsOpt<double>(val))
			{
				CGL_DebugLog(ToS(i) + " : " + ToS(opt.value()));
				data[i] = opt.value();
			}
			else if (auto opt = AsOpt<int>(val))
			{
				CGL_DebugLog(ToS(i) + " : " + ToS(opt.value()));
				data[i] = opt.value();
			}
			else
			{
				CGL_Error("存在しない参照をsatに指定した");
			}
		}

		return true;
	}

	double OptimizationProblemSat::eval(std::shared_ptr<Context> pEnv)
	{
		if (!candidateExpr)
		{
			return 0.0;
		}

		if (data.empty())
		{
			CGL_WarnLog("free式に有効な変数が指定されていません。");
			return 0.0;
		}

		/*{
			CGL_DebugLog("data:");
			for(int i=0;i<data.size();++i)
			{
				CGL_DebugLog(std::string("  ID(") + ToS(i) + ") -> " + ToS(data[i]));
			}

			CGL_DebugLog("refs:");
			for (int i = 0; i<refs.size(); ++i)
			{
				CGL_DebugLog(std::string("  ID(") + ToS(i) + ") -> Address(" + refs[i].toString() + ")");
			}

			CGL_DebugLog("invRefs:");
			for(const auto& keyval : invRefs)
			{
				CGL_DebugLog(std::string("  Address(") + keyval.first.toString() + ") -> ID(" + ToS(keyval.second) + ")");
			}

			CGL_DebugLog("env:");
			pEnv->printContext();

			CGL_DebugLog("expr:");
			printExpr(expr.value());
		}*/
		
		EvalSatExpr evaluator(pEnv, data, refs, invRefs);
		const Val evaluated = boost::apply_visitor(evaluator, candidateExpr.value());
		
		if (IsType<double>(evaluated))
		{
			return As<double>(evaluated);
		}
		else if (IsType<int>(evaluated))
		{
			return As<int>(evaluated);
		}

		CGL_Error("sat式の評価結果が不正");
	}

	//中身のアドレスを全て一つの値にまとめる
	class ValuePacker : public boost::static_visitor<PackedVal>
	{
	public:
		ValuePacker(const Context& context) :
			context(context)
		{}

		const Context& context;

		PackedVal operator()(bool node)const { return node; }
		PackedVal operator()(int node)const { return node; }
		PackedVal operator()(double node)const { return node; }
		PackedVal operator()(const CharString& node)const { return node; }
		PackedVal operator()(const List& node)const { return node.packed(context); }
		PackedVal operator()(const KeyValue& node)const { return node; }
		PackedVal operator()(const Record& node)const { return node.packed(context); }
		PackedVal operator()(const FuncVal& node)const { return node; }
		PackedVal operator()(const Jump& node)const { return node; }
	};

	//中身のアドレスを全て展開する
	class ValueUnpacker : public boost::static_visitor<Val>
	{
	public:
		ValueUnpacker(Context& context) :
			context(context)
		{}

		Context& context;

		Val operator()(bool node)const { return node; }
		Val operator()(int node)const { return node; }
		Val operator()(double node)const { return node; }
		Val operator()(const CharString& node)const { return node; }
		Val operator()(const PackedList& node)const { return node.unpacked(context); }
		Val operator()(const KeyValue& node)const { return node; }
		Val operator()(const PackedRecord& node)const { return node.unpacked(context); }
		Val operator()(const FuncVal& node)const { return node; }
		Val operator()(const Jump& node)const { return node; }
	};

	Val PackedList::unpacked(Context& context)const
	{
		ValueUnpacker unpacker(context);

		List result;

		for (const auto& val : data)
		{
			const Address address = val.address;

			const PackedVal& packedValue = val.value;
			const Val value = boost::apply_visitor(unpacker, packedValue);

			if (address.isValid())
			{
				context.TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, value);
				result.add(address);
			}
			else
			{
				result.add(context.makeTemporaryValue(value));
			}
		}

		return result;
	}

	PackedVal List::packed(const Context& context)const
	{
		ValuePacker packer(context);

		PackedList result;

		for (const Address address : data)
		{
			const Val& value = context.expand(address);
			const PackedVal packedValue = boost::apply_visitor(packer, value);

			result.add(address, packedValue);
		}

		return result;
	}

	Val PackedRecord::unpacked(Context& context)const
	{
		ValueUnpacker unpacker(context);

		Record result;

		for (const auto& keyval : values)
		{
			const Address address = keyval.second.address;

			const PackedVal& packedValue = keyval.second.value;
			const Val value = boost::apply_visitor(unpacker, packedValue);

			if (address.isValid())
			{
				context.TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, value);
				result.add(keyval.first, address);
			}
			else
			{
				result.add(keyval.first, context.makeTemporaryValue(value));
			}
		}

		result.problem = problem;
		result.freeVariables = freeVariables;
		result.freeVariableRefs = freeVariableRefs;
		result.type = type;
		result.isSatisfied = isSatisfied;
		result.pathPoints = pathPoints;
		
		return result;
	}

	PackedVal Record::packed(const Context& context)const
	{
		ValuePacker packer(context);

		PackedRecord result;

		for (const auto& keyval : values)
		{
			const Val& value = context.expand(keyval.second);
			const PackedVal packedValue = boost::apply_visitor(packer, value);

			result.add(keyval.first, keyval.second, packedValue);
		}

		result.problem = problem;
		result.freeVariables = freeVariables;
		result.freeVariableRefs = freeVariableRefs;
		result.type = type;
		result.isSatisfied = isSatisfied;
		result.pathPoints = pathPoints;

		return result;
	}
}
