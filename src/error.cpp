#include "error.hpp"

#include <cassert>

namespace cmkr {
namespace error {

Status::Status(Code ec) noexcept : ec_(ec) {}

Status::operator int() const noexcept { return static_cast<int>(ec_); }

Status::Code Status::code() const noexcept { return ec_; }

} // namespace error
} // namespace cmkr

// strings for cmkr::error::Status::Code
static const char *err_string[] = {
    "Success", "Runtime error", "Initialization error", "CMake generation error", "Build error", "Clean error", "Install error",
};

const char *cmkr_error_status(int i) {
    assert(i >= 0 && i < (sizeof(err_string) / sizeof(*(err_string))));
    return err_string[i];
}
