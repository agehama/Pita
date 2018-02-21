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
			: true; //Reference と Evaluated は常に有効であるものとする
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
			return std::string("Evaluated(...)");
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
			const Evaluated val = pEnv->expand(refs[i]);
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
		const Evaluated evaluated = boost::apply_visitor(evaluator, candidateExpr.value());
		
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

	class IsPacked : public boost::static_visitor<bool>
	{
	public:
		IsPacked() = default;

		bool operator()(bool node)const { return true; }
		bool operator()(int node)const { return true; }
		bool operator()(double node)const { return true; }
		bool operator()(const CharString& node)const { return true; }
		bool operator()(const List& node)const { return node.isPacked(); }
		bool operator()(const KeyValue& node)const { return true; }
		bool operator()(const Record& node)const { return node.isPacked(); }
		bool operator()(const FuncVal& node)const { return true; }
		bool operator()(const Jump& node)const { return true; }
	};

	class IsUnpacked : public boost::static_visitor<bool>
	{
	public:
		IsUnpacked(const Context& context):
			context(context)
		{}

		const Context& context;

		bool operator()(bool node)const { return true; }
		bool operator()(int node)const { return true; }
		bool operator()(double node)const { return true; }
		bool operator()(const CharString& node)const { return true; }
		bool operator()(const List& node)const { return node.isUnpacked(context); }
		bool operator()(const KeyValue& node)const { return true; }
		bool operator()(const Record& node)const { return node.isUnpacked(context); }
		bool operator()(const FuncVal& node)const { return true; }
		bool operator()(const Jump& node)const { return true; }
	};

	//中身のアドレスを全て一つの値にまとめる
	class ValuePacker : public boost::static_visitor<void>
	{
	public:
		ValuePacker(const Context& context) :
			context(context)
		{}

		const Context& context;
		
		void operator()(bool node) {}
		void operator()(int node) {}
		void operator()(double node) {}
		void operator()(CharString& node) {}
		void operator()(List& node) { node.pack(context); }
		void operator()(KeyValue& node) {}
		void operator()(Record& node) { node.pack(context); }
		void operator()(FuncVal& node) {}
		void operator()(Jump& node) {}
	};

	//中身のアドレスを全て展開する
	class ValueUnpacker : public boost::static_visitor<void>
	{
	public:
		ValueUnpacker(Context& context):
			context(context)
		{}
		
		Context& context;

		void operator()(bool node) {}
		void operator()(int node) {}
		void operator()(double node) {}
		void operator()(CharString& node) {}
		void operator()(List& node) { node.unpack(context); }
		void operator()(KeyValue& node) {}
		void operator()(Record& node) { node.unpack(context); }
		void operator()(FuncVal& node) {}
		void operator()(Jump& node) {}
	};

	bool List::isPacked()const
	{
		if (!IsType<PackedList>(data))
		{
			return false;
		}

		IsPacked isPacked;
		
		const PackedList& packedList = As<PackedList>(data);
		for (const auto& val : packedList.data)
		{
			Evaluated evaluated = val.value;
			if (!boost::apply_visitor(isPacked, evaluated))
			{
				return false;
			}
		}

		return true;
	}

	bool List::isUnpacked(const Context& context)const
	{
		if (!IsType<UnpackedList>(data))
		{
			return false;
		}

		IsUnpacked isUnpacked(context);

		const UnpackedList& unpackedList = As<UnpackedList>(data);
		for (const Address address : unpackedList.data)
		{
			if (!boost::apply_visitor(isUnpacked, context.expand(address)))
			{
				return false;
			}
		}

		return true;
	}

	void List::pack(const Context& context)
	{
		ValuePacker packer(context);

		PackedList result;

		if (IsType<PackedList>(data))
		{
			const PackedList& packedList = As<PackedList>(data);
			for (const auto& val : packedList.data)
			{
				Evaluated evaluated = val.value;
				boost::apply_visitor(packer, evaluated);

				result.add(val.address, evaluated);
			}
		}
		else
		{
			const UnpackedList& unpackedList = As<UnpackedList>(data);
			for (const Address address : unpackedList.data)
			{
				Evaluated evaluated = context.expand(address);
				boost::apply_visitor(packer, evaluated);

				result.add(address, evaluated);
			}
		}

		data = result;
	}

	void List::unpack(Context& context)
	{
		ValueUnpacker unpacker(context);

		UnpackedList result;

		if (IsType<PackedList>(data))
		{
			const PackedList& packedList = As<PackedList>(data);
			for (const auto& val : packedList.data)
			{
				const Address address = val.address;
				
				Evaluated evaluated = val.value;
				boost::apply_visitor(unpacker, evaluated);

				context.TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, evaluated);

				result.add(address);
			}
		}
		else
		{
			const UnpackedList& unpackedList = As<UnpackedList>(data);
			for (const Address address : unpackedList.data)
			{
				Evaluated evaluated = context.expand(address);
				boost::apply_visitor(unpacker, evaluated);

				context.TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, evaluated);

				result.add(address);
			}
		}

		data = result;
	}


	List List::packed(const Context& context)const
	{
		List list(*this);
		list.pack(context);
		return list;
	}

	List List::unpacked(Context& context)const
	{
		List list(*this);
		list.unpack(context);
		return list;
	}

	bool Record::isPacked()const
	{
		if (!IsType<PackedRecord>(values))
		{
			return false;
		}

		IsPacked isPacked;

		const PackedRecord& packedRecord = As<PackedRecord>(values);
		for (const auto& keyval : packedRecord.values)
		{
			if (!boost::apply_visitor(isPacked, keyval.second.value))
			{
				return false;
			}
		}

		return true;
	}

	bool Record::isUnpacked(const Context& context)const
	{
		if (!IsType<UnpackedRecord>(values))
		{
			return false;
		}

		IsUnpacked isUnpacked(context);

		const UnpackedRecord& unpackedRecord = As<UnpackedRecord>(values);
		for (const auto& keyval : unpackedRecord.values)
		{
			if (!boost::apply_visitor(isUnpacked, context.expand(keyval.second)))
			{
				return false;
			}
		}

		return true;
	}

	void Record::pack(const Context& context)
	{
		if (isPacked())
		{
			return;
		}

		ValuePacker packer(context);

		PackedRecord result;

		if (IsType<PackedRecord>(values))
		{
			const PackedRecord& packedRecord = As<PackedRecord>(values);
			for (const auto& keyval : packedRecord.values)
			{
				Evaluated evaluated = keyval.second.value;
				boost::apply_visitor(packer, evaluated);
				
				result.add(keyval.first, keyval.second.address, evaluated);
			}
		}
		else
		{
			const UnpackedRecord& unpackedRecord = As<UnpackedRecord>(values);
			for (const auto& keyval : unpackedRecord.values)
			{
				Evaluated evaluated = context.expand(keyval.second);
				boost::apply_visitor(packer, evaluated);

				result.add(keyval.first, keyval.second, evaluated);
			}
		}

		values = result;
	}

	void Record::unpack(Context& context)
	{
		if (isUnpacked(context))
		{
			return;
		}

		ValueUnpacker unpacker(context);

		UnpackedRecord result;

		if (IsType<PackedRecord>(values))
		{
			const PackedRecord& packedRecord = As<PackedRecord>(values);
			for (const auto& keyval : packedRecord.values)
			{
				const Address address = keyval.second.address;

				Evaluated evaluated = keyval.second.value;
				boost::apply_visitor(unpacker, evaluated);

				context.TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, evaluated);

				result.add(keyval.first, address);
			}
		}
		else
		{
			const UnpackedRecord& unpackedRecord = As<UnpackedRecord>(values);
			for (const auto& keyval : unpackedRecord.values)
			{
				const Address address = keyval.second;

				Evaluated evaluated = context.expand(keyval.second);
				boost::apply_visitor(unpacker, evaluated);

				context.TODO_Remove__ThisFunctionIsDangerousFunction__AssignToObject(address, evaluated);

				result.add(keyval.first, address);
			}
		}

		values = result;
	}

	Record Record::packed(const Context& context)const
	{
		Record record(*this);
		record.pack(context);
		return record;
	}

	Record Record::unpacked(Context& context)const
	{
		Record record(*this);
		record.unpack(context);
		return record;
	}
}
