#include "project_parser.hpp"

#include "fs.hpp"
#include <deque>
#include <stdexcept>
#include <toml.hpp>

namespace cmkr {
namespace parser {

const char *targetTypeNames[target_last] = {"executable", "library", "shared", "static", "interface", "custom", "object", "template"};

static TargetType parse_targetType(const std::string &name) {
    for (int i = 0; i < target_last; i++) {
        if (name == targetTypeNames[i]) {
            return static_cast<TargetType>(i);
        }
    }
    return target_last;
}

const char *msvcRuntimeTypeNames[msvc_last] = {"dynamic", "static"};

static MsvcRuntimeType parse_msvcRuntimeType(const std::string &name) {
    for (int i = 0; i < msvc_last; i++) {
        if (name == msvcRuntimeTypeNames[i]) {
            return static_cast<MsvcRuntimeType>(i);
        }
    }
    return msvc_last;
}

using TomlBasicValue = toml::basic_value<toml::discard_comments, tsl::ordered_map, std::vector>;

static std::string format_key_message(const std::string &message, const toml::key &ky, const TomlBasicValue &value) {
    auto loc = value.location();
    auto line_number_str = std::to_string(loc.line());
    auto line_width = line_number_str.length();
    const auto &line_str = loc.line_str();

    std::ostringstream oss;
    oss << message << "\n";
    oss << " --> " << loc.file_name() << ':' << loc.line() << '\n';

    oss << std::string(line_width + 2, ' ') << "|\n";
    oss << ' ' << line_number_str << " | " << line_str << '\n';

    oss << std::string(line_width + 2, ' ') << '|';
    auto key_start = line_str.substr(0, loc.column() - 1).rfind(ky);
    if (key_start == std::string::npos) {
        key_start = line_str.find(ky);
    }
    if (key_start != std::string::npos) {
        oss << std::string(key_start + 1, ' ') << std::string(ky.length(), '~');
    }

    return oss.str();
}

static void throw_key_error(const std::string &error, const toml::key &ky, const TomlBasicValue &value) {
    throw std::runtime_error(format_key_message("[error] " + error, ky, value));
}

static void print_key_warning(const std::string &message, const toml::key &ky, const TomlBasicValue &value) {
    puts(format_key_message("[warning] " + message, ky, value).c_str());
}

class TomlChecker {
    const TomlBasicValue &m_v;
    tsl::ordered_set<toml::key> m_visited;
    tsl::ordered_set<toml::key> m_conditionVisited;

  public:
    TomlChecker(const TomlBasicValue &v, const toml::key &ky) : m_v(toml::find(v, ky)) {
    }
    explicit TomlChecker(const TomlBasicValue &v) : m_v(v) {
    }
    TomlChecker(const TomlChecker &) = delete;
    TomlChecker(TomlChecker &&) = delete;

    template <typename T>
    void optional(const toml::key &ky, Condition<T> &destination) {
        // TODO: this algorithm in O(n) over the amount of keys, kinda bad
        const auto &table = m_v.as_table();
        for (const auto &itr : table) {
            const auto &key = itr.first;
            const auto &value = itr.second;
            if (value.is_table()) {
                if (value.contains(ky)) {
                    destination[key] = toml::find<T>(value, ky);
                }
            } else if (key == ky) {
                destination[""] = toml::find<T>(m_v, ky);
            }
        }

        // Handle visiting logic
        for (const auto &itr : destination) {
            if (!itr.first.empty()) {
                m_conditionVisited.emplace(itr.first);
            }
        }
        visit(ky);
    }

    template <typename T>
    void optional(const toml::key &ky, T &destination) {
        // TODO: this currently doesn't allow you to get an optional map<string, X>
        if (m_v.contains(ky)) {
            destination = toml::find<T>(m_v, ky);
        }
        visit(ky);
    }

    template <typename T>
    void required(const toml::key &ky, T &destination) {
        destination = toml::find<T>(m_v, ky);
        visit(ky);
    }

    bool contains(const toml::key &ky) {
        visit(ky);
        return m_v.contains(ky);
    }

    const TomlBasicValue &find(const toml::key &ky) {
        visit(ky);
        return toml::find(m_v, ky);
    }

    void visit(const toml::key &ky) {
        m_visited.emplace(ky);
    }

