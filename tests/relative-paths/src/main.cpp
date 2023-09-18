// Created by Anthony Printup on 9/18/2023.
#include <cstdio>

#ifdef WIN32
extern "C" void library_function();
#endif
int main() {
    puts("Hello from cmkr(relative-paths)!");
#ifdef WIN32
    library_function();
#endif
}
