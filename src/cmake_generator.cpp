#include "cmake_generator.hpp"
#include "error.hpp"
#include "literals.hpp"
#include <resources/cmkr.hpp>

#include "fs.hpp"
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace cmkr {
namespace gen {

static std::string format(const char *format, tsl::ordered_map<std::string, std::string> variables) {
    std::string s = format;
    for (const auto &itr : variables) {
        size_t start_pos = 0;
        while ((start_pos = s.find(itr.first, start_pos)) != std::string::npos) {
            s.replace(start_pos, itr.first.length(), itr.second);
            start_pos += itr.second.length();
        }
    }
    return s;
}

static std::vector<std::string> expand_cmake_path(const fs::path &name, const fs::path &toml_dir, bool root_project) {
    std::vector<std::string> temp;

    auto extract_suffix = [](const fs::path &base, const fs::path &full) {
        auto fullpath = full.string();
        auto base_len = base.string().length();
        auto delet = fullpath.substr(base_len + 1, fullpath.length() - base_len);
        return delet;
    };

    auto stem = name.filename().stem().string();
    auto ext = name.extension();

    if (root_project && stem == "**" && name == name.filename()) {
        throw std::runtime_error("Recursive globbing not allowed in project root: " + name.string());
    }

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

static std::vector<std::string> expand_cmake_paths(const std::vector<std::string> &sources, const fs::path &toml_dir, bool root_project) {
    // TODO: add duplicate checking
    std::vector<std::string> result;
    for (const auto &src : sources) {
        auto expanded = expand_cmake_path(src, toml_dir, root_project);
        for (const auto &f : expanded) {
            result.push_back(f);
        }
    }
    return result;
}

static void create_file(const fs::path &path, const std::string &contents) {
    if (!path.parent_path().empty()) {
        fs::create_directories(path.parent_path());
    }
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to create " + path.string());
    }
    ofs << contents;
}

void generate_project(const std::string &type) {
    const auto name = fs::current_path().stem().string();
    if (fs::exists(fs::current_path() / "cmake.toml")) {
        throw std::runtime_error("Cannot initialize a project when cmake.toml already exists!");
    }

    tsl::ordered_map<std::string, std::string> variables = {
        {"@name", name},
        {"@type", type},
    };

    if (!fs::is_empty(fs::current_path())) {
        // Make a backup of an existing CMakeLists.txt if it exists
        std::error_code ec;
        fs::rename("CMakeLists.txt", "CMakeLists.txt.bak", ec);
        // Create an empty cmake.toml for migration purporses
        create_file("cmake.toml", format(toml_migration, variables));
        return;
    }

    if (type == "executable") {
        create_file("cmake.toml", format(toml_executable, variables));
        create_file("src/" + name + "/main.cpp", format(cpp_executable, variables));
    } else if (type == "static" || type == "shared" || type == "library") {
        create_file("cmake.toml", format(toml_library, variables));
        create_file("src/" + name + "/" + name + ".cpp", format(cpp_library, variables));
        create_file("include/" + name + "/" + name + ".hpp", format(hpp_library, variables));
    } else if (type == "interface") {
        create_file("cmake.toml", format(toml_interface, variables));
        create_file("include/" + name + "/" + name + ".hpp", format(hpp_interface, variables));
    } else {
        throw std::runtime_error("Unknown project type " + type + "! Supported types are: executable, library, shared, static, interface");
    }
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

    ~Command() noexcept(false) {
        if (!generated) {
            throw std::runtime_error("Incorrect usage of cmd(), you probably forgot ()");
        }
    }

    static std::string quote(const std::string &str) {
        // Quote an empty string
        if (str.empty()) {
            return "\"\"";
        }
        // Don't quote arguments that don't need quoting
        // https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#unquoted-argument
        // NOTE: Normally '/' does not require quoting according to the documentation but this has been the case here
        //       previously, so for backwards compatibility its still here.
        if (str.find_first_of("()#\"\\'> |/") == str.npos)
            return str;
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
    Generator(const parser::Project &project) : project(project) {}
    Generator(const Generator &) = delete;

    const parser::Project &project;
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
                    cmd("if", condition)(RawArg(project.conditions.at(condition)));
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

    void conditional_includes(const parser::ConditionVector &include) {
        handle_condition(include, [this](const std::string &, const std::vector<std::string> &includes) { inject_includes(includes); });
    }

    void conditional_cmake(const parser::Condition<std::string> &cmake) {
        handle_condition(cmake, [this](const std::string &, const std::string &cmake) { inject_cmake(cmake); });
    }
};

struct ConditionScope {
    Generator &gen;
    bool endif = false;

    ConditionScope(Generator &gen, const std::string &condition) : gen(gen) {
        if (!condition.empty()) {
            gen.cmd("if", condition)(RawArg(gen.project.conditions.at(condition)));
            endif = true;
        }
    }

    ConditionScope(const ConditionScope &) = delete;
    ConditionScope(ConditionScope &&) = delete;

    ~ConditionScope() {
        if (endif) {
            gen.cmd("endif")();
        }
    }
};

static bool vcpkg_valid_identifier(const std::string &name) {
    // prn|aux|nul|con|lpt[1-9]|com[1-9]|core|default
    auto is_reserved = [](const std::string &s) {
        if (s == "prn" || s == "aux" || s == "nul" || s == "con" || s == "core" || s == "default") {
            return true;
        }
        if (s.length() == 4 && (s.compare(0, 3, "lpt") == 0 || s.compare(0, 3, "com") == 0) && (s[3] >= '1' && s[3] <= '9')) {
            return true;
        }
        return false;
    };
    // [a-z0-9]+(-[a-z0-9]+)*
    auto is_identifier = [](const std::string &s) {
        for (size_t i = 0; i < s.length(); i++) {
            auto c = s[i];
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || (i > 0 && c == '-')) {
                continue;
            }
            return false;
        }
        return true;
    };
    return is_identifier(name) && !is_reserved(name);
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

void generate_cmake(const char *path, const parser::Project *parent_project) {
    if (!fs::exists(fs::path(path) / "cmake.toml")) {
        throw std::runtime_error("No cmake.toml found!");
    }

    // Root project doesn't have a parent
    auto root_project = parent_project == nullptr;

    parser::Project project(parent_project, path, false);
    Generator gen(project);

    // Helper lambdas for more convenient CMake generation
    auto &ss = gen.ss;
    auto cmd = [&gen](const std::string &command) { return gen.cmd(command); };
    auto comment = [&gen](const std::string &comment) { return gen.comment(comment); };
    auto endl = [&gen]() { gen.endl(); };

    std::string cmkr_url = "https://github.com/build-cpp/cmkr";
    comment("This file is automatically generated from cmake.toml - DO NOT EDIT");
    comment("See " + cmkr_url + " for more information");
    endl();

    if (root_project) {
        cmd("cmake_minimum_required")("VERSION", project.cmake_version).endl();

        if (!project.allow_in_tree) {
            // clang-format off
            cmd("if")("CMAKE_SOURCE_DIR", "STREQUAL", "CMAKE_BINARY_DIR");
                cmd("message")("FATAL_ERROR", "In-tree builds are not supported. Run CMake from a separate directory: cmake -B build");
            cmd("endif")().endl();
            // clang-format on
        }

        comment("Regenerate CMakeLists.txt automatically in the root project");
        cmd("set")("CMKR_ROOT_PROJECT", "OFF");
        // clang-format off
        cmd("if")("CMAKE_CURRENT_SOURCE_DIR", "STREQUAL", "CMAKE_SOURCE_DIR");
            cmd("set")("CMKR_ROOT_PROJECT", "ON").endl();

            if (!project.cmkr_include.empty()) {
                comment("Bootstrap cmkr");
                cmd("include")(project.cmkr_include, "OPTIONAL", "RESULT_VARIABLE", "CMKR_INCLUDE_RESULT");
                cmd("if")("CMKR_INCLUDE_RESULT");
                    cmd("cmkr")();
                cmd("endif")().endl();
            }

            comment("Enable folder support");
            cmd("set_property")("GLOBAL", "PROPERTY", "USE_FOLDERS", "ON");
        cmd("endif")().endl();
        // clang-format on

        fs::path cmkr_include(project.cmkr_include);
        if (!project.cmkr_include.empty() && !fs::exists(cmkr_include) && cmkr_include.is_relative()) {
            create_file(cmkr_include, resources::cmkr);
        }
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

    if (!project.options.empty()) {
        comment("Options");
        for (const auto &opt : project.options) {
            cmd("option")(opt.name, RawArg(Command::quote(opt.comment)), opt.val ? "ON" : "OFF");
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

    gen.conditional_includes(project.include_before);
    gen.conditional_cmake(project.cmake_before);

    if (!project.project_name.empty()) {
        auto languages = std::make_pair("LANGUAGES", project.project_languages);
        auto version = std::make_pair("VERSION", project.project_version);
        auto description = std::make_pair("DESCRIPTION", project.project_description);
        cmd("project")(project.project_name, languages, version, description).endl();
    }

    gen.conditional_includes(project.include_after);
    gen.conditional_cmake(project.cmake_after);

    if (!project.vcpkg.packages.empty()) {
        // Allow the user to specify a url or derive it from the version
        auto url = project.vcpkg.url;
        auto version_name = url;
        if (url.empty()) {
            if (project.vcpkg.version.empty()) {
                throw std::runtime_error("You need either [vcpkg].version or [vcpkg].url");
            }
            url = "https://github.com/microsoft/vcpkg/archive/refs/tags/" + project.vcpkg.version + ".tar.gz";
            version_name = project.vcpkg.version;
        }

        // Show a nicer error than vcpkg when specifying an invalid package name
        for (const auto &package : project.vcpkg.packages) {
            if (!vcpkg_valid_identifier(package.name)) {
                throw std::runtime_error("Invalid [vcpkg].packages name '" + package.name + "' (needs to be lowercase alphanumeric)");
            }
        }

        // CMake to bootstrap vcpkg and download the packages
        // clang-format off
        cmd("if")("CMKR_ROOT_PROJECT", "AND", "NOT", "CMKR_DISABLE_VCPKG");
            cmd("include")("FetchContent");
            cmd("message")("STATUS", "Fetching vcpkg (" + version_name + ")...");
            cmd("FetchContent_Declare")("vcpkg", "URL", url);
            // Not using FetchContent_MakeAvailable here in case vcpkg adds CMakeLists.txt
            cmd("FetchContent_GetProperties")("vcpkg");
            cmd("if")("NOT", "vcpkg_POPULATED");
                cmd("FetchContent_Populate")("vcpkg");
                cmd("include")("${vcpkg_SOURCE_DIR}/scripts/buildsystems/vcpkg.cmake");
            cmd("endif")();
        cmd("endif")();
        endl();
        // clang-format on

        // Generate vcpkg.json (sorry for the ugly string handling, nlohmann compiles very slowly)
        std::ofstream ofs("vcpkg.json", std::ios::binary);
        if (!ofs) {
            throw std::runtime_error("Failed to create a vcpkg.json manifest file!");
        }
        ofs << R"({
  "$cmkr": "This file is automatically generated from cmake.toml - DO NOT EDIT",
  "$cmkr-url": "https://github.com/build-cpp/cmkr",
  "$schema": "https://raw.githubusercontent.com/microsoft/vcpkg/master/scripts/vcpkg.schema.json",
  "dependencies": [
)";

        const auto &packages = project.vcpkg.packages;
        for (size_t i = 0; i < packages.size(); i++) {
            const auto &package = packages[i];
            const auto &features = package.features;
            if (!vcpkg_valid_identifier(package.name)) {
                throw std::runtime_error("Invalid vcpkg package name '" + package.name + "'");
            }
            for (const auto &feature : features) {
                if (!vcpkg_valid_identifier(feature)) {
                    throw std::runtime_error("Invalid vcpkg package feature '" + feature + "'");
                }
            }
            if (features.empty()) {
                ofs << "    \"" << package.name << '\"';
            } else {
                ofs << "    {\n";
                ofs << "      \"name\": \"" << package.name << "\",\n";
                ofs << "      \"features\": [";
                for (size_t j = 0; j < features.size(); j++) {
                    const auto &feature = features[j];
                    ofs << '\"' << feature << '\"';
                    if (j + 1 < features.size()) {
                        ofs << ',';
                    }
                }
                ofs << "]\n";
                ofs << "    }";
            }
            if (i + 1 < packages.size()) {
                ofs << ',';
            }
            ofs << '\n';
        }

        auto escape = [](const std::string &str) {
            std::string result;
            for (auto ch : str) {
                if (ch == '\\' || ch == '\"') {
                    result += '\\';
                }
                result += ch;
            }
            return result;
        };

        ofs << "  ],\n";
        ofs << "  \"description\": \"" << escape(project.project_description) << "\",\n";
        ofs << "  \"name\": \"" << escape(vcpkg_escape_identifier(project.project_name)) << "\",\n";
        ofs << R"(  "version-string": "")" << '\n';
        ofs << "}\n";
    }

    if (!project.contents.empty()) {
        cmd("include")("FetchContent").endl();
        for (const auto &content : project.contents) {
            ConditionScope cs(gen, content.condition);

            gen.conditional_includes(content.include_before);
            gen.conditional_cmake(content.cmake_before);

            std::string version_info = "";
            if (content.arguments.contains("GIT_TAG")) {
                version_info = " (" + content.arguments.at("GIT_TAG") + ")";
            } else if (content.arguments.contains("SVN_REVISION")) {
                version_info = " (" + content.arguments.at("SVN_REVISION") + ")";
            }
            cmd("message")("STATUS", "Fetching " + content.name + version_info + "...");
            cmd("FetchContent_Declare")(content.name, content.arguments);
            cmd("FetchContent_MakeAvailable")(content.name).endl();

            gen.conditional_includes(content.include_after);
            gen.conditional_cmake(content.cmake_after);
        }
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
            ConditionScope cs(gen, dep.condition);
            cmd("find_package")(dep.name, version, required, config, components).endl();
        }
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
        ConditionScope cs(gen, subdir.condition);

        gen.conditional_includes(subdir.include_before);
        gen.conditional_cmake(subdir.cmake_before);

        add_subdir(subdir.name);

        gen.conditional_includes(subdir.include_after);
        gen.conditional_cmake(subdir.cmake_after);
    }

    if (!project.targets.empty()) {
        for (size_t i = 0; i < project.targets.size(); i++) {
            if (i > 0) {
                endl();
            }

            const auto &target = project.targets[i];
            const parser::Template *tmplate = nullptr;
            std::unique_ptr<ConditionScope> tmplate_cs{};

            comment("Target " + target.name);

            // Check if this target is using a template.
            if (target.type == parser::target_template) {
                for (const auto &t : project.templates) {
                    if (target.type_name == t.outline.name) {
                        tmplate = &t;
                        tmplate_cs = std::unique_ptr<ConditionScope>(new ConditionScope(gen, tmplate->outline.condition));
                    }
                }
            }

            ConditionScope cs(gen, target.condition);

            cmd("set")("CMKR_TARGET", target.name);

            if (tmplate != nullptr) {
                gen.conditional_includes(tmplate->outline.include_before);
                gen.conditional_cmake(tmplate->outline.cmake_before);
            }

            gen.conditional_includes(target.include_before);
            gen.conditional_cmake(target.cmake_before);

            auto sources_var = target.name + "_SOURCES";

            bool added_toml = false;
            cmd("set")(sources_var, RawArg("\"\"")).endl();

            if (tmplate != nullptr) {
                gen.handle_condition(tmplate->outline.sources, [&](const std::string &condition, const std::vector<std::string> &condition_sources) {
                    auto sources = expand_cmake_paths(condition_sources, path, root_project);
                    if (sources.empty()) {
                        auto source_key = condition.empty() ? "sources" : (condition + ".sources");
                        throw std::runtime_error(target.name + " " + source_key + " wildcard found 0 files");
                    }
                    cmd("list")("APPEND", sources_var, sources);
                });
            }

            gen.handle_condition(target.sources, [&](const std::string &condition, const std::vector<std::string> &condition_sources) {
                auto sources = expand_cmake_paths(condition_sources, path, root_project);
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

            auto target_type = target.type;

            if (tmplate != nullptr) {
                target_type = tmplate->outline.type;
            }

            if (!added_toml && target_type != parser::target_interface) {
                cmd("list")("APPEND", sources_var, std::vector<std::string>{"cmake.toml"}).endl();
            }
            cmd("set")("CMKR_SOURCES", "${" + sources_var + "}");

            std::string add_command;
            std::string target_type_string;
            std::string target_scope;

            switch (target_type) {
            case parser::target_executable:
                add_command = "add_executable";
                target_type_string = "";
                target_scope = "PRIVATE";
                break;
            case parser::target_library:
                add_command = "add_library";
                target_type_string = "";
                target_scope = "PUBLIC";
                break;
            case parser::target_shared:
                add_command = "add_library";
                target_type_string = "SHARED";
                target_scope = "PUBLIC";
                break;
            case parser::target_static:
                add_command = "add_library";
                target_type_string = "STATIC";
                target_scope = "PUBLIC";
                break;
            case parser::target_interface:
                add_command = "add_library";
                target_type_string = "INTERFACE";
                target_scope = "INTERFACE";
                break;
            case parser::target_custom:
                // TODO: add proper support, this is hacky
                add_command = "add_custom_target";
                target_type_string = "SOURCES";
                target_scope = "PUBLIC";
                break;
            case parser::target_object:
                // NOTE: This is properly supported since 3.12
                add_command = "add_library";
                target_type_string = "OBJECT";
                target_scope = "PUBLIC";
                break;
            default:
                throw std::runtime_error("Unimplemented enum value");
            }

            // Handle custom add commands from templates.
            if (tmplate != nullptr && !tmplate->add_function.empty()) {
                add_command = tmplate->add_function;
                target_type_string = ""; // TODO: let templates supply options to the add_command here?

                if (tmplate->pass_sources_to_add_function) {
                    cmd(add_command)(target.name, target_type_string, "${" + sources_var + "}");
                } else {
                    cmd(add_command)(target.name, target_type_string).endl();

                    // clang-format off
                    cmd("if")(sources_var);
                        cmd("target_sources")(target.name, target_type == parser::target_interface ? "INTERFACE" : "PRIVATE", "${" + sources_var + "}");
                    cmd("endif")().endl();
                    // clang-format on
                }
            } else {
                cmd(add_command)(target.name, target_type_string).endl();

                // clang-format off
                cmd("if")(sources_var);
                    cmd("target_sources")(target.name, target_type == parser::target_interface ? "INTERFACE" : "PRIVATE", "${" + sources_var + "}");
                cmd("endif")().endl();
                // clang-format on
            }

            // The first executable target will become the Visual Studio startup project
            if (target_type == parser::target_executable) {
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

            auto gen_target_cmds = [&](const parser::Target &t) {
                target_cmd("target_compile_definitions", t.compile_definitions, target_scope);
                target_cmd("target_compile_definitions", t.private_compile_definitions, "PRIVATE");

                target_cmd("target_compile_features", t.compile_features, target_scope);
                target_cmd("target_compile_features", t.private_compile_features, "PRIVATE");

                target_cmd("target_compile_options", t.compile_options, target_scope);
                target_cmd("target_compile_options", t.private_compile_options, "PRIVATE");

                target_cmd("target_include_directories", t.include_directories, target_scope);
                target_cmd("target_include_directories", t.private_include_directories, "PRIVATE");

                target_cmd("target_link_directories", t.link_directories, target_scope);
                target_cmd("target_link_directories", t.private_link_directories, "PRIVATE");

                target_cmd("target_link_libraries", t.link_libraries, target_scope);
                target_cmd("target_link_libraries", t.private_link_libraries, "PRIVATE");

                target_cmd("target_link_options", t.link_options, target_scope);
                target_cmd("target_link_options", t.private_link_options, "PRIVATE");

                target_cmd("target_precompile_headers", t.precompile_headers, target_scope);
                target_cmd("target_precompile_headers", t.private_precompile_headers, "PRIVATE");
            };

            if (tmplate != nullptr) {
                gen_target_cmds(tmplate->outline);
            }

            gen_target_cmds(target);

            if (!target.properties.empty() || (tmplate != nullptr && !tmplate->outline.properties.empty())) {
                auto props = target.properties;

                if (tmplate != nullptr) {
                    props.insert(tmplate->outline.properties.begin(), tmplate->outline.properties.end());
                }

                gen.handle_condition(props, [&](const std::string &, const tsl::ordered_map<std::string, std::string> &properties) {
                    cmd("set_target_properties")(target.name, "PROPERTIES", properties);
                });
            }

            gen.conditional_includes(target.include_after);
            gen.conditional_cmake(target.cmake_after);

            if (tmplate != nullptr) {
                gen.conditional_includes(tmplate->outline.include_after);
                gen.conditional_cmake(tmplate->outline.cmake_after);
            }

            cmd("unset")("CMKR_TARGET");
            cmd("unset")("CMKR_SOURCES");
        }

        endl();
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
            ConditionScope cs(gen, test.condition);
            cmd("add_test")(name, configurations, working_directory, command, arguments).endl();
        }
    }

    if (!project.installs.empty()) {
        for (const auto &inst : project.installs) {
            auto targets = std::make_pair("TARGETS", inst.targets);
            auto dirs = std::make_pair("DIRS", inst.dirs);
            std::vector<std::string> files_data;
            if (!inst.files.empty()) {
                files_data = expand_cmake_paths(inst.files, path, root_project);
                if (files_data.empty()) {
                    throw std::runtime_error("[[install]] files wildcard did not resolve to any files");
                }
            }
            auto files = std::make_pair("FILES", inst.files);
            auto configs = std::make_pair("CONFIGURATIONS", inst.configs);
            auto destination = std::make_pair("DESTINATION", inst.destination);
            auto component_name = inst.component;
            if (component_name.empty() && !inst.targets.empty()) {
                component_name = inst.targets.front();
            }
            auto component = std::make_pair("COMPONENT", component_name);
            auto optional = inst.optional ? "OPTIONAL" : "";
            ConditionScope cs(gen, inst.condition);
            cmd("install")(targets, dirs, files, configs, destination, component, optional);
        }
    }

    // Generate CMakeLists.txt
    auto list_path = fs::path(path) / "CMakeLists.txt";

    auto should_regenerate = [&list_path, &ss]() {
        if (!fs::exists(list_path))
            return true;

        std::ifstream ifs(list_path, std::ios::binary);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to read " + list_path.string());
        }

        std::string data((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        return data != ss.str();
    }();

    if (should_regenerate) {
        create_file(list_path, ss.str());
    }

    auto generate_subdir = [path, &project](const fs::path &sub) {
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
            generate_cmake(subpath.string().c_str(), &project);
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
}
} // namespace gen
} // namespace cmkr
