#include "error.hpp"

#include <cassert>

namespace cmkr {
namespace error {

Status::Status(Code ec) noexcept : ec_(ec) {}

Status::operator int() const noexcept { return static_cast<int>(ec_); }

Status::Code Status::code() const noexcept { return ec_; }

} // namespace error
} // namespace cmkr

const char *err_string[] = {
    "Success", "Runtime error", "Initialization error", "CMake generation error", "Build error",
};

const char *cmkr_error_status(int i) {
    assert(i >= 0 && i < 5);
    return err_string[i];
}