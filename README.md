# cmkr

`cmkr`, pronounced "cmaker", is a modern build system based on [CMake](https://cmake.org/) and [TOML](https://toml.io).

`cmkr` parses `cmake.toml` files and generates a modern, idiomatic `CMakeLists.txt` for you. A minimal example:

```toml
[project]
name = "cmkr_for_beginners"
description = "A minimal cmkr project."

[target.hello_world]
type = "executable"
sources = ["src/main.cpp"]
```

`cmkr` can bootstrap itself from CMake and consumers of your project do not need to install anything to work with it.

## Getting started

The easiest way to get started is to use the [cmkr_for_beginners](https://github.com/build-cpp/cmkr_for_beginners) template repository. Either open it in [Gitpod](https://gitpod.io/#https://github.com/build-cpp/cmkr_for_beginners), or clone the repository and run:

```sh
cmake -B build
cmake --build build
```

Alternatively you can check out the [cmkr topic](https://github.com/topics/cmkr), the [build-cpp organization](https://github.com/build-cpp) or the [tests](https://github.com/build-cpp/cmkr/tree/main/tests) for more examples and templates. You can also check out the [documentation](https://build-cpp.github.io/cmkr).

## Command line

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

## Credits

- https://github.com/gulrak/filesystem
- https://github.com/Tessil/ordered-map
- https://github.com/ToruNiina/toml11
- https://github.com/mpark/variant
- https://www.svgrepo.com/svg/192268/hammer
