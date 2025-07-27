#include "kernel/linux/fs/procfs.h"
#include "kernel/linux/fs/fs.h"

using namespace kernel;

int main() {
    FS fs;

    int pid = 916;
    std::string programPath = "/home/user/my_program";
    fs.resetProcFS(pid, programPath);

    auto errnoOfBuffer = fs.readlink("/proc/self/exe", 256);
    if(errnoOfBuffer.isError()) {
        return 1;
    } else {
        std::string link;
        int ret = errnoOfBuffer.errorOrWith<int>([&](const Buffer& buf) {
            link = std::string((const char*)buf.data(), buf.size());
            return 0;
        });
        if(ret != 0) return 1;
        if(link != programPath) {
            return 1;
        }
    }
    return 0;
}