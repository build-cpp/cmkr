#pragma once

static const char *cpp_executable = &R"lit(
#include <iostream>
#include <cstdlib>

int main() {
    std::cout << "Hello from cmkr!\n";
    return EXIT_SUCCESS;
}
)lit"[1]; // skip initial newline

static const char *cpp_library = &R"lit(
#include <@name/@name.hpp>

#include <iostream>

namespace @name {

void hello() {
    std::cout << "Hello from cmkr!\n";
}

} // namespace @name
)lit"[1]; // skip initial newline

static const char *hpp_library = &R"lit(
#pragma once

namespace @name {

void hello();

} // namespace @name
)lit"[1]; // skip initial newline

static const char *hpp_interface = &R"lit(
#pragma once

#include <iostream>

namespace @name {

inline void hello() {
    std::cout << "Hello from cmkr!\n";
}

} // namespace @name
)lit"[1]; // skip initial newline

static const char *toml_executable = &R"lit(
# Reference: https://build-cpp.github.io/cmkr/cmake-toml
[project]
name = "@name"

[target.@name]
type = "executable"
sources = ["src/@name/main.cpp"]
compile-features = ["cxx_std_11"]
)lit"[1]; // skip initial newline

static const char *toml_library = &R"lit(
# Reference: https://build-cpp.github.io/cmkr/cmake-toml
[project]
name = "@name"

[target.@name]
type = "@type"
sources = [
    "src/@name/@name.cpp",
    "include/@name/@name.hpp"
]
include-directories = ["include"]
compile-features = ["cxx_std_11"]
)lit"[1]; // skip initial newline

static const char *toml_interface = &R"lit(
# Reference: https://build-cpp.github.io/cmkr/cmake-toml
[project]
name = "@name"

[target.@name]
type = "interface"
include-directories = ["include"]
compile-features = ["cxx_std_11"]
)lit"[1]; // skip initial newline

static const char *toml_migration = &R"lit(
# Reference: https://build-cpp.github.io/cmkr/cmake-toml
[project]
name = "@name"

# TODO: define a target for each of your executables/libraries like this:
#[target.@name]
#type = "executable"
#sources = ["src/@name/*.cpp", "include/@name/*.hpp"]
#include-directories = ["include"]
#compile-features = ["cxx_std_11"]
#link-libraries = ["other-targets"]
)lit"[1]; // skip initial newline
