---
# Automatically generated from tests/interface/cmake.toml - DO NOT EDIT
layout: default
title: Header-only library
permalink: /examples/interface
parent: Examples
nav_order: 1
---

# Header-only library



```toml
[project]
name = "interface"
description = "Header-only library"

[target.mylib]
type = "interface"
include-directories = ["include"]
compile-features = ["cxx_std_11"]

[target.example]
type = "executable"
sources = ["src/main.cpp"]
link-libraries = ["mylib"]
```



<sup><sub>This page was automatically generated from [tests/interface/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/interface/cmake.toml).</sub></sup>
