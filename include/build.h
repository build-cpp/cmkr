#pragma once

#ifdef __cplusplus

namespace cmkr::build {

int run(int argc, char **argv);

} // namespace cmkr::build
extern "C" {
#endif

int cmkr_build_run(int argc, char **argv);

#ifdef __cplusplus
}
#endif
