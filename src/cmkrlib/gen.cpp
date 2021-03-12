#include "gen.h"
#include "cmake.hpp"
#include "error.h"
#include "literals.h"

#include "fs.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <new>
#include <sstream>
#include <stdexcept>
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
        for (const auto &f : fs::directory_iterator(p.parent_path(), fs::directory_options::follow_directory_symlink)) {
            if (!f.is_directory() && f.path().extension() == ext) {
                temp.push_back(f.path().string());
            }
        }
    } else if (stem == "**") {
        for (const auto &f : fs::recursive_directory_iterator(p.parent_path(), fs::directory_options::follow_directory_symlink)) {
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
        throw std::runtime_error("Unknown project type " + std::string(str) +
                                 "! Supported types are: executable, library, shared, static, interface");
    }

    const auto tomlbuf = detail::format(cmake_toml, dir_name.c_str(), dir_name.c_str(), str, installed.c_str(), target.c_str(), dest.c_str());

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

struct CommandEndl {
    std::stringstream &ss;
    CommandEndl(std::stringstream &ss) : ss(ss) {}
    void endl() { ss << '\n'; }
};

// Credit: JustMagic
struct Command {
    std::stringstream &ss;
    int depth = 0;
    std::string command;
    bool first_arg = true;
    bool generated = false;

    Command(std::stringstream &ss, int depth, const std::string &command) : ss(ss), depth(depth), command(command) {}

    ~Command() {
        if (!generated) {
            assert(false && "Incorrect usage of cmd()");
        }
    }

    const char *indent(int n) {
        for (int i = 0; i < n; i++) {
            ss << '\t';
        }
        return "";
    }

    template <class T>
    bool print_arg(const std::vector<T> &vec) {
        if (vec.empty()) {
            return true;
        }

        ss << '\n';
        for (const auto &value : vec) {
            ss << indent(depth + 1) << value << '\n';
        }
        return true;
    }

    template <class Key, class Value>
    bool print_arg(const std::map<Key, Value> &map) {
        if (map.empty()) {
            return true;
        }

        ss << '\n';
        for (const auto &itr : map) {
            ss << indent(depth + 1) << itr.first << ' ' << itr.second << '\n';
        }
        return true;
    }

    bool print_arg(const std::string &value) {
        if (value.empty()) {
            return true;
        }

        if (first_arg) {
            first_arg = false;
        } else {
            ss << ' ';
        }
        ss << value;
        return true;
    }

    template <class T>
    bool print_arg(const T &value) {
        if (first_arg) {
            first_arg = false;
        } else {
            ss << ' ';
        }
        ss << value;
        return true;
    }

    template <class... Ts>
    CommandEndl operator()(Ts &&... values) {
        generated = true;
        ss << indent(depth) << command << '(';
        std::initializer_list<bool>{print_arg(values)...};
        ss << ")\n";
        return CommandEndl(ss);
    }
};

int generate_cmake(const char *path) {
    if (fs::exists(fs::path(path) / "cmake.toml")) {
        cmake::CMake cmake(path, false);
        std::stringstream ss;

        int indent = 0;
        auto cmd = [&ss, &indent](const std::string &command) {
            if (command == "if") {
                indent++;
                return Command(ss, indent - 1, command);
            } else if (command == "else" || command == "elseif") {
                return Command(ss, indent - 1, command);
            } else if (command == "endif") {
                indent--;
            }
            return Command(ss, indent, command);
        };
        auto comment = [&ss](const char *comment) {
            ss << "# " << comment << '\n';
            return CommandEndl(ss);
        };
        auto endl = [&ss]() { ss << '\n'; };

        comment("This file was generated automatically by cmkr.").endl();

        comment("Regenerate CMakeLists.txt file when necessary");
        cmd("include")("cmkr.cmake", "OPTIONAL", "RESULT_VARIABLE", "CMKR_INCLUDE_RESULT").endl();

        cmd("if")("CMKR_INCLUDE_RESULT");
        cmd("cmkr")();
        cmd("endif")().endl();

        cmd("cmake_minimum_required")("VERSION", cmake.cmake_version).endl();

        cmd("set_property")("GLOBAL", "PROPERTY", "USE_FOLDERS", "ON").endl();

        // TODO: remove support and replace with global compile-features
        if (!cmake.cppflags.empty()) {
            ss << "set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} \"";
            for (const auto &flag : cmake.cppflags) {
                ss << flag << " ";
            }
            ss << "\")\n\n";
        }

        // TODO: remove support and replace with global compile-features
        if (!cmake.cflags.empty()) {
            ss << "set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} \"";
            for (const auto &flag : cmake.cflags) {
                ss << flag << " ";
            }
            ss << "\")\n\n";
        }

        // TODO: remove support and replace with global linker-flags
        if (!cmake.linkflags.empty()) {
            ss << "set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} \"";
            for (const auto &flag : cmake.linkflags) {
                ss << flag << " ";
            }
            ss << "\")\n\n";
        }

        // TODO: support cmake.toml subdirectory generation
        if (!cmake.subdirs.empty()) {
            for (const auto &dir : cmake.subdirs) {
                cmd("add_subdirectory")(dir);
            }
            ss << '\n';
        }

        if (!cmake.proj_name.empty() && !cmake.proj_version.empty()) {
            auto name = cmake.proj_name;
            auto version = cmake.proj_version;
            cmd("set")(name + "_PROJECT_VERSION", version);
            cmd("project")(name, "VERSION", "${" + name + "_PROJECT_VERSION}").endl();
        }

        if (!cmake.packages.empty()) {
            for (const auto &dep : cmake.packages) {
                ss << "find_package(" << dep.name << ' ';
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
                ss << "option(" << opt.name << " \"" << opt.comment << "\" " << (opt.val ? "ON" : "OFF") << ")\n\n";
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
                std::string target_scope;
                if (target.type == "executable") {
                    add_command = "add_executable";
                    target_type = "";
                    target_scope = "PRIVATE";
                } else if (target.type == "shared" || target.type == "static" || target.type == "interface") {
                    add_command = "add_library";
                    target_type = detail::to_upper(target.type);
                    target_scope = target_type == "INTERFACE" ? target_type : "PUBLIC";
                } else if (target.type == "library") {
                    add_command = "add_library";
                    target_type = "";
                    target_scope = "PUBLIC";
                } else {
                    throw std::runtime_error("Unknown binary type " + target.type +
                                             "! Supported types are: executable, library, shared, static, interface");
                }

                if (!target.sources.empty()) {
                    std::vector<std::string> sources;
                    for (const auto &src : target.sources) {
                        auto path = fs::path(src);
                        auto expanded = detail::expand_cmake_path(path);
                        for (const auto &f : expanded) {
                            sources.push_back(f);
                        }
                    }
                    if (sources.empty()) {
                        throw std::runtime_error(target.name + " sources wildcard found 0 files");
                    }
                    if (target.type != "interface") {
                        sources.push_back("cmake.toml");
                    }
                    cmd("set")(target.name + "_SOURCES", sources).endl();
                }

                cmd(add_command)(target.name, target_type, "${" + target.name + "_SOURCES}").endl();

                if (!target.sources.empty()) {
                    cmd("source_group")("TREE", "${PROJECT_SOURCE_DIR}", "FILES", "${" + target.name + "_SOURCES}").endl();
                }

                if (!target.alias.empty()) {
                    cmd("add_library")(target.alias, "ALIAS", target.name);
                }

                if (!target.include_directories.empty()) {
                    cmd("target_include_directories")(target.name, target_scope, target.include_directories).endl();
                }

                if (!target.link_libraries.empty()) {
                    cmd("target_link_libraries")(target.name, target_scope, target.link_libraries).endl();
                }

                if (!target.compile_features.empty()) {
                    cmd("target_compile_features")(target.name, target_scope, target.compile_features).endl();
                }

                if (!target.compile_definitions.empty()) {
                    cmd("target_compile_definitions")(target.name, target_scope, target.compile_definitions).endl();
                }

                if (!target.properties.empty()) {
                    cmd("set_target_properties")(target.name, "PROPERTIES", target.properties).endl();
                }
            }
        }

        if (!cmake.tests.empty()) {
            cmd("include")("CTest");
            cmd("enable_testing")().endl();
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
                        throw std::runtime_error("[[install]] files wildcard did not resolve to any files");
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
