#include "parser/disassembler.h"
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

std::vector<std::string> Disassembler::disassembleSection(const std::string& filepath, const std::string& section) {
    int pipefd[2];
    ::pipe(pipefd);

    pid_t pid = ::fork();
    if (pid == 0) {
        ::close(pipefd[0]);    // close reading end in the child

        ::dup2(pipefd[1], 1);  // send stdout to the pipe
        ::dup2(pipefd[1], 2);  // send stderr to the pipe

        ::close(pipefd[1]);    // this descriptor is no longer needed

        ::execl("/usr/bin/objdump", "objdump", "-d", "-C", "-Mintel", "-j", section.c_str(), filepath.c_str(), (char*)nullptr);
    } else {
        // parent
        ::close(pipefd[1]);  // close the write end of the pipe in the parent

        std::stringstream ss;
        char buffer[1024];

        int bytesRead = 0;
        while (bytesRead = ::read(pipefd[0], buffer, sizeof(buffer)), bytesRead != 0) {
            ss << std::string(buffer, buffer+bytesRead);
        }

        int wstatus = 0;
        ::waitpid(pid, &wstatus, 0);

        std::vector<std::string> result;
        std::string line;
        while (std::getline(ss, line)) {
            result.push_back(line);
        }
        
        return result;
    }
    __builtin_unreachable();
}