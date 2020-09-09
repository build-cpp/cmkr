#include "gen.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <toml.hpp>

namespace fs = std::filesystem;

namespace cmkr::gen {
namespace detail {
inline std::string to_upper(const std::string &str) {
    std::string temp;
    temp.reserve(str.size());
    for (auto c : str) {
        temp.push_back(toupper(c));
    }
    return temp;
}
} // namespace detail

void generate_project(const std::string &str) {
    fs::create_directory("src");
    auto dir_name = fs::current_path().stem();
    if (str == "app") {
        std::ofstream ofs("src/main.cpp");
        if (ofs.is_open()) {
            ofs << "#include <iostream>\n\nint main() {\n\tstd::cout << \"Hello world!\" << "
                   "std::endl;\n}";
        }
        ofs.flush();
        ofs.close();

        std::ofstream ofs2("cmake.toml");
        if (ofs2.is_open()) {
            ofs2 << "[cmake]\nminimum_required = \"3.0\"\n\n[project]\nname = \""
                 << dir_name.string()
                 << "\"\nversion = "
                    "\"0.1.0\"\n\n[[app]]\nname = \""
                 << dir_name.string() << "\"\nsources = [\"src/main.cpp\"]\n";
        }
        ofs2.flush();
        ofs2.close();
    } else if (str == "static") {
        std::ofstream ofs2("cmake.toml");
        if (ofs2.is_open()) {
            ofs2 << "[cmake]\nminimum_required = \"3.0\"\n\n[project]\nname = \""
                 << dir_name.string()
                 << "\"\nversion = "
                    "\"0.1.0\"\n\n[[lib]]\nname = \""
                 << dir_name.string() << "\"\nsources = [\"src/main.cpp\"]\ntype = \"static\"\n";
        }
        ofs2.flush();
        ofs2.close();
    } else if (str == "shared") {
        std::ofstream ofs2("cmake.toml");
        if (ofs2.is_open()) {
            ofs2 << "[cmake]\nminimum_required = \"3.0\"\n\n[project]\nname = \""
                 << dir_name.string()
                 << "\"\nversion = "
                    "\"0.1.0\"\n\n[[lib]]\nname = \""
                 << dir_name.string() << "\"\nsources = [\"src/main.cpp\"]\ntype = \"shared\"\n";
        }
        ofs2.flush();
        ofs2.close();
    } else {
        throw std::runtime_error("Unknown project type. Types are app, shared, static!");
    }
}

void generate_cmake() {
    std::stringstream ss;

    const auto toml = toml::parse("cmake.toml");
    const auto &cmake = toml::find(toml, "cmake");
    const std::string cmake_min = toml::find(cmake, "minimum_required").as_string();
    const auto &project = toml::find(toml, "project");
    const std::string proj_name = toml::find(project, "name").as_string();
    const std::string proj_version = toml::find(project, "version").as_string();

    ss << "cmake_minimum_required(VERSION " << cmake_min << ")\n\n"
       << "project(" << proj_name << " VERSION " << proj_version << ")\n\n"
       << "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n";

    if (toml.contains("dependencies")) {
        std::map<std::string, std::string> deps =
            toml::find<std::map<std::string, std::string>>(toml, "dependencies");
        for (const auto& dep : deps) {
            ss << "find_package(" << dep.first;
            if (dep.second != "*") {
                ss << " " << dep.second << " CONFIG REQUIRED)\n";
            } else {
                ss << " CONFIG REQUIRED)\n";
            }
        }
    }

    ss << "\n";

    if (toml.contains("app")) {
        const auto &bins = toml::find(toml, "app");

        for (auto i = 0; i < bins.size(); ++i) {
            const auto bin = toml::find(bins, i);
            const std::string bin_name = toml::find(bin, "name").as_string();

            const auto srcs = toml::find(bin, "sources");
            ss << "set(" << detail::to_upper(bin_name) << "_SOURCES\n";
            for (auto j = 0; j < srcs.size(); ++j) {
                const std::string source = toml::find(srcs, i).as_string();
                ss << "\t" << source << "\n";
            }
            ss << "\t)\n\n"
               << "add_executable(" << bin_name << " ${" << detail::to_upper(bin_name)
               << "_SOURCES})\n\n";

            if (bin.contains("include_directories")) {
                ss << "target_include_directories(" << bin_name << " PUBLIC\n\t";
                const auto includes = toml::find(bin, "link_libraries");
                for (auto k = 0; k < includes.size(); ++k) {
                    const std::string inc = toml::find(includes, i).as_string();
                    ss << inc << "\n\t";
                }
                ss << ")";
            }
            if (bin.contains("link_libraries")) {
                ss << "target_link_libraries(" << bin_name << " PUBLIC\n\t";
                const auto includes = toml::find(bin, "link_libraries");
                for (auto k = 0; k < includes.size(); ++k) {
                    const std::string inc = toml::find(includes, i).as_string();
                    ss << inc << "\n\t";
                }
                ss << ")";
            }
            if (bin.contains("compile_features")) {
                ss << "target_compile_features(" << bin_name << " PUBLIC\n\t";
                const auto includes = toml::find(bin, "compile_features");
                for (auto k = 0; k < includes.size(); ++k) {
                    const std::string inc = toml::find(includes, i).as_string();
                    ss << inc << "\n\t";
                }
                ss << ")";
            }
        }
    }

    if (toml.contains("lib")) {
        const auto &libs = toml::find(toml, "lib");

        for (auto i = 0; i < libs.size(); ++i) {
            const auto lib = toml::find(libs, i);
            const std::string lib_name = toml::find(lib, "name").as_string();
            const std::string type = toml::find(lib, "type").as_string();

            const auto srcs = toml::find(lib, "sources");
            ss << "set(" << detail::to_upper(lib_name) << "_SOURCES\n";
            for (auto j = 0; j < srcs.size(); ++j) {
                const std::string source = toml::find(srcs, i).as_string();
                ss << "\t" << source << "\n";
            }
            ss << "\t)\n\n"
               << "add_library(" << lib_name << " " << detail::to_upper(type) << " ${"
               << detail::to_upper(lib_name) << "_SOURCES})\n\n";

            if (lib.contains("include_directories")) {
                ss << "target_include_directories(" << lib_name << " PUBLIC\n\t";
                const auto includes = toml::find(lib, "include_directories");
                for (auto k = 0; k < includes.size(); ++k) {
                    const std::string inc = toml::find(includes, i).as_string();
                    ss << inc << "\n\t";
                }
                ss << ")";
            }
            if (lib.contains("link_libraries")) {
                ss << "target_link_libraries(" << lib_name << " PUBLIC\n\t";
                const auto includes = toml::find(lib, "link_libraries");
                for (auto k = 0; k < includes.size(); ++k) {
                    const std::string inc = toml::find(includes, i).as_string();
                    ss << inc << "\n\t";
                }
                ss << ")";
            }
            if (lib.contains("compile_features")) {
                ss << "target_compile_features(" << lib_name << " PUBLIC\n\t";
                const auto includes = toml::find(lib, "compile_features");
                for (auto k = 0; k < includes.size(); ++k) {
                    const std::string inc = toml::find(includes, i).as_string();
                    ss << inc << "\n\t";
                }
                ss << ")";
            }
        }

        std::ofstream ofs("CMakeLists.txt");
        if (ofs.is_open()) {
            ofs << ss.rdbuf();
        }
        ofs.flush();
        ofs.close();
    }
}
} // namespace cmkr::gen