#include "cmake.hpp"

#include "enum_helper.hpp"
#include "fs.hpp"
#include <stdexcept>
#include <toml.hpp>
#include <tsl/ordered_map.h>

template <>
const char *enumStrings<cmkr::cmake::TargetType>::data[] = {"executable", "library", "shared", "static", "internface", "custom"};

namespace cmkr {
namespace cmake {

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

static std::vector<std::string> optional_array(const TomlBasicValue &v, const toml::key &ky) {
    return toml::find_or(v, ky, std::vector<std::string>());
};

CMake::CMake(const std::string &path, bool build) {
    if (!fs::exists(fs::path(path) / "cmake.toml")) {
        throw std::runtime_error("No cmake.toml was found!");
    }
    const auto toml = toml::parse<toml::preserve_comments, tsl::ordered_map, std::vector>((fs::path(path) / "cmake.toml").string());
    if (build) {
        if (toml.contains("cmake")) {
            const auto &cmake = toml::find(toml, "cmake");

            if (cmake.contains("bin-dir")) {
                throw std::runtime_error("bin-dir has been renamed to build-dir");
            }

            build_dir = toml::find_or(cmake, "build-dir", "");
            generator = toml::find_or(cmake, "generator", "");
            config = toml::find_or(cmake, "config", "");
            gen_args = optional_array(cmake, "arguments");
        }
    } else {
        if (toml.contains("cmake")) {
            const auto &cmake = toml::find(toml, "cmake");
            cmake_version = toml::find(cmake, "minimum").as_string();
            cmkr_include = toml::find_or(cmake, "cmkr-include", "cmkr.cmake");
            cppflags = optional_array(cmake, "cpp-flags");
            cflags = optional_array(cmake, "c-flags");
            linkflags = optional_array(cmake, "link-flags");
        }

        if (toml.contains("project")) {
            const auto &project = toml::find(toml, "project");
            project_name = toml::find(project, "name").as_string();
            project_version = toml::find_or(project, "version", "");
            project_description = toml::find_or(project, "description", "");
            project_languages = optional_array(project, "languages");
            cmake_before = toml::find_or(project, "cmake-before", "");
            cmake_after = toml::find_or(project, "cmake-after", "");
            include_before = optional_array(project, "include-before");
            include_after = optional_array(project, "include-after");
            subdirs = optional_array(project, "subdirs");
        }

        if (toml.contains("settings")) {
            using set_map = std::map<std::string, TomlBasicValue>;
            const auto &sets = toml::find<set_map>(toml, "settings");
            for (const auto set : sets) {
                Setting s;
                s.name = set.first;
                if (set.second.is_boolean()) {
                    s.val = set.second.as_boolean();
                } else if (set.second.is_string()) {
                    s.val = set.second.as_string();
                } else {
                    s.comment = toml::find_or(set.second, "comment", "");
                    if (set.second.contains("value")) {
                        auto v = toml::find(set.second, "value");
                        if (v.is_boolean()) {
                            s.val = v.as_boolean();
                        } else {
                            s.val = v.as_string();
                        }
                    }
                    s.cache = toml::find_or(set.second, "cache", false);
                    s.force = toml::find_or(set.second, "force", false);
                }
                settings.push_back(s);
            }
        }

        if (toml.contains("options")) {
            using opts_map = tsl::ordered_map<std::string, TomlBasicValue>;
            const auto &opts = toml::find<opts_map>(toml, "options");
            for (const auto opt : opts) {
                Option o;
                o.name = opt.first;
                if (opt.second.is_boolean()) {
                    o.val = opt.second.as_boolean();
                } else {
                    o.comment = toml::find_or(opt.second, "comment", "");
                    o.val = toml::find_or(opt.second, "value", false);
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
                    p.version = toml::find_or(pkg.second, "version", "");
                    p.required = toml::find_or(pkg.second, "required", false);
                    p.components = toml::find_or(pkg.second, "components", std::vector<std::string>());
                }
                packages.push_back(p);
            }
        }

        // TODO: refactor to std::vector<Content> instead of this hacky thing?
        contents = toml::find_or<decltype(contents)>(toml, "fetch-content", {});

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

                target.sources = optional_array(t, "sources");

#define renamed(from, to)                                                                                                                            \
    if (t.contains(from)) {                                                                                                                          \
        throw std::runtime_error(from "has been renamed to " to);                                                                                    \
    }

                renamed("include-dirs", "include-directories");
                renamed("link-libs", "link-libraries");
                renamed("defines", "compile-definitions");
                renamed("features", "compile-features");

#undef renamed

                target.compile_definitions = optional_array(t, "compile-definitions");
                target.compile_features = optional_array(t, "compile-features");
                target.compile_options = optional_array(t, "compile-options");
                target.include_directories = optional_array(t, "include-directories");
                target.link_directories = optional_array(t, "link-directories");
                target.link_libraries = optional_array(t, "link-libraries");
                target.link_options = optional_array(t, "link-options");
                target.precompile_headers = optional_array(t, "precompile-headers");

                if (t.contains("alias")) {
                    target.alias = toml::find(t, "alias").as_string();
                }

                if (t.contains("properties")) {
                    using prop_map = tsl::ordered_map<std::string, std::string>;
                    target.properties = toml::find<prop_map>(t, "properties");
                }

                target.cmake_before = toml::find_or(t, "cmake-before", "");
                target.cmake_after = toml::find_or(t, "cmake-after", "");
                target.include_before = optional_array(t, "include-before");
                target.include_after = optional_array(t, "include-after");

                targets.push_back(target);
            }
        }

        if (toml.contains("test")) {
            const auto &ts = toml::find(toml, "test").as_array();
            for (const auto &t : ts) {
                Test test;
                test.name = toml::find(t, "name").as_string();
                test.cmd = toml::find(t, "type").as_string();
                test.args = optional_array(t, "arguments");
                tests.push_back(test);
            }
        }

        if (toml.contains("install")) {
            const auto &ts = toml::find(toml, "install").as_array();
            for (const auto &t : ts) {
                Install inst;
                inst.targets = optional_array(t, "targets");
                inst.files = optional_array(t, "files");
                inst.dirs = optional_array(t, "dirs");
                inst.configs = optional_array(t, "configs");
                inst.destination = toml::find(t, "destination").as_string();
                installs.push_back(inst);
            }
        }
    }
}
} // namespace cmake
} // namespace cmkr
