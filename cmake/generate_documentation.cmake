option(CMKR_GENERATE_DOCUMENTATION "Generate cmkr documentation" ${CMKR_ROOT_PROJECT})
set(CMKR_TESTS "" CACHE INTERNAL "List of test directories in the order declared in tests/cmake.toml")

if(CMKR_GENERATE_DOCUMENTATION)
    # Hook the add_test function to capture the tests in the order declared in tests/cmake.toml
    function(add_test)
        cmake_parse_arguments(TEST "" "WORKING_DIRECTORY" "" ${ARGN})
        list(APPEND CMKR_TESTS "${TEST_WORKING_DIRECTORY}")
        set(CMKR_TESTS "${CMKR_TESTS}" CACHE INTERNAL "")
        _add_test(${test} ${ARGN})
    endfunction()
endif()

function(generate_documentation)
    if(CMKR_GENERATE_DOCUMENTATION)
        message(STATUS "[cmkr] Generating documentation...")
        
        # Delete previously generated examples
        set(example_folder "${PROJECT_SOURCE_DIR}/docs/examples")
        file(GLOB example_files "${example_folder}/*.md")
        list(REMOVE_ITEM example_files "${example_folder}/index.md")
        file(REMOVE ${example_files})

        message(DEBUG "[cmkr] Test directories: ${CMKR_TESTS}")
        set(test_index 0)
        foreach(test_dir ${CMKR_TESTS})
            set(test_name "${test_dir}")
            set(test_dir "${PROJECT_SOURCE_DIR}/tests/${test_dir}")
            set(test_toml "${test_dir}/cmake.toml")
            if(IS_DIRECTORY "${test_dir}" AND EXISTS "${test_toml}")
                message(DEBUG "[cmkr] Generating documentation for: ${test_toml} (index: ${test_index})")

                # Set template variables
                set(EXAMPLE_PERMALINK "${test_name}")
                set(EXAMPLE_INDEX ${test_index})
                math(EXPR test_index "${test_index}+1")

                # Read cmake.toml file
                file(READ "${test_toml}" test_contents NO_HEX_CONVERSION)
                string(LENGTH "${test_contents}" toml_length)
                
                # Extract header text
                string(REGEX MATCH "^(\n*(#[^\n]+\n)+\n*)" EXAMPLE_HEADER "${test_contents}")
                string(LENGTH "${EXAMPLE_HEADER}" header_length)
                string(STRIP "${EXAMPLE_HEADER}" EXAMPLE_HEADER)
                string(REGEX REPLACE "\n# ?" "\n" EXAMPLE_HEADER "\n${EXAMPLE_HEADER}")
                string(STRIP "${EXAMPLE_HEADER}" EXAMPLE_HEADER)
                
                # Extract footer text
                string(REGEX MATCH "(((#[^\n]+)(\n+|$))+)$" EXAMPLE_FOOTER "${test_contents}")
                string(LENGTH "${EXAMPLE_FOOTER}" footer_length)
                string(STRIP "${EXAMPLE_FOOTER}" EXAMPLE_FOOTER)
                string(REGEX REPLACE "\n# ?" "\n" EXAMPLE_FOOTER "\n${EXAMPLE_FOOTER}")
                string(STRIP "${EXAMPLE_FOOTER}" EXAMPLE_FOOTER)
                
                # Extract toml body
                math(EXPR toml_length "${toml_length}-${header_length}-${footer_length}")
                string(SUBSTRING "${test_contents}" ${header_length} ${toml_length} EXAMPLE_TOML)
                string(STRIP "${EXAMPLE_TOML}" EXAMPLE_TOML)

                # Extract title from description
                if("${EXAMPLE_TOML}" MATCHES "description *= *\"([^\"]+)\"")
                    set(EXAMPLE_TITLE "${CMAKE_MATCH_1}")
    
                    # Generate documentation markdown page
                    configure_file("${PROJECT_SOURCE_DIR}/cmake/example.md.in" "${example_folder}/${EXAMPLE_PERMALINK}.md" @ONLY NEWLINE_STYLE LF)
                else()
                    message(DEBUG "[cmkr] Skipping documentation generation for ${test_name} because description is missing")
                endif()
            endif()
        endforeach()
    endif()
endfunction()
