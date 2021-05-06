---
layout: default
title: Import content from existing folders
permalink: /examples/import-from-folder
parent: Examples
nav_order: 4
---

# Import content from existing folders

Importing one project by targeting the folder path

```toml
[fetch-content]
zydis = { git = "https://github.com/zyantific/zydis.git", tag = "v3.1.0" }

[target.example]
type = "executable"
link-libraries = ["zydis"]
```
