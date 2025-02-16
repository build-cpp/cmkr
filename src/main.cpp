#include "arguments.hpp"

#include <iostream>
#include <stdexcept> 

int main(int argc, char **argv) {

    try {
        std::string output = cmkr::args::handle_args(argc, argv);
        std::string format = "[cmkr]" + output + "\n";

        if (format.find('\n') != std::string::npos)
            format = output + "\n";

        std::cerr << format;
        return EXIT_SUCCESS;

    } catch (const std::exception &e) {
        std::string error = e.what();
        std::string format = "[cmkr] error: " + error + "\n";

        if (error.find('\n') != std::string::npos)
            format = error + "\n";

        std::cerr  << format;
        return EXIT_FAILURE;
    }
}