    bool visisted(const toml::key &ky) const {
        return m_visited.contains(ky);
    }

    void check(const tsl::ordered_map<std::string, std::string> &conditions) const {
        for (const auto &itr : m_v.as_table()) {
            const auto &ky = itr.first;
            if (m_conditionVisited.contains(ky)) {
                if (!conditions.contains(ky)) {
                    throw_key_error("Unknown condition '" + ky + "'", ky, itr.second);
                }

                for (const auto &jtr : itr.second.as_table()) {
                    if (!m_visited.contains(jtr.first)) {
                        throw_key_error("Unknown key '" + jtr.first + "'", jtr.first, jtr.second);
                    }
                }
            } else if (!m_visited.contains(ky)) {
                if (itr.second.is_table()) {
                    for (const auto &jtr : itr.second.as_table()) {
                        if (!m_visited.contains(jtr.first)) {
                            throw_key_error("Unknown key '" + jtr.first + "'", jtr.first, jtr.second);
                        }
                    }
                }
                throw_key_error("Unknown key '" + ky + "'", ky, itr.second);
            } else if (ky == "condition") {
                std::string condition = itr.second.as_string();
                if (!conditions.contains(condition)) {
                    throw_key_error("Unknown condition '" + condition + "'", condition, itr.second);
                }
            }
        }
    }
};

class TomlCheckerRoot {
    const TomlBasicValue &m_root;
    std::deque<TomlChecker> m_checkers;
    tsl::ordered_set<toml::key> m_visisted;
    bool m_checked = false;

  public:
    explicit TomlCheckerRoot(const TomlBasicValue &root) : m_root(root) {
    }
    TomlCheckerRoot(const TomlCheckerRoot &) = delete;
    TomlCheckerRoot(TomlCheckerRoot &&) = delete;

    bool contains(const toml::key &ky) {
        m_visisted.emplace(ky);
        return m_root.contains(ky);
    }

    TomlChecker &create(const TomlBasicValue &v) {
        m_checkers.emplace_back(v);
        return m_checkers.back();
    }

    TomlChecker &create(const TomlBasicValue &v, const toml::key &ky) {
        m_checkers.emplace_back(v, ky);
        return m_checkers.back();
    }

    void check(const tsl::ordered_map<std::string, std::string> &conditions, bool check_root) {
        if (check_root) {
            for (const auto &itr : m_root.as_table()) {
                if (!m_visisted.contains(itr.first)) {
                    throw_key_error("Unknown key '" + itr.first + "'", itr.first, itr.second);
                }
            }
        }
        for (const auto &checker : m_checkers) {
            checker.check(conditions);
        }
    }
};

Project::Project(const Project *parent, const std::string &path, bool build) : parent(parent) {
    const auto toml_path = fs::path(path) / "cmake.toml";
    if (!fs::exists(toml_path)) {
        throw std::runtime_error("File not found '" + toml_path.string() + "'");
    }
    const auto toml = toml::parse<toml::discard_comments, tsl::ordered_map, std::vector>(toml_path.string());
    if (toml.size() == 0) {
        throw std::runtime_error("Empty TOML '" + toml_path.string() + "'");
    }

    TomlCheckerRoot checker(toml);

    if (checker.contains("cmake")) {
        auto &cmake = checker.create(toml, "cmake");

        cmake.required("version", cmake_version);

        if (cmake.contains("bin-dir")) {
            throw_key_error("bin-dir has been renamed to build-dir", "bin-dir", cmake.find("bin-dir"));
        }

        cmake.optional("build-dir", build_dir);
        cmake.optional("generator", generator);
        cmake.optional("config", config);
        cmake.optional("arguments", gen_args);
        cmake.optional("allow-in-tree", allow_in_tree);

        if (cmake.contains("cmkr-include")) {
            const auto &cmkr_include_kv = cmake.find("cmkr-include");
            if (cmkr_include_kv.is_string()) {
                cmkr_include = cmkr_include_kv.as_string();
            } else {
                // Allow disabling this feature with cmkr-include = false
                cmkr_include = "";
            }
        }

        cmake.optional("cpp-flags", cppflags);
        cmake.optional("c-flags", cflags);
        cmake.optional("link-flags", linkflags);
    }

    // Skip the rest of the parsing when building
    if (build) {
        checker.check(conditions, false);
        return;
    }

    // Reasonable default conditions (you can override these if you desire)
    if (parent == nullptr) {
        conditions["windows"] = R"cmake(WIN32)cmake";
        conditions["macos"] = R"cmake(CMAKE_SYSTEM_NAME MATCHES "Darwin")cmake";
        conditions["unix"] = R"cmake(UNIX)cmake";
        conditions["bsd"] = R"cmake(CMAKE_SYSTEM_NAME MATCHES "BSD")cmake";
        conditions["linux"] = conditions["lunix"] = R"cmake(CMAKE_SYSTEM_NAME MATCHES "Linux")cmake";
        conditions["gcc"] = R"cmake(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")cmake";
        conditions["msvc"] = R"cmake(MSVC)cmake";
        conditions["clang"] =
            R"cmake((CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "^MSVC$") OR (CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_C_COMPILER_FRONTEND_VARIANT MATCHES "^MSVC$"))cmake";
        conditions["clang-cl"] =
            R"cmake((CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND CMAKE_CXX_COMPILER_FRONTEND_VARIANT MATCHES "^MSVC$") OR (CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_C_COMPILER_FRONTEND_VARIANT MATCHES "^MSVC$"))cmake";
        conditions["clang-any"] = R"cmake(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "Clang")cmake";
        conditions["root"] = R"cmake(CMKR_ROOT_PROJECT)cmake";
        conditions["x64"] = R"cmake(CMAKE_SIZEOF_VOID_P EQUAL 8)cmake";
        conditions["x32"] = R"cmake(CMAKE_SIZEOF_VOID_P EQUAL 4)cmake";
    } else {
        conditions = parent->conditions;
        templates = parent->templates;
    }

