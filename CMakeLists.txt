# This file was generated automatically by cmkr.

# Create a configure-time dependency on cmake.toml to improve IDE support
configure_file(cmake.toml cmake.toml COPYONLY)

cmake_minimum_required(VERSION 2.8...3.8)

# Regenerate CMakeLists.txt file when necessary
set(CMKR_ROOT_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(CMKR_ROOT_PROJECT ON)

	include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)
	if(CMKR_INCLUDE_RESULT)
		cmkr()
	endif()

endif()

# Enable folder support
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Hack to hide a warning during cmkr bootstrapping on Windows
if(CMAKE_BUILD_TYPE)
endif()


project(cmkr
	LANGUAGES
		CXX
	VERSION
		0.1.3
	DESCRIPTION
		"CMakeLists generator from TOML"
)

set(CMKR_CMAKE_FOLDER ${CMAKE_FOLDER})
if(CMAKE_FOLDER)
	set(CMAKE_FOLDER "${CMAKE_FOLDER}/third_party")
else()
	set(CMAKE_FOLDER third_party)
endif()
add_subdirectory(third_party)
set(CMAKE_FOLDER ${CMKR_CMAKE_FOLDER})


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

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT cmkr)
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${cmkr_SOURCES})

target_link_libraries(cmkr PRIVATE
	cmkrlib
)

install(
	TARGETS
		cmkr
	DESTINATION
		"${CMAKE_INSTALL_PREFIX}/bin"
	COMPONENT
		cmkr
)
