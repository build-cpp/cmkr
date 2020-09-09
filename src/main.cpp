#include "args.hpp"
#include <iostream>
#include <string>
#include <vector>

int main(int argc, char **argv) try {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);

    auto output = cmkr::args::handle_args(args);
    std::cout << output << std::endl;
    return 0;
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
}