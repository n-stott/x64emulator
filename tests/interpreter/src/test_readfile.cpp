#include <fstream>
#include <iostream>

int main() {
    std::ifstream file("testfile.txt");
    if(!file.good()) return 1;
    std::string s;
    file >> s;
    std::cout << s << std::endl;
}