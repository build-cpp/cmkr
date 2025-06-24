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

### Languages

Supported languages are (see [`enable_language`](https://cmake.org/cmake/help/latest/command/enable_language.html) for more information):

- `C`
- `CXX` → C++
- `CSharp` → C#
- `CUDA`
- `OBJC` → Objective-C
- `OBJCXX` → Objective-C++
- `Fortran`
- `HIP`
- `ISPC`
- `Swift`
- `ASM`
- `ASM_MASM` → [Microsoft Macro Assembler (MASM)](https://learn.microsoft.com/en-US/cpp/assembler/masm/masm-for-x64-ml64-exe)
- `ASM_NASM` → [Netwide Assembler (NASM)](https://www.nasm.us)
- `ASM_MARMASM` [Microsoft ARM Assembler](https://learn.microsoft.com/en-us/cpp/assembler/arm/arm-assembler-command-line-reference)
- `ASM-ATT`
- `Java` (undocumented)
- `RC` (undocumented)

After a language is enabled, adding sources files with the corresponding extension to your target will automatically use the appropriate compiler/assembler for it.

_Note_: It is generally discouraged to disable the `C` language, unless you are absolutely sure it is not used. Sometimes projects added with `fetch-content` implicitly require it and the error messages can be extremely confusing.

## Conditions

You can specify your own named conditions and use them in any `condition` field:

```toml
[conditions]
ptr64 = "CMAKE_SIZEOF_VOID_P EQUAL 8"
ptr32 = "CMAKE_SIZEOF_VOID_P EQUAL 4"
```

This will make the `ptr64` and `ptr32` conditions available with their respective CMake expressions.

**Note**: condition names can only contain lower-case alphanumeric characters (`[0-9a-z]`) and dashes (`-`).

You can also prefix most keys with `condition.` to represent a conditional:

```toml
[target]
type = "executable"
sources = ["src/main.cpp"]
ptr64.sources = ["src/ptr64_only.cpp"]
```

Instead of a named condition you can also specify a [CMake expression](https://cmake.org/cmake/help/latest/command/if.html#condition-syntax) in quotes. Instances of `$<name>` are replaced with the corresponding condition. For example: `"CONDITIONS_BUILD_TESTS AND $<linux>"` becomes `CONDITIONS_BUILD_TESTS AND (CMAKE_SYSTEM_NAME MATCHES "Linux")` in the final `CMakeLists.txt` file.

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
msvc = "MSVC"
clang = "(CMAKE_CXX_COMPILER_ID MATCHES \"Clang\" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES \"^MSVC$\") OR (CMAKE_C_COMPILER_ID MATCHES \"Clang\" AND NOT CMAKE_C_COMPILER_FRONTEND_VARIANT MATCHES \"^MSVC$\")"
clang-cl = "(CMAKE_CXX_COMPILER_ID MATCHES \"Clang\" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES \"^MSVC$\") OR (CMAKE_C_COMPILER_ID MATCHES \"Clang\" AND CMAKE_C_COMPILER_FRONTEND_VARIANT MATCHES \"^MSVC$\")"
clang-any = "CMAKE_CXX_COMPILER_ID MATCHES \"Clang\" OR CMAKE_C_COMPILER_ID MATCHES \"Clang\""
root = "CMKR_ROOT_PROJECT"
x64 = "CMAKE_SIZEOF_VOID_P EQUAL 8"
x32 = "CMAKE_SIZEOF_VOID_P EQUAL 4"
android = "ANDROID"
apple = "APPLE"
bsd = "BSD"
cygwin = "CYGWIN"
ios = "IOS"
xcode = "XCODE"
wince = "WINCE"
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

Options correspond to [CMake cache variables](https://cmake.org/cmake/help/book/mastering-cmake/chapter/CMake%20Cache.html) that can be used to customize your project at configure-time. You can configure with `cmake -DMYPROJECT_BUILD_TESTS=ON` to enable the option. Every option automatically gets a corresponding [condition](#conditions). Additionally, a normalized condition is created based on the `[project].name` (i.e. `MYPROJECT_BUILD_TESTS` becomes `build-tests`).

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
version = "2024.11.16"
url = "https://github.com/microsoft/vcpkg/archive/refs/tags/2024.11.16.tar.gz"
packages = ["fmt", "zlib"]
overlay-ports = ["my-ports"]
overlay-triplets = ["my-triplets"]
```

The vcpkg `version` will automatically generate the `url` from the [official repository](https://github.com/microsoft/vcpkg/releases). For a custom registry you can specify your own `url` (and omit the `version`). You can browse available packages on [vcpkg.io](https://vcpkg.io/en/packages.html) or [vcpkg.link](https://vcpkg.link).

To specify package features you can use the following syntax: `imgui[docking-experimental,freetype,sdl2-binding,opengl3-binding]`. To disable the [default features](https://learn.microsoft.com/en-us/vcpkg/concepts/default-features) you can do: `cpp-httplib[core,openssl]`

The `overlay-ports` feature allows you to embed vcpkg ports inside your project, without having to fork the main vcpkg registry or creating a custom registry. You can find more information in the relevant [documentation](https://learn.microsoft.com/en-us/vcpkg/concepts/overlay-ports). The `overlay-triplets` feature allows you to customize triplets and change the default behavior (for example always preferring static libraries on Windows). To specify both in one go you can do `[vcpkg].overlay = "vcpkg-overlay"`.

For a concrete example of all the features, take a look at the [vcpkg_template](https://github.com/build-cpp/vcpkg_template) repository.

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
# Include CMake project from git
[fetch-content.gitcontent]
condition = "mycondition"
git = "https://github.com/myuser/gitcontent"
tag = "v0.1"
shallow = false # shallow clone (--depth 1)
system = false
subdir = "" # folder containing CMakeLists.txt

# Include a CMake project from a URL
[fetch-content.urlcontent]
condition = "mycondition"
url = "https://content-host.com/urlcontent.zip"
# Other supported algorithms:
# md5, sha1, sha224, sha256, sha384, sha512, sha3_224, sha3_256, sha3_384, sha3_512
hash = "SHA1 502a4e25b8b209889c99c7fa0732102682c2e4ff"
sha1 = "502a4e25b8b209889c99c7fa0732102682c2e4ff"

[fetch-content.svncontent]
condition = "mycondition"
svn = "https://svn-host.com/url"
rev = "svn_rev"
```

Table keys that match CMake variable names (`[A-Z_]+`) will be passed to the [`FetchContent_Declare`](https://cmake.org/cmake/help/latest/module/FetchContent.html#command:fetchcontent_declare) command.

## Targets

```toml
[target.mytarget]
condition = "mycondition"
alias = "mytarget::mytarget"
type = "static" # executable, library, shared (DLL), static, interface, object, custom
headers = ["src/mytarget.h"]
sources = ["src/mytarget.cpp"]
msvc-runtime = "" # dynamic (implicit default), static

# The keys below match the target_xxx CMake commands
# Keys prefixed with private- will get PRIVATE visibility
compile-definitions = [""] # preprocessor define (-DFOO)
private-compile-definitions = [""]
compile-features = [""] # C++ standard version (cxx_std_20)
private-compile-features = [""]
compile-options = [""] # compiler flags
private-compile-options = [""]
include-directories = [""] # include paths/directories
private-include-directories = [""]
link-directories = [""] # library directories
private-link-directories = [""]
link-libraries = [""] # dependencies
private-link-libraries = [""]
link-options = [""] # linker flags
private-link-options = [""]
precompile-headers = [""] # precompiled headers
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

A table mapping the cmkr features to the relevant CMake construct and the relevant documentation pages:

| cmkr | CMake construct | Description |
| ---- | ----- | ----------- |
| `alias` | [Alias Libraries](https://cmake.org/cmake/help/latest/command/add_library.html#alias-libraries) | Create an [alias target](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#alias-targets), used for namespacing or clarity. |
| `sources` | [`target_sources`](https://cmake.org/cmake/help/latest/command/target_sources.html) | Source files (`PRIVATE` except `interface` targets). |
| `headers` | [`target_sources`](https://cmake.org/cmake/help/latest/command/target_sources.html) | For readability (and future packaging). |
| `msvc-runtime` | [`MSVC_RUNTIME_LIBRARY`](https://cmake.org/cmake/help/latest/prop_tgt/MSVC_RUNTIME_LIBRARY.html) | The [CMP0091](https://cmake.org/cmake/help/latest/policy/CMP0091.html) policy is set automatically. |
| `compile-definitions` | [`target_compile_definitions`](https://cmake.org/cmake/help/latest/command/target_compile_definitions.html) | Adds a macro definition (define, `-DMYMACRO=XXX`). |
| `compile-features` | [`target_compile_features`](https://cmake.org/cmake/help/latest/command/target_compile_features.html) | Specifies the C++ standard version (`cxx_std_20`). |
| `compile-options` | [`target_compile_options`](https://cmake.org/cmake/help/latest/command/target_compile_options.html) | Adds compiler flags. |
| `include-directories` | [`target_include_directories`](https://cmake.org/cmake/help/latest/command/target_include_directories.html) | Adds include directories. |
| `link-directories` | [`target_link_directories`](https://cmake.org/cmake/help/latest/command/target_link_directories.html) | Adds library directories. |
| `link-libraries` | [`target_link_libraries`](https://cmake.org/cmake/help/latest/command/target_link_libraries.html) | Adds library dependencies. Use `::mylib` to make sure a target exists. |
| `link-options` | [`target_link_options`](https://cmake.org/cmake/help/latest/command/target_link_options.html) | Adds linker flags. |
| `precompile-headers` | [`target_precompile_headers`](https://cmake.org/cmake/help/latest/command/target_precompile_headers.html) | Specifies precompiled headers. |
| `properties` | [`set_target_properties`](https://cmake.org/cmake/help/latest/command/set_target_properties.html) | See [properties on targets](https://cmake.org/cmake/help/latest/manual/cmake-properties.7.html#properties-on-targets) for more information. |

The default [visibility](/basics) is as follows:

| Type         | Visibility  |
| ------------ | ----------- |
| `executable` | `PRIVATE`   |
| `library`    | `PUBLIC`    |
| `shared`     | `PUBLIC`    |
| `static`     | `PUBLIC`    |
| `object`     | `PUBLIC`    |
| `interface`  | `INTERFACE` |

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