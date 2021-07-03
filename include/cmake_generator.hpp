#pragma once

#include "project_parser.hpp"

namespace cmkr {
namespace gen {

int generate_project(const char *typ);

int generate_cmake(const char *path, const parser::Project *parent_project = nullptr);

} // namespace gen
} // namespace cmkr

int cmkr_gen_generate_project(const char *typ);

int cmkr_gen_generate_cmake(const char *path);
