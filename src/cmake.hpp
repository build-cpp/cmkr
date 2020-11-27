#pragma once

#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace cmkr::cmake {

namespace detail {
template <typename T, typename O>
struct Variant {
    T first;
    O second;
    Variant() {}
    Variant(const T &t) : first(t), tag(0) {}
    Variant(const O &o) : second(o), tag(1) {}
    Variant &operator=(const T &t) {
        tag = 0;
        first = t;
        return *this;
    }
    Variant &operator=(const O &o) {
        tag = 1;
        second = o;
        return *this;
    }
    char index() const {
        if (tag == -1)
            throw std::runtime_error("[cmkr] error: Internal error!");
        return tag;
    }

  private:
    char tag = -1;
};
} // namespace detail

struct Setting {
    std::string name;
    std::string comment;
    detail::Variant<bool, std::string> val;
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