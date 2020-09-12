#pragma once

#ifdef __cplusplus
namespace cmkr::help {

const char *version() noexcept;

const char *message() noexcept;

} // namespace cmkr::help

extern "C" {
#endif

const char *cmkr_help_version(void);

const char *cmkr_help_message(void);

#ifdef __cplusplus
}
#endif
