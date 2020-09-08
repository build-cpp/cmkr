#include <iostream>
#include <string>
#include <toml.hpp>
#include <vector>

int main(int argc, char **argv) try {
    std::vector<std::string> args;
    for (int i = 0; i < argc; ++i)
        args.push_back(argv[i]);
    const auto data = toml::parse("../cmake.toml");
    const auto &cmake = toml::find(data, "cmake");
    const auto cmake_min = toml::find(cmake, "minimum_required");
    const auto &project = toml::find(data, "project");
    const auto name = toml::find(project, "name");
    const auto version = toml::find(project, "version");
    const auto &bin = toml::find(data, "executable");
    const auto bin1 = toml::find(bin, 0);
    const auto bin1_name = toml::find(bin1, "name");
    std::cout << cmake_min << std::endl;
    std::cout << name << std::endl;
    std::cout << version << std::endl;
    std::cout << bin.size() << std::endl;
    std::cout << bin1_name << std::endl;
} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
}