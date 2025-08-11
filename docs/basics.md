---
layout: page
title: Basics
nav_order: 1
---

# Basics

To effectively use cmkr it helps to understand the basic concepts of CMake.

## Projects

A CMake **project** is a collection of targets. In the context of libraries the project usually corresponds to the **package** that other projects can depend on.

<sub>Visual Studio: a CMake **project** corresponds to a _solution_.</sub>

## Targets

The basic unit of CMake is called a **target**. A target (also referred to as [binary target](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#binary-targets) in the CMake documentation) corresponds to an executable or static/dynamic library you can build. There are also [pseudo targets](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#pseudo-targets), but we ignore them for now.

<sub>Visual Studio: a **target** corresponds to a _project_.</sub>

Target types:

|Type|Purpose|
|-|-|
|Executable|Program you run on your computer (`.exe`)|
|Static library|Library code _compiled into_ the final executable (`.lib`, `.a`)|
|Dynamic library|Library code loaded at runtime (`.dll`, `.so`, `.dylib`)|
|Interface|Used for organizational purposes (common flags/includes/libraries)|

## Target Properties

Targets have a collection of **properties** that describe how to build them.

The most commonly-used properties:

- [_Sources_](https://cmake.org/cmake/help/latest/command/target_sources.html): the `*.cpp` files used to build the target.
- [_Include directories_](https://cmake.org/cmake/help/latest/command/target_include_directories.html): Paths to directories the compiler will search when doing `#include`.
- [_Compile definitions_](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html): Preprocessor macro definitions used for compilation (`-DMYOPTION`).
- [_Compile options_](https://cmake.org/cmake/help/latest/command/target_compile_options.html): Command line flags for the compiler (platform/compiler specific).
- [_Compile features_](https://cmake.org/cmake/help/latest/command/target_compile_features.html): C/C++ standard version to build with.
- [_Link libraries_](https://cmake.org/cmake/help/latest/command/target_link_libraries.html): The **dependencies** required to build this target.
- [_Link options_](https://cmake.org/cmake/help/latest/command/target_link_options.html): Command line flags for the linker (platform/linker specific).

<sub>See the [CMake documentation](https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-targets) for an exhaustive list of target properties.</sub>

**Important**: The term **link** has a slightly different meaning in CMake than you might expect. In addition to adding a library to the command line of the linker, CMake also (transitively) propagates properties of the target you link to based on their _visibility_.

<sub>You can think of **linking** as _depending on_.</sub>

The propagation of properties depends on their **visibility**:

- **Private**: properties that are only used when building the target itself.
- **Interface**: properties that are used when depending on this target.
- **Public**: combination of private and interface.

In practice you default to **private**, unless consumers of your library _require_ the property to build their target. In that case you use **public**.

## Example

The most intuitive example is with _include directories_. Imagine there are two targets:

1. `StringUtils`: A library with string utilities.
   - _Sources_: `StringUtils/src/stringutils.cpp`
   - _Include directories_: `StringUtils/include`
2. `DataProcessor`: An executable that uses functionality from `StringUtils` to process some data.
   - _Sources_: `DataProcessor/src/main.cpp`
   - _Link libraries_: `StringUtils`

The _include directories_ property of `StringUtils` has to be **public**. If it was **private**, the `DataProcessor` target would fail to `#include <stringutils.hpp>` since the _include directories_ property is not propagated.

The `cmake.toml` for this example would look something like this:

```toml
[project]
name = "DataProcessor"

[target.StringUtils]
type = "static"
sources = ["StringUtils/src/stringutils.cpp"]
headers = ["StringUtils/include/stringutils.hpp"]
include-directories = ["StringUtils/include"] # public (default for libraries)

[target.DataProcessor]
type = "executable"
sources = ["DataProcessor/src/main.cpp"]
link-libraries = ["StringUtils"] # private (default for executables)
```

The generated `CMakeLists.txt` (simplified) looks like this:

```cmake
project(DataProcessor)

# Target: StringUtils
add_library(StringUtils STATIC
    "StringUtils/include/stringutils.hpp"
    "StringUtils/src/stringutils.cpp"
)

target_sources(StringUtils PRIVATE ${StringUtils_SOURCES})

target_include_directories(StringUtils PUBLIC
	"StringUtils/include"
)

# Target: DataProcessor
add_executable(DataProcessor
    "DataProcessor/src/main.cpp"
)

target_sources(DataProcessor PRIVATE ${DataProcessor_SOURCES})

target_link_libraries(DataProcessor PRIVATE
    StringUtils
)
```
