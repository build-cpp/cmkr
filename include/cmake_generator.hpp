#pragma once

#include "project_parser.hpp"

namespace cmkr {
namespace gen {

void generate_project(const std::string &type);

void generate_cmake(const char *path, const parser::Project *parent_project = nullptr);

} // namespace gen
} // namespace cmkr
