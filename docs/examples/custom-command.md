---
# Automatically generated from tests/custom-command/cmake.toml - DO NOT EDIT
layout: default
title: Tests add_custom_command and add_custom_target support
permalink: /examples/custom-command
parent: Examples
nav_order: 12
---

# Tests add_custom_command and add_custom_target support

This test demonstrates add_custom_command and add_custom_target support in cmkr.

```toml
#
# There are two forms of add_custom_command in CMake:
# 1. Output form: generates files that can be consumed by other targets
# 2. Target form: runs commands at build time for a specific target (pre-build, pre-link, post-build)

[project]
name = "custom-command"
description = "Tests add_custom_command and add_custom_target support"

# -----------------------------------------------------------------------------
# Simple executable demonstrating post-build events
# -----------------------------------------------------------------------------

[target.hello]
type = "executable"
sources = ["src/hello.cpp"]

# This post-build command runs after the 'hello' executable is built.
# build-event can be: "pre-build", "pre-link", or "post-build"
[[target.hello.custom-command]]
build-event = "post-build"
command = ["${CMAKE_COMMAND}", "-E", "echo", "Built executable: $<TARGET_FILE_NAME:hello>"]
comment = "Print the built executable name"

# -----------------------------------------------------------------------------
# Executable with code generation (output form custom command)
# -----------------------------------------------------------------------------

[target.custom-command]
type = "executable"
sources = ["src/main.cpp"]
include-directories = ["${CMAKE_CURRENT_BINARY_DIR}/generated"]

# Output form: This custom command generates source files before building.
# The outputs are automatically added as sources to the target.
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

# Target form: This post-build command runs after 'custom-command' is built.
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

# -----------------------------------------------------------------------------
# Custom target (add_custom_target)
# -----------------------------------------------------------------------------

# A custom target runs commands independently of any executable/library.
# Setting 'all = true' makes it run as part of the default build.
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
