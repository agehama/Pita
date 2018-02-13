cmake_minimum_required(VERSION 3.1)

if (MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /source-charset:utf-8")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /source-charset:utf-8")
endif()

add_executable(pita_test pita_test.cpp)
target_link_libraries(pita_test pita_lib)
add_test(
	NAME pita_test1
	COMMAND $<TARGET_FILE:pita_test>
)
set_tests_properties(pita_test1 PROPERTIES LABELS pita)

add_executable(pita_examples pita_examples.cpp)
target_link_libraries(pita_examples pita_lib)
add_test(
	NAME pita_examples1
	COMMAND $<TARGET_FILE:pita_examples>
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/examples
)
set_tests_properties(pita_examples1 PROPERTIES LABELS pita)
