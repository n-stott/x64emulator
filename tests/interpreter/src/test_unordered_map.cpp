#include <unordered_map>
#include "fmt/core.h"

int main() {
    std::unordered_map<int, int> m;
    m.reserve(1);
    fmt::print("map size:{}\n", m.size());
    m[1] = 1;
    fmt::print("map size:{}\n", m.size());
    m[2] = 2;
    fmt::print("map size:{}\n", m.size());
}