#pragma once

#ifdef __cplusplus
namespace cmkr::gen {

void generate_project(const char *typ);

void generate_cmake(const char *path);

} // namespace cmkr::gen

extern "C" {
#endif

void cmkr_gen_generate_project(const char *typ);

void cmkr_gen_generate_cmake(const char *path);

#ifdef __cplusplus
}
#endif
