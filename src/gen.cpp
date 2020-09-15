#include "gen.h"
#include "cmake.hpp"
#include "error.h"
#include "literals.h"

#include <filesystem>
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <string>

namespace fs = std::filesystem;

namespace cmkr::gen {

namespace detail {

inline std::string to_upper(const std::string &str) {
    std::string temp;
    temp.reserve(str.size());
    for (auto c : str) {
        temp.push_back(::toupper(c));
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
    fs::create_directory("include");
    const auto dir_name = fs::current_path().stem().string();
    std::string mainbuf;
    const auto tomlbuf =
        detail::format(cmake_toml, dir_name.c_str(), dir_name.c_str(), str, dir_name.c_str());
    if (!strcmp(str, "exe")) {
        mainbuf = detail::format(hello_world, "main");
    } else if (!strcmp(str, "static") || !strcmp(str, "shared") || !strcmp(str, "lib")) {
        mainbuf = detail::format(hello_world, "test");
    } else if (!strcmp(str, "interface")) {
        // Nothing special!
    } else {
        throw std::runtime_error(
            "Unknown project type. Types are exe, lib, shared, static, interface!");
    }

    if (strcmp(str, "interface")) {
        std::ofstream ofs("src/main.cpp");
        if (ofs.is_open()) {
            ofs << mainbuf;
        }
        ofs.flush();
        ofs.close();
    }

    std::ofstream ofs2("cmake.toml");
    if (ofs2.is_open()) {
        ofs2 << tomlbuf;
    }
    ofs2.flush();
    ofs2.close();

    return 0;
}

int generate_cmake(const char *path) {
    if (fs::exists(fs::path(path) / "cmake.toml")) {
        cmake::CMake cmake(path, false);
        std::stringstream ss;
        ss << "# This file was generated automatically by cmkr.\n\n";

        if (!cmake.cmake_version.empty()) {
            ss << "cmake_minimum_required(VERSION " << cmake.cmake_version << ")\n\n";

            ss << "set(CMAKE_EXPORT_COMPILE_COMMANDS ON)\n\n";
        }

        if (!cmake.cppflags.empty()) {
            ss << "set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} \"";
            for (const auto &flag : cmake.cppflags) {
                ss << flag << " ";
            }
            ss << "\")\n\n";
        }

        if (!cmake.cflags.empty()) {
            ss << "set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} \"";
            for (const auto &flag : cmake.cflags) {
                ss << flag << " ";
            }
            ss << "\")\n\n";
        }

        if (!cmake.linkflags.empty()) {
            ss << "set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} \"";
            for (const auto &flag : cmake.linkflags) {
                ss << flag << " ";
            }
            ss << "\")\n\n";
        }

        if (!cmake.subdirs.empty()) {
            for (const auto &dir : cmake.subdirs) {
                ss << "add_subdirectory(" << dir << ")\n";
            }
            ss << "\n\n";
        }

        if (!cmake.proj_name.empty() && !cmake.proj_version.empty()) {
            ss << "project(" << cmake.proj_name << " VERSION " << cmake.proj_version << ")\n\n";
        }

        if (!cmake.packages.empty()) {
            for (const auto &dep : cmake.packages) {
                ss << "find_package(" << dep.name << " ";
                if (dep.version != "*") {
                    ss << dep.version << " ";
                }
                if (dep.required) {
                    ss << "REQUIRED ";
                }
                if (!dep.components.empty()) {
                    ss << "COMPONENTS ";
                    for (const auto &comp : dep.components) {
                        ss << comp << " ";
                    }
                }
                ss << ")\n\n";
            }
        }

        if (!cmake.contents.empty()) {
            ss << "include(FetchContent)\n\n";
            for (const auto &dep : cmake.contents) {
                ss << "FetchContent_Declare(\n\t" << dep.first << "\n";
                for (const auto &arg : dep.second) {
                    ss << "\t" << arg.first << " " << arg.second << "\n";
                }
                ss << "\t)\n\n"
                   << "FetchContent_MakeAvailable(" << dep.first << ")\n\n";
            }
        }

        if (!cmake.options.empty()) {
            for (const auto &opt : cmake.options) {
                ss << "option(" << opt.name << " \"" << opt.comment << "\" "
                   << (opt.val ? "ON" : "OFF") << ")\n";
            }
        }

        ss << "\n";

        if (!cmake.binaries.empty()) {
            for (const auto &bin : cmake.binaries) {
                std::string bin_type;
                std::string add_command;
                if (bin.type == "exe") {
                    bin_type = "";
                    add_command = "add_executable";
                } else if (bin.type == "shared" || bin.type == "static" ||
                           bin.type == "interface") {
                    bin_type = detail::to_upper(bin.type);
                    add_command = "add_library";
                } else if (bin.type == "lib") {
                    bin_type = "";
                    add_command = "add_library";
                } else {
                    throw std::runtime_error(
                        "Unknown binary type! Supported types are exe, shared and static");
                }

                if (!bin.sources.empty()) {
                    ss << "set(" << detail::to_upper(bin.name) << "_SOURCES\n";
                    for (const auto &src : bin.sources) {
                        auto path = fs::path(src);
                        if (path.filename().stem().string() == "*") {
                            auto ext = path.extension();
                            for (const auto &f : fs::directory_iterator(path.parent_path())) {
                                if (f.path().extension() == ext) {
                                    ss << "\t" << f.path() << "\n";
                                }
                            }
                        } else {
                            ss << "\t" << path << "\n";
                        }
                    }
                    ss << "\t)\n\n";
                }

                ss << add_command << "(" << bin.name << " " << bin_type;

                if (!bin.sources.empty()) {
                    ss << " ${" << detail::to_upper(bin.name) << "_SOURCES})\n\n";
                } else {
                    ss << ")\n\n";
                }

                if (!bin.alias.empty()) {
                    ss << "add_library(" << bin.alias << " ALIAS " << bin.name << ")\n\n";
                }

                if (!bin.include_dirs.empty()) {
                    ss << "target_include_directories(" << bin.name << " PUBLIC\n\t";
                    for (const auto &inc : bin.include_dirs) {
                        ss << fs::path(inc) << "\n\t";
                    }
                    ss << ")\n\n";
                }

                if (!bin.link_libs.empty()) {
                    ss << "target_link_libraries(" << bin.name << " PUBLIC\n\t";
                    for (const auto &l : bin.link_libs) {
                        ss << l << "\n\t";
                    }
                    ss << ")\n\n";
                }

                if (!bin.features.empty()) {
                    ss << "target_compile_features(" << bin.name << " PUBLIC\n\t";
                    for (const auto &feat : bin.features) {
                        ss << feat << "\n\t";
                    }
                    ss << ")\n\n";
                }

                if (!bin.defines.empty()) {
                    ss << "target_add_definitions(" << bin.name << " PUBLIC\n\t";
                    for (const auto &def : bin.defines) {
                        ss << def << "\n\t";
                    }
                    ss << ")\n\n";
                }
            }
        }

        if (!cmake.tests.empty()) {
            ss << "include(CTest)\n"
               << "enable_testing()\n\n";
            for (const auto &test : cmake.tests) {
                ss << "add_test(NAME " << test.name << " COMMAND " << test.cmd;
                if (!test.args.empty()) {
                    for (const auto &arg : test.args) {
                        ss << " " << arg;
                    }
                }
                ss << ")\n\n";
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

        for (const auto &sub : cmake.subdirs) {
            if (fs::exists(fs::path(sub) / "cmake.toml"))
                generate_cmake(sub.c_str());
        }
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
