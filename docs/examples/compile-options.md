---
# Automatically generated from tests/compile-options/cmake.toml - DO NOT EDIT
layout: default
title: Compiler flags
permalink: /examples/compile-options
parent: Examples
nav_order: 9
---

# Compiler flags

Example project that sets compiler flags and preprocessor defines for various platforms.

```toml
[project]
name = "compile-options"
description = "Compiler flags"

[target.hello]
type = "executable"
sources = ["src/main.cpp"]
msvc.compile-options = ["/W2"]
msvc.compile-definitions = ["PLATFORM=\"msvc\""]
gcc.compile-options = ["-Wall"]
gcc.compile-definitions = ["PLATFORM=\"gcc\""]
clang.compile-options = ["-Wall"]
clang.compile-definitions = ["PLATFORM=\"clang\""]
```

The `hello` target uses [conditions](/cmake-toml#conditions) to set different compiler flags and definitions depending on the platform. See the [targets](/cmake-toml/#targets) documentation for other things you can set.

_Note_: In general you only want to specify flags _required_ to compile your code without errors.

<sup><sub>This page was automatically generated from [tests/compile-options/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/compile-options/cmake.toml).</sub></sup>
