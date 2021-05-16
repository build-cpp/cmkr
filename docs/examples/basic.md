---
# Automatically generated from tests/basic/cmake.toml - DO NOT EDIT
layout: default
title: Minimal example
permalink: /examples/basic
parent: Examples
nav_order: 0
---

# Minimal example

A minimal `cmake.toml` project:

```toml
[project]
name = "basic"
description = "Minimal example"

[target.basic]
type = "executable"
sources = ["src/basic.cpp"]
```

Declares an executable target called `basic` with `src/basic.cpp` as a source file. Equivalent to CMake's [add_executable](https://cmake.org/cmake/help/latest/command/add_executable.html)`(basic src/basic.cpp)`.

<sup><sub>This page was automatically generated from [tests/basic/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/basic/cmake.toml).</sub></sup>
