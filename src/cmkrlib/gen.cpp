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

static std::vector<std::string> expand_cmake_path(const fs::path &name, const fs::path &toml_dir) {
    std::vector<std::string> temp;

    auto extract_suffix = [](const fs::path &base, const fs::path &full) {
        // TODO: delet this
        auto fullpath = full.string();
        auto base_len = base.string().length();
        auto delet = fullpath.substr(base_len + 1, fullpath.length() - base_len);
        return delet;
    };

    auto stem = name.filename().stem().string();
    auto ext = name.extension();

    if (stem == "*") {
        for (const auto &f : fs::directory_iterator(toml_dir / name.parent_path(), fs::directory_options::follow_directory_symlink)) {
            if (!f.is_directory() && f.path().extension() == ext) {
                temp.push_back(extract_suffix(toml_dir, f));
            }
        }
    } else if (stem == "**") {
        for (const auto &f : fs::recursive_directory_iterator(toml_dir / name.parent_path(), fs::directory_options::follow_directory_symlink)) {
            if (!f.is_directory() && f.path().extension() == ext) {
                temp.push_back(extract_suffix(toml_dir, f.path()));
            }
        }
    } else {
        temp.push_back(name.string());
    }
    // Normalize all paths to work with CMake (it needs a / on Windows as well)
    for (auto &path : temp) {
        std::replace(path.begin(), path.end(), '\\', '/');
    }
    return temp;
}

static std::vector<std::string> expand_cmake_paths(const std::vector<std::string> &sources, const fs::path &toml_dir) {
    // TODO: add duplicate checking
    std::vector<std::string> result;
    for (const auto &src : sources) {
        auto expanded = expand_cmake_path(src, toml_dir);
        for (const auto &f : expanded) {
            result.push_back(f);
        }
    }
    return result;
}

