---
layout: page
title: Command line
permalink: /command-line/
nav_order: 2
---

# Command line

Optionally you can install `cmkr` in your `PATH` and use it as a utility from the command line:

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