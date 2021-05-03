# cmkr

cmkr, pronounced "cmaker", is a modern build system based on CMake and TOML. It was originally created by [Mohammed Alyousef](https://github.com/MoAlyousef).

**NOTE**: The documentation is currently a work-in-progress due to breaking changes since `0.1.4`. For examples you can check the [cmkr GitHub topic](https://github.com/topics/cmkr) and the [tests](https://github.com/build-cpp/cmkr/tree/main/tests).

## Getting started

`cmkr` parses `cmake.toml` files and generates a modern, idomatic `CMakeLists.txt` for you. A basic hello world format with the minimum required fields:

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

## Building

`cmkr` requires a C++11 compiler and CMake >= ~3.x (exact minimum version is not yet specified). C++11 was picked to allow the broadest possible set of compilers to bootstrap `cmkr`.

```sh
git clone https://github.com/moalyousef/cmkr
cd cmkr
cmake -Bbin
cmake --build bin --parallel
```

## Command line

The `cmkr` executable can be run from the command-line:

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

The build command invokes `cmake` and the default build-system on your platform (unless a generator is specified), it also accepts extra build arguments:

```sh
cmkr build --config Release 
```
