cmake_minimum_required(VERSION 3.1)

if (MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /source-charset:utf-8")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /source-charset:utf-8")
endif()


add_executable(pita_test pita_test.cpp)
target_link_libraries(pita_test pita_lib)

add_test(
	NAME pita_test
	COMMAND $<TARGET_FILE:pita_test>
)
set_tests_properties(pita_test PROPERTIES LABELS pita)


add_executable(pita_examples pita_examples.cpp)
target_link_libraries(pita_examples pita_lib)

add_test(
	NAME pita_strict
	COMMAND $<TARGET_FILE:pita_examples> --run_test=cgl/test_examples_strict
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples
)
set_tests_properties(pita_strict PROPERTIES LABELS pita)

add_test(
	NAME pita_easy
	COMMAND $<TARGET_FILE:pita_examples> --run_test=cgl/test_examples_easy
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples
)
set_tests_properties(pita_easy PROPERTIES LABELS pita)
