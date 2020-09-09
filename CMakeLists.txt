cmake_minimum_required(VERSION 3.0)

project(cmkr VERSION 0.1.0)


set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMKRLIB_SOURCES
	"src/args.cpp"
	"src/gen.cpp"
	"src/help.cpp"
	)

add_library(cmkrlib STATIC ${CMKRLIB_SOURCES})

target_include_directories(cmkrlib PUBLIC
	"vendor"
	)

target_compile_features(cmkrlib PUBLIC
	"cxx_std_17"
	)

set(CMKR_SOURCES
	"src/main.cpp"
	)

add_executable(cmkr  ${CMKR_SOURCES})

target_link_libraries(cmkr PUBLIC
	"cmkrlib"
	)



