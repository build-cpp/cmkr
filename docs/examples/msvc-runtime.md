---
# Automatically generated from tests/msvc-runtime/cmake.toml - DO NOT EDIT
layout: default
title: Static MSVC runtime
permalink: /examples/msvc-runtime
parent: Examples
nav_order: 8
---

# Static MSVC runtime



```toml
[project]
name = "msvc-runtime"
description = "Static MSVC runtime"
msvc-runtime = "static"

# This target will compile with a static runtime
[target.static-runtime]
type = "executable"
sources = ["src/main.cpp"]

# This target overrides the [project].msvc-runtime
[target.dynamic-runtime]
type = "executable"
sources = ["src/main.cpp"]
msvc-runtime = "dynamic"
```



<sup><sub>This page was automatically generated from [tests/msvc-runtime/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/msvc-runtime/cmake.toml).</sub></sup>
