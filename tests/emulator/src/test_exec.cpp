#include <unistd.h>
#include <array>
#include <string>

int main() {
    std::string pathname = "/usr/bin/ls";
    std::string dir = "/usr/bin/";
    std::array<char*, 3> argv = { (char*)pathname.c_str(), (char*)dir.c_str(), nullptr };
    std::array<char*, 1> envp = { nullptr };
    return execve(pathname.c_str(), argv.data(), envp.data());
}