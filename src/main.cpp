#include "args.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

int main(int argc, char **argv) try {
    auto output = cmkr::args::handle_args(argc, argv);
    auto format = "[cmkr] %s\n";
    if (strchr(output, '\n') != nullptr)
        format = "%s\n";
    (void)fprintf(stderr, format, output);
    return EXIT_SUCCESS;
} catch (const std::exception &e) {
    auto format = "[cmkr] error: %s\n";
    if (strchr(e.what(), '\n') != nullptr)
        format = "%s\n";
    (void)fprintf(stderr, format, e.what());
    return EXIT_FAILURE;
}
