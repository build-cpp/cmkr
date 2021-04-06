#pragma once

static const char *hello_world = &R"lit(
#include <iostream>

int %s() {
    std::cout << "Hello World!\n";
    return 0;
}
)lit"[1]; // skip initial newline

static const char *cmake_toml = &R"lit(
[cmake]
version = "3.15"
# subdirs = []
# build-dir = ""
# cpp-flags = []
# c-flags = []
# link-flags = []
# generator = ""
# arguments = []

[project]
name = "%s"
version = "0.1.0"

# [find-package]

# [fetch-content]

# [options]

[[target]]
name = "%s"
type = "%s"
sources = ["src/*.cpp"]
include-directories = ["include"]
# alias = ""
# compile-features = []
# compile-definitions = []
# link-libraries = []

[[install]]
%s = ["%s"]
destination = "${CMAKE_INSTALL_PREFIX}/%s"
)lit"[1]; // skip initial newline
