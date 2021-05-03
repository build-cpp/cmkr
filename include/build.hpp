#pragma once

namespace cmkr {
namespace build {

int run(int argc, char **argv);

int clean();

int install();

} // namespace build
} // namespace cmkr

int cmkr_build_run(int argc, char **argv);

int cmkr_build_clean();

int cmkr_build_install();
