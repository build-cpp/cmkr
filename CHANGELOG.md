# CHANGELOG

## 0.1.4 - 2021-04-11
- Use `GIT_SHALLOW ON` by default when getting git dependencies.
- Add cmake.description field.
- Change bin.defines to bin.definitions.

## 0.1.3 - 2020-11-27
- Support building with C++11.
- @mrexodia implemented CMake integration and bootstrapping.
- Add dependency on ghc_filesystem and mpark_variant which are fetched automatically using FetchContent.

## 0.1.2 - 2020-11-20
- Add support for target properties.
- Add installs.
- Require cmake >= 3.15.
- Support settings and caching settings.
- Support config when running cmkr build.
- Add prompt prior to CMakeLists.txt generation if already existent in the current dir.

## 0.1.1 - 2020-11-19
- Add support for globbing.
- Add support for find_package components.
- Add options.
- Support aliases.
- Support interface libs (header-only libs).
- Support testing.