    if (checker.contains("conditions")) {
        auto conds = toml::find<decltype(conditions)>(toml, "conditions");
        for (const auto &cond : conds) {
            conditions[cond.first] = cond.second;
        }
    }

    if (checker.contains("project")) {
        auto &project = checker.create(toml, "project");
        project.required("name", project_name);
        project.optional("version", project_version);
        project.optional("description", project_description);
        project.optional("languages", project_languages);
        project.optional("allow-unknown-languages", project_allow_unknown_languages);
        project.optional("cmake-before", cmake_before);
        project.optional("cmake-after", cmake_after);
        project.optional("include-before", include_before);
        project.optional("include-after", include_after);
        project.optional("subdirs", project_subdirs);

        std::string msvc_runtime;
        project.optional("msvc-runtime", msvc_runtime);
        if (!msvc_runtime.empty()) {
            project_msvc_runtime = parse_msvcRuntimeType(msvc_runtime);
            if (project_msvc_runtime == msvc_last) {
                std::string error = "Unknown runtime '" + msvc_runtime + "'\n";
                error += "Available types:\n";
                for (std::string type_name : msvcRuntimeTypeNames) {
                    error += "  - " + type_name + "\n";
                }
                error.pop_back(); // Remove last newline
                throw_key_error(error, msvc_runtime, project.find("msvc-runtime"));
            }
        }
    }

    if (checker.contains("subdir")) {
        const auto &subs = toml::find(toml, "subdir").as_table();
        for (const auto &itr : subs) {
            Subdir subdir;
            subdir.name = itr.first;

            auto &sub = checker.create(itr.second);
            sub.optional("condition", subdir.condition);
            sub.optional("cmake-before", subdir.cmake_before);
            sub.optional("cmake-after", subdir.cmake_after);
            sub.optional("include-before", subdir.include_before);
            sub.optional("include-after", subdir.include_after);

            subdirs.push_back(subdir);
        }
    }

