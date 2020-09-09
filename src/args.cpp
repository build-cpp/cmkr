#include "args.hpp"
#include "gen.hpp"
#include "help.hpp"
#include <cstddef>
#include <cstdlib>
#include <stdexcept>

namespace cmkr::args {
std::string handle_args(std::vector<std::string> &args) {
    if (args.size() < 2)
        throw std::runtime_error("Please provide command line arguments!");
    std::string main_arg = args[1];
    if (main_arg == "gen") {
        cmkr::gen::generate_cmake();
        return "Generation successful!";
    } else if (main_arg == "help") {
        return cmkr::help::help_msg;
    } else if (main_arg == "version") {
        return cmkr::help::version;
    } else if (main_arg == "init") {
        if (args.size() < 3)
            throw std::runtime_error("Please provide a project type!");
        cmkr::gen::generate_project(args[2]);
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