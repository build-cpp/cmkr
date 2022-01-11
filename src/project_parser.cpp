#include "project_parser.hpp"

#include "enum_helper.hpp"
#include "fs.hpp"
#include <deque>
#include <stdexcept>
#include <toml.hpp>
#include <tsl/ordered_map.h>

template <>
std::vector<std::string> enumStrings<cmkr::parser::TargetType>::data{"executable", "library", "shared", "static",
                                                                     "interface",  "custom",  "object", "template"};

namespace cmkr {
namespace parser {

using TomlBasicValue = toml::basic_value<toml::discard_comments, tsl::ordered_map, std::vector>;

template <typename EnumType>
static EnumType to_enum(const std::string &str, const std::string &help_name) {
    EnumType value;
    try {
        std::stringstream ss(str);
        ss >> enumFromString(value);
    } catch (std::invalid_argument &) {
        std::string supported;
        for (const auto &s : enumStrings<EnumType>::data) {
            if (!supported.empty()) {
                supported += ", ";
            }
            supported += s;
        }
        throw std::runtime_error("Unknown " + help_name + "'" + str + "'! Supported types are: " + supported);
    }
    return value;
}

static std::string format_key_error(const std::string &error, const toml::key &ky, const TomlBasicValue &value) {
    auto loc = value.location();
    auto line_number_str = std::to_string(loc.line());
    auto line_width = line_number_str.length();
    auto line_str = loc.line_str();

    std::ostringstream oss;
    oss << "[error] " << error << '\n';
    oss << " --> " << loc.file_name() << '\n';

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
    oss << '\n';

    return oss.str();
}

class TomlChecker {
    const TomlBasicValue &m_v;
    tsl::ordered_map<toml::key, bool> m_visited;
    tsl::ordered_map<toml::key, bool> m_conditionVisited;

  public:
    TomlChecker(const TomlBasicValue &v, const toml::key &ky) : m_v(toml::find(v, ky)) {}
    TomlChecker(const TomlBasicValue &v) : m_v(v) {}
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
                m_conditionVisited.emplace(itr.first, true);
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

    void visit(const toml::key &ky) { m_visited.emplace(ky, true); }

    void check(const tsl::ordered_map<std::string, std::string> &conditions) const {
        for (const auto &itr : m_v.as_table()) {
            const auto &ky = itr.first;
            if (m_conditionVisited.contains(ky)) {
                if (!conditions.contains(ky)) {
                    throw std::runtime_error(format_key_error("Unknown condition '" + ky + "'", ky, itr.second));
                }

                for (const auto &jtr : itr.second.as_table()) {
                    if (!m_visited.contains(jtr.first)) {
                        throw std::runtime_error(format_key_error("Unknown key '" + jtr.first + "'", jtr.first, jtr.second));
                    }
                }
            } else if (!m_visited.contains(ky)) {
                if (itr.second.is_table()) {
                    for (const auto &jtr : itr.second.as_table()) {
                        if (!m_visited.contains(jtr.first)) {
                            throw std::runtime_error(format_key_error("Unknown key '" + jtr.first + "'", jtr.first, jtr.second));
                        }
                    }
                }
                throw std::runtime_error(format_key_error("Unknown key '" + ky + "'", ky, itr.second));
            } else if (ky == "condition") {
                std::string condition = itr.second.as_string();
                if (!conditions.contains(condition)) {
                    throw std::runtime_error(format_key_error("Unknown condition '" + condition + "'", condition, itr.second));
                }
            }
        }
    }
};

class TomlCheckerRoot {
    std::deque<TomlChecker> m_checkers;
    bool m_checked = false;

  public:
    TomlCheckerRoot() = default;
    TomlCheckerRoot(const TomlCheckerRoot &) = delete;
    TomlCheckerRoot(TomlCheckerRoot &&) = delete;

    TomlChecker &create(const TomlBasicValue &v) {
        m_checkers.emplace_back(v);
        return m_checkers.back();
    }

    TomlChecker &create(const TomlBasicValue &v, const toml::key &ky) {
        m_checkers.emplace_back(v, ky);
        return m_checkers.back();
    }

