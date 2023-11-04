#include <fstream>
#include <iostream>

int readText() {
    std::ifstream file("testfile.txt");
    if(!file.good()) return 1;
    std::string s;
    file >> s;
    std::cout << s << std::endl;
    return 0;
}

int readBin() {
    std::ifstream file("testfile.txt", std::ios::in | std::ios::binary);
    if(!file.good()) return 2;
    return 0;
}

int main() {
    int rc = 0;
    rc |= readText();
    rc |= readBin();
    return rc;
}