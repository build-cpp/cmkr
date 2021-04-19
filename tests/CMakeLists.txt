# This file was generated automatically by cmkr.

# Create a configure-time dependency on cmake.toml to improve IDE support
if(CMKR_ROOT_PROJECT)
	configure_file(cmake.toml cmake.toml COPYONLY)
endif()

enable_testing()

add_test(
	NAME
		basic
	WORKING_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}/basic"
	COMMAND
		$<TARGET_FILE:cmkr>
		build
)

