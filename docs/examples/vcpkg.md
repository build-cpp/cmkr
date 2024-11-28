---
# Automatically generated from tests/vcpkg/cmake.toml - DO NOT EDIT
layout: default
title: Dependencies from vcpkg
permalink: /examples/vcpkg
parent: Examples
nav_order: 4
---

# Dependencies from vcpkg

Downloads [fmt v7.1.3](https://fmt.dev/7.1.3/) using [vcpkg](https://github.com/microsoft/vcpkg) and links an `example` target to it:

```toml
[project]
name = "vcpkg"
description = "Dependencies from vcpkg"

# See https://github.com/microsoft/vcpkg/releases for vcpkg versions
# See https://vcpkg.io/en/packages or https://vcpkg.link for available packages
[vcpkg]
version = "2024.11.16"
packages = ["fmt"]

[find-package.fmt]

[target.example]
type = "executable"
sources = ["src/main.cpp"]
link-libraries = ["fmt::fmt"]
```

The bootstrapping of vcpkg is fully automated and no user interaction is necessary. You can disable vcpkg by setting `CMKR_DISABLE_VCPKG=ON`.

To specify package features you can use the following syntax: `imgui[docking-experimental,freetype,sdl2-binding,opengl3-binding]`.

<sup><sub>This page was automatically generated from [tests/vcpkg/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/vcpkg/cmake.toml).</sub></sup>
