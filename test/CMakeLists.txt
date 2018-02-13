cmake_minimum_required(VERSION 3.1)

enable_testing()

if (MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /source-charset:utf-8")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /source-charset:utf-8")
endif()

add_executable(pita_test pita_test.cpp)
target_link_libraries(pita_test pita_lib)
add_test(
	NAME pita_test1
	COMMAND $<TARGET_FILE:pita_test>
	CONFIGURATIONS Release
	WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
)
set_tests_properties(pita_test1 PROPERTIES LABELS pita)
