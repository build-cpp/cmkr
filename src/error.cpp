#include "error.h"

#include <assert.h>

namespace cmkr::error {

Status::Status(Code ec) noexcept : ec_(ec) {}

Status::operator int() noexcept { return static_cast<int>(ec_); }

} // namespace cmkr::error

const char *err_string[] = {
    "Success", "Runtime error", "Initialization error", "CMake generation error", "Build error",
};

const char *cmkr_error_status(int i) {
    assert(i >= 0 && i < 5);
    return err_string[i];
}