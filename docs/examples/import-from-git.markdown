---
layout: default
title: Import content from Github
permalink: /examples/import-from-git
parent: Examples
nav_order: 2
---

# Import content from Github

Importing an existing project called Zydis to my project

tag is optional but you can target any branch with it

```toml
[fetch-content]
zydis = { git = "https://github.com/zyantific/zydis.git", tag = "v3.1.0" }

[target.example]
type = "executable"
link-libraries = ["zydis"]
```
