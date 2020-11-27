#pragma once

#ifdef __cplusplus
namespace cmkr {
namespace args {
const char *handle_args(int argc, char **argv);
}
} // namespace cmkr

extern "C" {
#endif

const char *cmkr_args_handle_args(int, char **);

#ifdef __cplusplus
}
#endif
