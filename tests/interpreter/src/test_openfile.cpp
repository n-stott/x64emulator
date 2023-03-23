#include <fstream>

int main() {
    std::ifstream file("testfile.txt");
    if(!file.good()) return 1;
}