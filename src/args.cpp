#include "args.h"
#include "build.h"
#include "gen.h"
#include "help.h"

#include "fs.hpp"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace cmkr {
namespace args {
const char *handle_args(int argc, char **argv) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    if (args.size() < 2)
        return "Please provide command line arguments!";
    std::string main_arg = args[1];
    if (main_arg == "gen") {
        bool cont = false;
        if (args.size() > 2 && args[2] == "-y")
            cont = true;
        auto current_path = fs::current_path();
        if (fs::exists(current_path / "CMakeLists.txt") && cont == false) {
            std::cout
                << "A CMakeLists.txt file already exists in the current directory.\nWould you "
                   "like to overwrite it?[y/n]"
                << std::endl;
            std::string resp;
            std::cin >> resp;
            if (resp != "y")
                return "CMake generation aborted!";
        }
        auto ret = cmkr::gen::generate_cmake(current_path.string().c_str());
        if (ret)
            return "CMake generation error!";
        return "CMake generation successful!";
    } else if (main_arg == "help") {
        return cmkr::help::message();
    } else if (main_arg == "version") {
        return cmkr::help::version();
    } else if (main_arg == "init") {
        std::string typ = "exe";
        if (args.size() > 2)
            typ = args[2];
        auto ret = cmkr::gen::generate_project(typ.c_str());
        if (ret)
            return "Initialization failure!";
        return "Directory initialized!";
    } else if (main_arg == "build") {
        auto ret = build::run(argc, argv);
        if (ret)
            return "CMake build error!";
        return "CMake run completed!";
    } else if (main_arg == "install") {
        auto ret = build::install();
        if (ret)
            return "CMake install error!";
        return "CMake install completed!";
    } else if (main_arg == "clean") {
        auto ret = build::clean();
        if (ret)
            return "CMake clean error!";
        return "Cleaned build directory!";
    } else {
        return "Unknown argument!";
    }
}
} // namespace args
} // namespace cmkr

const char *cmkr_args_handle_args(int argc, char **argv) {
    try {
        return cmkr::args::handle_args(argc, argv);
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "Unknown error!";
    }
}