    if (checker.contains("variables")) {
        using set_map = tsl::ordered_map<std::string, TomlBasicValue>;
        auto vars = toml::find<set_map>(toml, "variables");
        if (checker.contains("settings")) {
            print_key_warning("[settings] has been renamed to [variables]", "settings", toml.at("settings"));
            const auto &sets = toml::find<set_map>(toml, "settings");
            for (const auto &itr : sets) {
                if (!vars.insert(itr).second) {
                    throw_key_error("Key '" + itr.first + "' shadows existing variable", itr.first, itr.second);
                }
            }
        }

        for (const auto &itr : vars) {
            Variable s;
            s.name = itr.first;
            const auto &value = itr.second;
            if (value.is_boolean()) {
                s.value = value.as_boolean();
            } else if (value.is_string()) {
                s.value = value.as_string();
            } else {
                auto &setting = checker.create(value);
                setting.optional("help", s.help);
                if (setting.contains("value")) {
                    const auto &v = setting.find("value");
                    if (v.is_boolean()) {
                        s.value = v.as_boolean();
                    } else {
                        s.value = v.as_string();
                    }
                }
                setting.optional("cache", s.cache);
                setting.optional("force", s.force);
            }
            variables.push_back(s);
        }
    }

    if (checker.contains("options")) {
        using opts_map = tsl::ordered_map<std::string, TomlBasicValue>;
        const auto &opts = toml::find<opts_map>(toml, "options");
        for (const auto &itr : opts) {
            Option o;
            o.name = itr.first;
            const auto &value = itr.second;
            if (value.is_boolean()) {
                o.value = value.as_boolean();
            } else if (value.is_string()) {
                auto str = std::string(value.as_string());
                if (str == "root") {
                    o.value = std::string("${CMKR_ROOT_PROJECT}");
                } else {
                    throw_key_error("Unsupported option value '" + str + "'", str, value);
                }
            } else if (value.is_table()) {
                auto &option = checker.create(value);
                option.optional("help", o.help);
                if (option.contains("value")) {
                    const auto &ovalue = option.find("value");
                    if (ovalue.is_boolean()) {
                        o.value = ovalue.as_boolean();
                    } else if (ovalue.is_string()) {
                        auto str = std::string(ovalue.as_string());
                        if (str == "root") {
                            o.value = std::string("${CMKR_ROOT_PROJECT}");
                        } else {
                            throw_key_error("Unsupported option value '" + str + "'", str, value);
                        }
                    } else {
                        throw_key_error(toml::concat_to_string("Unsupported value type: ", ovalue.type()), "value", value);
                    }
                }
            } else {
                throw_key_error(toml::concat_to_string("Unsupported value type: ", itr.second.type()), itr.first, itr.second);
            }
            options.push_back(o);
            conditions.emplace(o.name, o.name);
        }
    }

    if (checker.contains("find-package")) {
        using pkg_map = tsl::ordered_map<std::string, TomlBasicValue>;
        const auto &pkgs = toml::find<pkg_map>(toml, "find-package");
        for (const auto &itr : pkgs) {
            Package p;
            p.name = itr.first;
            const auto &value = itr.second;
            if (itr.second.is_string()) {
                p.version = itr.second.as_string();
            } else {
                auto &pkg = checker.create(value);
                pkg.optional("condition", p.condition);
                pkg.optional("version", p.version);
                pkg.optional("required", p.required);
                pkg.optional("config", p.config);
                pkg.optional("components", p.components);
            }
            packages.push_back(p);
        }
    }

