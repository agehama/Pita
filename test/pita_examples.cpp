#define NOMINMAX

#include <vector>
#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <chrono>

//https://stackoverflow.com/questions/14861018/center-text-in-fixed-width-field-with-stream-manipulators-in-c
template<typename charT, typename traits = std::char_traits<charT> >
class center_helper {
	std::basic_string<charT, traits> str_;
public:
	center_helper(std::basic_string<charT, traits> str) : str_(str) {}
	template<typename a, typename b>
	friend std::basic_ostream<a, b>& operator<<(std::basic_ostream<a, b>& s, const center_helper<a, b>& c);
};

template<typename charT, typename traits = std::char_traits<charT> >
center_helper<charT, traits> centered(std::basic_string<charT, traits> str) {
	return center_helper<charT, traits>(str);
}

// redeclare for std::string directly so we can support anything that implicitly converts to std::string
center_helper<std::string::value_type, std::string::traits_type> centered(const std::string& str) {
	return center_helper<std::string::value_type, std::string::traits_type>(str);
}

template<typename charT, typename traits>
std::basic_ostream<charT, traits>& operator<<(std::basic_ostream<charT, traits>& s, const center_helper<charT, traits>& c) {
	std::streamsize w = s.width();
	if (w > static_cast<int>(c.str_.length())) {
		std::streamsize left = (w + c.str_.length()) / 2;
		s.width(left);
		s << c.str_;
		s.width(w - left);
		s << "";
	}
	else {
		s << c.str_;
	}
	return s;
}

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
		"triforce",
		"fib1",
		"fib2",
		"grid",
		"constraint_dependency",
		"koch_curve",
		"koch_snowflake",
		"rec_shape1",
		"rec_shape2",
		"rec_shape3",
		"skeleton",
		"pita_p3_",
		"pita_characteristics",
		"pita_logo"
	});

	return filenames;
}

BOOST_AUTO_TEST_SUITE(cgl)

BOOST_AUTO_TEST_CASE(test_examples_strict)
{
	const auto filenames = exampleFiles();

	std::unordered_map<std::string, ProfileResult> profileTimes;
	for (const auto& filename : filenames)
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
			profileTimes[filename] = program.profileResult();
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

	std::vector<std::string> names({ "", "parse time","execute time","output time" });
	names.insert(names.end(), filenames.begin(), filenames.end());

	const auto longestNameIt = std::max_element(names.begin(), names.end(), [](const std::string& a, const std::string& b) {return a.length() < b.length(); });
	const size_t longuestNameSize = longestNameIt->length() + 1;

	{
		std::stringstream ss;
		for (size_t i = 0; i < 4; ++i)
		{
			ss << std::setiosflags(std::ios::fixed)
				<< std::setprecision(6)
				<< std::setw(longuestNameSize)
				<< centered(names[i]);
			if (i + 1 != 4)
			{
				ss << "|";
			}
		}
		std::cout << ss.str() << "\n";

		const size_t tableWidth = ss.str().length();
		for (size_t i = 0; i < tableWidth; ++i)
		{
			std::cout << "-";
		}

		std::cout << "\n";
	}

	for (const auto& filename : filenames)
	{
		std::cout << std::setiosflags(std::ios::fixed)
			<< std::setprecision(6)
			<< std::setw(longuestNameSize)
			<< std::left
			<< filename << "|";

		auto it = profileTimes.find(filename);
		if (it == profileTimes.end())
		{
			continue;
		}

		const auto& times = it->second;
		for (size_t i = 0; i < 3; ++i)
		{
			std::cout << std::setiosflags(std::ios::fixed)
				<< std::setprecision(6)
				<< std::setw(longuestNameSize)
				<< std::right << times[i];
			if (i + 1 != 3)
			{
				std::cout << "|";
			}
		}

		std::cout << "\n";
	}
}

/*
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
*/

BOOST_AUTO_TEST_SUITE_END()
