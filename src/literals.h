#pragma once

const char *hello_world = R"lit(
#include <iostream>

int %s() {
    std::cout << "Hello World!\n";
    return 0;
}
)lit";

const char *cmake_toml = R"lit(
[cmake]
minimum = "3.14"
# bin-dir = ""
# cpp-flags = []
# c-flags = []
# link-flags = []
# subdirs = []
# generator = ""
# arguments = []

[project]
name = "%s"
version = "0.1.0"

# [find-package]

# [fetch-content]

[[bin]]
name = "%s"
type = "%s"
sources = ["src/main.cpp"]
# include-dirs = []
# features = []
# defines = []
# link-libs = []
)lit";
