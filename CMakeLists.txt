cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0024 OLD)

project(Pita)

set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD 17)

set(Boost_USE_STATIC_LIBS ON)
set(GEOS_ENABLE_TESTS OFF CACHE BOOL "Disable geos tests" FORCE)
set(GEOS_BUILD_SHARED OFF CACHE BOOL "Build geos static library" FORCE)
set(BUILD_PYTHON OFF CACHE BOOL "Build libcmaes python binding" FORCE)

set(BUILD_TESTING OFF CACHE BOOL "Build test off" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build test off" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples off" FORCE)

if (MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /source-charset:utf-8")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /source-charset:utf-8")
endif()

set(CGL_EIGEN_TARGET_DIR "eigen")
set(CGL_GEOS_TARGET_DIR "geos")
set(CGL_LIBCMAES_TARGET_DIR "libcmaes")

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
include_directories(external/Siv3D)

#message("${PROJECT_BINARY_DIR}/${CGL_GEOS_TARGET_DIR}/lib")

link_directories(${PROJECT_BINARY_DIR}/${CGL_GEOS_TARGET_DIR}/lib)
link_directories(${PROJECT_BINARY_DIR}/${CGL_LIBCMAES_TARGET_DIR}/src)

add_subdirectory(external/Eigen ${CGL_EIGEN_TARGET_DIR})
add_subdirectory(external/geos ${CGL_GEOS_TARGET_DIR})

#message("SOURCE: ${PROJECT_SOURCE_DIR}/external/Eigen/cmake/UseEigen3.cmake")
#message("Destination: ${PROJECT_BINARY_DIR}/${CGL_EIGEN_TARGET_DIR}/")
configure_file(${PROJECT_SOURCE_DIR}/external/Eigen/cmake/UseEigen3.cmake
	${PROJECT_BINARY_DIR}/${CGL_EIGEN_TARGET_DIR}/UseEigen3.cmake COPYONLY)

set(Eigen3_DIR "${PROJECT_BINARY_DIR}/${CGL_EIGEN_TARGET_DIR}" CACHE PATH "set Eigen3_DIR to ${PROJECT_BINARY_DIR}/${CGL_EIGEN_TARGET_DIR}")

add_subdirectory(external/libcmaes ${CGL_LIBCMAES_TARGET_DIR})

include_directories(${PROJECT_BINARY_DIR}/${CGL_GEOS_TARGET_DIR}/include)

#message("Include: ${PROJECT_BINARY_DIR}/${CGL_LIBCMAES_TARGET_DIR}/src")
#link_directories(${PROJECT_BINARY_DIR}/${CGL_LIBCMAES_TARGET_DIR}/src)
include_directories(${PROJECT_BINARY_DIR}/${CGL_LIBCMAES_TARGET_DIR}/src)
include_directories(${PROJECT_BINARY_DIR}/${CGL_LIBCMAES_TARGET_DIR})

include_directories(external/stb)

include_directories(include)

add_executable(pita source/Source.cpp)
target_link_libraries(pita geos cmaes)
