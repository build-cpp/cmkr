cmake_minimum_required(VERSION 3.0)
project(cmk)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMK_SRCS
    src/main.cpp
)

add_executable(cmk ${CMK_SRCS})
target_compile_features(cmk PRIVATE cxx_std_17)
target_include_directories(cmk PRIVATE ${CMAKE_CURRENT_LIST_DIR}/vendor)