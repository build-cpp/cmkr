#pragma once

#ifdef __cplusplus

namespace cmkr::build {

int run(int argc, char **argv);

int clean();

} // namespace cmkr::build
extern "C" {
#endif

int cmkr_build_run(int argc, char **argv);

int cmkr_build_clean(void);

#ifdef __cplusplus
}
#endif
