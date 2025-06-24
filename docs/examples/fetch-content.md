---
# Automatically generated from tests/fetch-content/cmake.toml - DO NOT EDIT
layout: default
title: Fetching from git
permalink: /examples/fetch-content
parent: Examples
nav_order: 2
---

# Fetching from git

Downloads [fmt v7.1.3](https://fmt.dev/7.1.3/) from [GitHub](https://github.com) and links an `example` target to it:

```toml
[project]
name = "fetch-content"
description = "Fetching from git"

[fetch-content.fmt]
git = "https://github.com/fmtlib/fmt"
tag = "7.1.3"

[target.example]
type = "executable"
sources = ["src/main.cpp"]
link-libraries = ["fmt::fmt"]
```

This is equivalent to calling CMake's [FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html).

<sup><sub>This page was automatically generated from [tests/fetch-content/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/fetch-content/cmake.toml).</sub></sup>
