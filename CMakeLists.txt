cmake_minimum_required(VERSION 3.1)

project(parserTest2)

set(CMAKE_CXX_STANDARD 14)

set(Boost_USE_STATIC_LIBS ON)

find_package(Boost 1.57.0)
if (Boost_FOUND)
	include_directories (${Boost_INCLUDE_DIRS})
endif ()

include_directories(external/Eigen)
include_directories(external/CppNumericalSolvers/include)

add_executable(core Source.cpp)
