if(NOT DEFINED INPUT)
    message(FATAL_ERROR "INPUT is required")
endif()

if(NOT EXISTS "${INPUT}")
    message(FATAL_ERROR "Expected file not found: ${INPUT}")
endif()

message(STATUS "Verified file exists: ${INPUT}")
