cmake_minimum_required(VERSION 3.20)

find_package(Git REQUIRED)

execute_process(COMMAND
    "${GIT_EXECUTABLE}" name-rev --tags --name-only HEAD
    OUTPUT_VARIABLE GIT_TAG
)

string(FIND "${GIT_TAG}" "\n" NEWLINE_POS)
string(SUBSTRING "${GIT_TAG}" 0 ${NEWLINE_POS} GIT_TAG)
string(STRIP "${GIT_TAG}" GIT_TAG)

if("${GIT_TAG}" STREQUAL "")
    message(FATAL_ERROR "Failed to retrieve git tag!")
endif()

message(STATUS "CMKR_TAG: '${GIT_TAG}'")

file(READ "cmake/cmkr.cmake" CMKR_CMAKE)
string(REGEX REPLACE "CMKR_TAG \"[^\"]+\"" "CMKR_TAG \"${GIT_TAG}\"" CMKR_CMAKE "${CMKR_CMAKE}")
file(CONFIGURE
    OUTPUT "cmake/cmkr.cmake"
    CONTENT "${CMKR_CMAKE}"
    @ONLY
    NEWLINE_STYLE LF
)