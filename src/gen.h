#pragma once

#ifdef __cplusplus
namespace cmkr::gen {

int generate_project(const char *typ);

int generate_cmake(const char *path);

} // namespace cmkr::gen

extern "C" {
#endif

int cmkr_gen_generate_project(const char *typ);

int cmkr_gen_generate_cmake(const char *path);

#ifdef __cplusplus
}
#endif
