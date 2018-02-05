#pragma once
#include <Eigen/Core>

#include "Node.hpp"
#include "Printer.hpp"
#include "Environment.hpp"
#include "Evaluator.hpp"
#include "OptimizationEvaluator.hpp"
#include "Parser.hpp"
#include "Vectorizer.hpp"

namespace cgl
{
	class Program
	{
	public:
		Program() :
			pEnv(Environment::Make()),
			evaluator(pEnv)
		{}

		boost::optional<Expr> parse(const std::string& program);

		boost::optional<Evaluated> execute(const std::string& program);

		bool draw(const std::string& program, bool logOutput = true);

		void execute1(const std::string& program, bool logOutput = true);

		void clear();

		bool test(const std::string& program, const Expr& expr);

		std::shared_ptr<Environment> getEnvironment()
		{
			return pEnv;
		}

		boost::optional<Evaluated>& getEvaluated()
		{
			return evaluated;
		}

		bool isSucceeded()const
		{
			return succeeded;
		}

	private:
		std::shared_ptr<Environment> pEnv;
		Eval evaluator;
		boost::optional<Evaluated> evaluated;
		bool succeeded;
	};
}
