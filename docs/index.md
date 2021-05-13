---
layout: home
title: Index
nav_order: 0
---

# Index

`cmkr`, pronounced "cmaker", is a modern build system based on [CMake](https://cmake.org/) and [TOML](https://toml.io). It was originally created by [Mohammed Alyousef](https://github.com/MoAlyousef).

**NOTE**: The documentation is currently a work-in-progress due to breaking changes since `0.1.4`. For examples you can check the [cmkr GitHub topic](https://github.com/topics/cmkr) and the [tests](https://github.com/build-cpp/cmkr/tree/main/tests).

`cmkr` parses `cmake.toml` files and generates a modern, idiomatic `CMakeLists.txt` for you. A minimal example:

```toml
[project]
name = "cmkr_for_beginners"
description = "A minimal cmkr project."

[target.hello_world]
type = "executable"
sources = ["src/main.cpp"]
```

`cmkr` can bootstrap itself from CMake and consumers of your project do not need to install anything to use it.