    if (checker.contains("fetch-content")) {
        const auto &fc = toml::find(toml, "fetch-content").as_table();
        for (const auto &itr : fc) {
            Content content;
            content.name = itr.first;

            auto &c = checker.create(itr.second);
            c.optional("condition", content.condition);
            c.optional("cmake-before", content.cmake_before);
            c.optional("cmake-after", content.cmake_after);
            c.optional("include-before", content.include_before);
            c.optional("include-after", content.include_after);
            c.optional("system", content.system);

            // Check if the minimum version requirement is satisfied (CMake 3.25)
            if (c.contains("system") && !this->cmake_minimum_version(3, 25)) {
                throw_key_error("The system argument is only supported on CMake version 3.25 and above.\nSet the CMake version in cmake.toml:\n"
                                "[cmake]\n"
                                "version = \"3.25\"\n",
                                "system", "");
            }

            for (const auto &argItr : itr.second.as_table()) {
                std::string value;
                if (argItr.second.is_array()) {
                    for (const auto &list_val : argItr.second.as_array()) {
                        if (!value.empty()) {
                            value += ';';
                        }
                        value += list_val.as_string();
                    }
                } else if (argItr.second.is_boolean()) {
                    value = argItr.second.as_boolean() ? "ON" : "OFF";
                } else {
                    value = argItr.second.as_string();
                }

                auto is_cmake_arg = [](const std::string &s) {
                    for (auto c : s) {
                        if (!(std::isdigit(c) || std::isupper(c) || c == '_')) {
                            return false;
                        }
                    }
                    return true;
                };

                // https://cmake.org/cmake/help/latest/command/string.html#supported-hash-algorithms
                tsl::ordered_set<std::string> hash_algorithms = {
                    "md5", "sha1", "sha224", "sha256", "sha384", "sha512", "sha3_224", "sha3_256", "sha3_384", "sha3_512",
                };

                auto key = argItr.first;
                if (key == "git") {
                    key = "GIT_REPOSITORY";
                } else if (key == "tag") {
                    key = "GIT_TAG";
                } else if (key == "shallow") {
                    key = "GIT_SHALLOW";
                } else if (key == "svn") {
                    key = "SVN_REPOSITORY";
                } else if (key == "rev") {
                    key = "SVN_REVISION";
                } else if (key == "url") {
                    key = "URL";
                } else if (hash_algorithms.contains(key)) {
                    std::string algo;
                    for (auto c : key) {
                        algo.push_back(std::toupper(c));
                    }
                    key = "URL_HASH";
                    value = algo + "=" + value;
                } else if (key == "hash") {
                    key = "URL_HASH";
                } else if (key == "subdir") {
                    key = "SOURCE_SUBDIR";
                } else if (is_cmake_arg(key)) {
                    // allow passthrough of ExternalProject options
                } else if (!c.visisted(key)) {
                    throw_key_error("Unknown key '" + argItr.first + "'", argItr.first, argItr.second);
                }

                // Make sure not to emit keys like "condition" in the FetchContent call
                if (!c.visisted(key)) {
                    content.arguments.emplace(key, value);
                }

                c.visit(argItr.first);
            }
            contents.emplace_back(std::move(content));
        }
    }

    if (checker.contains("bin")) {
        throw_key_error("[[bin]] has been renamed to [target.<name>]", "", toml.at("bin"));
    }

