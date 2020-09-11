#pragma once

#ifdef __cplusplus
namespace cmkr::error {

struct Status {
    enum class Code {
        Success = 0,
        RuntimeError,
        InitError,
        GenerationError,
        BuildError,
    };
    Status(Code ec) noexcept;
    operator int() const noexcept;
    Code code() const noexcept;

  private:
    Code ec_ = Code::Success;
};

} // namespace cmkr::error

extern "C" {
#endif

const char *cmkr_error_status(int);

#ifdef __cplusplus
}
#endif
