#include "help.h"

namespace cmkr::help {

const char *version() noexcept { return "cmkr version 0.1.0"; }

const char *message() noexcept {
    return R"lit(
Usage: cmkr [arguments]
arguments:
    init     [exe|shared|static]    Starts a new project in the same directory.
    gen                             Generates CMakeLists.txt file.
    build    <extra cmake args>     Run cmake and build.
    clean                           Clean the build directory.
    help                            Show help.
    version                         Current cmkr version.
            )lit";
}
} // namespace cmkr::help

const char *cmkr_help_version(void) { return cmkr::help::version(); }

const char *cmkr_help_message(void) { return cmkr::help::message(); }