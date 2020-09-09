cmake_minimum_required(VERSION 3.0)

project(cmkr VERSION 0.1.0)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)



set(CMKR_SOURCES
	"src/main.cpp"
	"src/args.cpp"
	"src/gen.cpp"
	)

add_executable(cmkr ${CMKR_SOURCES})

target_include_directories(cmkr PUBLIC
	"vendor"
	)

target_compile_features(cmkr PUBLIC
	"cxx_std_17"
	)

