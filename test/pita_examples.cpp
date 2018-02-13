#define NOMINMAX

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <Pita/Program.hpp>

std::ofstream ofs;
bool calculating;

BOOST_AUTO_TEST_SUITE(cgl)

BOOST_AUTO_TEST_CASE(test_3rects)
{
	Program program;

	std::ifstream ifs("3rects.cgl");
	std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	
	program.run(sourceCode, false);
	BOOST_CHECK(program.isSucceeded());
}

BOOST_AUTO_TEST_CASE(test_skeleton)
{
	Program program;

	std::ifstream ifs("skeleton.cgl");
	std::string sourceCode((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
	
	program.run(sourceCode, false);
	BOOST_CHECK(program.isSucceeded());
}

BOOST_AUTO_TEST_SUITE_END()
