# This file was generated automatically by cmkr.

cmake_minimum_required(VERSION 3.15)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(cmkr VERSION 0.1.1)

include(FetchContent)

FetchContent_Declare(
	toml11
	GIT_REPOSITORY https://github.com/ToruNiina/toml11
	)

FetchContent_MakeAvailable(toml11)


set(CMKRLIB_SOURCES
	"src/cmake.cpp"
	"src/gen.cpp"
	"src/help.cpp"
	"src/build.cpp"
	"src/error.cpp"
	)

add_library(cmkrlib STATIC ${CMKRLIB_SOURCES})

target_include_directories(cmkrlib PUBLIC
	"include"
	)

target_link_libraries(cmkrlib PUBLIC
	toml11::toml11
	)

target_compile_features(cmkrlib PUBLIC
	cxx_std_17
	)

set(CMKR_SOURCES
	"src/main.cpp"
	"src/args.cpp"
	)

add_executable(cmkr  ${CMKR_SOURCES})

target_link_libraries(cmkr PUBLIC
	cmkrlib
	)

install(
	TARGETS cmkr 
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	)



