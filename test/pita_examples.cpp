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

std::vector<std::string> exampleFiles()
{
	const std::vector<std::string> filenames({
		"3rects.cgl",
		"cross.cgl",
		"path1.cgl",
		"path2.cgl",
		"skeleton.cgl",
		"str.cgl",
		"str2.cgl",
		"triforce.cgl",
		"fib1.cgl",
		"fib2.cgl",
		"grid.cgl",
		"rec_shape1.cgl",
		"rec_shape2.cgl",
		"rec_shape3.cgl",
		"constraint_dependency.cgl"
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

		std::ifstream ifs(filename);
		std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		auto start = std::chrono::system_clock::now();

		Program program;
		try
		{
			program.run(sourceCode, false);
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

		std::ifstream ifs(filename);
		std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		auto start = std::chrono::system_clock::now();

		Program program;
		try
		{
			program.run(sourceCode, false);
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
