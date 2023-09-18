// Created by Anthony Printup on 9/18/2023.
#include <cstdio>

extern "C" void library_function();
int main() {
    puts("Hello from cmkr(relative-paths)!");
    library_function();
}
