# This file was generated automatically by cmkr.

# Regenerate CMakeLists.txt file when necessary
include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)

if(CMKR_INCLUDE_RESULT)
	cmkr()
endif()

cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(example_PROJECT_VERSION 0.1.0)
project(example VERSION ${example_PROJECT_VERSION})

set(EXAMPLE_SOURCES
	src/example.cpp
)

add_executable(example ${EXAMPLE_SOURCES})



