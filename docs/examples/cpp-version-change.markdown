---
layout: default
title: Changing C/C++ version
permalink: /examples/cpp-version-change
parent: Examples
nav_order: 3
---

# Changing C/C++ version

Simple example changing C++ to version 20 and C standard to the version 11

```toml
[target.example]
type = "executable"
compile-features = [ "cxx_std_20", "c_std_11" ]
```
