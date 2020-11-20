#pragma once

#include <map>
#include <string>
#include <vector>
#include <variant>

namespace cmkr::cmake {


struct Setting {
    std::string name;
    std::string comment;
    std::variant<bool, std::string> val;
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

struct Bin {
    std::string name;
    std::string type;
    std::vector<std::string> sources;
    std::vector<std::string> include_dirs;
    std::vector<std::string> features;
    std::vector<std::string> defines;
    std::vector<std::string> link_libs;
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
    std::string cmake_version = "3.15";
    std::string bin_dir = "bin";
    std::string generator;
    std::string config;
    std::vector<std::string> subdirs;
    std::vector<std::string> cppflags;
    std::vector<std::string> cflags;
    std::vector<std::string> linkflags;
    std::vector<std::string> gen_args;
    std::vector<std::string> build_args;
    std::string proj_name;
    std::string proj_version;
    std::vector<Setting> settings;
    std::vector<Option> options;
    std::vector<Package> packages;
    std::map<std::string, std::map<std::string, std::string>> contents;
    std::vector<Bin> binaries;
    std::vector<Test> tests;
    std::vector<Install> installs;
    CMake(const std::string &path, bool build);
};

} // namespace cmkr::cmake