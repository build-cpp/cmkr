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
minimum = "3.15"
# subdirs = []
# bin-dir = ""
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

[[bin]]
name = "%s"
type = "%s"
sources = ["src/*.cpp"]
include-dirs = ["include"]
# alias = ""
# features = []
# definitions = []
# link-libs = []

[[install]]
%s = ["%s"]
destination = "${CMAKE_INSTALL_PREFIX}/%s"

)lit";
