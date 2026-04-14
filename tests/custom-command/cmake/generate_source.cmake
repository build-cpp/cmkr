if(NOT DEFINED OUTPUT_CPP)
    message(FATAL_ERROR "OUTPUT_CPP is required")
endif()

if(NOT DEFINED OUTPUT_HPP)
    message(FATAL_ERROR "OUTPUT_HPP is required")
endif()

get_filename_component(_output_dir "${OUTPUT_CPP}" DIRECTORY)
file(MAKE_DIRECTORY "${_output_dir}")

file(WRITE "${OUTPUT_HPP}" "#pragma once\n#define GENERATED_VALUE 42\n")
file(WRITE "${OUTPUT_CPP}" "#include \"generated.hpp\"\nint generated_value() { return GENERATED_VALUE; }\n")
