cmake_minimum_required(VERSION 3.0)
project(cmk)

set(CMK_SRCS
    src/main.cpp
)

add_executable(cmk ${CMK_SRCS})

target_include_directories(cmk PRIVATE ${CMAKE_CURRENT_LIST_DIR}/vendor)