    auto parse_target = [&](const std::string &name, TomlChecker &t, bool isTemplate) {
        Target target;
        target.name = name;

        t.required("type", target.type_name);
        target.type = parse_targetType(target.type_name);

        // Users cannot set this target type
        if (target.type == target_template) {
            target.type = target_last;
        }

        if (!isTemplate && target.type == target_last) {
            for (const auto &tmplate : templates) {
                if (target.type_name == tmplate.outline.name) {
                    target.type = target_template;
                    break;
                }
            }
        }

        if (target.type == target_last) {
            std::string error = "Unknown target type '" + target.type_name + "'\n";
            error += "Available types:\n";
            for (std::string type_name : targetTypeNames) {
                if (type_name != "template") {
                    error += "  - " + type_name + "\n";
                }
            }
            if (!isTemplate && !templates.empty()) {
                error += "Available templates:\n";
                for (const auto &tmplate : templates) {
                    error += "  - " + tmplate.outline.name + "\n";
                }
            }
            error.pop_back(); // Remove last newline
            throw_key_error(error, target.type_name, t.find("type"));
        }

        t.optional("sources", target.sources);

        // Merge the headers into the sources
        ConditionVector headers;
        t.optional("headers", headers);
        for (const auto &itr : headers) {
            auto &dest = target.sources[itr.first];
            for (const auto &jtr : itr.second) {
                dest.push_back(jtr);
            }
        }

        t.optional("compile-definitions", target.compile_definitions);
        t.optional("private-compile-definitions", target.private_compile_definitions);

        t.optional("compile-features", target.compile_features);
        t.optional("private-compile-features", target.private_compile_features);

        t.optional("compile-options", target.compile_options);
        t.optional("private-compile-options", target.private_compile_options);

        t.optional("include-directories", target.include_directories);
        t.optional("private-include-directories", target.private_include_directories);

        t.optional("link-directories", target.link_directories);
        t.optional("private-link-directories", target.private_link_directories);

        t.optional("link-libraries", target.link_libraries);
        t.optional("private-link-libraries", target.private_link_libraries);

        // Add support for relative paths for (private-)link-libraries
        const auto fix_relative_paths = [&name, &path](ConditionVector &libraries, const char *key) {
            for (const auto &library_entries : libraries) {
                for (auto &library_path : libraries[library_entries.first]) {
                    // Skip processing paths with potential CMake macros in them (this check isn't perfect)
                    // https://cmake.org/cmake/help/latest/manual/cmake-language.7.html#variable-references
                    if ((library_path.find("${") != std::string::npos || library_path.find("$ENV{") != std::string::npos ||
                         library_path.find("$CACHE{") != std::string::npos) &&
                        library_path.find('}') != std::string::npos) {
                        continue;
                    }

                    // Skip paths that don't contain backwards or forwards slashes
                    if (library_path.find_first_of(R"(\/)") == std::string::npos) {
                        continue;
                    }

                    // Check if the new file path exists, otherwise emit an error
                    const auto expected_library_file_path = fs::path{path} / library_path;
                    if (!fs::exists(expected_library_file_path)) {
                        throw std::runtime_error("Attempted to link against a library file that doesn't exist for target \"" + name + "\" in \"" +
                                                 key + "\": " + library_path);
                    }

                    // Prepend ${CMAKE_CURRENT_SOURCE_DIR} to the path
                    library_path.insert(0, "${CMAKE_CURRENT_SOURCE_DIR}/");
                }
            }
        };
        fix_relative_paths(target.link_libraries, "link-libraries");
        fix_relative_paths(target.private_link_libraries, "private-link-libraries");

        t.optional("link-options", target.link_options);
        t.optional("private-link-options", target.private_link_options);

        t.optional("precompile-headers", target.precompile_headers);
        t.optional("private-precompile-headers", target.private_precompile_headers);

        Condition<std::string> msvc_runtime;
        t.optional("msvc-runtime", msvc_runtime);
        for (const auto &condItr : msvc_runtime) {
            switch (parse_msvcRuntimeType(condItr.second)) {
            case msvc_dynamic:
                target.properties[condItr.first]["MSVC_RUNTIME_LIBRARY"] = "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL";
                break;
            case msvc_static:
                target.properties[condItr.first]["MSVC_RUNTIME_LIBRARY"] = "MultiThreaded$<$<CONFIG:Debug>:Debug>";
                break;
            default: {
                std::string error = "Unknown runtime '" + condItr.second + "'\n";
                error += "Available types:\n";
                for (std::string type_name : msvcRuntimeTypeNames) {
                    error += "  - " + type_name + "\n";
                }
                error.pop_back(); // Remove last newline
                const TomlBasicValue *report;
                if (condItr.first.empty()) {
                    report = &t.find("msvc-runtime");
                } else {
                    report = &t.find(condItr.first).as_table().find("msvc-runtime").value();
                }
                throw_key_error(error, condItr.second, *report);
            }
            }
        }

        t.optional("condition", target.condition);
        t.optional("alias", target.alias);

        if (t.contains("properties")) {
            auto store_property = [&target](const toml::key &k, const TomlBasicValue &v, const std::string &condition) {
                if (v.is_array()) {
                    std::string property_list;
                    for (const auto &list_val : v.as_array()) {
                        if (!property_list.empty()) {
                            property_list += ';';
                        }
                        property_list += list_val.as_string();
                    }
                    target.properties[condition][k] = property_list;
                } else if (v.is_boolean()) {
                    target.properties[condition][k] = v.as_boolean() ? "ON" : "OFF";
                } else {
                    target.properties[condition][k] = v.as_string();
                }
            };

            const auto &props = t.find("properties").as_table();
            for (const auto &propKv : props) {
                const auto &k = propKv.first;
                const auto &v = propKv.second;
                if (v.is_table()) {
                    for (const auto &condKv : v.as_table()) {
                        store_property(condKv.first, condKv.second, k);
                    }
                } else {
                    store_property(k, v, "");
                }
            }
        }

        t.optional("cmake-before", target.cmake_before);
        t.optional("cmake-after", target.cmake_after);
        t.optional("include-before", target.include_before);
        t.optional("include-after", target.include_after);

        return target;
    };

