#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>
#include <mpark/variant.hpp>
#include <tsl/ordered_map.h>

namespace cmkr {
namespace cmake {

struct Setting {
    std::string name;
    std::string comment;
    mpark::variant<bool, std::string> val;
    bool cache = false;
    bool force = false;
};

struct Option {
    std::string name;
    std::string comment;
    bool val;
};

struct Package {
    std::string name;
    std::string version;
    bool required = true;
    std::vector<std::string> components;
};

struct Target {
    std::string name;
    std::string type;
    std::vector<std::string> sources;
    std::vector<std::string> include_directories;
    std::vector<std::string> compile_features;
    std::vector<std::string> compile_definitions;
    std::vector<std::string> link_libraries;
    std::string alias;
    std::map<std::string, std::string> properties;
};

struct Test {
    std::string name;
    std::string cmd;
    std::vector<std::string> args;
};

struct Install {
    std::vector<std::string> targets;
    std::vector<std::string> files;
    std::vector<std::string> dirs;
    std::vector<std::string> configs;
    std::string destination;
};

struct CMake {
    std::string cmake_version;
    std::string build_dir = "build";
    std::string generator;
    std::string config;
    std::vector<std::string> subdirs;
    std::vector<std::string> cppflags;
    std::vector<std::string> cflags;
    std::vector<std::string> linkflags;
    std::vector<std::string> gen_args;
    std::vector<std::string> build_args;
    std::string project_name;
    std::string project_version;
    std::string inject_before;
    std::string inject_after;
    std::vector<std::string> include_before;
    std::vector<std::string> include_after;
    std::vector<Setting> settings;
    std::vector<Option> options;
    std::vector<Package> packages;
    tsl::ordered_map<std::string, std::map<std::string, std::string>> contents;
    std::vector<Target> targets;
    std::vector<Test> tests;
    std::vector<Install> installs;
    CMake(const std::string &path, bool build);
};

} // namespace cmake
} // namespace cmkr