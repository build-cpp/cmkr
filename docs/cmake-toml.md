---
layout: page
title: Reference
permalink: /cmake-toml/
nav_order: 3
---

# Reference

This page is a reference for the options in the `cmake.toml` file. If you think anything is missing or unclear, please [edit this page](https://github.com/build-cpp/cmkr/edit/main/docs/cmake-toml.md) or open an [issue](https://github.com/build-cpp/cmkr/issues).

This is a reference page. Check out the [examples](/examples) and the [cmkr topic](https://github.com/topics/cmkr) as well.
{:.info}

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
msvc-runtime = "" # dynamic (implicit default), static
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
arch64 = "CMAKE_SIZEOF_VOID_P EQUAL 8"
arch32 = "CMAKE_SIZEOF_VOID_P EQUAL 4"
```

This will make the `arch64` and `arch32` conditions available with their respective CMake expressions. 

You can also prefix most keys with `condition.` to represent a conditional:

```toml
[target]
type = "executable"
sources = ["src/main.cpp"]
windows.sources = ["src/windows_specific.cpp"]
```

### Predefined conditions

The following conditions are predefined (you can override them if you desire):

```toml
[conditions]
windows = "WIN32"
macos = "CMAKE_SYSTEM_NAME MATCHES \"Darwin\""
unix = "UNIX"
bsd = "CMAKE_SYSTEM_NAME MATCHES \"BSD\""
linux = "CMAKE_SYSTEM_NAME MATCHES \"Linux\""
gcc = "CMAKE_CXX_COMPILER_ID STREQUAL \"GNU\" OR CMAKE_C_COMPILER_ID STREQUAL \"GNU\""
clang = "(CMAKE_CXX_COMPILER_ID MATCHES \"Clang\" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES \"^MSVC$\") OR (CMAKE_C_COMPILER_ID MATCHES \"Clang\" AND NOT CMAKE_C_COMPILER_FRONTEND_VARIANT MATCHES \"^MSVC$\")"
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

## Options

```toml
[options]
MYPROJECT_BUILD_TESTS = false
MYPROJECT_SPECIAL_OPTION = { value = true, help = "Docstring for this option." }
MYPROJECT_BUILD_EXAMPLES = "root"
```

Options correspond to [CMake cache variables](https://cmake.org/cmake/help/book/mastering-cmake/chapter/CMake%20Cache.html) that can be used to customize your project at configure-time. You can configure with `cmake -DMYPROJECT_BUILD_TESTS=ON` to enable the option. Every option automatically gets a corresponding [condition](#conditions).

The special value `root` can be used to set the option to `true` if the project is compiled as the root project (it will be `false` if someone is including your project via `[fetch-content]` or `[subdir]`).

## Variables

```toml
[variables]
MYBOOL = true
MYSTRING = "hello"
```

Variables emit a [`set`](https://cmake.org/cmake/help/latest/command/set.html) and can be used to configure subprojects and packages.

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
shallow = false
system = false

[fetch-content.svncontent]
condition = "mycondition"
svn = "https://svn-host.com/url"
rev = "svn_rev"

[fetch-content.urlcontent]
condition = "mycondition"
url = "https://content-host.com/urlcontent.zip"
# These are equivalent, supported algorithms:
# md5, sha1, sha224, sha256, sha384, sha512, sha3_224, sha3_256, sha3_384, sha3_512
hash = "SHA1 502a4e25b8b209889c99c7fa0732102682c2e4ff"
sha1 = "502a4e25b8b209889c99c7fa0732102682c2e4ff"
```

## Targets

```toml
[target.mytarget]
condition = "mycondition"
alias = "mytarget::mytarget"
type = "static" # executable, shared (DLL), static, interface, object, library, custom
headers = ["src/mytarget.h"]
sources = ["src/mytarget.cpp"]
msvc-runtime = "" # dynamic (implicit default), static

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

## Templates

To avoid repeating yourself you can create your own target type and use it in your targets:

```toml
[template.example]
condition = "MYPROJECT_BUILD_EXAMPLES"
type = "executable"
link-libraries = ["myproject::mylib"]
add-function = ""
pass-sources = false

# Properties from the template are merged with the ones here
[target.myexample]
type = "example"
sources = ["src/myexample.cpp"]
```

The properties declared on a `template` are the same as the ones you use for targets. The only exceptions are:

- `add-function`: Specifies a custom add function. Projects like [pybind11](https://pybind11.readthedocs.io/en/stable/cmake/index.html#new-findpython-mode) have their own `add_xxx` function, which you can specify here.
- `pass-sources`: Pass sources directly to the add function instead of using `target_sources`.

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