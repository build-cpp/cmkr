#include "build.hpp"
#include "cmake_generator.hpp"
#include "project_parser.hpp"

#include "fs.hpp"
#include <cstdlib>
#include <sstream>

namespace cmkr {
namespace build {

int run(int argc, char **argv) {
    parser::Project project(nullptr, ".", true);
    if (argc > 2) {
        for (int i = 2; i < argc; ++i) {
            project.build_args.emplace_back(argv[i]);
        }
    }
    std::stringstream ss;

    gen::generate_cmake(fs::current_path().string().c_str());

    ss << "cmake -DCMKR_BUILD_SKIP_GENERATION=ON -B" << project.build_dir << " ";

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
    bool success = false;
    parser::Project project(nullptr, ".", true);
    if (fs::exists(project.build_dir)) {
        success = fs::remove_all(project.build_dir);
        fs::create_directory(project.build_dir);
    }
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

int install() {
    parser::Project project(nullptr, ".", false);
    auto cmd = "cmake --install " + project.build_dir;
    return ::system(cmd.c_str());
}
} // namespace build
} // namespace cmkr
