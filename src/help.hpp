#pragma once

namespace cmkr::help {

const char *version = "cmkr version 0.1.0";

const char *help_msg = R"lit(
Usage: cmkr [arguments]
arguments:
    init     [exe|shared|static]    Starts a new project in the same directory.
    gen                             Generates CMakeLists.txt file.
    build    [cmake args...]        Run cmake and build.
    help                            Show help.
    version                         Current cmkr version.
    )lit";

} // namespace cmkr::help
