#include "args.h"
#include "build.h"
#include "gen.h"
#include "help.h"

#include <exception>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace cmkr::args {
const char *handle_args(int argc, char **argv) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    if (args.size() < 2)
        return "Please provide command line arguments!";
    std::string main_arg = args[1];
    if (main_arg == "gen") {
        auto ret = cmkr::gen::generate_cmake(std::filesystem::current_path().string().c_str());
        if (ret)
            return "CMake generation error!";
        return "CMake generation successful!";
    } else if (main_arg == "help") {
        return cmkr::help::message();
    } else if (main_arg == "version") {
        return cmkr::help::version();
    } else if (main_arg == "init") {
        if (args.size() < 3)
            return "Please provide a project type!";
        auto ret = cmkr::gen::generate_project(args[2].c_str());
        if (ret)
            return "Initialization failure!";
        return "Directory initialized!";
    } else if (main_arg == "build") {
        auto ret = build::run(argc, argv);
        if (ret)
            return "CMake build error!";
        return "CMake run completed!";
    } else if (main_arg == "clean") {
        auto ret = build::clean();
        if (ret)
            return "CMake clean error!";
        return "Cleaned build directory!";
    } else {
        return "Unknown argument!";
    }
}
} // namespace cmkr::args

const char *cmkr_args_handle_args(int argc, char **argv) {
    try {
        return cmkr::args::handle_args(argc, argv);
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "Unknown error!";
    }
}
