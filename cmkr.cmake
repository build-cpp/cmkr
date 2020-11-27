include_guard()

# Disable cmkr if no cmake.toml file is found
if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/cmake.toml)
    message(STATUS "[cmkr] Not found: ${CMAKE_CURRENT_LIST_DIR}/cmake.toml")
    macro(cmkr)
    endmacro()
    return()
endif()

# Add a build-time dependency on the contents of cmake.toml to regenerate the CMakeLists.txt when modified
configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake.toml ${CMAKE_CURRENT_BINARY_DIR}/cmake.toml COPYONLY)

# Helper macro to execute a process (COMMAND_ERROR_IS_FATAL ANY is 3.19 and higher)
macro(cmkr_exec)
    execute_process(COMMAND ${ARGV} RESULT_VARIABLE CMKR_EXEC_RESULT)
    if(NOT CMKR_EXEC_RESULT EQUAL 0)
        message(FATAL_ERROR "cmkr_exec(${ARGV}) failed (exit code ${CMKR_EXEC_RESULT})")
    endif()
endmacro()

# Windows-specific hack (CMAKE_EXECUTABLE_PREFIX is not set at the moment)
if(WIN32)
    set(CMKR_EXECUTABLE_NAME "cmkr.exe")
else()
    set(CMKR_EXECUTABLE_NAME "cmkr")
endif()

# Use cached cmkr if found
if(DEFINED CACHE{CMKR_EXECUTABLE} AND EXISTS ${CMKR_EXECUTABLE})
    message(VERBOSE "[cmkr] Found cmkr: '${CMKR_EXECUTABLE}'")
else()
    if(DEFINED CACHE{CMKR_EXECUTABLE})
        message(VERBOSE "[cmkr] '${CMKR_EXECUTABLE}' not found")
    endif()
    set(CMKR_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/_cmkr)
    set(CMKR_EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/_cmkr/bin/${CMKR_EXECUTABLE_NAME} CACHE INTERNAL "Full path to cmkr executable")
    if(NOT EXISTS ${CMKR_EXECUTABLE})
        message(VERBOSE "[cmkr] Bootstrapping '${CMKR_EXECUTABLE}'")
        message(STATUS "[cmkr] Fetching cmkr...")
        if(EXISTS ${CMKR_DIRECTORY})
            cmkr_exec(${CMAKE_COMMAND} -E rm -rf ${CMKR_DIRECTORY})
        endif()
        find_package(Git QUIET REQUIRED)
        set(CMKR_REPO "https://github.com/moalyousef/cmkr")
        cmkr_exec(${GIT_EXECUTABLE} clone ${CMKR_REPO} ${CMKR_DIRECTORY})
        cmkr_exec(${CMAKE_COMMAND} --no-warn-unused-cli ${CMKR_DIRECTORY} -B${CMKR_DIRECTORY}/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${CMKR_DIRECTORY})
        cmkr_exec(${CMAKE_COMMAND} --build ${CMKR_DIRECTORY}/build --parallel --config Release)
        cmkr_exec(${CMAKE_COMMAND} --install ${CMKR_DIRECTORY}/build --config Release --prefix ${CMKR_DIRECTORY} --component cmkr)
        if(NOT EXISTS ${CMKR_EXECUTABLE})
            message(FATAL_ERROR "[cmkr] Failed to bootstrap '${CMKR_EXECUTABLE}'")
        endif()
        cmkr_exec(${CMKR_EXECUTABLE} version OUTPUT_VARIABLE CMKR_VERSION)
        string(STRIP ${CMKR_VERSION} CMKR_VERSION)
        message(STATUS "[cmkr] Bootstrapped ${CMKR_EXECUTABLE}")
    else()
        message(VERBOSE "[cmkr] Found cmkr: '${CMKR_EXECUTABLE}'")
    endif()
endif()
execute_process(COMMAND ${CMKR_EXECUTABLE} version
    OUTPUT_VARIABLE CMKR_VERSION
    RESULT_VARIABLE CMKR_EXEC_RESULT
)
if(NOT CMKR_EXEC_RESULT EQUAL 0)
    unset(CMKR_EXECUTABLE CACHE)
    message(FATAL_ERROR "[cmkr] Failed to get version, try clearing the cache and rebuilding")
endif()
string(STRIP ${CMKR_VERSION} CMKR_VERSION)
message(STATUS "[cmkr] Using ${CMKR_VERSION}")

# This is the macro that contains black magic
macro(cmkr)
    # When this macro is called from the generated file, fake some internal CMake variables
    get_source_file_property(CMKR_CURRENT_LIST_FILE ${CMAKE_CURRENT_LIST_FILE} CMKR_CURRENT_LIST_FILE)
    if(CMKR_CURRENT_LIST_FILE)
        set(CMAKE_CURRENT_LIST_FILE ${CMKR_CURRENT_LIST_FILE})
        get_filename_component(CMAKE_CURRENT_LIST_DIR ${CMAKE_CURRENT_LIST_FILE} DIRECTORY)
    endif()

    # File-based include guard (include_guard is not documented to work)
    get_source_file_property(CMKR_INCLUDE_GUARD ${CMAKE_CURRENT_LIST_FILE} CMKR_INCLUDE_GUARD)
    if(NOT CMKR_INCLUDE_GUARD)
        set_source_files_properties(${CMAKE_CURRENT_LIST_FILE} PROPERTIES CMKR_INCLUDE_GUARD TRUE)

        # Generate CMakeLists.txt
        cmkr_exec(${CMKR_EXECUTABLE} gen -y
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE CMKR_GEN_OUTPUT
        )
        string(STRIP ${CMKR_GEN_OUTPUT} CMKR_GEN_OUTPUT)
        message(STATUS "[cmkr] ${CMKR_GEN_OUTPUT}")

        # Copy the now-generated CMakeLists.txt to CMakerLists.txt
        # This is done because you cannot include() a file you are currently in
        set(CMKR_TEMP_FILE ${CMAKE_CURRENT_LIST_DIR}/CMakerLists.txt)
        configure_file(CMakeLists.txt ${CMKR_TEMP_FILE} COPYONLY)
        
        # Add the macro required for the hack at the start of the cmkr macro
        set_source_files_properties(${CMKR_TEMP_FILE} PROPERTIES
            CMKR_CURRENT_LIST_FILE ${CMAKE_CURRENT_LIST_FILE}
        )
        
        # 'Execute' the newly-generated CMakeLists.txt
        include(${CMKR_TEMP_FILE})
        
        # Delete the generated file
        file(REMOVE ${CMKR_TEMP_FILE})
        
        # Do not execute the rest of the original CMakeLists.txt
        return()
    endif()
endmacro()
