#include "build.h"
#include "error.h"
#include "gen.h"

#include <filesystem>
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <system_error>
#include <toml.hpp>

namespace cmkr::build {

int run(int argc, char **argv) {
    std::stringstream ss;
    std::string bin_dir = "bin";

    if (!std::filesystem::exists("CMakeLists.txt"))
        if (gen::generate_cmake("."))
            throw std::runtime_error("CMake generation failure!");

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
    try {
        return cmkr::build::run(argc, argv);
    } catch (const std::system_error &e) {
        return e.code().value();
    } catch (...) {
        return cmkr::error::Status(cmkr::error::Status::Code::BuildError);
    }
}