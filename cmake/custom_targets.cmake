generate_resources(cmkr)

add_custom_target(regenerate-cmake
        COMMAND "$<TARGET_FILE:cmkr>" gen
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
)

if(CMAKE_CONFIGURATION_TYPES)
    add_custom_target(run-tests
            COMMAND "${CMAKE_CTEST_COMMAND}" -C $<CONFIG>
            WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/tests"
    )
else()
    add_custom_target(run-tests
            COMMAND "${CMAKE_CTEST_COMMAND}"
            WORKING_DIRECTORY "${PROJECT_BINARY_DIR}/tests"
    )
endif()
