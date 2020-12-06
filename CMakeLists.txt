# This file was generated automatically by cmkr.

# Regenerate CMakeLists.txt file when necessary
include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)

if(CMKR_INCLUDE_RESULT)
	cmkr()
endif()

cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(cmkr_PROJECT_VERSION 0.1.3)
project(cmkr VERSION ${cmkr_PROJECT_VERSION})

include(FetchContent)

FetchContent_Declare(
	filesystem
	GIT_REPOSITORY https://github.com/gulrak/filesystem
	)

FetchContent_MakeAvailable(filesystem)

FetchContent_Declare(
	mpark_variant
	URL https://github.com/mpark/variant/archive/v1.4.0.tar.gz
	)

FetchContent_MakeAvailable(mpark_variant)

FetchContent_Declare(
	toml11
	GIT_REPOSITORY https://github.com/ToruNiina/toml11
	)

FetchContent_MakeAvailable(toml11)

set(CMKRLIB_SOURCES
	src/cmake.cpp
	src/gen.cpp
	src/help.cpp
	src/build.cpp
	src/error.cpp
	)

add_library(cmkrlib STATIC ${CMKRLIB_SOURCES})

target_include_directories(cmkrlib PUBLIC
	include
	)

target_link_libraries(cmkrlib PUBLIC
	toml11::toml11
	ghc_filesystem
	mpark_variant
	)

target_compile_features(cmkrlib PUBLIC
	cxx_std_11
	)

set(CMKR_SOURCES
	src/main.cpp
	src/args.cpp
	)

add_executable(cmkr  ${CMKR_SOURCES})

target_link_libraries(cmkr PUBLIC
	cmkrlib
	)

install(
	TARGETS cmkr 
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	COMPONENT cmkr
	)



