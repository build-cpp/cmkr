---
# Automatically generated from tests/cxx-standard/cmake.toml - DO NOT EDIT
layout: default
title: Changing C++ standard
permalink: /examples/cxx-standard
parent: Examples
nav_order: 5
---

# Changing C++ standard

Require a C++11 compiler for the target `example`.

```toml
[project]
name = "cxx-standard"
description = "Changing C++ standard"

[target.example]
type = "executable"
sources = ["src/main.cpp"]
compile-features = ["cxx_std_11"]
```

This is equivalent to CMake's [target_compile_features](https://cmake.org/cmake/help/latest/command/target_compile_features.html)`(example PRIVATE cxx_std_11)`. For more information on available C/C++ standards and features see [cmake-compile-features(7)](https://cmake.org/cmake/help/latest/manual/cmake-compile-features.7.html).

<sup><sub>This page was automatically generated from [tests/cxx-standard/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/cxx-standard/cmake.toml).</sub></sup>
