---
# Automatically generated from tests/custom-command/cmake.toml - DO NOT EDIT
layout: default
title: Tests add_custom_command and add_custom_target support
permalink: /examples/custom-command
parent: Examples
nav_order: 12
---

# Tests add_custom_command and add_custom_target support



```toml
[project]
name = "custom-command"
description = "Tests add_custom_command and add_custom_target support"

[target.custom-command]
type = "executable"
sources = ["src/main.cpp"]
include-directories = ["${CMAKE_CURRENT_BINARY_DIR}/generated"]

[[target.custom-command.custom-command]]
outputs = ["${CMAKE_CURRENT_BINARY_DIR}/generated/generated.cpp"]
byproducts = ["${CMAKE_CURRENT_BINARY_DIR}/generated/generated.hpp"]
depends = ["cmake/generate_source.cmake"]
command = [
    "${CMAKE_COMMAND}",
    "-DOUTPUT_CPP=${CMAKE_CURRENT_BINARY_DIR}/generated/generated.cpp",
    "-DOUTPUT_HPP=${CMAKE_CURRENT_BINARY_DIR}/generated/generated.hpp",
    "-P",
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_source.cmake",
]
comment = "Generate source files"
verbatim = true

[[target.custom-command.custom-command]]
build-event = "post-build"
command = [
    "${CMAKE_COMMAND}",
    "-E",
    "touch",
    "${CMAKE_CURRENT_BINARY_DIR}/custom-command-post-build.stamp",
]
byproducts = ["${CMAKE_CURRENT_BINARY_DIR}/custom-command-post-build.stamp"]
comment = "Create a post-build stamp file"
verbatim = true

[target.custom-codegen]
type = "custom"
all = true
command = [
    "${CMAKE_COMMAND}",
    "-E",
    "touch",
    "${CMAKE_CURRENT_BINARY_DIR}/custom-target.stamp",
]
byproducts = ["${CMAKE_CURRENT_BINARY_DIR}/custom-target.stamp"]
comment = "Run custom target"
verbatim = true
```



<sup><sub>This page was automatically generated from [tests/custom-command/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/custom-command/cmake.toml).</sub></sup>
