#include <cstdio>

int main() {
#if defined(APP_A)
    puts("Hello from app A!");
#elif defined(APP_B)
    puts("Hello from app B!");
#endif
}
