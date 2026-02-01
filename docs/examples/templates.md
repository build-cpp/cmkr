---
# Automatically generated from tests/templates/cmake.toml - DO NOT EDIT
layout: default
title: Target templates
permalink: /examples/templates
parent: Examples
nav_order: 7
---

# Target templates

To avoid repeating yourself in targets you can create your own target type (template). All properties of the template are inherited when used as a target type.

```toml
[project]
name = "templates"
description = "Target templates"

[template.app]
type = "executable"
sources = ["src/templates.cpp"]
compile-definitions = ["IS_APP"]
# Unlike interface targets you can also inherit properties
properties = {
    CXX_STANDARD = "11",
    CXX_STANDARD_REQUIRED = true,
}

[target.app-a]
type = "app"
compile-definitions = ["APP_A"]

[target.app-b]
type = "app"
compile-definitions = ["APP_B"]
```

**Note**: In most cases you probably want to use an [interface](/examples/interface) target instead.

<sup><sub>This page was automatically generated from [tests/templates/cmake.toml](https://github.com/build-cpp/cmkr/tree/main/tests/templates/cmake.toml).</sub></sup>
