#include <fstream>
#include <iostream>

int main() {
    std::ifstream file1("testfile.txt");
    if(!file1.good()) return 1;

    std::ifstream file2("testfile.txt");
    if(!file2.good()) return 1;

    std::string s1;
    std::string s2;

    file1 >> s1;
    file2 >> s2;

    if(s1 != s2) return 1;

    return 0;
}