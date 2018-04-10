cmake_minimum_required(VERSION 3.1)

cmake_policy(SET CMP0024 OLD)

if(WIN32)
  if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio ([0-9]+)")
    set(CMAKE_GENERATOR_TOOLSET "host=x64" CACHE STRING "CMAKE_GENERATOR_TOOLSET" FORCE)
  endif()
endif()

project(Pita)

if(MSVC)
  set(CMAKE_CXX_STANDARD 17)
else()
  set(CMAKE_CXX_STANDARD 14)
endif()

set(Boost_USE_STATIC_LIBS ON)

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build libraries static" FORCE)

set(GEOS_ENABLE_TESTS OFF CACHE BOOL "Disable geos tests" FORCE)
set(GEOS_BUILD_SHARED OFF CACHE BOOL "Build geos as static library" FORCE)
set(GEOS_ENABLE_INLINE OFF CACHE BOOL "Disable geos inline option" FORCE)
set(BUILD_PYTHON OFF CACHE BOOL "Build libcmaes python binding" FORCE)

set(BUILD_TESTING OFF CACHE BOOL "Build test off" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "Build test off" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build examples off" FORCE)

find_package(OpenMP)
if(OPENMP_FOUND)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT /source-charset:utf-8")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd /source-charset:utf-8")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024 -stdlib=libc++")
#  include_directories(/usr/include/libcxxabi)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=1024")
endif()

set(CGL_EIGEN_TARGET_DIR "eigen")
set(CGL_GEOS_TARGET_DIR "geos")
set(CGL_LIBCMAES_TARGET_DIR "libcmaes")

find_package(Boost 1.57.0)
if(Boost_FOUND)
  include_directories (${Boost_INCLUDE_DIRS})
endif()

include_directories(external/Eigen)
include_directories(external/CppNumericalSolvers/include)
include_directories(external/geos/include)
include_directories(external/geos/capi)
include_directories(external/geos)
include_directories(external/libcmaes/src)
include_directories(external/Siv3D)
include_directories(external/cxxopts/include)

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

add_library(pita_lib STATIC source/BinaryEvaluator.cpp source/Context.cpp source/Evaluator.cpp source/FontShape.cpp source/Geometry.cpp source/Node.cpp source/OptimizationEvaluator.cpp source/Printer.cpp source/Program.cpp source/Parser.cpp source/Vectorizer.cpp)
if(MSVC)
  target_link_libraries(pita_lib geos cmaes)
else()
  target_link_libraries(pita_lib geos cmaes stdc++fs)
endif()

add_executable(pita source/Source.cpp)
target_link_libraries(pita pita_lib)

enable_testing()
add_subdirectory(test)
