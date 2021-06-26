#include "cmake_generator.hpp"
#include "error.hpp"
#include "literals.hpp"
#include "project_parser.hpp"

#include "fs.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <new>
#include <nlohmann/json.hpp>
#include <regex>
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
    // Sort paths alphabetically for consistent cross-OS generation
    std::sort(temp.begin(), temp.end());
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

struct RawArg {
    RawArg() = default;
    RawArg(std::string arg) : arg(std::move(arg)) {}

    std::string arg;
};

// Credit: JustMagic
struct Command {
    std::stringstream &ss;
    int depth = 0;
    std::string command;
    bool first_arg = true;
    bool had_newline = false;
    bool generated = false;
    std::string post_comment;

    Command(std::stringstream &ss, int depth, std::string command, std::string post_comment)
        : ss(ss), depth(depth), command(std::move(command)), post_comment(std::move(post_comment)) {}

    ~Command() {
        if (!generated) {
            assert(false && "Incorrect usage of cmd(), you probably forgot ()");
        }
    }

    std::string quote(const std::string &str) {
        // Don't quote arguments that don't need quoting
        if (str.find(' ') == std::string::npos && str.find('\"') == std::string::npos && str.find('/') == std::string::npos &&
            str.find(';') == std::string::npos) {
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

    static std::string indent(int n) {
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
        if (kv.second.empty()) {
            return true;
        }

        had_newline = true;
        print_arg(kv.first);
        depth++;
        print_arg(kv.second);
        depth--;

        return true;
    }

    bool print_arg(const RawArg &arg) {
        if (arg.arg.empty()) {
            return true;
        }

        if (had_newline) {
            first_arg = false;
            ss << '\n' << indent(depth + 1);
        } else if (first_arg) {
            first_arg = false;
        } else {
            ss << ' ';
        }

        ss << arg.arg;
        return true;
    }

    template <class T>
    bool print_arg(const T &value) {
        std::stringstream tmp;
        tmp << value;
        auto str = tmp.str();
        if (str.empty()) {
            return true;
        }

        if (had_newline) {
            first_arg = false;
            ss << '\n' << indent(depth + 1);
        } else if (first_arg) {
            first_arg = false;
        } else {
            ss << ' ';
        }

        ss << quote(str);
        return true;
    }

    template <class... Ts>
    CommandEndl operator()(Ts &&...values) {
        generated = true;
        ss << indent(depth) << command << '(';
        (void)std::initializer_list<bool>{print_arg(values)...};
        if (had_newline)
            ss << '\n' << indent(depth);
        ss << ")";
        if (!post_comment.empty()) {
            ss << " # " << post_comment;
        }
        ss << "\n";
        return CommandEndl(ss);
    }
};

static std::string tolf(const std::string &str) {
    std::string result;
    for (char ch : str) {
        if (ch != '\r') {
            result += ch;
        }
    }
    return result;
};

struct Generator {
    Generator(parser::Project &project) : project(project) {}
    Generator(const Generator &) = delete;

    parser::Project &project;
    std::stringstream ss;
    int indent = 0;

    Command cmd(const std::string &command, const std::string &post_comment = "") {
        if (command.empty())
            throw std::invalid_argument("command cannot be empty");
        if (command == "if") {
            indent++;
            return Command(ss, indent - 1, command, post_comment);
        } else if (command == "else" || command == "elseif") {
            return Command(ss, indent - 1, command, post_comment);
        } else if (command == "endif") {
            indent--;
        }
        return Command(ss, indent, command, post_comment);
    }

    CommandEndl comment(const std::string &comment) {
        ss << Command::indent(indent) << "# " << comment << '\n';
        return CommandEndl(ss);
    }

    void endl() { ss << '\n'; }

    void inject_includes(const std::vector<std::string> &includes) {
        if (!includes.empty()) {
            for (const auto &file : includes) {
                if (!fs::is_regular_file(file)) {
                    throw std::runtime_error("Include '" + file + "' does not exist");
                }
                cmd("include")(file);
            }
        }
    }

    void inject_cmake(const std::string &cmake) {
        if (!cmake.empty()) {
            if (cmake.back() == '\"') {
                throw std::runtime_error("Detected additional \" at the end of cmake block");
            }
            auto cmake_lf = tolf(cmake);
            while (cmake_lf.back() == '\n')
                cmake_lf.pop_back();
            bool did_indent = false;
            for (char ch : cmake_lf) {
                if (!did_indent) {
                    ss << Command::indent(indent);
                    did_indent = true;
                } else if (ch == '\n') {
                    did_indent = false;
                }
                ss << ch;
            }
            ss << '\n';
        }
    }

    template <typename T, typename Lambda>
    void handle_condition(const parser::Condition<T> &value, const Lambda &fn) {
        if (!value.empty()) {
            for (const auto &itr : value) {
                const auto &condition = itr.first;
                if (!condition.empty()) {
                    if (project.conditions.count(condition) == 0) {
                        // TODO: somehow print line number information here?
                        throw std::runtime_error("Unknown condition '" + condition + "'");
                    }
                    cmd("if", condition)(RawArg(project.conditions[condition]));
                }

                if (!itr.second.empty()) {
                    fn(condition, itr.second);
                }

                if (!condition.empty()) {
                    cmd("endif")();
                }
                endl();
            }
        }
    }
};

static bool vcpkg_valid_identifier(const std::string &name) {
    const std::regex reserved("prn|aux|nul|con|lpt[1-9]|com[1-9]|core|default");
    const std::regex ok("[a-z0-9]+(-[a-z0-9]+)*");
    std::cmatch m;
    return !std::regex_match(name.c_str(), m, reserved) && std::regex_match(name.c_str(), m, ok);
}

static std::string vcpkg_escape_identifier(const std::string &name) {
    // Do a reasonable effort to escape the project name for use with vcpkg
    std::string escaped;
    for (char ch : name) {
        if ((ch & 0x80) != 0) {
            throw std::runtime_error("Non-ASCII characters are not allowed in [project].name when using [vcpkg]");
        }

        if (ch == '_' || ch == ' ') {
            ch = '-';
        }

        escaped += std::tolower(ch);
    }

    if (!vcpkg_valid_identifier(escaped)) {
        throw std::runtime_error("The escaped project name '" + escaped + "' is not usable with [vcpkg]");
    }
    return escaped;
}

int generate_cmake(const char *path, bool root) {
    if (!fs::exists(fs::path(path) / "cmake.toml")) {
        throw std::runtime_error("No cmake.toml found!");
    }

    parser::Project project(path, false);
    Generator gen(project);

    // Helper lambdas for more convenient CMake generation
    auto &ss = gen.ss;
    auto cmd = [&gen](const std::string &command) { return gen.cmd(command); };
    auto comment = [&gen](const std::string &comment) { return gen.comment(comment); };
    auto endl = [&gen]() { gen.endl(); };
    auto inject_includes = [&gen](const std::vector<std::string> &includes) { gen.inject_includes(includes); };
    auto inject_cmake = [&gen](const std::string &cmake) { gen.inject_cmake(cmake); };

    std::string cmkr_url = "https://github.com/build-cpp/cmkr";
    comment("This file is automatically generated from cmake.toml - DO NOT EDIT");
    comment("See " + cmkr_url + " for more information");
    endl();

    if (root) {
        cmd("cmake_minimum_required")("VERSION", project.cmake_version).endl();

        comment("Regenerate CMakeLists.txt automatically in the root project");
        cmd("set")("CMKR_ROOT_PROJECT", "OFF");
        // clang-format off
        cmd("if")("CMAKE_CURRENT_SOURCE_DIR", "STREQUAL", "CMAKE_SOURCE_DIR");
            cmd("set")("CMKR_ROOT_PROJECT", "ON").endl();

            comment("Bootstrap cmkr");
            cmd("include")(project.cmkr_include, "OPTIONAL", "RESULT_VARIABLE", "CMKR_INCLUDE_RESULT");
            cmd("if")("CMKR_INCLUDE_RESULT");
                cmd("cmkr")();
            cmd("endif")().endl();

            comment("Enable folder support");
            cmd("set_property")("GLOBAL", "PROPERTY", "USE_FOLDERS", "ON");
        cmd("endif")().endl();
        // clang-format on
    }

    // clang-format off
    comment("Create a configure-time dependency on cmake.toml to improve IDE support");
    cmd("if")("CMKR_ROOT_PROJECT");
        cmd("configure_file")("cmake.toml", "cmake.toml", "COPYONLY");
    cmd("endif")().endl();
    // clang-format on

    // TODO: remove support and replace with global compile-features
    if (!project.cppflags.empty()) {
        ss << "set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} \"";
        for (const auto &flag : project.cppflags) {
            ss << flag << " ";
        }
        ss << "\")\n\n";
    }

    // TODO: remove support and replace with global compile-features
    if (!project.cflags.empty()) {
        ss << "set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} \"";
        for (const auto &flag : project.cflags) {
            ss << flag << " ";
        }
        ss << "\")\n\n";
    }

    // TODO: remove support and replace with global linker-flags
    if (!project.linkflags.empty()) {
        ss << "set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} \"";
        for (const auto &flag : project.linkflags) {
            ss << flag << " ";
        }
        ss << "\")\n\n";
    }

    gen.handle_condition(project.include_before, [&](const std::string &, const std::vector<std::string> &includes) { inject_includes(includes); });
    gen.handle_condition(project.cmake_before, [&](const std::string &, const std::string &cmake) { inject_cmake(cmake); });

    if (!project.project_name.empty()) {
        auto languages = std::make_pair("LANGUAGES", project.project_languages);
        auto version = std::make_pair("VERSION", project.project_version);
        auto description = std::make_pair("DESCRIPTION", project.project_description);
        cmd("project")(project.project_name, languages, version, description).endl();
    }

    gen.handle_condition(project.include_after, [&](const std::string &, const std::vector<std::string> &includes) { inject_includes(includes); });
    gen.handle_condition(project.cmake_after, [&](const std::string &, const std::string &cmake) { inject_cmake(cmake); });

    if (!project.contents.empty()) {
        cmd("include")("FetchContent").endl();
        for (const auto &dep : project.contents) {
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

    if (!project.vcpkg.packages.empty()) {
        // Allow the user to specify a url or derive it from the version
        auto url = project.vcpkg.url;
        if (url.empty()) {
            if (project.vcpkg.version.empty()) {
                throw std::runtime_error("You need either [vcpkg].version or [vcpkg].url");
            }
            url = "https://github.com/microsoft/vcpkg/archive/refs/tags/" + project.vcpkg.version + ".tar.gz";
        }

        // Show a nicer error than vcpkg when specifying an invalid package name
        for (const auto &package : project.vcpkg.packages) {
            if (!vcpkg_valid_identifier(package)) {
                throw std::runtime_error("Invalid [vcpkg].packages name '" + package + "' (needs to be lowercase alphanumeric)");
            }
        }

        // CMake to bootstrap vcpkg and download the packages
        // clang-format off
        cmd("if")("CMKR_ROOT_PROJECT", "AND", "NOT", "CMKR_DISABLE_VCPKG");
            cmd("include")("FetchContent");
            cmd("message")("STATUS", "Fetching vcpkg...");
            cmd("FetchContent_Declare")("vcpkg", "URL", url);
            cmd("FetchContent_MakeAvailable")("vcpkg");
            cmd("include")("${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake");
        cmd("endif")();
        endl();
        // clang-format on

        // Generate vcpkg.json
        using namespace nlohmann;
        json j;
        j["$cmkr"] = "This file is automatically generated from cmake.toml - DO NOT EDIT";
        j["$cmkr-url"] = cmkr_url;
        j["$schema"] = "https://raw.githubusercontent.com/microsoft/vcpkg/master/scripts/vcpkg.schema.json";
        j["name"] = vcpkg_escape_identifier(project.project_name);
        j["version-string"] = project.project_version;
        j["dependencies"] = project.vcpkg.packages;
        if (!project.project_description.empty()) {
            j["description"] = project.project_description;
        }

        std::ofstream ofs("vcpkg.json");
        if (!ofs) {
            throw std::runtime_error("Failed to create a vcpkg.json manifest file!");
        }
        ofs << std::setw(2) << j << std::endl;
    }

    if (!project.packages.empty()) {
        comment("Packages");
        for (const auto &dep : project.packages) {
            auto version = dep.version;
            if (version == "*")
                version.clear();
            auto required = dep.required ? "REQUIRED" : "";
            auto config = dep.config ? "CONFIG" : "";
            auto components = std::make_pair("COMPONENTS", dep.components);
            cmd("find_package")(dep.name, version, required, config, components).endl();
        }
    }

    if (!project.options.empty()) {
        comment("Options");
        for (const auto &opt : project.options) {
            cmd("option")(opt.name, opt.comment, opt.val ? "ON" : "OFF");
        }
        endl();
    }

    if (!project.settings.empty()) {
        comment("Settings");
        for (const auto &set : project.settings) {
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

    auto add_subdir = [&](const std::string &dir) {
        // clang-format off
        comment(dir);
        cmd("set")("CMKR_CMAKE_FOLDER", "${CMAKE_FOLDER}");
        cmd("if")("CMAKE_FOLDER");
            cmd("set")("CMAKE_FOLDER", "${CMAKE_FOLDER}/" + dir);
        cmd("else")();
            cmd("set")("CMAKE_FOLDER", dir);
        cmd("endif")();
        // clang-format on

        cmd("add_subdirectory")(dir);
        cmd("set")("CMAKE_FOLDER", "${CMKR_CMAKE_FOLDER}").endl();
    };

    // generate_cmake is called on the subdirectories recursively later
    if (!project.project_subdirs.empty()) {
        gen.handle_condition(project.project_subdirs, [&](const std::string &, const std::vector<std::string> &subdirs) {
            for (const auto &dir : subdirs) {
                add_subdir(dir);
            }
        });
        endl();
    }
    for (const auto &subdir : project.subdirs) {
        if (!subdir.condition.empty()) {
            const auto &condition = subdir.condition;
            if (project.conditions.count(condition) == 0) {
                throw std::runtime_error("Unknown condition '" + condition + "' for [subdir." + subdir.name + "]");
            }
            gen.cmd("if", condition)(RawArg(project.conditions[condition]));
        }

        gen.handle_condition(subdir.include_before,
                             [&](const std::string &, const std::vector<std::string> &includes) { inject_includes(includes); });
        gen.handle_condition(subdir.cmake_before, [&](const std::string &, const std::string &cmake) { inject_cmake(cmake); });

        add_subdir(subdir.name);

        gen.handle_condition(subdir.include_after, [&](const std::string &, const std::vector<std::string> &includes) { inject_includes(includes); });
        gen.handle_condition(subdir.cmake_after, [&](const std::string &, const std::string &cmake) { inject_cmake(cmake); });

        if (!subdir.condition.empty()) {
            cmd("endif")();
        }
    }

    if (!project.targets.empty()) {
        for (const auto &target : project.targets) {
            comment("Target " + target.name);

            if (!target.condition.empty()) {
                const auto &condition = target.condition;
                if (project.conditions.count(condition) == 0) {
                    throw std::runtime_error("Unknown condition '" + condition + "' for [target." + target.name + "]");
                }
                gen.cmd("if", condition)(RawArg(project.conditions[condition]));
            }

            cmd("set")("CMKR_TARGET", target.name);

            gen.handle_condition(target.include_before,
                                 [&](const std::string &, const std::vector<std::string> &includes) { inject_includes(includes); });
            gen.handle_condition(target.cmake_before, [&](const std::string &, const std::string &cmake) { inject_cmake(cmake); });

            auto sources_var = target.name + "_SOURCES";

            bool added_toml = false;
            cmd("set")(sources_var, RawArg("\"\"")).endl();
            gen.handle_condition(target.sources, [&](const std::string &condition, const std::vector<std::string> &condition_sources) {
                auto sources = expand_cmake_paths(condition_sources, path);
                if (sources.empty()) {
                    auto source_key = condition.empty() ? "sources" : (condition + ".sources");
                    throw std::runtime_error(target.name + " " + source_key + " wildcard found 0 files");
                }
                // Do not add cmake.toml twice
                if (!added_toml && std::find(sources.begin(), sources.end(), "cmake.toml") != sources.end()) {
                    added_toml = true;
                }
                cmd("list")("APPEND", sources_var, sources);
            });

            if (!added_toml && target.type != parser::target_interface) {
                cmd("list")("APPEND", sources_var, std::vector<std::string>{"cmake.toml"}).endl();
            }
            cmd("set")("CMKR_SOURCES", "${" + sources_var + "}");

            std::string add_command;
            std::string target_type;
            std::string target_scope;
            switch (target.type) {
            case parser::target_executable:
                add_command = "add_executable";
                target_type = "";
                target_scope = "PRIVATE";
                break;
            case parser::target_library:
                add_command = "add_library";
                target_type = "";
                target_scope = "PUBLIC";
                break;
            case parser::target_shared:
                add_command = "add_library";
                target_type = "SHARED";
                target_scope = "PUBLIC";
                break;
            case parser::target_static:
                add_command = "add_library";
                target_type = "STATIC";
                target_scope = "PUBLIC";
                break;
            case parser::target_interface:
                add_command = "add_library";
                target_type = "INTERFACE";
                target_scope = "INTERFACE";
                break;
            case parser::target_custom:
                // TODO: add proper support, this is hacky
                add_command = "add_custom_target";
                target_type = "SOURCES";
                target_scope = "PUBLIC";
                break;
            default:
                assert("Unimplemented enum value" && false);
            }

            cmd(add_command)(target.name, target_type).endl();

            // clang-format off
            cmd("if")(sources_var);
                cmd("target_sources")(target.name, target.type == parser::target_interface ? "INTERFACE" : "PRIVATE", "${" + sources_var + "}");
            cmd("endif")().endl();
            // clang-format on

            // The first executable target will become the Visual Studio startup project
            if (target.type == parser::target_executable) {
                cmd("get_directory_property")("CMKR_VS_STARTUP_PROJECT", "DIRECTORY", "${PROJECT_SOURCE_DIR}", "DEFINITION", "VS_STARTUP_PROJECT");
                // clang-format off
                cmd("if")("NOT", "CMKR_VS_STARTUP_PROJECT");
                    cmd("set_property")("DIRECTORY", "${PROJECT_SOURCE_DIR}", "PROPERTY", "VS_STARTUP_PROJECT", target.name);
                cmd("endif")().endl();
                // clang-format on
            }

            if (!target.sources.empty()) {
                cmd("source_group")("TREE", "${CMAKE_CURRENT_SOURCE_DIR}", "FILES", "${" + sources_var + "}").endl();
            }

            if (!target.alias.empty()) {
                cmd("add_library")(target.alias, "ALIAS", target.name);
            }

            auto target_cmd = [&](const char *command, const parser::ConditionVector &cargs, const std::string &scope) {
                gen.handle_condition(cargs,
                                     [&](const std::string &, const std::vector<std::string> &args) { cmd(command)(target.name, scope, args); });
            };

            target_cmd("target_compile_definitions", target.compile_definitions, target_scope);
            target_cmd("target_compile_definitions", target.private_compile_definitions, "PRIVATE");

            target_cmd("target_compile_features", target.compile_features, target_scope);
            target_cmd("target_compile_features", target.private_compile_features, "PRIVATE");

            target_cmd("target_compile_options", target.compile_options, target_scope);
            target_cmd("target_compile_options", target.private_compile_options, "PRIVATE");

            target_cmd("target_include_directories", target.include_directories, target_scope);
            target_cmd("target_include_directories", target.private_include_directories, "PRIVATE");

            target_cmd("target_link_directories", target.link_directories, target_scope);
            target_cmd("target_link_directories", target.private_link_directories, "PRIVATE");

            target_cmd("target_link_libraries", target.link_libraries, target_scope);
            target_cmd("target_link_libraries", target.private_link_libraries, "PRIVATE");

            target_cmd("target_link_options", target.link_options, target_scope);
            target_cmd("target_link_options", target.private_link_options, "PRIVATE");

            target_cmd("target_precompile_headers", target.precompile_headers, target_scope);
            target_cmd("target_precompile_headers", target.private_precompile_headers, "PRIVATE");

            if (!target.properties.empty()) {
                gen.handle_condition(target.properties, [&](const std::string &, const tsl::ordered_map<std::string, std::string> &properties) {
                    cmd("set_target_properties")(target.name, "PROPERTIES", properties);
                });
            }

            gen.handle_condition(target.include_after,
                                 [&](const std::string &, const std::vector<std::string> &includes) { inject_includes(includes); });
            gen.handle_condition(target.cmake_after, [&](const std::string &, const std::string &cmake) { inject_cmake(cmake); });

            cmd("unset")("CMKR_TARGET");
            cmd("unset")("CMKR_SOURCES");

            if (!target.condition.empty()) {
                cmd("endif")();
            }

            endl();
        }
    }

    if (!project.tests.empty()) {
        cmd("enable_testing")().endl();
        for (const auto &test : project.tests) {
            auto name = std::make_pair("NAME", test.name);
            auto configurations = std::make_pair("CONFIGURATIONS", test.configurations);
            auto dir = test.working_directory;
            if (fs::is_directory(fs::path(path) / dir)) {
                dir = "${CMAKE_CURRENT_LIST_DIR}/" + dir;
            }
            auto working_directory = std::make_pair("WORKING_DIRECTORY", dir);
            auto command = std::make_pair("COMMAND", test.command);
            auto arguments = std::make_pair("", test.arguments);
            cmd("add_test")(name, configurations, working_directory, command, arguments).endl();
        }
    }

    if (!project.installs.empty()) {
        for (const auto &inst : project.installs) {
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

    auto generate_subdir = [path](const fs::path &sub) {
        // Skip generating for subdirectories that have a cmake.toml with a [project] in it
        fs::path subpath;
        for (const auto &p : sub) {
            subpath /= p;
            if (parser::is_root_path((path / subpath).string())) {
                return;
            }
        }

        subpath = path / sub;
        if (fs::exists(subpath / "cmake.toml")) {
            generate_cmake(subpath.string().c_str(), false);
        }
    };
    for (const auto &itr : project.project_subdirs) {
        for (const auto &sub : itr.second) {
            generate_subdir(sub);
        }
    }
    for (const auto &subdir : project.subdirs) {
        generate_subdir(subdir.name);
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
