#define NOMINMAX

#include <vector>
#include <string>
#include <fstream>

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
	});

	return filenames;
}

BOOST_AUTO_TEST_SUITE(cgl)

BOOST_AUTO_TEST_CASE(test_examples_strict)
{
	for (const auto& filename : exampleFiles())
	{
		constraintViolationCount = 0;

		Program program;

		std::ifstream ifs(filename);
		std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		program.run(sourceCode, false);
		BOOST_CHECK(program.isSucceeded());
		BOOST_CHECK_EQUAL(constraintViolationCount, 0);
	}
}

BOOST_AUTO_TEST_CASE(test_examples_easy)
{
	for (const auto& filename : exampleFiles())
	{
		Program program;

		std::ifstream ifs(filename);
		std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

		program.run(sourceCode, false);
		BOOST_CHECK(program.isSucceeded());
	}
}

BOOST_AUTO_TEST_SUITE_END()
