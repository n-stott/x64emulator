#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

int main() {
    std::string filename = "tmp_test_file.txt";
    std::string message = "This is a small message";

    std::ofstream outfile(filename);
    outfile << message << std::endl;
    outfile.close();

    std::ifstream infile(filename);
    std::string s;
    std::getline(infile, s);

    std::cout << "original=\"" << message << "\"" << std::endl;
    std::cout << "read    =\"" << s << "\"" << std::endl;

    std::remove(filename.c_str());
    
    assert(s == message);

    if(s != message) return 1;
    return 0;
}