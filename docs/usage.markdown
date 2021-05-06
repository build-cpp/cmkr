---
layout: page
title: Usage
permalink: /usage/
nav_order: 2
---

# Usage

cmkr parses cmake.toml files (using toml11 by Toru Niina) at the project directory. A basic hello world format with the minimum required fields:

```toml
[cmake]
minimum = "3.15"

[project]
name = "app"
version = "0.1.0"

[target.app]
type = "executable"
sources = ["src/main.cpp"]
```

**NOTE**: The documentation is currently a work-in-progress due to breaking changes since `0.1.4`. For examples you can check the [cmkr topic](https://github.com/topics/cmkr).

The cmkr executable can be run from the command-line:

```
Usage: cmkr [arguments]
arguments:
    init    [executable|library|shared|static|interface] Starts a new project in the same directory.
    gen                                                  Generates CMakeLists.txt file.
    build   <extra cmake args>                           Run cmake and build.
    install                                              Run cmake --install. Needs admin privileges.
    clean                                                Clean the build directory.
    help                                                 Show help.
    version                                              Current cmkr version.
```

The build command invokes cmake and the default build-system on your platform (unless a generator is specified), it also accepts extra cmake build arguments:

```sh
cmkr build --config Release 
```