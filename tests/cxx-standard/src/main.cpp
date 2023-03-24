#include <cstdio>
#include <tuple>

int main() {
    auto tpl = std::make_tuple(1, 2);
    printf("Hello from C++11 %d\n", std::get<0>(tpl));
}