cmake_minimum_required(VERSION 3.0)
project(cmkr)

project(cmkr VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(cmkr_SOURCES
    src/main.cpp
    src/args.cpp
    src/gen.cpp
)

add_executable(cmkr ${cmkr_SOURCES})

target_compile_features(cmkr PRIVATE cxx_std_17)

target_include_directories(cmkr PRIVATE ${CMAKE_CURRENT_LIST_DIR}/vendor)