    void check(const tsl::ordered_map<std::string, std::string> &conditions) {
        for (const auto &checker : m_checkers) {
            checker.check(conditions);
        }
    }
};

Project::Project(const Project *parent, const std::string &path, bool build) {
    const auto toml_path = fs::path(path) / "cmake.toml";
    if (!fs::exists(toml_path)) {
        throw std::runtime_error("No cmake.toml was found!");
    }
    const auto toml = toml::parse<toml::discard_comments, tsl::ordered_map, std::vector>(toml_path.string());

    TomlCheckerRoot checker;

    if (toml.contains("cmake")) {
        auto &cmake = checker.create(toml, "cmake");

        cmake.required("version", cmake_version);

        if (cmake.contains("bin-dir")) {
            throw std::runtime_error("bin-dir has been renamed to build-dir");
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
        checker.check(conditions);
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
        conditions["clang"] = R"cmake(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_C_COMPILER_ID MATCHES "Clang")cmake";
        conditions["msvc"] = R"cmake(MSVC)cmake";
        conditions["root"] = R"cmake(CMKR_ROOT_PROJECT)cmake";
        conditions["x64"] = R"cmake(CMAKE_SIZEOF_VOID_P EQUAL 8)cmake";
        conditions["x32"] = R"cmake(CMAKE_SIZEOF_VOID_P EQUAL 4)cmake";
    } else {
        conditions = parent->conditions;
    }

    if (toml.contains("conditions")) {
        auto conds = toml::find<decltype(conditions)>(toml, "conditions");
        for (const auto &cond : conds) {
            conditions[cond.first] = cond.second;
        }
    }

    if (toml.contains("project")) {
        auto &project = checker.create(toml, "project");
        project.required("name", project_name);
        project.optional("version", project_version);
        project.optional("description", project_description);
        project.optional("languages", project_languages);
        project.optional("cmake-before", cmake_before);
        project.optional("cmake-after", cmake_after);
        project.optional("include-before", include_before);
        project.optional("include-after", include_after);
        project.optional("subdirs", project_subdirs);
    }

    if (toml.contains("subdir")) {
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

    if (toml.contains("settings")) {
        using set_map = tsl::ordered_map<std::string, TomlBasicValue>;
        const auto &sets = toml::find<set_map>(toml, "settings");
        for (const auto &itr : sets) {
            Setting s;
            s.name = itr.first;
            const auto &value = itr.second;
            if (value.is_boolean()) {
                s.val = value.as_boolean();
            } else if (value.is_string()) {
                s.val = value.as_string();
            } else {
                auto &setting = checker.create(value);
                setting.optional("comment", s.comment);
                if (setting.contains("value")) {
                    const auto &v = setting.find("value");
                    if (v.is_boolean()) {
                        s.val = v.as_boolean();
                    } else {
                        s.val = v.as_string();
                    }
                }
                setting.optional("cache", s.cache);
                setting.optional("force", s.force);
            }
            settings.push_back(s);
        }
    }

    if (toml.contains("options")) {
        using opts_map = tsl::ordered_map<std::string, TomlBasicValue>;
        const auto &opts = toml::find<opts_map>(toml, "options");
        for (const auto &itr : opts) {
            Option o;
            o.name = itr.first;
            const auto &value = itr.second;
            if (value.is_boolean()) {
                o.val = value.as_boolean();
            } else {
                auto &option = checker.create(value);
                option.optional("comment", o.comment);
                option.optional("value", o.val);
            }
            options.push_back(o);
        }
    }

    if (toml.contains("find-package")) {
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

    // TODO: perform checking here
    if (toml.contains("fetch-content")) {
        const auto &fc = toml::find(toml, "fetch-content").as_table();
        for (const auto &itr : fc) {
            Content content;
            content.name = itr.first;
            for (const auto &argItr : itr.second.as_table()) {
                auto key = argItr.first;
                if (key == "condition") {
                    content.condition = argItr.second.as_string();
                    continue;
                }

                if (key == "git") {
                    key = "GIT_REPOSITORY";
                } else if (key == "tag") {
                    key = "GIT_TAG";
                } else if (key == "svn") {
                    key = "SVN_REPOSITORY";
                } else if (key == "rev") {
                    key = "SVN_REVISION";
                } else if (key == "url") {
                    key = "URL";
                } else if (key == "hash") {
                    key = "URL_HASH";
                } else {
                    // don't change arg
                }
                content.arguments.emplace(key, argItr.second.as_string());
            }
            contents.emplace_back(std::move(content));
        }
    }

    if (toml.contains("bin")) {
        throw std::runtime_error("[[bin]] has been renamed to [[target]]");
    }

    auto parse_target = [&](const std::string &name, TomlChecker& t) {
        Target target;
        target.name = name;

        t.required("type", target.type_string);

        target.type = to_enum<TargetType>(target.type_string, "target type");

        if (target.type >= target_COUNT) {
            target.type = target_template;
        }

        t.optional("headers", target.headers);
        t.optional("sources", target.sources);

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

        t.optional("link-options", target.link_options);
        t.optional("private-link-options", target.private_link_options);

        t.optional("precompile-headers", target.precompile_headers);
        t.optional("private-precompile-headers", target.private_precompile_headers);

        if (!target.headers.empty()) {
            auto &sources = target.sources.nth(0).value();
            const auto &headers = target.headers.nth(0)->second;
            sources.insert(sources.end(), headers.begin(), headers.end());
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

    if (toml.contains("template")) {
        const auto &ts = toml::find(toml, "template").as_table();

        for (const auto &itr : ts) {
            Template tmplate;
            auto& t = checker.create(itr.second);
            tmplate.outline = parse_target(itr.first, t);

            t.optional("add-function", tmplate.add_function);
            t.optional("pass-sources-to-add-function", tmplate.pass_sources_to_add_function);

            templates.push_back(tmplate);
            enumStrings<TargetType>::data.push_back(tmplate.outline.name);
        }
    }

    if (toml.contains("target")) {
        const auto &ts = toml::find(toml, "target").as_table();

        for (const auto &itr : ts) {
            auto& t = checker.create(itr.second);
            targets.push_back(parse_target(itr.first, t));
        }
    }

    if (toml.contains("test")) {
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

    if (toml.contains("install")) {
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
            installs.push_back(inst);
        }
    }

    if (toml.contains("vcpkg")) {
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
                throw std::runtime_error("Invalid vcpkg package '" + package_str + "'");
            }
            vcpkg.packages.emplace_back(std::move(package));
        }
    }

    checker.check(conditions);
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
