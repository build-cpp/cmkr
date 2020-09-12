#include "cmake.hpp"

#include <filesystem>
#include <stdexcept>
#include <toml.hpp>

namespace fs = std::filesystem;

namespace cmkr::cmake {

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
    const auto toml = toml::parse(fs::path(path) / "cmake.toml");
    if (build) {
        if (toml.contains("cmake")) {
            const auto &cmake = toml::find(toml, "cmake");

            if (cmake.contains("bin-dir")) {
                bin_dir = toml::find(cmake, "bin-dir").as_string();
            }

            if (cmake.contains("generator")) {
                generator = toml::find(cmake, "generator").as_string();
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
            proj_name = toml::find(project, "name").as_string();
            proj_version = toml::find(project, "version").as_string();
        }

        if (toml.contains("find-package")) {
            using pkg_map = std::map<std::string, std::string>;
            packages = toml::find<pkg_map>(toml, "find-package");
        }

        if (toml.contains("fetch-content")) {
            using content_map = std::map<std::string, std::map<std::string, std::string>>;
            contents = toml::find<content_map>(toml, "fetch-content");
        }

        if (toml.contains("bin")) {
            const auto &bins = toml::find(toml, "bin").as_array();

            for (const auto &bin : bins) {
                Bin b;
                b.name = toml::find(bin, "name").as_string();
                b.type = toml::find(bin, "type").as_string();

                b.sources = detail::to_string_vec(toml::find(bin, "sources").as_array());

                if (bin.contains("include-dirs")) {
                    b.include_dirs =
                        detail::to_string_vec(toml::find(bin, "include-dirs").as_array());
                }

                if (bin.contains("link-libs")) {
                    b.link_libs = detail::to_string_vec(toml::find(bin, "link-libs").as_array());
                }

                if (bin.contains("features")) {
                    b.features = detail::to_string_vec(toml::find(bin, "features").as_array());
                }

                if (bin.contains("defines")) {
                    b.defines = detail::to_string_vec(toml::find(bin, "defines").as_array());
                }
                binaries.push_back(b);
            }
        }
    }
}
} // namespace cmkr::cmake
