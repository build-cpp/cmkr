# Enumerates the target sources and automatically generates include/resources/${RESOURCE_NAME}.h from all .cmake files
function(generate_resources target)
    get_property(TARGET_SOURCES
        TARGET ${target}
        PROPERTY SOURCES
    )
    foreach(SOURCE ${TARGET_SOURCES})
        get_filename_component(RESOURCE_NAME "${SOURCE}" NAME_WE)
        set(RESOURCE_HEADER "include/resources/${RESOURCE_NAME}.hpp")

        if(SOURCE MATCHES ".cmake$")
            # Add configure-time dependency on the source file
            configure_file("${SOURCE}" "${RESOURCE_HEADER}" COPYONLY)
            # Generate the actual resource into the header
            file(READ "${SOURCE}" RESOURCE_CONTENTS)
            configure_file("${PROJECT_SOURCE_DIR}/cmake/resource.hpp.in" "${RESOURCE_HEADER}" @ONLY)
            message(STATUS "[cmkr] Generated ${RESOURCE_HEADER}")
        elseif(SOURCE MATCHES ".in$")
            configure_file("${SOURCE}" "${RESOURCE_HEADER}" @ONLY)
            message(STATUS "[cmkr] Generated ${RESOURCE_HEADER}")
        endif()
    endforeach()
    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/include")
endfunction()