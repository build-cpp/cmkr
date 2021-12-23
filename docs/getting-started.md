---
layout: page
title: Getting started
permalink: /getting-started/
nav_order: 1
---

# Getting started

The easiest way to get started is to use the [cmkr_for_beginners](https://github.com/build-cpp/cmkr_for_beginners) template repository. Either open it in [Gitpod](https://gitpod.io/#https://github.com/build-cpp/cmkr_for_beginners), or clone the repository and run:

```sh
cmake -B build
cmake --build build
```

Alternatively you can check out the [cmkr topic](https://github.com/topics/cmkr) or the [build-cpp organization](https://github.com/build-cpp) for more examples and templates.

### Migrating an existing project

When migrating an existing project it's easiest to download a [cmkr release](https://github.com/build-cpp/cmkr/releases) and put `cmkr` in your PATH. Then go to your project directory and run:

```
cmkr init
```

This will bootstrap `cmake.toml` and `CMakeLists.txt` that you can then build as normal with CMake.