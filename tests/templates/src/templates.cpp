#include <cstdio>

#if !defined(IS_APP)
#error Something went wrong with the template
#endif // IS_APP

int main() {
#if defined(APP_A)
    puts("Hello from app A!");
#elif defined(APP_B)
    puts("Hello from app B!");
#endif
}
