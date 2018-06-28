#define NOMINMAX

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <Pita/Program.hpp>

BOOST_AUTO_TEST_SUITE(cgl)

BOOST_AUTO_TEST_CASE(test_case1)
{
	std::vector<std::string> testCases({
u8R"(
a = 1
+2
)"
	});

	for (const auto& source: testCases)
	{
		Program program;
		program.executeInline(source, false);
		BOOST_CHECK(program.isSucceeded());
	}
}

BOOST_AUTO_TEST_SUITE_END()
