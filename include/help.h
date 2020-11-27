#pragma once

#ifdef __cplusplus
namespace cmkr {
namespace help {

const char *version() noexcept;

const char *message() noexcept;

} // namespace help
} // namespace cmkr

extern "C" {
#endif

const char *cmkr_help_version(void);

const char *cmkr_help_message(void);

#ifdef __cplusplus
}
#endif
