#include "build.hpp"
#include "cmake_generator.hpp"
#include "error.hpp"
#include "project_parser.hpp"

#include "fs.hpp"
#include <sstream>
#include <stddef.h>
#include <stdexcept>
#include <stdlib.h>
#include <system_error>

namespace cmkr {
namespace build {

int run(int argc, char **argv) {
    parser::Project project(nullptr, ".", true);
    if (argc > 2) {
        for (int i = 2; i < argc; ++i) {
            project.build_args.push_back(argv[i]);
        }
    }
    std::stringstream ss;

    if (gen::generate_cmake(fs::current_path().string().c_str()))
        throw std::runtime_error("CMake generation failure!");

    ss << "cmake -S. -DCMKR_SKIP_GENERATION=ON -B" << project.build_dir << " ";

    if (!project.generator.empty()) {
        ss << "-G \"" << project.generator << "\" ";
    }
    if (!project.config.empty()) {
        ss << "-DCMAKE_BUILD_TYPE=" << project.config << " ";
    }
    if (!project.gen_args.empty()) {
        for (const auto &arg : project.gen_args) {
            ss << "-D" << arg << " ";
        }
    }
    ss << "&& cmake --build " << project.build_dir << " --parallel";
    if (argc > 2) {
        for (const auto &arg : project.build_args) {
            ss << " " << arg;
        }
    }

    return ::system(ss.str().c_str());
}

int clean() {
    bool ret = false;
    parser::Project project(nullptr, ".", true);
    if (fs::exists(project.build_dir)) {
        ret = fs::remove_all(project.build_dir);
        fs::create_directory(project.build_dir);
    }
    return !ret;
}

int install() {
    parser::Project project(nullptr, ".", false);
    auto cmd = "cmake --install " + project.build_dir;
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