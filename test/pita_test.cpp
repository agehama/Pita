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
#ifdef comment
u8R"*(
(
	Print("--- Parse tests ---")

	a = 1
	+ 2
	Assert(a == 3, "comma separation(0)")
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
)
)*",
#endif
u8R"(
Print("--- ClosureMaker tests ---")
/*
(
	a = 1
	f = (->Print(a))
	Print(f)
	f()
)
//*/
(
	/*record = {
		//a: 1
	}*/
	a = 2
	//f = (->Print(a))

	f = (->@a = 3,Print(a))
	//f = (->record{@a = 3/*,Print(a),Print(@a)*/})

	Print(f)
	r = f()
	Print(a)
	//Assert(@a == 3, "ClosureMaker[0]")
	//Assert(r.a == 3, "ClosureMaker[1]")
)
/*
(
	makeFunc = (-> {a: 1})
	a = 2
	f = (->makeFunc(){get: (->@a)})

	record = f()
	record.a = 3
	a = 4

	Assert(record.get() == 3, "ClosureMaker[2]")
)
*/
)"
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
