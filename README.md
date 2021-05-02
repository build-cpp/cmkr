# cmkr

cmkr, pronounced "cmaker", is A CMakeLists.txt generator from TOML.

See the [cmkr topic](https://github.com/topics/cmkr) for examples. Feel free to add the `cmkr` topic to your projects if you used cmkr!

## Building

cmkr requires a C++11 compiler, cmake >= 3.15.

```
git clone https://github.com/moalyousef/cmkr
cd cmkr
cmake -Bbin
cmake --build bin --parallel
```

## Usage

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

```
cmkr build --config Release 
```

## Binary types

### executable

Executable binary. Equivalent to [add_executable(name)](https://cmake.org/cmake/help/latest/command/add_executable.html).

### library

Library, can be static or shared depending on the BUILD_SHARED_LIBS variable. Equivalent to [add_library()](https://cmake.org/cmake/help/latest/command/add_library.html).

### static

Static library/archive. Equivalent to [add_library(name STATIC)](https://cmake.org/cmake/help/latest/command/add_library.html).

### shared

Shared/dynamic library. Equivalent to [add_library(name SHARED)](https://cmake.org/cmake/help/latest/command/add_library.html).

### interface

Header-only library. Equivalent to [add_library(name INTERFACE)](https://cmake.org/cmake/help/latest/command/add_library.html).

## Roadmap

- Support more cmake fields.
