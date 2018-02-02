cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0024 OLD)

project(parserTest2)

set(CMAKE_CXX_STANDARD 14)

set(Boost_USE_STATIC_LIBS ON)
set(GEOS_ENABLE_TESTS OFF CACHE BOOL "Disable geos tests" FORCE)
set(GEOS_BUILD_SHARED OFF CACHE BOOL "Build geos static library" FORCE)
set(BUILD_PYTHON OFF CACHE BOOL "Build libcmaes python binding" FORCE)

if (MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

find_package(Boost 1.57.0)
if (Boost_FOUND)
	include_directories (${Boost_INCLUDE_DIRS})
endif ()

include_directories(external/Eigen)
include_directories(external/CppNumericalSolvers/include)

include_directories(external/geos/include)
include_directories(external/geos/capi)
include_directories(external/geos)

include_directories(external/libcmaes/src)

#message("${PROJECT_BINARY_DIR}/lib-geos/lib")
link_directories(${PROJECT_BINARY_DIR}/lib-geos/lib)

add_subdirectory(external/Eigen lib-eigen)
add_subdirectory(external/geos lib-geos)

#message("SOURCE: ${PROJECT_SOURCE_DIR}/external/Eigen/cmake/UseEigen3.cmake")
#message("Destination: ${PROJECT_BINARY_DIR}/lib-eigen/")
configure_file(${PROJECT_SOURCE_DIR}/external/Eigen/cmake/UseEigen3.cmake
	${PROJECT_BINARY_DIR}/lib-eigen/UseEigen3.cmake COPYONLY)

add_subdirectory(external/libcmaes lib-libcmaes)

include_directories(${PROJECT_BINARY_DIR}/lib-geos/include)

#message("Include: ${PROJECT_BINARY_DIR}/lib-libcmaes/src")
link_directories(${PROJECT_BINARY_DIR}/lib-libcmaes/src)
include_directories(${PROJECT_BINARY_DIR}/lib-libcmaes/src)
include_directories(${PROJECT_BINARY_DIR}/lib-libcmaes)

include_directories(external/stb)

add_executable(core Source.cpp)
target_link_libraries(core geos cmaes)

