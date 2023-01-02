---
layout: page
title: Philosophy
nav_order: 3
---

# Philosophy

## Problem statement

Similar to writing good C++, writing good CMake is difficult. The main difference is that nobody actually wants to learn CMake. The build system is something that should "just work".

There have been many attempts at creating new build systems (with varying levels of success). Naturally this causes [competing standards](https://xkcd.com/927/), which is undesirable. CMake is pretty much the de facto standard, and it has seen extensive use in complex software projects.

One of the main issues of CMake is the turing-complete scripting language you use to describe your projects. As your project gets more complex this can be very helpful, but it also unnecessarily complicates simple projects. There have been discussions about a declarative language on the [CMake issue tracker](https://gitlab.kitware.com/cmake/cmake/-/issues/19891) to solve this problem, but it looks like a "LISP-like" language is seriously being considered...

## The solution

The way cmkr (pronounced "cmaker") solves this problem is by using [TOML](https://toml.io/). Below is a minimal `cmake.toml` project:

```toml
[project]
name = "cmkr_for_beginners"

[target.hello_world]
type = "executable"
sources = ["src/main.cpp"]
```

The key difference between `cmkr` and other build systems is that it _generates_ CMake. This means that your projects are fully compatible with the CMake ecosystem, and you can try it out without having to rewrite your whole build system.

TODO: link/include more examples? Talk about conditions, packaging (missing)

### Another layer?!

A common issue people have with cmkr is that it introduces an additional layer of indirection to your build system. CMake is already a meta-buildsystem, which means you could call cmkr a "meta-meta-buildsystem".

Presumably the reason for this friction is that additional layers of abstraction introduce additional complexity. Because of this cmkr has been designed to be completely seamless:

- The user doesn't have to install any additional software to use cmkr. All you need is a semi-recent version of CMake and a C++ compiler.
- Modifying `cmake.toml` automatically triggers a regeneration of `CMakeLists.txt`. There is no change to your build process.
- The `CMakeLists.txt` is generated to be human-readable. This means you can easily "eject" from cmkr and go back to CMake.

An additional argument for cmkr is that anecdotally people say it "just works". Because of its simplicity it's also easy to teach, even to people without programming experience.

There is also precedent in the JavaScript community. Bundlers and translators are the norm there and their developer experience is miles ahead of C++.

<sub>Not to say the JavaScript ecosystem is without its flaws, but generators does not appear to be one of them</sub>

### Unsupported features

Because cmkr is still in early in development there are many missing/unfinished features. It was decided that users can bridge the gap by including CMake at arbitrary locations.

This has the advantage that it forces complexity in the build system to be self-contained and modular, a problem all too common in projects as they age.

## Enterprise

Words like "bootstrapping" and "generating code" might worry engineers working in an enterprise environment, and rightly so. From the beginning it has been a priority to make cmkr suitable for use in big corporations with strict protocols for security and reproducibility.

### No additional dependencies

As mentioned above, the only thing you need is a working C++ compiler and a semi-recent version of CMake. It is assumed that you are already building (and executing) C++ projects on your servers, so cmkr does not introduce additional requirements.

All the logic for downloading and compiling the `cmkr` executable is self-contained in a ~250 line `cmkr.cmake` script. You can easily audit it and see if it's up to your standards.

### Reproducibility

Per default the `cmkr.cmake` bootstrapping script contains the exact version of cmkr used to generate your project. As long as the cmkr repository is available you will build the exact same version of cmkr, which will generate the exact same `CMakeLists.txt` file.

This also means that cmkr can decide to break backwards compatibility without affecting legacy projects. An effort will always be made to maintain backwards compatibility though.

### Integrity

As an additional safeguard you can modify `cmkr.cmake` to pin the version tag to a commit hash. This hash is checked to ensure the integrity of the upstream repository.

### Availability

You can easily point `cmkr.cmake` to a mirror of the cmkr repository to ensure availability should something catastrophic happen.

### Not executed in CI

The final (and key) feature is that the bootstrapping process is never executed in CI environments. This means `cmkr` is only ever executed on your developer's machines and not on your infrastructure.