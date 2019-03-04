cmake_minimum_required(VERSION 3.12)
project(glvdp C)

option(GLVDP_BUILD_EXAMPLES "Build the examples" ON)

set(CMAKE_C_STANDARD 99)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(stringify.cmake)
add_executable(bin2c bin2c.c)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(externals/glfw)

set(GL3W_SOURCE_DIR "${CMAKE_SOURCE_DIR}/externals/gl3w")
if(NOT EXISTS "${GL3W_SOURCE_DIR}/src/gl3w.c")
	message(STATUS "Generating gl3w loader")
	find_package(PythonInterp REQUIRED)
	execute_process(COMMAND "${PYTHON_EXECUTABLE}" "${GL3W_SOURCE_DIR}/gl3w_gen.py" WORKING_DIRECTORY "${GL3W_SOURCE_DIR}")
endif()
add_library(gl3w STATIC "${GL3W_SOURCE_DIR}/src/gl3w.c")
include_directories("${GL3W_SOURCE_DIR}/include")

set(LODEPNG_SOURCE_DIR "${CMAKE_SOURCE_DIR}/externals/lodepng")
if(NOT EXISTS "${LODEPNG_SOURCE_DIR}/lodepng.c")
	file(RENAME "${LODEPNG_SOURCE_DIR}/lodepng.cpp" "${LODEPNG_SOURCE_DIR}/lodepng.c")
endif()
add_library(lodepng STATIC "${LODEPNG_SOURCE_DIR}/lodepng.c")
include_directories("${LODEPNG_SOURCE_DIR}")

include_directories(include)

add_subdirectory(src)

if(GLVDP_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
