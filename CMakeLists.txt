# This file was generated automatically by cmkr.

# Regenerate CMakeLists.txt file when necessary
include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)

if(CMKR_INCLUDE_RESULT)
	cmkr()
endif()

cmake_minimum_required(VERSION 3.15)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(cmkr_PROJECT_VERSION 0.1.3)
project(cmkr VERSION ${cmkr_PROJECT_VERSION})

add_subdirectory(third_party)

set(cmkrlib_SOURCES
	src/cmkrlib/args.cpp
	src/cmkrlib/build.cpp
	src/cmkrlib/cmake.cpp
	src/cmkrlib/error.cpp
	src/cmkrlib/gen.cpp
	src/cmkrlib/help.cpp
	src/cmkrlib/cmake.hpp
	src/cmkrlib/fs.hpp
	include/args.h
	include/build.h
	include/error.h
	include/gen.h
	include/help.h
	include/literals.h
	cmake.toml
)

add_library(cmkrlib STATIC ${cmkrlib_SOURCES})

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${cmkrlib_SOURCES})

target_include_directories(cmkrlib PUBLIC
	include
)

target_link_libraries(cmkrlib PUBLIC
	toml11
	ghc_filesystem
	mpark_variant
)

target_compile_features(cmkrlib PUBLIC
	cxx_std_11
)

set(cmkr_SOURCES
	src/main.cpp
	cmake.toml
)

add_executable(cmkr ${cmkr_SOURCES})

source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${cmkr_SOURCES})

target_link_libraries(cmkr PRIVATE
	cmkrlib
)

install(
	TARGETS cmkr 
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	COMPONENT cmkr
)



