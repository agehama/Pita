cmake_minimum_required(VERSION 3.1)

project(parserTest2)

set(CMAKE_CXX_STANDARD 14)

set(Boost_USE_STATIC_LIBS ON)
set(GEOS_ENABLE_TESTS OFF CACHE BOOL "Disable geos tests" FORCE)
set(GEOS_BUILD_SHARED OFF CACHE BOOL "Build geos static library" FORCE)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

find_package(Boost 1.57.0)
if (Boost_FOUND)
	include_directories (${Boost_INCLUDE_DIRS})
endif ()

include_directories(external/Eigen)
include_directories(external/CppNumericalSolvers/include)

include_directories(external/geos/include)
include_directories(external/geos/capi)
include_directories(external/geos)

add_executable(core Source.cpp)
#add_subdirectory(external/geos)