---
layout: default
title: Quickstart
permalink: /examples/quickstart
parent: Examples
nav_order: 1
---

# Quickstart

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