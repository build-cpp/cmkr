# Enumerates the target sources and automatically generates include/resources/${RESOURCE_NAME}.h from all .cmake files
function(generate_resources target)
    get_property(TARGET_SOURCES
        TARGET ${target}
        PROPERTY SOURCES
    )
    foreach(SOURCE ${TARGET_SOURCES})
        if(SOURCE MATCHES ".cmake$")
            get_filename_component(RESOURCE_NAME "${SOURCE}" NAME_WE)
            set(RESOURCE_HEADER "include/resources/${RESOURCE_NAME}.h")
            configure_file("${SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_HEADER}")
            file(READ "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_HEADER}" RESOURCE_CONTENTS)
            file(GENERATE
                OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${RESOURCE_HEADER}"
                CONTENT "namespace cmkr {\nnamespace resources {\nstatic const char* ${RESOURCE_NAME} = R\"RESOURCE(${RESOURCE_CONTENTS})RESOURCE\";\n}\n}"
            )
            message(STATUS "[cmkr] Generated ${RESOURCE_HEADER}")
        endif()
    endforeach()
    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
endfunction()