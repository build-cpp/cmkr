#include "build.h"
#include "cmake.hpp"
#include "error.h"
#include "gen.h"

#include "fs.hpp"
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <stdlib.h>
#include <system_error>

namespace cmkr {
namespace build {

int run(int argc, char **argv) {
    cmake::CMake cmake(".", true);
    if (argc > 2) {
        for (size_t i = 2; i < argc; ++i) {
            cmake.build_args.push_back(argv[i]);
        }
    }
    std::stringstream ss;

    if (!fs::exists("CMakeLists.txt"))
        if (gen::generate_cmake(fs::current_path().string().c_str()))
            throw std::runtime_error("CMake generation failure!");

    ss << "cmake -S. -B" << cmake.build_dir << " ";

    if (!cmake.generator.empty()) {
        ss << "-G \"" << cmake.generator << "\" ";
    }
    if (!cmake.config.empty()) {
        ss << "-DCMAKE_BUILD_TYPE=" << cmake.config << " ";
    }
    if (!cmake.gen_args.empty()) {
        for (const auto &arg : cmake.gen_args) {
            ss << "-D" << arg << " ";
        }
    }
    ss << "&& cmake --build " << cmake.build_dir << " --parallel";
    if (argc > 2) {
        for (const auto &arg : cmake.build_args) {
            ss << " " << arg;
        }
    }

    return ::system(ss.str().c_str());
}

int clean() {
    bool ret = false;
    cmake::CMake cmake(".", true);
    if (fs::exists(cmake.build_dir)) {
        ret = fs::remove_all(cmake.build_dir);
        fs::create_directory(cmake.build_dir);
    }
    return !ret;
}

int install() {
    cmake::CMake cmake(".", false);
    auto cmd = "cmake --install " + cmake.build_dir;
    return ::system(cmd.c_str());
}
} // namespace build
} // namespace cmkr

int cmkr_build_run(int argc, char **argv) {
    try {
        return cmkr::build::run(argc, argv);
    } catch (const std::system_error &e) {
        return e.code().value();
    } catch (...) {
        return cmkr::error::Status(cmkr::error::Status::Code::BuildError);
    }
}

int cmkr_build_clean(void) {
    try {
        return cmkr::build::clean();
    } catch (const std::system_error &e) {
        return e.code().value();
    } catch (...) {
        return cmkr::error::Status(cmkr::error::Status::Code::CleanError);
    }
}

int cmkr_build_install(void) {
    try {
        return cmkr::build::install();
    } catch (const std::system_error &e) {
        return e.code().value();
    } catch (...) {
        return cmkr::error::Status(cmkr::error::Status::Code::InstallError);
    }
}