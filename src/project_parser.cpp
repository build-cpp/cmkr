#include "project_parser.hpp"

#include "enum_helper.hpp"
#include "fs.hpp"
#include <stdexcept>
#include <toml.hpp>
#include <tsl/ordered_map.h>

template <>
const char *enumStrings<cmkr::parser::TargetType>::data[] = {"executable", "library", "shared", "static", "interface", "custom"};

namespace cmkr {
namespace parser {

using TomlBasicValue = toml::basic_value<toml::preserve_comments, tsl::ordered_map, std::vector>;

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

template <typename T>
static void get_optional(const TomlBasicValue &v, const toml::key &ky, T &destination);

template <typename T>
static void get_optional(const TomlBasicValue &v, const toml::key &ky, Condition<T> &destination) {
    // TODO: this algorithm in O(n) over the amount of keys, kinda bad
    const auto &table = v.as_table();
    for (const auto &itr : table) {
        const auto &key = itr.first;
        const auto &value = itr.second;
        if (value.is_table()) {
            if (value.contains(ky)) {
                destination[key] = toml::find<T>(value, ky);
            }
        } else if (key == ky) {
            destination[""] = toml::find<T>(v, ky);
        }
    }
}

template <typename T>
static void get_optional(const TomlBasicValue &v, const toml::key &ky, T &destination) {
    // TODO: this currently doesn't allow you to get an optional map<string, X>
    if (v.contains(ky)) {
        destination = toml::find<T>(v, ky);
    }
}

Project::Project(const std::string &path, bool build) {
    const auto toml_path = fs::path(path) / "cmake.toml";
    if (!fs::exists(toml_path)) {
        throw std::runtime_error("No cmake.toml was found!");
    }
    const auto toml = toml::parse<toml::preserve_comments, tsl::ordered_map, std::vector>(toml_path.string());
    if (build) {
        if (toml.contains("cmake")) {
            const auto &cmake = toml::find(toml, "cmake");

            if (cmake.contains("bin-dir")) {
                throw std::runtime_error("bin-dir has been renamed to build-dir");
            }

            get_optional(cmake, "build-dir", build_dir);
            get_optional(cmake, "generator", generator);
            get_optional(cmake, "config", config);
            get_optional(cmake, "arguments", gen_args);
        }
    } else {
        if (toml.contains("cmake")) {
            const auto &cmake = toml::find(toml, "cmake");
            cmake_version = toml::find(cmake, "version").as_string();
            get_optional(cmake, "cmkr-include", cmkr_include);
            get_optional(cmake, "cpp-flags", cppflags);
            get_optional(cmake, "c-flags", cflags);
            get_optional(cmake, "link-flags", linkflags);
        }

        if (toml.contains("project")) {
            const auto &project = toml::find(toml, "project");
            project_name = toml::find(project, "name").as_string();
            get_optional(project, "version", project_version);
            get_optional(project, "description", project_description);
            get_optional(project, "languages", project_languages);
            get_optional(project, "cmake-before", cmake_before);
            get_optional(project, "cmake-after", cmake_after);
            get_optional(project, "include-before", include_before);
            get_optional(project, "include-after", include_after);
            get_optional(project, "subdirs", project_subdirs);
        }

        if (toml.contains("subdir")) {
            const auto &subs = toml::find(toml, "subdir").as_table();
            for (const auto &sub : subs) {
                Subdir subdir;
                subdir.name = sub.first;
                get_optional(sub.second, "condition", subdir.condition);
                get_optional(sub.second, "cmake-before", subdir.cmake_before);
                get_optional(sub.second, "cmake-after", subdir.cmake_after);
                get_optional(sub.second, "include-before", subdir.include_before);
                get_optional(sub.second, "include-after", subdir.include_after);
                subdirs.push_back(subdir);
            }
        }

        if (toml.contains("settings")) {
            using set_map = std::map<std::string, TomlBasicValue>;
            const auto &sets = toml::find<set_map>(toml, "settings");
            for (const auto &set : sets) {
                Setting s;
                s.name = set.first;
                if (set.second.is_boolean()) {
                    s.val = set.second.as_boolean();
                } else if (set.second.is_string()) {
                    s.val = set.second.as_string();
                } else {
                    get_optional(set.second, "comment", s.comment);
                    if (set.second.contains("value")) {
                        auto v = toml::find(set.second, "value");
                        if (v.is_boolean()) {
                            s.val = v.as_boolean();
                        } else {
                            s.val = v.as_string();
                        }
                    }
                    get_optional(set.second, "cache", s.cache);
                    get_optional(set.second, "force", s.force);
                }
                settings.push_back(s);
            }
        }

        if (toml.contains("options")) {
            using opts_map = tsl::ordered_map<std::string, TomlBasicValue>;
            const auto &opts = toml::find<opts_map>(toml, "options");
            for (const auto &opt : opts) {
                Option o;
                o.name = opt.first;
                if (opt.second.is_boolean()) {
                    o.val = opt.second.as_boolean();
                } else {
                    get_optional(opt.second, "comment", o.comment);
                    get_optional(opt.second, "value", o.val);
                }
                options.push_back(o);
            }
        }

        if (toml.contains("find-package")) {
            using pkg_map = tsl::ordered_map<std::string, TomlBasicValue>;
            const auto &pkgs = toml::find<pkg_map>(toml, "find-package");
            for (const auto &pkg : pkgs) {
                Package p;
                p.name = pkg.first;
                if (pkg.second.is_string()) {
                    p.version = pkg.second.as_string();
                } else {
                    get_optional(pkg.second, "version", p.version);
                    get_optional(pkg.second, "required", p.required);
                    get_optional(pkg.second, "config", p.config);
                    get_optional(pkg.second, "components", p.components);
                }
                packages.push_back(p);
            }
        }

        // TODO: refactor to std::vector<Content> instead of this hacky thing?
        if (toml.contains("fetch-content")) {
            contents = toml::find<decltype(contents)>(toml, "fetch-content");
        }

        if (toml.contains("bin")) {
            throw std::runtime_error("[[bin]] has been renamed to [[target]]");
        }

        if (toml.contains("target")) {
            const auto &ts = toml::find(toml, "target").as_table();

            for (const auto &itr : ts) {
                const auto &t = itr.second;
                Target target;
                target.name = itr.first;
                target.type = to_enum<TargetType>(toml::find(t, "type").as_string(), "target type");

                get_optional(t, "headers", target.headers);
                get_optional(t, "sources", target.sources);

                get_optional(t, "compile-definitions", target.compile_definitions);
                get_optional(t, "private-compile-definitions", target.private_compile_definitions);

                get_optional(t, "compile-features", target.compile_features);
                get_optional(t, "private-compile-features", target.private_compile_features);

                get_optional(t, "compile-options", target.compile_options);
                get_optional(t, "private-compile-options", target.private_compile_options);

                get_optional(t, "include-directories", target.include_directories);
                get_optional(t, "private-include-directories", target.private_include_directories);

                get_optional(t, "link-directories", target.link_directories);
                get_optional(t, "private-link-directories", target.private_link_directories);

                get_optional(t, "link-libraries", target.link_libraries);
                get_optional(t, "private-link-libraries", target.private_link_libraries);

                get_optional(t, "link-options", target.link_options);
                get_optional(t, "private-link-options", target.private_link_options);

                get_optional(t, "precompile-headers", target.precompile_headers);
                get_optional(t, "private-precompile-headers", target.private_precompile_headers);

                if (!target.headers.empty()) {
                    auto &sources = target.sources.nth(0).value();
                    const auto &headers = target.headers.nth(0)->second;
                    sources.insert(sources.end(), headers.begin(), headers.end());
                }

                if (t.contains("condition")) {
                    target.condition = toml::find(t, "condition").as_string();
                }

                if (t.contains("alias")) {
                    target.alias = toml::find(t, "alias").as_string();
                }

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

                    const auto &props = toml::find(t, "properties").as_table();
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

                get_optional(t, "cmake-before", target.cmake_before);
                get_optional(t, "cmake-after", target.cmake_after);
                get_optional(t, "include-before", target.include_before);
                get_optional(t, "include-after", target.include_after);

                targets.push_back(target);
            }
        }

        if (toml.contains("test")) {
            const auto &ts = toml::find(toml, "test").as_array();
            for (const auto &t : ts) {
                Test test;
                test.name = toml::find(t, "name").as_string();
                get_optional(t, "configurations", test.configurations);
                get_optional(t, "working-directory", test.working_directory);
                test.command = toml::find(t, "command").as_string();
                get_optional(t, "arguments", test.arguments);
                tests.push_back(test);
            }
        }

        if (toml.contains("install")) {
            const auto &ts = toml::find(toml, "install").as_array();
            for (const auto &t : ts) {
                Install inst;
                get_optional(t, "targets", inst.targets);
                get_optional(t, "files", inst.files);
                get_optional(t, "dirs", inst.dirs);
                get_optional(t, "configs", inst.configs);
                inst.destination = toml::find(t, "destination").as_string();
                installs.push_back(inst);
            }
        }

        if (toml.contains("vcpkg")) {
            const auto &v = toml::find(toml, "vcpkg");
            get_optional(v, "url", vcpkg.url);
            get_optional(v, "version", vcpkg.version);
            vcpkg.packages = toml::find<decltype(vcpkg.packages)>(v, "packages");
        }

        // Reasonable default conditions (you can override these if you desire)
        conditions["windows"] = R"cmake(WIN32)cmake";
        conditions["macos"] = R"cmake("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")cmake";
        conditions["unix"] = R"cmake(UNIX)cmake";
        conditions["bsd"] = R"cmake("${CMAKE_SYSTEM_NAME}" MATCHES "BSD")cmake";
        conditions["linux"] = conditions["lunix"] = R"cmake("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")cmake";

        if (toml.contains("conditions")) {
            auto conds = toml::find<decltype(conditions)>(toml, "conditions");
            for (const auto &cond : conds) {
                conditions[cond.first] = cond.second;
            }
        }
    }
}

bool is_root_path(const std::string &path) {
    const auto toml_path = fs::path(path) / "cmake.toml";
    if (!fs::exists(toml_path)) {
        return false;
    }
    const auto toml = toml::parse<toml::preserve_comments, tsl::ordered_map, std::vector>(toml_path.string());
    return toml.contains("project");
}

} // namespace parser
} // namespace cmkr