int generate_project(const char *str) {
    fs::create_directory("src");
    fs::create_directory("include");
    const auto dir_name = fs::current_path().stem().string();
    std::string mainbuf;
    std::string installed;
    std::string target;
    std::string dest;
    if (!strcmp(str, "executable")) {
        mainbuf = format(hello_world, "main");
        installed = "targets";
        target = dir_name;
        dest = "bin";
    } else if (!strcmp(str, "static") || !strcmp(str, "shared") || !strcmp(str, "library")) {
        mainbuf = format(hello_world, "test");
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

    const auto tomlbuf = format(cmake_toml, dir_name.c_str(), dir_name.c_str(), str, installed.c_str(), target.c_str(), dest.c_str());

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
    bool had_newline = false;
    bool generated = false;

    Command(std::stringstream &ss, int depth, const std::string &command) : ss(ss), depth(depth), command(command) {}

    ~Command() {
        if (!generated) {
            assert(false && "Incorrect usage of cmd()");
        }
    }

    std::string quote(const std::string &str) {
        // Don't quote arguments that don't need quoting
        if (str.find(' ') == std::string::npos && str.find('\"') == std::string::npos && str.find('/') == std::string::npos) {
            return str;
        }
        std::string result;
        result += "\"";
        for (char ch : str) {
            switch (ch) {
            case '\\':
            case '\"':
                result += '\\';
            default:
                result += ch;
                break;
            }
        }
        result += "\"";
        return result;
    }

    std::string indent(int n) {
        std::string result;
        for (int i = 0; i < n; i++) {
            result += '\t';
        }
        return result;
    }

    template <class T>
    bool print_arg(const std::vector<T> &vec) {
        if (vec.empty()) {
            return true;
        }

        had_newline = true;
        for (const auto &value : vec) {
            print_arg(value);
        }

        return true;
    }

    template <class Key, class Value>
    bool print_arg(const tsl::ordered_map<Key, Value> &map) {
        if (map.empty()) {
            return true;
        }

        for (const auto &itr : map) {
            print_arg(itr);
        }

        return true;
    }

    template <class K>
    bool print_arg(const std::pair<K, std::vector<std::string>> &kv) {
        if (kv.second.empty()) {
            return true;
        }

        had_newline = true;
        print_arg(kv.first);
        depth++;
        for (const auto &s : kv.second) {
            print_arg(s);
        }
        depth--;

        return true;
    }

    template <class K, class V>
    bool print_arg(const std::pair<K, V> &kv) {
        std::stringstream tmp;
        tmp << kv.second;
        auto str = tmp.str();

        if (kv.second.empty()) {
            return true;
        }

        had_newline = true;
        print_arg(kv.first);
        depth++;
        print_arg(str);
        depth--;

        return true;
    }

    template <class T>
    bool print_arg(const T &value, bool indentation = true) {
        std::stringstream tmp;
        tmp << value;
        auto str = tmp.str();
        if (str.empty()) {
            return true;
        }

        if (indentation) {
            if (had_newline) {
                first_arg = false;
                ss << '\n' << indent(depth + 1);
            } else if (first_arg) {
                first_arg = false;
            } else {
                ss << ' ';
            }
        }

        ss << quote(str);
        return true;
    }

    template <class... Ts>
    CommandEndl operator()(Ts &&...values) {
        generated = true;
        ss << indent(depth) << command << '(';
        std::initializer_list<bool>{print_arg(values)...};
        if (had_newline)
            ss << '\n';
        ss << ")\n";
        return CommandEndl(ss);
    }
};

int generate_cmake(const char *path, bool root) {
    if (!fs::exists(fs::path(path) / "cmake.toml")) {
        throw std::runtime_error("No cmake.toml found!");
    }

    // Helper lambdas for more convenient CMake generation
    std::stringstream ss;
    int indent = 0;
    auto cmd = [&ss, &indent](const std::string &command) {
        if (command.empty())
            throw std::invalid_argument("command cannot be empty");
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
    auto comment = [&ss](const std::string &comment) {
        ss << "# " << comment << '\n';
        return CommandEndl(ss);
    };
    auto endl = [&ss]() { ss << '\n'; };
    auto tolf = [](const std::string &str) {
        std::string result;
        for (char ch : str) {
            if (ch != '\r') {
                result += ch;
            }
        }
        return result;
    };
    auto inject_includes = [&cmd, &endl](const std::vector<std::string> &includes) {
        if (!includes.empty()) {
            for (const auto &file : includes) {
                // TODO: warn/error if file doesn't exist?
                cmd("include")(file);
            }
            endl();
        }
    };
    auto inject_cmake = [&ss, &tolf](const std::string &cmake) {
        if (!cmake.empty()) {
            if (cmake.back() == '\"') {
                throw std::runtime_error("Detected additional \" at the end of cmake block");
            }
            ss << tolf(cmake) << "\n\n";
        }
    };

    // TODO: add link with proper documentation
    comment("This file was generated automatically by cmkr.").endl();

    comment("Create a configure-time dependency on cmake.toml to improve IDE support");
    cmd("configure_file")("cmake.toml", "cmake.toml", "COPYONLY").endl();

    cmake::CMake cmake(path, false);

    if (root) {
        comment("Regenerate CMakeLists.txt file when necessary");
        cmd("include")(cmake.cmkr_include, "OPTIONAL", "RESULT_VARIABLE", "CMKR_INCLUDE_RESULT").endl();

        // clang-format off
        cmd("if")("CMKR_INCLUDE_RESULT");
            cmd("cmkr")();
        cmd("endif")().endl();
        // clang-format on

        cmd("cmake_minimum_required")("VERSION", cmake.cmake_version).endl();

        cmd("set_property")("GLOBAL", "PROPERTY", "USE_FOLDERS", "ON").endl();
    }

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

    inject_includes(cmake.include_before);
    inject_cmake(cmake.cmake_before);

    if (!cmake.project_name.empty()) {
        auto languages = std::make_pair("LANGUAGES", cmake.project_languages);
        auto version = std::make_pair("VERSION", cmake.project_version);
        auto description = std::make_pair("DESCRIPTION", cmake.project_description);
        cmd("project")(cmake.project_name, languages, version, description).endl();
    }

    inject_includes(cmake.include_after);
    inject_cmake(cmake.cmake_after);

    if (!cmake.packages.empty()) {
        for (const auto &dep : cmake.packages) {
            auto version = dep.version;
            if (version == "*")
                version.clear();
            auto required = dep.required ? "REQUIRED" : "";
            auto components = std::make_pair("COMPONENTS", dep.components);
            cmd("find_package")(dep.name, version, required, components).endl();
        }
    }

    if (!cmake.contents.empty()) {
        cmd("include")("FetchContent").endl();
        for (const auto &dep : cmake.contents) {
            cmd("message")("STATUS", "Fetching " + dep.first + "...");
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
                ss << "\t" << first_arg << "\n\t\t" << arg.second << "\n";
            }
            ss << ")\n";
            cmd("FetchContent_MakeAvailable")(dep.first).endl();
        }
    }

    if (!cmake.options.empty()) {
        for (const auto &opt : cmake.options) {
            cmd("option")(opt.name, opt.comment, opt.val ? "ON" : "OFF");
        }
        endl();
    }

    if (!cmake.settings.empty()) {
        for (const auto &set : cmake.settings) {
            std::string set_val;
            if (set.val.index() == 1) {
                set_val = mpark::get<1>(set.val);
            } else {
                set_val = mpark::get<0>(set.val) ? "ON" : "OFF";
            }

            if (set.cache) {
                auto typ = set.val.index() == 1 ? "STRING" : "BOOL";
                auto force = set.force ? "FORCE" : "";
                cmd("set")(set.name, set_val, typ, set.comment, force);
            } else {
                cmd("set")(set.name, set_val);
            }
        }
        endl();
    }

    // generate_cmake is called on the subdirectories recursively later
    if (!cmake.subdirs.empty()) {
        for (const auto &dir : cmake.subdirs) {
            // clang-format off
            cmd("set")("CMKR_CMAKE_FOLDER", "${CMAKE_FOLDER}");
            cmd("if")("CMAKE_FOLDER");
                cmd("set")("CMAKE_FOLDER", "${CMAKE_FOLDER}/" + dir);
            cmd("else")();
                cmd("set")("CMAKE_FOLDER", dir);
            cmd("endif")();
            // clang-format on
            cmd("add_subdirectory")(dir);
            cmd("set")("CMAKE_FOLDER", "${CMKR_CMAKE_FOLDER}").endl();
        }
        endl();
    }

    if (!cmake.targets.empty()) {
        for (const auto &target : cmake.targets) {
            comment("Target " + target.name);
            inject_includes(target.include_before);
            inject_cmake(target.cmake_before);

            if (!target.sources.empty()) {
                auto sources = expand_cmake_paths(target.sources, path);
                if (sources.empty()) {
                    throw std::runtime_error(target.name + " sources wildcard found 0 files");
                }
                if (target.type != cmake::target_interface) {
                    // Do not add cmake.toml twice
                    if (std::find(sources.begin(), sources.end(), "cmake.toml") == sources.end()) {
                        sources.push_back("cmake.toml");
                    }
                }
                cmd("set")(target.name + "_SOURCES", sources).endl();
            }

            std::string add_command;
            std::string target_type;
            std::string target_scope;
            switch (target.type) {
            case cmake::target_executable:
                add_command = "add_executable";
                target_type = "";
                target_scope = "PRIVATE";
                break;
            case cmake::target_library:
                add_command = "add_library";
                target_type = "";
                target_scope = "PUBLIC";
                break;
            case cmake::target_shared:
                add_command = "add_library";
                target_type = "SHARED";
                target_scope = "PUBLIC";
                break;
            case cmake::target_static:
                add_command = "add_library";
                target_type = "STATIC";
                target_scope = "PUBLIC";
                break;
            case cmake::target_interface:
                add_command = "add_library";
                target_type = "INTERFACE";
                target_scope = "INTERFACE";
                break;
            case cmake::target_custom:
                // TODO: add proper support, this is hacky
                add_command = "add_custom_target";
                target_type = "SOURCES";
                target_scope = "PUBLIC";
                break;
            default:
                assert("Unimplemented enum value" && false);
            }

            cmd(add_command)(target.name, target_type, "${" + target.name + "_SOURCES}").endl();

            // The first executable target will become the Visual Studio startup project
            if (target.type == cmake::target_executable) {
                cmd("get_directory_property")("CMKR_VS_STARTUP_PROJECT", "DIRECTORY", "${PROJECT_SOURCE_DIR}", "DEFINITION", "VS_STARTUP_PROJECT");
                // clang-format off
                cmd("if")("NOT", "CMKR_VS_STARTUP_PROJECT");
                    cmd("set_property")("DIRECTORY", "${PROJECT_SOURCE_DIR}", "PROPERTY", "VS_STARTUP_PROJECT", target.name);
                cmd("endif")().endl();
                // clang-format on
            }

            if (!target.sources.empty()) {
                cmd("source_group")("TREE", "${CMAKE_CURRENT_SOURCE_DIR}", "FILES", "${" + target.name + "_SOURCES}").endl();
            }

            if (!target.alias.empty()) {
                cmd("add_library")(target.alias, "ALIAS", target.name);
            }

            auto target_cmd = [&](const char *command, const std::vector<std::string> &args) {
                if (!args.empty()) {
                    cmd(command)(target.name, target_scope, args).endl();
                }
            };

            target_cmd("target_compile_definitions", target.compile_definitions);
            target_cmd("target_compile_features", target.compile_features);
            target_cmd("target_compile_options", target.compile_options);
            target_cmd("target_include_directories", target.include_directories);
            target_cmd("target_link_directories", target.link_directories);
            target_cmd("target_link_libraries", target.link_libraries);
            target_cmd("target_precompile_headers", target.precompile_headers);

            if (!target.properties.empty()) {
                cmd("set_target_properties")(target.name, "PROPERTIES", target.properties).endl();
            }

            inject_includes(target.include_after);
            inject_cmake(target.cmake_after);
        }
    }

    if (!cmake.tests.empty()) {
        cmd("include")("CTest");
        cmd("enable_testing")().endl();
        for (const auto &test : cmake.tests) {
            auto name = std::make_pair("NAME", test.name);
            auto command = std::make_pair("COMMAND", test.cmd);
            cmd("add_test")(name, command, test.args).endl();
        }
    }

    if (!cmake.installs.empty()) {
        for (const auto &inst : cmake.installs) {
            auto targets = std::make_pair("TARGETS", inst.targets);
            auto dirs = std::make_pair("DIRS", inst.dirs);
            std::vector<std::string> files_data;
            if (!inst.files.empty()) {
                files_data = expand_cmake_paths(inst.files, path);
                if (files_data.empty()) {
                    throw std::runtime_error("[[install]] files wildcard did not resolve to any files");
                }
            }
            auto files = std::make_pair("FILES", inst.files);
            auto configs = std::make_pair("CONFIGURATIONS", inst.configs);
            auto destination = std::make_pair("DESTINATION", inst.destination);
            auto component = std::make_pair("COMPONENT", inst.targets.empty() ? "" : inst.targets.front());
            cmd("install")(targets, dirs, files, configs, destination, component);
        }
    }

    // Generate CMakeLists.txt
    auto list_path = fs::path(path) / "CMakeLists.txt";

    auto should_regenerate = [&list_path, &ss]() {
        if (!fs::exists(list_path))
            return true;

        std::ifstream ifs(list_path, std::ios_base::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to read " + list_path.string());
        }

        std::string data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return data != ss.str();
    }();

    if (should_regenerate) {
        std::ofstream ofs(list_path, std::ios_base::binary);
        if (ofs.is_open()) {
            ofs << ss.str();
        } else {
            throw std::runtime_error("Failed to write " + list_path.string());
        }
    }

    for (const auto &sub : cmake.subdirs) {
        auto subpath = fs::path(path) / fs::path(sub);
        if (fs::exists(subpath / "cmake.toml"))
            generate_cmake(subpath.string().c_str(), false);
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
