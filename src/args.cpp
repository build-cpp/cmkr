#include "args.h"
#include "gen.h"
#include "help.h"

#include <filesystem>
#include <stddef.h>
#include <stdexcept>
#include <stdlib.h>
#include <string>
#include <vector>

namespace cmkr::args {
const char *handle_args(int argc, char **argv) {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    if (args.size() < 2)
        throw std::runtime_error("Please provide command line arguments!");
    std::string main_arg = args[1];
    if (main_arg == "gen") {
        cmkr::gen::generate_cmake(std::filesystem::current_path().string().c_str());
        return "Generation successful!";
    } else if (main_arg == "help") {
        return cmkr::help::message();
    } else if (main_arg == "version") {
        return cmkr::help::version();
    } else if (main_arg == "init") {
        if (args.size() < 3)
            throw std::runtime_error("Please provide a project type!");
        cmkr::gen::generate_project(args[2].c_str());
        return "Directory initialized!";
    } else if (main_arg == "build") {
        std::string command = "cmake -S. -Bbin ";
        if (args.size() > 2) {
            for (size_t i = 2; i < args.size(); ++i) {
                command += args[i] + " ";
            }
        }
        command += "&& cmake --build bin";
        auto ret = system(command.c_str());
        if (ret)
            return "Run faced an error!";
        return "Run completed!";
    } else {
        return "Unknown argument!";
    }
}
} // namespace cmkr::args

const char *cmkr_args_handle_args(int argc, char **argv) {
    return cmkr::args::handle_args(argc, argv);
}
