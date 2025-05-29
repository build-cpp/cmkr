# cmkr

[![Ask DeepWiki](https://deepwiki.com/badge.svg)](https://deepwiki.com/build-cpp/cmkr)

`cmkr`, pronounced "cmaker", is a modern build system based on [CMake](https://cmake.org/) and [TOML](https://toml.io).

`cmkr` parses `cmake.toml` files and generates a modern, idiomatic `CMakeLists.txt` for you. A minimal example:

```toml
[project]
name = "cmkr_for_beginners"

[target.hello_world]
type = "executable"
sources = ["src/main.cpp"]
```

`cmkr` can bootstrap itself and you only need CMake and a C++ compiler to use it.

## Getting started

To get started, run the following commands from your project directory:

```sh
curl https://raw.githubusercontent.com/build-cpp/cmkr/main/cmake/cmkr.cmake -o cmkr.cmake
cmake -P cmkr.cmake
```

After the bootstrapping process finishes, customize [`cmake.toml`](https://build-cpp.github.io/cmkr/cmake-toml) for your project and run CMake:

```sh
cmake -B build
cmake --build build
```

Once bootstrapped, `cmkr` does not introduce extra steps to your workflow. After modifying `cmake.toml` you simply build/configure your CMake project and `cmkr` will automatically regenerate `CMakeLists.txt` when necessary.

<sub>**Note**: The `cmake.toml` project file, generated `CMakeLists.txt` and `cmkr.cmake` bootstrapping script are all intended to be added to source control.</sub>

In CI environments the `cmkr` bootstrapping process is skipped, so there is no additional overhead in your pipelines.

## Template repositories

Another way to get started is to use the [cmkr_for_beginners](https://github.com/build-cpp/cmkr_for_beginners) template repository. Either open it in [Gitpod](https://gitpod.io/#https://github.com/build-cpp/cmkr_for_beginners), or clone the repository and run:

```sh
cmake -B build
cmake --build build
```

Check out the [cmkr topic](https://github.com/topics/cmkr), the [build-cpp organization](https://github.com/build-cpp) or the [tests](https://github.com/build-cpp/cmkr/tree/main/tests) for more examples and templates.

## Command line

Optionally you can put a [`cmkr` release](https://github.com/build-cpp/cmkr/releases) in your `PATH` and use it as a utility from the command line:

```
Usage: cmkr [arguments]
arguments:
    init    [executable|library|shared|static|interface] Create a project.
    gen                                                  Generates CMakeLists.txt file.
    build   <extra cmake args>                           Run cmake and build.
    install                                              Run cmake --install.
    clean                                                Clean the build directory.
    help                                                 Show help.
    version                                              Current cmkr version.
```

## Credits

- [gulrak/filesystem](https://github.com/gulrak/filesystem)
- [Tessil/ordered-map](https://github.com/Tessil/ordered-map)
- [ToruNiina/toml11](https://github.com/ToruNiina/toml11)
- [mpark/variant](https://github.com/mpark/variant)
- [SVG Repo Hammer](https://www.svgrepo.com/svg/192268/hammer)
- [can1357](https://github.com/can1357) for buying `cmkr.build` ❤️
- [JustasMasiulis](https://github.com/JustasMasiulis) for fixing the dark theme ❤️
