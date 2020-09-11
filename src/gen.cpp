#include "gen.h"
#include "error.h"
#include "literals.h"

#include <filesystem>
#include <fstream>
#include <map>
#include <new>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <string>
#include <string_view>
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

template <typename... Args>
std::string format(const char *fmt, Args... args) {
    auto sz = snprintf(nullptr, 0, fmt, args...) + 1;
    char *buf = new char[sz];
    int ret = snprintf(buf, sz, fmt, args...);
    if (ret != sz - 1)
        throw std::runtime_error("Error formatting string!");
    std::string temp(buf, buf + sz - 1);
    delete[] buf;
    return temp;
}

} // namespace detail

int generate_project(const char *str) {
    fs::create_directory("src");
    const auto dir_name = fs::current_path().stem().string();
    std::string mainbuf;
    const auto tomlbuf = detail::format(cmake_toml, dir_name.c_str(), dir_name.c_str(), str);
    if (!strcmp(str, "exe")) {
        mainbuf = detail::format(hello_world, "main");
    } else if (!strcmp(str, "static") || !strcmp(str, "shared")) {
        fs::create_directory("include");
        mainbuf = detail::format(hello_world, "test");
    } else {
        throw std::runtime_error("Unknown project type. Types are exe, shared, static!");
    }

    std::ofstream ofs("src/main.cpp");
    if (ofs.is_open()) {
        ofs << mainbuf;
    }
    ofs.flush();
    ofs.close();

    std::ofstream ofs2("cmake.toml");
    if (ofs2.is_open()) {
        ofs2 << tomlbuf;
    }
    ofs2.flush();
    ofs2.close();

    return 0;
}

