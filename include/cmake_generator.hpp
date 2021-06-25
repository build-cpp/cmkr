#pragma once

namespace cmkr {
namespace gen {

int generate_project(const char *typ);

int generate_cmake(const char *path, bool root = true);

} // namespace gen
} // namespace cmkr

int cmkr_gen_generate_project(const char *typ);

int cmkr_gen_generate_cmake(const char *path);
