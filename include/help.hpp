#pragma once

namespace cmkr {
namespace help {

const char *version() noexcept;

const char *message() noexcept;

} // namespace help
} // namespace cmkr

const char *cmkr_help_version(void);

const char *cmkr_help_message(void);
