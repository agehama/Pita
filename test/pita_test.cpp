#define NOMINMAX

#define BOOST_TEST_MAIN
#include <boost/test/included/unit_test.hpp>

#include <Pita/Program.hpp>

extern bool isDebugMode;
extern bool isBlockingMode;

BOOST_AUTO_TEST_SUITE(cgl)

BOOST_AUTO_TEST_CASE(test_case1)
{
	std::vector<std::string> testCases({
u8R"*(
(
	Print("--- Parse tests ---")

	a = 1
	+ 2
	Assert(a == 3, "comma separation(0)")

	Print("Passed")
)
(
	Print("--- Comment tests ---")

	//Assert(false, "line comment")

	/*
	Assert(false, "scope comment(0)")
	*/

	a = 1
	/*
	a = 2
	//*/
	Assert(a == 1, "scope comment(1)")

	a = 1
	//*
	a = 2
	//*/
	Assert(a == 2, "scope comment(2)")

	/*
	Assert(false, "scope nest comment")
	/*
	Assert(false, "scope nest comment")
	*/
	Assert(false, "scope nest comment")
	*/

	Print("Passed")
)
)*",
u8R"*(
Print("--- ClosureMaker tests ---")
(
	Base = {
		func1: (->0)
	}

	Derived = Base{
		func2: (->func1())
	}

	Assert(Derived.func2() == 0, "closure maker(1)")
)
(
	record = {
		a: 1
	}
	a = 2
	f = (->record{a = 3})

	r = f()

	Assert(a == 2, "closure maker(2)")

	//TODO
	//Assert(r.a == 3, "closure maker(3)")

	Print("Passed")
)
)*"
	});

	isDebugMode = true;
	isBlockingMode = false;

	for (const auto& source: testCases)
	{
		Program program;
		program.executeInline(source, false);
		BOOST_CHECK(program.isSucceeded());
	}
}

BOOST_AUTO_TEST_SUITE_END()