    if (checker.contains("template")) {
        const auto &ts = toml::find(toml, "template").as_table();
        for (const auto &itr : ts) {
            auto &t = checker.create(itr.second);
            const auto &name = itr.first;

            for (const auto &type_name : targetTypeNames) {
                if (name == type_name) {
                    throw_key_error("Reserved template name '" + name + "'", name, itr.second);
                }
            }

            for (const auto &tmplate : templates) {
                if (name == tmplate.outline.name) {
                    throw_key_error("Template '" + name + "' already defined", name, itr.second);
                }
            }

            Template tmplate;
            tmplate.outline = parse_target(name, t, true);

            t.optional("add-function", tmplate.add_function);
            t.optional("pass-sources-to-add-function", tmplate.pass_sources_to_add_function);
            t.optional("pass-sources", tmplate.pass_sources_to_add_function);

            templates.push_back(tmplate);
        }
    }

    if (checker.contains("target")) {
        const auto &ts = toml::find(toml, "target").as_table();
        for (const auto &itr : ts) {
            auto &t = checker.create(itr.second);
            targets.push_back(parse_target(itr.first, t, false));
        }
    }

    if (checker.contains("test")) {
        const auto &ts = toml::find(toml, "test").as_array();
        for (const auto &value : ts) {
            auto &t = checker.create(value);
            Test test;
            t.required("name", test.name);
            t.optional("condition", test.condition);
            t.optional("configurations", test.configurations);
            t.optional("working-directory", test.working_directory);
            t.required("command", test.command);
            t.optional("arguments", test.arguments);
            tests.push_back(test);
        }
    }

    if (checker.contains("install")) {
        const auto &is = toml::find(toml, "install").as_array();
        for (const auto &value : is) {
            auto &i = checker.create(value);
            Install inst;
            i.optional("condition", inst.condition);
            i.optional("targets", inst.targets);
            i.optional("files", inst.files);
            i.optional("dirs", inst.dirs);
            i.optional("configs", inst.configs);
            i.required("destination", inst.destination);
            i.optional("component", inst.component);
            i.optional("optional", inst.optional);
            installs.push_back(inst);
        }
    }

    if (checker.contains("vcpkg")) {
        auto &v = checker.create(toml, "vcpkg");
        v.optional("url", vcpkg.url);
        v.optional("version", vcpkg.version);

        for (const auto &p : v.find("packages").as_array()) {
            Vcpkg::Package package;
            const auto &package_str = p.as_string().str;
            const auto open_bracket = package_str.find('[');
            const auto close_bracket = package_str.find(']', open_bracket);
            if (open_bracket == std::string::npos && close_bracket == std::string::npos) {
                package.name = package_str;
            } else if (close_bracket != std::string::npos) {
                package.name = package_str.substr(0, open_bracket);
                const auto features = package_str.substr(open_bracket + 1, close_bracket - open_bracket - 1);
                std::istringstream feature_stream{features};
                std::string feature;
                while (std::getline(feature_stream, feature, ',')) {
                    package.features.emplace_back(feature);
                }
            } else {
                throw_key_error("Invalid package name '" + package_str + "'", "packages", p);
            }
            vcpkg.packages.emplace_back(std::move(package));
        }
    }

    checker.check(conditions, true);
}

const Project *Project::root() const {
    auto root = this;
    while (root->parent != nullptr)
        root = root->parent;
    return root;
}

bool Project::cmake_minimum_version(int major, int minor) const {
    // NOTE: this code is like pulling teeth, sorry
    auto root_version = root()->cmake_version;
    auto range_index = root_version.find("...");
    if (range_index != std::string::npos) {
        root_version.resize(range_index);
    }

    auto period_index = root_version.find('.');
    auto root_major = atoi(root_version.substr(0, period_index).c_str());
    int root_minor = 0;
    if (period_index != std::string::npos) {
        auto end_index = root_version.find('.', period_index + 1);
        root_minor = atoi(root_version.substr(period_index + 1, end_index).c_str());
    }

    return std::tie(root_major, root_minor) >= std::tie(major, minor);
}

bool is_root_path(const std::string &path) {
    const auto toml_path = fs::path(path) / "cmake.toml";
    if (!fs::exists(toml_path)) {
        return false;
    }
    const auto toml = toml::parse<toml::discard_comments, tsl::ordered_map, std::vector>(toml_path.string());
    return toml.contains("project");
}

} // namespace parser
} // namespace cmkr
