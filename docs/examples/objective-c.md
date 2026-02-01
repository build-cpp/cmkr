---
# Automatically generated from tests/objective-c/cmake.toml - DO NOT EDIT
layout: default
title: Objective-C
permalink: /examples/objective-c
parent: Examples
nav_order: 11
---

# Objective-C

Add Objective-C sources on Apple platforms:

```toml
[project]
name = "objective-c"
description = "Objective-C"
languages = ["C"]
apple.languages = ["OBJC"]

[target.hello]
type = "executable"
sources = ["src/main.c"]
apple.sources = ["src/apple.m"]
apple.link-libraries = ["$<LINK_LIBRARY:FRAMEWORK,Foundation>"]
```



<sup><sub>This page was automatically generated from [tests/objective-c/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/objective-c/cmake.toml).</sub></sup>
