#pragma once

#ifdef __cplusplus

namespace cmkr {
namespace build {

int run(int argc, char **argv);

int clean();

int install();

} // namespace build
} // namespace cmkr
extern "C" {
#endif

int cmkr_build_run(int argc, char **argv);

int cmkr_build_clean(void);

int cmkr_build_install(void);

#ifdef __cplusplus
}
#endif
