#define NOMINMAX

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <Pita/Program.hpp>

std::ofstream ofs;
bool calculating;

BOOST_AUTO_TEST_SUITE(cgl)

BOOST_AUTO_TEST_CASE(test_case1)
{
	int x = 2;
	BOOST_CHECK_EQUAL(x, 2);
}

BOOST_AUTO_TEST_CASE(test_case2)
{
	Program program;
	std::string test1 = u8R"(
a = 1
+2
)";
	program.run(test1, false);
	BOOST_CHECK(program.isSucceeded());
	auto opt = program.asIntOpt();
	if (opt)
	{
		BOOST_CHECK_EQUAL(opt.value(), 3);
	}
}

BOOST_AUTO_TEST_SUITE_END()
