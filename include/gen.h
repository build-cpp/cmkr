#pragma once

#ifdef __cplusplus
namespace cmkr {
namespace gen {

int generate_project(const char *typ);

int generate_cmake(const char *path, bool root = true);

} // namespace gen
} // namespace cmkr

extern "C" {
#endif

int cmkr_gen_generate_project(const char *typ);

int cmkr_gen_generate_cmake(const char *path);

#ifdef __cplusplus
}
#endif
