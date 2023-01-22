#include "fmt/core.h"
#include <cstdio>

int main(int argc, char* argv[]) {
    fmt::print("argc={}\n", argc);
    for(int i = 0; i < argc; ++i) {
        puts(argv[i]);
        // fmt::print("argv[{}]={}\n", i, argv[i]);
    }
    return 0;
}