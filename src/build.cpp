#include "build.h"

#include <filesystem>
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <toml.hpp>

namespace cmkr::build {

int run(int argc, char **argv) {
    std::stringstream ss;
    std::string bin_dir = "bin";
    if (!std::filesystem::exists("CMakeLists.txt"))
        throw std::runtime_error("No valid CMakeLists.txt found!");

    const auto toml = toml::parse("cmake.toml");
    if (toml.contains("cmake")) {
        const auto &cmake = toml::find(toml, "cmake");
        ss << "cmake -S. -B";
        if (cmake.contains("bin-dir")) {
            bin_dir = toml::find(cmake, "bin-dir").as_string();
        }
        ss << bin_dir << " ";
        if (cmake.contains("generator")) {
            const auto gen = toml::find(cmake, "generator").as_string();
            ss << "-G " << gen << " ";
        }
        if (cmake.contains("arguments")) {
            const auto args = toml::find(cmake, "arguments").as_array();
            for (const auto &arg : args) {
                ss << "-D" << arg << " ";
            }
        }
        ss << "&& cmake --build " << bin_dir;
        if (argc > 2) {
            for (size_t i = 2; i < argc; ++i) {
                ss << " " << argv[i];
            }
        }
    }
    return ::system(ss.str().c_str());
}

} // namespace cmkr::build

int cmkr_build_run(int argc, char **argv) {
    return cmkr::build::run(argc, argv);
}