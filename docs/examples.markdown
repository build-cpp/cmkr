---
layout: page
title: Examples
permalink: /examples/
---

# Examples

## Quickstart

Smallest possible start point you can have using cmkr 

```toml
[cmake]
version = "3.15"

[project]
name = "hello-world"

[target.hello-world]
type = "executable"
sources = [ "src/*.cpp" ]
```

## Changing C++/C version

Simple example changing C++ to version 20 and C standard to the version 11

```toml
[target.example]
type = "executable"
compile-features = [ "cxx_std_20", "c_std_11" ]
```

## Import from another Git repository

Importing an existing project called Zydis to my project

tag is optional but you can target any branch with it

```toml
[fetch-content]
zydis = { git = "https://github.com/zyantific/zydis.git", tag = "v3.1.0" }

[target.example]
type = "executable"
link-libraries = ["zydis"]
```

## Import from another folder

Importing an existing project called Zydis to my project

tag is optional but you can target any branch with it

```toml
[fetch-content]
zydis = { git = "https://github.com/zyantific/zydis.git", tag = "v3.1.0" }

[target.example]
type = "executable"
link-libraries = ["zydis"]
```
