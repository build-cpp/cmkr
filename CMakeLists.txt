# This file was generated automatically by cmkr.

# Regenerate CMakeLists.txt file when necessary
include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)

if(CMKR_INCLUDE_RESULT)
	cmkr()
endif()

cmake_minimum_required(VERSION 2.8...3.8)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project(cmkr
	LANGUAGES
		CXX
	VERSION 0.1.3
	DESCRIPTION "CMakeLists generator from TOML"
)

add_subdirectory(third_party)

# Target cmkrlib
set(cmkrlib_SOURCES
	"src/cmkrlib/args.cpp"
	"src/cmkrlib/build.cpp"
	"src/cmkrlib/cmake.cpp"
	"src/cmkrlib/error.cpp"
	"src/cmkrlib/gen.cpp"
	"src/cmkrlib/help.cpp"
	"src/cmkrlib/cmake.hpp"
	"src/cmkrlib/enum_helper.hpp"
	"src/cmkrlib/fs.hpp"
	"include/args.h"
	"include/build.h"
	"include/error.h"
	"include/gen.h"
	"include/help.h"
	"include/literals.h"
	cmake.toml
)

add_library(cmkrlib STATIC ${cmkrlib_SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${cmkrlib_SOURCES})

target_compile_features(cmkrlib PUBLIC
	cxx_std_11
)

target_include_directories(cmkrlib PUBLIC
	include
)

target_link_libraries(cmkrlib PUBLIC
	toml11
	ghc_filesystem
	mpark_variant
	ordered_map
)

# Target cmkr
set(cmkr_SOURCES
	"src/main.cpp"
	cmake.toml
)

add_executable(cmkr ${cmkr_SOURCES})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${cmkr_SOURCES})

target_link_libraries(cmkr PRIVATE
	cmkrlib
)

install(
	TARGETS cmkr 
	DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
	COMPONENT cmkr
)