int generate_cmake(const char *path) {
    std::stringstream ss;
    std::vector<std::string> subdirs;

    const auto toml = toml::parse((fs::path(path) / "cmake.toml").string());
    if (toml.contains("cmake")) {
        const auto &cmake = toml::find(toml, "cmake");
        const std::string cmake_min = toml::find(cmake, "minimum").as_string();
        ss << "cmake_minimum_required(VERSION " << cmake_min << ")\n\n";

        ss << "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n";

        if (cmake.contains("cpp-flags")) {
            ss << "set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS}\"";
            const auto flags = toml::find(cmake, "cpp-flags").as_array();
            for (const auto &flag : flags) {
                ss << " " << std::string_view(flag.as_string());
            }
            ss << "\")\n\n";
        }

        if (cmake.contains("c-flags")) {
            ss << "set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS}\"";
            const auto flags = toml::find(cmake, "c-flags").as_array();
            for (const auto &flag : flags) {
                ss << " " << std::string_view(flag.as_string());
            }
            ss << "\")\n\n";
        }

        if (cmake.contains("link-flags")) {
            ss << "set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS}\"";
            const auto flags = toml::find(cmake, "link-flags").as_array();
            for (const auto &flag : flags) {
                ss << " " << std::string_view(flag.as_string());
            }
            ss << "\")\n\n";
        }

        if (cmake.contains("subdirs")) {
            const auto dirs = toml::find(cmake, "subdirs").as_array();
            for (const auto &dir : dirs) {
                ss << "add_subdirectory(" << dir << ")\n";
                subdirs.push_back(dir.as_string());
            }
            ss << "\n\n";
        }
    }

    if (toml.contains("project")) {
        const auto &project = toml::find(toml, "project");
        const std::string proj_name = toml::find(project, "name").as_string();
        const std::string proj_version = toml::find(project, "version").as_string();

        ss << "project(" << proj_name << " VERSION " << proj_version << ")\n\n";
    }

    if (toml.contains("find-package")) {
        using pkg_map = std::map<std::string, std::string>;
        pkg_map deps =
            toml::find<pkg_map>(toml, "find-package");
        for (const auto &dep : deps) {
            ss << "find_package(" << dep.first;
            if (dep.second != "*") {
                ss << " " << dep.second << " CONFIG REQUIRED)\n";
            } else {
                ss << " CONFIG REQUIRED)\n";
            }
        }
    }

    if (toml.contains("fetch-content")) {
        using content_map = std::map<std::string, std::map<std::string, std::string>>;
        content_map deps = toml::find<content_map>(toml, "fetch-content");
        ss << "include(FetchContent)\n\n";
        for (const auto &dep : deps) {
            ss << "FetchContent_Declare(\n\t" << dep.first << "\n";
            for (const auto &arg : dep.second) {
                ss << "\t" << arg.first << " " << arg.second << "\n";
            }
            ss << "\t)\n\n"
               << "FetchContent_MakeAvailable(" << dep.first << ")\n\n";
        }
    }

    if (toml.contains("bin")) {
        const auto &bins = toml::find(toml, "bin").as_array();

        for (const auto &bin : bins) {
            const std::string bin_name = toml::find(bin, "name").as_string();
            const std::string type = toml::find(bin, "type").as_string();
            std::string bin_type;
            std::string add_command;
            if (type == "exe") {
                bin_type = "";
                add_command = "add_executable";
            } else if (type == "shared" || type == "static") {
                bin_type = detail::to_upper(type);
                add_command = "add_library";
            } else {
                throw std::runtime_error(
                    "Unknown binary type! Supported types are exe, shared and static");
            }

            const auto srcs = toml::find(bin, "sources").as_array();
            ss << "set(" << detail::to_upper(bin_name) << "_SOURCES\n";
            for (const auto &src : srcs) {
                ss << "\t" << src << "\n";
            }
            ss << "\t)\n\n"
               << add_command << "(" << bin_name << " " << bin_type << " ${"
               << detail::to_upper(bin_name) << "_SOURCES})\n\n";

            if (bin.contains("include-dirs")) {
                const auto includes = toml::find(bin, "include-dirs").as_array();
                ss << "target_include_directories(" << bin_name << " PUBLIC\n\t";
                for (const auto &inc : includes) {
                    ss << inc << "\n\t";
                }
                ss << ")\n\n";
            }

            if (bin.contains("link-libs")) {
                const auto libraries = toml::find(bin, "link-libs").as_array();
                ss << "target_link_libraries(" << bin_name << " PUBLIC\n\t";
                for (const auto &l : libraries) {
                    ss << l << "\n\t";
                }
                ss << ")\n\n";
            }

            if (bin.contains("features")) {
                const auto feats = toml::find(bin, "features").as_array();
                ss << "target_compile_features(" << bin_name << " PUBLIC\n\t";
                for (const auto &feat : feats) {
                    ss << feat << "\n\t";
                }
                ss << ")\n\n";
            }

            if (bin.contains("defines")) {
                const auto defs = toml::find(bin, "defines").as_array();
                ss << "target_add_definitions(" << bin_name << " PUBLIC\n\t";
                for (const auto &def : defs) {
                    ss << def << "\n\t";
                }
                ss << ")\n\n";
            }
        }
    }

    ss << "\n\n";
    // printf("%s\n", ss.str().c_str());
    std::ofstream ofs(fs::path(path) / "CMakeLists.txt");
    if (ofs.is_open()) {
        ofs << ss.rdbuf();
    }
    ofs.flush();
    ofs.close();
    for (const auto &sub : subdirs) {
        generate_cmake(sub.c_str());
    }
    return 0;
}
} // namespace cmkr::gen

int cmkr_gen_generate_project(const char *typ) {
    try {
        return cmkr::gen::generate_project(typ);
    } catch (const std::system_error &e) {
        return e.code().value();
    } catch (...) {
        return cmkr::error::Status(cmkr::error::Status::Code::InitError);
    }
}

int cmkr_gen_generate_cmake(const char *path) {
    try {
        return cmkr::gen::generate_cmake(path);
    } catch (const std::system_error &e) {
        return e.code().value();
    } catch (...) {
        return cmkr::error::Status(cmkr::error::Status::Code::GenerationError);
    }
}
