#define NOMINMAX

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <Pita/Program.hpp>

extern int constraintViolationCount;

//const std::string directory("../../Pita/examples/");
const std::string directory("./");

std::vector<std::string> exampleFiles()
{
	const std::vector<std::string> filenames({
		"3rects",
		"cross",
		"path1",
		"path2",
		"skeleton",
		"str",
		"str2",
		"triforce",
		"fib1",
		"fib2",
		"grid",
		"rec_shape1",
		"rec_shape2",
		"rec_shape3",
		"koch_curve",
		"koch_snowflake",
		"constraint_dependency"
	});

	return filenames;
}

BOOST_AUTO_TEST_SUITE(cgl)

BOOST_AUTO_TEST_CASE(test_examples_strict)
{
	for (const auto& filename : exampleFiles())
	{
		constraintViolationCount = 0;

		std::cout << "----------------------------------------------\n";
		std::cout << "run \"" << filename << "\" ...\n";

		//std::ifstream ifs(directory + filename + ".cgl");
		//std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		std::string sourcePath(directory + filename + ".cgl");

		auto start = std::chrono::system_clock::now();

		Program program;
		try
		{
			//program.execute1(sourceCode, filename + ".svg", false);
			program.execute1(sourcePath, filename + ".svg", false);
		}
		catch(const std::exception& e)
		{
			std::cout << "error: " << e.what() << "\n";
			BOOST_CHECK(false);
			continue;
		}
		
		auto end = std::chrono::system_clock::now();

		const double time = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
		std::cout << "time: " << time*0.001 << "[sec]\n";

		BOOST_CHECK(program.isSucceeded());
		BOOST_CHECK_EQUAL(constraintViolationCount, 0);
	}
}

BOOST_AUTO_TEST_CASE(test_examples_easy)
{
	for (const auto& filename : exampleFiles())
	{
		std::cout << "----------------------------------------------\n";
		std::cout << "run \"" << filename << "\" ...\n";

		//std::ifstream ifs(directory + filename + ".cgl");
		//std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		std::string sourcePath(directory + filename + ".cgl");

		auto start = std::chrono::system_clock::now();

		Program program;
		try
		{
			//program.run(sourceCode, false);
			//program.execute1(sourceCode, filename + ".svg", false);
			program.execute1(sourcePath, filename + ".svg", false);
		}
		catch (const std::exception& e)
		{
			std::cout << "error: " << e.what() << "\n";
			BOOST_CHECK(false);
			continue;
		}
		
		auto end = std::chrono::system_clock::now();

		const double time = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
		std::cout << "time: " << time*0.001 << "[sec]\n";

		BOOST_CHECK(program.isSucceeded());
	}
}

BOOST_AUTO_TEST_SUITE_END()
