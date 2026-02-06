#include <unistd.h>
#include <array>
#include <cstdio>
#include <string>

int main() {
    std::string pathname = "/abc/def";
    std::array<char*, 2> argv = { (char*)pathname.c_str(), nullptr };
    std::array<char*, 1> envp = { nullptr };
    int rc = execve(pathname.c_str(), argv.data(), envp.data());
    printf("exec(\"/abc/def\") = %d\n", rc);
    return 0;
}