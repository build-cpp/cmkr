#include <stdio.h>

#ifdef __APPLE__
void platform_specific();
#else
void platform_specific() {
    puts("This is not an Apple platform.");
}
#endif

int main() {
    platform_specific();
    return 0;
}
