#include "cmake.hpp"

#include "fs.hpp"
#include <stdexcept>
#include <toml.hpp>

namespace cmkr {
namespace cmake {

namespace detail {
std::vector<std::string> to_string_vec(
    const std::vector<toml::basic_value<toml::discard_comments, std::unordered_map, std::vector>>
        &vals) {
    std::vector<std::string> temp;
    for (const auto &val : vals)
        temp.push_back(val.as_string());
    return temp;
}
} // namespace detail

CMake::CMake(const std::string &path, bool build) {
    if (!fs::exists(fs::path(path) / "cmake.toml")) {
        throw std::runtime_error("No cmake.toml was found!");
    }
    const auto toml = toml::parse((fs::path(path) / "cmake.toml").string());
    if (build) {
        if (toml.contains("cmake")) {
            const auto &cmake = toml::find(toml, "cmake");

            if (cmake.contains("bin-dir")) {
                throw std::runtime_error("bin-dir has been renamed to build-dir");
            }

            if (cmake.contains("build-dir")) {
                build_dir = toml::find(cmake, "build-dir").as_string();
            }

            if (cmake.contains("generator")) {
                generator = toml::find(cmake, "generator").as_string();
            }

            if (cmake.contains("config")) {
                config = toml::find(cmake, "config").as_string();
            }

            if (cmake.contains("arguments")) {
                gen_args = detail::to_string_vec(toml::find(cmake, "arguments").as_array());
            }
        }
    } else {
        if (toml.contains("cmake")) {
            const auto &cmake = toml::find(toml, "cmake");
            cmake_version = toml::find(cmake, "minimum").as_string();

            if (cmake.contains("cpp-flags")) {
                cppflags = detail::to_string_vec(toml::find(cmake, "cpp-flags").as_array());
            }

            if (cmake.contains("c-flags")) {
                cflags = detail::to_string_vec(toml::find(cmake, "c-flags").as_array());
            }

            if (cmake.contains("link-flags")) {
                linkflags = detail::to_string_vec(toml::find(cmake, "link-flags").as_array());
            }

            if (cmake.contains("subdirs")) {
                subdirs = detail::to_string_vec(toml::find(cmake, "subdirs").as_array());
            }
        }

        if (toml.contains("project")) {
            const auto &project = toml::find(toml, "project");
            project_name = toml::find(project, "name").as_string();
            project_version = toml::find(project, "version").as_string();
            if (project.contains("include-before")) {
                include_before = detail::to_string_vec(toml::find(project, "include-before").as_array());
            }
            if (project.contains("include-after")) {
                include_after = detail::to_string_vec(toml::find(project, "include-after").as_array());
            }
        }

        if (toml.contains("settings")) {
            using set_map =
                std::map<std::string, toml::basic_value<toml::discard_comments, std::unordered_map,
                                                        std::vector>>;
            const auto &sets = toml::find<set_map>(toml, "settings");
            for (const auto set : sets) {
                Setting s;
                s.name = set.first;
                if (set.second.is_boolean()) {
                    s.val = set.second.as_boolean();
                } else if (set.second.is_string()) {
                    s.val = set.second.as_string();
                } else {
                    if (set.second.contains("comment")) {
                        s.comment = toml::find(set.second, "comment").as_string();
                    }
                    if (set.second.contains("value")) {
                        auto v = toml::find(set.second, "value");
                        if (v.is_boolean()) {
                            s.val = v.as_boolean();
                        } else {
                            s.val = v.as_string();
                        }
                    }
                    if (set.second.contains("cache")) {
                        s.cache = toml::find(set.second, "cache").as_boolean();
                    }
                    if (set.second.contains("force")) {
                        s.force = toml::find(set.second, "force").as_boolean();
                    }
                }
                settings.push_back(s);
            }
        }

        if (toml.contains("options")) {
            using opts_map =
                std::map<std::string, toml::basic_value<toml::discard_comments, std::unordered_map,
                                                        std::vector>>;
            const auto &opts = toml::find<opts_map>(toml, "options");
            for (const auto opt : opts) {
                Option o;
                o.name = opt.first;
                if (opt.second.is_boolean()) {
                    o.val = opt.second.as_boolean();
                } else {
                    if (opt.second.contains("comment")) {
                        o.comment = toml::find(opt.second, "comment").as_string();
                    }
                    if (opt.second.contains("value")) {
                        o.val = toml::find(opt.second, "value").as_boolean();
                    }
                }
                options.push_back(o);
            }
        }

        if (toml.contains("find-package")) {
            using pkg_map =
                std::map<std::string, toml::basic_value<toml::discard_comments, std::unordered_map,
                                                        std::vector>>;
            const auto &pkgs = toml::find<pkg_map>(toml, "find-package");
            for (const auto &pkg : pkgs) {
                Package p;
                p.name = pkg.first;
                if (pkg.second.is_string()) {
                    p.version = pkg.second.as_string();
                } else {
                    if (pkg.second.contains("version")) {
                        p.version = toml::find(pkg.second, "version").as_string();
                    }
                    if (pkg.second.contains("required")) {
                        p.required = toml::find(pkg.second, "required").as_boolean();
                    }
                    if (pkg.second.contains("components")) {
                        p.components =
                            detail::to_string_vec(toml::find(pkg.second, "components").as_array());
                    }
                }
                packages.push_back(p);
            }
        }

        if (toml.contains("fetch-content")) {
            using content_map = std::map<std::string, std::map<std::string, std::string>>;
            contents = toml::find<content_map>(toml, "fetch-content");
        }

        if (toml.contains("bin")) {
            throw std::runtime_error("[[bin]] has been renamed to [[target]]");
        }

        if (toml.contains("target")) {
            const auto &ts = toml::find(toml, "target").as_array();

            for (const auto &t : ts) {
                Target target;
                target.name = toml::find(t, "name").as_string();
                target.type = toml::find(t, "type").as_string();

                target.sources = detail::to_string_vec(toml::find(t, "sources").as_array());

#define renamed(from, to)                                                                          \
    if (t.contains(from)) {                                                                        \
        throw std::runtime_error(from "has been renamed to " to);                                  \
    }

                renamed("include-dirs", "include-directories");
                renamed("link-libs", "link-libraries");
                renamed("defines", "compile-definitions");
                renamed("features", "compile-features");

#undef renamed

                if (t.contains("include-directories")) {
                    target.include_directories =
                        detail::to_string_vec(toml::find(t, "include-directories").as_array());
                }

                if (t.contains("link-libraries")) {
                    target.link_libraries =
                        detail::to_string_vec(toml::find(t, "link-libraries").as_array());
                }

                if (t.contains("compile-features")) {
                    target.compile_features =
                        detail::to_string_vec(toml::find(t, "compile-features").as_array());
                }

                if (t.contains("compile-definitions")) {
                    target.compile_definitions =
                        detail::to_string_vec(toml::find(t, "compile-definitions").as_array());
                }

                if (t.contains("alias")) {
                    target.alias = toml::find(t, "alias").as_string();
                }

                if (t.contains("properties")) {
                    using prop_map = std::map<std::string, std::string>;
                    target.properties = toml::find<prop_map>(t, "properties");
                }

                targets.push_back(target);
            }
        }

        if (toml.contains("test")) {
            const auto &ts = toml::find(toml, "test").as_array();
            for (const auto &t : ts) {
                Test test;
                test.name = toml::find(t, "name").as_string();
                test.cmd = toml::find(t, "type").as_string();
                if (t.contains("arguments")) {
                    test.args = detail::to_string_vec(toml::find(t, "arguments").as_array());
                }
                tests.push_back(test);
            }
        }

        if (toml.contains("install")) {
            const auto &ts = toml::find(toml, "install").as_array();
            for (const auto &t : ts) {
                Install inst;
                if (t.contains("targets")) {
                    inst.targets = detail::to_string_vec(toml::find(t, "targets").as_array());
                }
                if (t.contains("files")) {
                    inst.files = detail::to_string_vec(toml::find(t, "files").as_array());
                }
                if (t.contains("dirs")) {
                    inst.dirs = detail::to_string_vec(toml::find(t, "dirs").as_array());
                }
                if (t.contains("configs")) {
                    inst.configs = detail::to_string_vec(toml::find(t, "configs").as_array());
                }
                inst.destination = toml::find(t, "destination").as_string();
                installs.push_back(inst);
            }
        }
    }
}
} // namespace cmake
} // namespace cmkr
