#include "help.hpp"

namespace cmkr {
namespace help {

const char *version() noexcept { return "cmkr version 0.1.3"; }

const char *message() noexcept {
    return R"lit(
Usage: cmkr [arguments]
arguments:
    init    [executable|library|shared|static|interface] Starts a new project in the same directory.
    gen                                                  Generates CMakeLists.txt file.
    build   <extra cmake args>                           Run cmake and build.
    install                                              Run cmake --install. Needs admin privileges.
    clean                                                Clean the build directory.
    help                                                 Show help.
    version                                              Current cmkr version.
)lit";
}
} // namespace help
} // namespace cmkr

const char *cmkr_help_version(void) { return cmkr::help::version(); }

const char *cmkr_help_message(void) { return cmkr::help::message(); }