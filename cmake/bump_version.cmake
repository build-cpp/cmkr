cmake_minimum_required(VERSION 3.20)

if(NOT CMAKE_SCRIPT_MODE_FILE)
    message(FATAL_ERROR "Usage: cmake -P bump_version.cmake [1.2.3]")
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake.toml")
    message(FATAL_ERROR "Cannot find cmake.toml")
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/cmkr.cmake")
    message(FATAL_ERROR "Cannot find cmkr.cmake")
endif()

# Validate branch
find_package(Git REQUIRED)
execute_process(COMMAND "${GIT_EXECUTABLE}" branch --show-current OUTPUT_VARIABLE GIT_BRANCH)
string(STRIP "${GIT_BRANCH}" GIT_BRANCH)
if(NOT GIT_BRANCH STREQUAL "main")
    message(FATAL_ERROR "You need to be on the main branch, you are on: ${GIT_BRANCH}")
endif()

file(READ "${CMAKE_SOURCE_DIR}/cmake.toml" CMAKE_TOML)
string(FIND "${CMAKE_TOML}" "[project]" PROJECT_INDEX)
string(SUBSTRING "${CMAKE_TOML}" ${PROJECT_INDEX} -1 CMAKE_TOML_PROJECT)
set(SEMVER_REGEX "([0-9]+)\\.([0-9]+)\\.([0-9]+)")
set(VERSION_REGEX "version = \"${SEMVER_REGEX}\"")
if(CMAKE_TOML_PROJECT MATCHES "${VERSION_REGEX}")
    set(MAJOR "${CMAKE_MATCH_1}")
    set(MINOR "${CMAKE_MATCH_2}")
    set(PATCH "${CMAKE_MATCH_3}")
    set(OLDVERSION "${MAJOR}.${MINOR}.${PATCH}")
else()
    message(FATAL_ERROR "Failed to match semantic version in cmake.toml")
endif()

if(CMAKE_ARGV3)
    if(NOT CMAKE_ARGV3 MATCHES "${SEMVER_REGEX}")
        message(FATAL_ERROR "Invalid semantic version number '${CMAKE_ARGV3}'")
    endif()
    set(NEWVERSION "${CMAKE_ARGV3}")
else()
    math(EXPR NEWPATCH "${PATCH} + 1")
    set(NEWVERSION "${MAJOR}.${MINOR}.${NEWPATCH}")
endif()

message(STATUS "Version ${OLDVERSION} -> ${NEWVERSION}")

find_program(CMKR_EXECUTABLE "cmkr" PATHS "${CMAKE_SOURCE_DIR}/build" NO_CACHE REQUIRED)
message(STATUS "Found cmkr: ${CMKR_EXECUTABLE}")

# Replace version in cmake.toml
string(REPLACE "version = \"${OLDVERSION}\"" "version = \"${NEWVERSION}\"" CMAKE_TOML "${CMAKE_TOML}")
file(CONFIGURE
    OUTPUT "${CMAKE_SOURCE_DIR}/cmake.toml"
    CONTENT "${CMAKE_TOML}"
    @ONLY
    NEWLINE_STYLE LF
)

# Run cmkr gen
execute_process(COMMAND "${CMKR_EXECUTABLE}" gen RESULT_VARIABLE CMKR_EXEC_RESULT)
if(NOT CMKR_EXEC_RESULT EQUAL 0)
    message(FATAL_ERROR "cmkr gen failed (exit code ${CMKR_EXEC_RESULT})")
endif()

# Replace version in cmkr.cmake
file(READ "${CMAKE_SOURCE_DIR}/cmake/cmkr.cmake" CMKR_CMAKE)
string(REGEX REPLACE "CMKR_TAG \"[^\"]+\"" "CMKR_TAG \"v${NEWVERSION}\"" CMKR_CMAKE "${CMKR_CMAKE}")
file(CONFIGURE
    OUTPUT "${CMAKE_SOURCE_DIR}/cmake/cmkr.cmake"
    CONTENT "${CMKR_CMAKE}"
    @ONLY
    NEWLINE_STYLE LF
)

# Print git commands
message(STATUS "Git commands to create new version:\ngit commit -a -m \"Bump to ${NEWVERSION}\"\ngit tag v${NEWVERSION}\ngit push origin main v${NEWVERSION}")