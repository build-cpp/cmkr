---
# Automatically generated from tests/generator-executable/cmake.toml - DO NOT EDIT
layout: default
title: Tests using an executable to generate sources for another target
permalink: /examples/generator-executable
parent: Examples
nav_order: 13
---

# Tests using an executable to generate sources for another target

This test demonstrates a common pattern: using an executable to generate sources

for another target. The generator runs at build time and produces code that is

consumed by the main executable.

```toml
#
# CMake handles the dependency automatically: the generator executable is built
# first, then it runs to produce the generated sources, and finally the main
# executable is built using those sources.

[project]
name = "generator-executable"
description = "Tests using an executable to generate sources for another target"

# -----------------------------------------------------------------------------
# Generator executable: produces code at build time
# -----------------------------------------------------------------------------

[target.generate_numbers]
type = "executable"
sources = ["src/generate_numbers.cpp"]

# Post-build: confirm the generator was built
[[target.generate_numbers.custom-command]]
build-event = "post-build"
command = ["${CMAKE_COMMAND}", "-E", "echo", "Generator built successfully: $<TARGET_FILE_NAME:generate_numbers>"]
comment = "Confirm generator executable was built"

# -----------------------------------------------------------------------------
# Main executable: uses generated sources
# -----------------------------------------------------------------------------

[target.main]
type = "executable"
sources = ["src/main.cpp"]
include-directories = ["${CMAKE_CURRENT_BINARY_DIR}/generated"]

# Output form custom command: runs the generate_numbers executable to generate sources.
# The outputs are automatically added as sources to this target.
# The DEPENDS on "generate_numbers" ensures the generator is built before this runs.
[[target.main.custom-command]]
outputs = ["${CMAKE_CURRENT_BINARY_DIR}/generated/numbers.cpp"]
depends = ["generate_numbers"]
command = ["$<TARGET_FILE:generate_numbers>", "${CMAKE_CURRENT_BINARY_DIR}/generated/numbers.cpp"]
comment = "Run generate_numbers to generate numbers.cpp"

# Post-build: run the executable to verify it works
[[target.main.custom-command]]
build-event = "post-build"
command = ["$<TARGET_FILE:main>"]
comment = "Run the main executable to verify generated code works"
```



<sup><sub>This page was automatically generated from [tests/generator-executable/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/generator-executable/cmake.toml).</sub></sup>
