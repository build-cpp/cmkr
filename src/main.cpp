#include "args.h"

#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) try {
    auto output = cmkr::args::handle_args(argc, argv);
    return fprintf(stdout, "%s\n", output) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
} catch (const std::exception &e) {
    (void)fprintf(stderr, "%s\n", e.what());
    return EXIT_FAILURE;
}
