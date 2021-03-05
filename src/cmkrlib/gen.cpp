#include "gen.h"
#include "cmake.hpp"
#include "error.h"
#include "literals.h"

#include "fs.hpp"
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <string>

namespace cmkr {
namespace gen {

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

static std::vector<std::string> expand_cmake_path(const fs::path &p) {
    std::vector<std::string> temp;
    auto stem = p.filename().stem().string();
    auto ext = p.extension();
    if (stem == "*") {
        for (const auto &f : fs::directory_iterator(
                 p.parent_path(), fs::directory_options::follow_directory_symlink)) {
            if (!f.is_directory() && f.path().extension() == ext) {
                temp.push_back(f.path().string());
            }
        }
    } else if (stem == "**") {
        for (const auto &f : fs::recursive_directory_iterator(
                 p.parent_path(), fs::directory_options::follow_directory_symlink)) {
            if (!f.is_directory() && f.path().extension() == ext) {
                temp.push_back(f.path().string());
            }
        }
    } else {
        temp.push_back(p.string());
    }
    // Normalize all paths to work with CMake (it needs a / on Windows as well)
    for (auto &path : temp) {
        std::replace(path.begin(), path.end(), '\\', '/');
    }
    return temp;
}

} // namespace detail

int generate_project(const char *str) {
    fs::create_directory("src");
    fs::create_directory("include");
    const auto dir_name = fs::current_path().stem().string();
    std::string mainbuf;
    std::string installed;
    std::string target;
    std::string dest;
    if (!strcmp(str, "executable")) {
        mainbuf = detail::format(hello_world, "main");
        installed = "targets";
        target = dir_name;
        dest = "bin";
    } else if (!strcmp(str, "static") || !strcmp(str, "shared") || !strcmp(str, "library")) {
        mainbuf = detail::format(hello_world, "test");
        installed = "targets";
        target = dir_name;
        dest = "lib";
    } else if (!strcmp(str, "interface")) {
        installed = "files";
        target = "include/*.h";
        dest = "include/" + dir_name;
    } else {
        throw std::runtime_error(
            "Unknown project type " + std::string(str) +
            "! Supported types are: executable, library, shared, static, interface");
    }

    const auto tomlbuf = detail::format(cmake_toml, dir_name.c_str(), dir_name.c_str(), str,
                                        installed.c_str(), target.c_str(), dest.c_str());

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
        ss << "# This file was generated automatically by cmkr.\n";
        ss << "\n";

        ss << "# Regenerate CMakeLists.txt file when necessary\n";
        ss << "include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)\n\n";
        ss << "if(CMKR_INCLUDE_RESULT)\n";
        ss << "\tcmkr()\n";
        ss << "endif()\n";
        ss << "\n";

        ss << "cmake_minimum_required(VERSION " << cmake.cmake_version << ")\n";
        ss << "\n";

        ss << "set_property(GLOBAL PROPERTY USE_FOLDERS ON)\n";
        ss << "\n";

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
            ss << "set(" << cmake.proj_name << "_PROJECT_VERSION " << cmake.proj_version << ")\n"
               << "project(" << cmake.proj_name << " VERSION "
               << "${" << cmake.proj_name << "_PROJECT_VERSION}"
               << ")\n\n";
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
                ss << "message(STATUS \"Fetching " << dep.first << "...\")\n";
                ss << "FetchContent_Declare(\n\t" << dep.first << "\n";
                for (const auto &arg : dep.second) {
                    std::string first_arg = arg.first;
                    if (first_arg == "git") {
                        first_arg = "GIT_REPOSITORY";
                    } else if (first_arg == "tag") {
                        first_arg = "GIT_TAG";
                    } else if (first_arg == "svn") {
                        first_arg = "SVN_REPOSITORY";
                    } else if (first_arg == "rev") {
                        first_arg = "SVN_REVISION";
                    } else if (first_arg == "url") {
                        first_arg = "URL";
                    } else if (first_arg == "hash") {
                        first_arg = "URL_HASH";
                    } else {
                        // don't change arg
                    }
                    ss << "\t" << first_arg << " " << arg.second << "\n";
                }
                ss << ")\n"
                   << "FetchContent_MakeAvailable(" << dep.first << ")\n\n";
            }
        }

        if (!cmake.options.empty()) {
            for (const auto &opt : cmake.options) {
                ss << "option(" << opt.name << " \"" << opt.comment << "\" "
                   << (opt.val ? "ON" : "OFF") << ")\n\n";
            }
        }

        if (!cmake.settings.empty()) {
            for (const auto &set : cmake.settings) {
                std::string set_val;
                if (set.val.index() == 1) {
                    set_val = mpark::get<1>(set.val);
                } else {
                    set_val = mpark::get<0>(set.val) ? "ON" : "OFF";
                }
                ss << "set(" << set.name << " " << set_val;
                ;
                if (set.cache) {
                    std::string typ;
                    if (set.val.index() == 1)
                        typ = "STRING";
                    else
                        typ = "BOOL";
                    ss << " CACHE " << typ << " \"" << set.comment << "\"";
                    if (set.force)
                        ss << " FORCE";
                }
                ss << ")\n\n";
            }
        }

        if (!cmake.targets.empty()) {
            for (const auto &target : cmake.targets) {
                std::string add_command;
                std::string target_type;
                if (target.type == "executable") {
                    add_command = "add_executable";
                    target_type = "";
                } else if (target.type == "shared" || target.type == "static" ||
                           target.type == "interface") {
                    add_command = "add_library";
                    target_type = detail::to_upper(target.type);
                } else if (target.type == "library") {
                    add_command = "add_library";
                    target_type = "";
                } else {
                    throw std::runtime_error(
                        "Unknown binary type " + target.type +
                        "! Supported types are: executable, library, shared, static, interface");
                }

                if (!target.sources.empty()) {
                    ss << "set(" << detail::to_upper(target.name) << "_SOURCES\n";
                    int sources_added = 0;
                    for (const auto &src : target.sources) {
                        auto path = fs::path(src);
                        auto expanded = detail::expand_cmake_path(path);
                        for (const auto &f : expanded) {
                            ss << "\t" << f << "\n";
                            sources_added++;
                        }
                    }
                    if (sources_added == 0) {
                        throw std::runtime_error(target.name + " sources wildcard found 0 files");
                    }
                    if (target.type != "interface") {
                        ss << "\tcmake.toml\n";
                    }
                    ss << ")\n\n";
                }

                ss << add_command << "(" << target.name;
                if (!target_type.empty()) {
                    ss << " " << target_type;
                }

                if (!target.sources.empty()) {
                    ss << " ${" << detail::to_upper(target.name) << "_SOURCES})\n\n";
                    ss << "source_group(TREE ${PROJECT_SOURCE_DIR} FILES ${"
                       << detail::to_upper(target.name) << "_SOURCES})\n\n";
                } else {
                    ss << ")\n\n";
                }

                if (!target.alias.empty()) {
                    ss << "add_library(" << target.alias << " ALIAS " << target.name << ")\n\n";
                }

                if (!target.include_directories.empty()) {
                    ss << "target_include_directories(" << target.name << " PUBLIC\n";
                    for (const auto &inc : target.include_directories) {
                        ss << "\t" << inc << "\n";
                    }
                    ss << ")\n\n";
                }

                if (!target.link_libraries.empty()) {
                    ss << "target_link_libraries(" << target.name << " PUBLIC\n";
                    for (const auto &l : target.link_libraries) {
                        ss << "\t" << l << "\n";
                    }
                    ss << ")\n\n";
                }

                if (!target.compile_features.empty()) {
                    ss << "target_compile_features(" << target.name << " PUBLIC\n";
                    for (const auto &feat : target.compile_features) {
                        ss << "\t" << feat << "\n";
                    }
                    ss << ")\n\n";
                }

                if (!target.compile_definitions.empty()) {
                    ss << "target_compile_definitions(" << target.name << " PUBLIC\n";
                    for (const auto &def : target.compile_definitions) {
                        ss << "\t" << def << "\n";
                    }
                    ss << ")\n\n";
                }

                if (!target.properties.empty()) {
                    ss << "set_target_properties(" << target.name << " PROPERTIES\n";
                    for (const auto &prop : target.properties) {
                        ss << "\t" << prop.first << " " << prop.second << "\n";
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

        if (!cmake.installs.empty()) {
            for (const auto &inst : cmake.installs) {
                ss << "install(\n";
                if (!inst.targets.empty()) {
                    ss << "\tTARGETS ";
                    for (const auto &target : inst.targets) {
                        ss << target << " ";
                    }
                }
                if (!inst.dirs.empty()) {
                    ss << "\tDIRS ";
                    for (const auto &dir : inst.dirs) {
                        ss << dir << " ";
                    }
                }
                if (!inst.files.empty()) {
                    ss << "\tFILES ";
                    int files_added = 0;
                    for (const auto &file : inst.files) {
                        auto path = detail::expand_cmake_path(fs::path(file));
                        for (const auto &f : path) {
                            ss << f << " ";
                            files_added++;
                        }
                    }
                    if (files_added == 0) {
                        throw std::runtime_error(
                            "[[install]] files wildcard did not resolve to any files");
                    }
                }
                if (!inst.configs.empty()) {
                    ss << "\tCONFIGURATIONS";
                    for (const auto &conf : inst.configs) {
                        ss << conf << " ";
                    }
                }
                ss << "\n\tDESTINATION " << inst.destination << "\n\t";
                if (!inst.targets.empty())
                    ss << "COMPONENT " << inst.targets[0] << "\n)\n\n";
                else
                    ss << "\n)\n\n";
            }
        }

        ss << "\n\n";
        // printf("%s\n", ss.str().c_str());

        std::ofstream ofs(fs::path(path) / "CMakeLists.txt", std::ios_base::binary);
        if (ofs.is_open()) {
            ofs << ss.rdbuf();
        }
        ofs.flush();
        ofs.close();

        for (const auto &sub : cmake.subdirs) {
            if (fs::exists(fs::path(sub) / "cmake.toml"))
                generate_cmake(sub.c_str());
        }
    } else {
        throw std::runtime_error("No cmake.toml found!");
    }
    return 0;
}
} // namespace gen
} // namespace cmkr

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
