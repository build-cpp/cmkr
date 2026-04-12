---
# Automatically generated from tests/protobuf/cmake.toml - DO NOT EDIT
layout: default
title: Demonstrates protobuf integration with cmkr custom commands
permalink: /examples/protobuf
parent: Examples
nav_order: 14
---

# Demonstrates protobuf integration with cmkr custom commands

This test demonstrates using protobuf with cmkr for code generation.

```toml
#
# The example:
# 1. Fetches protobuf from GitHub using FetchContent
# 2. Uses a custom command to generate C++ sources from a .proto file
# 3. Links an executable against the generated sources and protobuf library
# 4. Serializes and deserializes a simple message
#
# Note: protobuf v21.x is used because newer versions (v22+) require abseil-cpp
# as a dependency, which significantly complicates the build.

[cmake]
version = "3.18...3.31"

[project]
name = "protobuf-example"
description = "Demonstrates protobuf integration with cmkr custom commands"

# -----------------------------------------------------------------------------
# Fetch protobuf from GitHub
# Using v21.12 (last version before abseil-cpp became required)
# -----------------------------------------------------------------------------

[fetch-content.protobuf]
git = "https://github.com/protocolbuffers/protobuf"
tag = "v21.12"
options = {
  protobuf_BUILD_TESTS = false,
  protobuf_BUILD_EXAMPLES = false,
  protobuf_BUILD_LIBPROTOC = false,
  protobuf_BUILD_SHARED_LIBS = false,
  protobuf_MSVC_STATIC_RUNTIME = false,
  SKIP_INSTALL_ALL = true,
  protobuf_WITH_ZLIB = false,
}

# -----------------------------------------------------------------------------
# Main executable using protobuf
# -----------------------------------------------------------------------------

[target.protobuf_example]
type = "executable"
sources = [
    "src/main.cpp",
]
include-directories = [
    "${CMAKE_CURRENT_BINARY_DIR}/generated",
]
link-libraries = ["protobuf::libprotobuf"]
compile-features = ["cxx_std_17"]

# Create the generated directory before running protoc
cmake-before = """
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
"""

# Custom command to generate C++ sources from .proto file
# This uses protoc from the fetched protobuf repository
[[target.protobuf_example.custom-command]]
outputs = [
    "${CMAKE_CURRENT_BINARY_DIR}/generated/addressbook.pb.h",
    "${CMAKE_CURRENT_BINARY_DIR}/generated/addressbook.pb.cc",
]
depends = [
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/addressbook.proto",
    "protobuf::protoc",  # Ensure protoc is built first
]
command = [
    "$<TARGET_FILE:protobuf::protoc>",
    "--cpp_out=${CMAKE_CURRENT_BINARY_DIR}/generated",
    "-I${CMAKE_CURRENT_SOURCE_DIR}/proto",
    "${CMAKE_CURRENT_SOURCE_DIR}/proto/addressbook.proto",
]
comment = "Generate C++ sources from addressbook.proto"
verbatim = true

# Post-build: run the executable to verify it works
[[target.protobuf_example.custom-command]]
build-event = "post-build"
command = ["$<TARGET_FILE:protobuf_example>"]
comment = "Run protobuf_example to verify serialization/deserialization works"
```



<sup><sub>This page was automatically generated from [tests/protobuf/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/protobuf/cmake.toml).</sub></sup>
