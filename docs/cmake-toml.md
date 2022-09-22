---
layout: page
title: Reference
permalink: /cmake-toml/
nav_order: 1
---

# Reference

This page is a reference for the options in the `cmake.toml` file. If you think anything is missing or unclear, please [edit this page](https://github.com/build-cpp/cmkr/edit/main/docs/cmake-toml.md) or open an [issue](https://github.com/build-cpp/cmkr/issues).

See the [examples](/examples) section for concrete examples.

## CMake configuration

```toml
[cmake]
version = "3.15"
cmkr-include = "cmkr.cmake"
```

## Project configuration

```toml
[project]
name = "myproject"
version = "1.0.0"
description = "Description of the project"
languages = ["C", "CXX"]
cmake-before = """
message(STATUS "CMake injected before the project() call")
"""
cmake-after = """
message(STATUS "CMake injected after the project() call")
"""
include-before = ["cmake/before-project.cmake"]
include-after = ["cmake/after-project.cmake"]
```

## Conditions

You can specify your own conditions and use them in any `condition` field:

```toml
[conditions]
arch64 = "CMAKE_SIZEOF_VOID_P EQUALS 8"
arch32 = "CMAKE_SIZEOF_VOID_P EQUALS 4"
```

This will make the `arch64` and `arch32` conditions available with their respective CMake expressions. 

You can also prefix most keys with `condition.` to represent a conditional:

```toml
[target]
type = "executable"
sources = ["src/main.cpp"]
windows.sources = ["src/windows_specific.cpp"]
```

The following conditions are predefined (you can override them if you desire):

```toml
[conditions]
windows = "WIN32"
macos = "CMAKE_SYSTEM_NAME MATCHES \"Darwin\""
unix = "UNIX"
bsd = "CMAKE_SYSTEM_NAME MATCHES \"BSD\""
linux = "CMAKE_SYSTEM_NAME MATCHES \"Linux\""
gcc = "CMAKE_CXX_COMPILER_ID STREQUAL \"GNU\" OR CMAKE_C_COMPILER_ID STREQUAL \"GNU\""
clang = "CMAKE_CXX_COMPILER_ID MATCHES \"Clang\" OR CMAKE_C_COMPILER_ID MATCHES \"Clang\""
msvc = "MSVC"
root = "CMKR_ROOT_PROJECT"
x64 = "CMAKE_SIZEOF_VOID_P EQUAL 8"
x32 = "CMAKE_SIZEOF_VOID_P EQUAL 4"
```

## Subdirectories

```toml
[subdir.mysubdir]
condition = "mycondition"
cmake-before = """
message(STATUS "CMake injected before the add_subdirectory() call"
"""
cmake-after = """
message(STATUS "CMake injected after the add_subdirectory() call")
"""
include-before = ["cmake/before-subdir.cmake"]
include-after = ["cmake/after-subdir.cmake"]
```

## Vcpkg

```toml
[vcpkg]
version = "2021.05.12"
url = "https://github.com/microsoft/vcpkg/archive/refs/tags/2021.05.12.tar.gz"
packages = ["fmt", "zlib"]
```

The vcpkg `version` will automatically generate the `url` from the [official repository](https://github.com/microsoft/vcpkg/releases). For a custom registry you can specify your own `url` (and omit the `version`). You can browse available packages on [vcpkg.io](https://vcpkg.io/en/packages.html).

To specify package features you can use the following syntax: `imgui[docking-experimental,freetype,sdl2-binding,opengl3-binding]`.

## Packages

```toml
[find-package.mypackage]
condition = "mycondition"
version = "1.0"
required = true
config = true
components = ["mycomponent"]
```

## FetchContent

**Note**: The `[fetch-content]` feature is unpolished and will likely change in a future release.

```toml
[fetch-content.gitcontent]
condition = "mycondition"
git = "https://github.com/myuser/gitcontent"
tag = "v0.1"

[fetch-content.svncontent]
condition = "mycondition"
svn = "https://svn-host.com/url"
rev = "svn_rev"

[fetch-content.urlcontent]
condition = "mycondition"
url = "https://content-host.com/urlcontent.zip"
hash = "123123123123"
```

## Targets

```toml
[target.mytarget]
condition = "mycondition"
alias = "mytarget::mytarget"
type = "static" # executable, shared (DLL), static, interface, object, library, custom
headers = ["src/mytarget.h"]
sources = ["src/mytarget.cpp"]

# The keys below match the target_xxx CMake commands
# Keys prefixed with private- will get PRIVATE visibility
compile-definitions = [""]
private-compile-definitions = [""]
compile-features = [""]
private-compile-features = [""]
compile-options = [""]
private-compile-options = [""]
include-directories = [""]
private-include-directories = [""]
link-directories = [""]
private-link-directories = [""]
link-libraries = [""]
private-link-libraries = [""]
link-options = [""]
private-link-options = [""]
precompile-headers = [""]
private-precompile-headers = [""]

cmake-before = """
message(STATUS "CMake injected before the target")
"""
cmake-after = """
message(STATUS "CMake injected after the target")
"""
include-before = "cmake/target-before.cmake"
include-after = "cmake/target-after.cmake"

# See https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-targets for a list of target properties
[target.mytarget.properties]
CXX_STANDARD = 17
CXX_STANDARD_REQUIRED = true
FOLDER = "MyFolder"
```

## Tests and installation (unfinished)

**Note**: The `[[test]]` and `[[install]]` are unfinished features and will likely change in a future release.

```toml
# You can declare as many as you want like this, but the name has to be unique
[[test]]
condition = "mycondition"
name = "mytest"
command = "$<TARGET_FILE:mytest>"
arguments = ["arg1", "arg2"]
configurations = ["Debug", "Release", "RelWithDebInfo", "MinSizeRelease"]
working-directory = "mytest-dir"
```

```toml
[[install]]
condition = "mycondition"
targets = ["mytarget", "mytest"]
destination = ["bin"]
component = "mycomponent"
files = ["content/my.png"]
dirs = ["include"]
configs = ["Release", "Debug"]
optional = false
```