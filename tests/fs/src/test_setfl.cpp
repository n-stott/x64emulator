#include "kernel/linux/fs/fs.h"
#include <fcntl.h>

using namespace kernel;
using namespace kernel::gnulinux;

int real() {
    int flags = ::fcntl(0, F_GETFL, 0);
    if(flags < 0) return 1;
    int access = flags & O_ACCMODE;
    if(access != 2) return 1;

    flags = (flags & ~O_ACCMODE) | (0 & O_ACCMODE);
    int ret = ::fcntl(0, F_SETFL, flags);
    if(ret != 0) return 1;

    flags = ::fcntl(0, F_GETFL, 0);
    if(flags < 0) return 1;
    access = flags & O_ACCMODE;
    if(access != 2) return 1;

    return 0;
}

int emulated() {
    FS fs;

    FileDescriptors fds(fs);
    
    int flags = fds.fcntl(FD{0}, F_GETFL, 0);
    if(flags < 0) return 1;
    int access = flags & O_ACCMODE;
    if(access != 2) return 1;

    flags = (flags & ~O_ACCMODE) | (0 & O_ACCMODE);
    int ret = fds.fcntl(FD{0}, F_SETFL, flags);
    if(ret != 0) return 1;

    flags = fds.fcntl(FD{0}, F_GETFL, 0);
    if(flags < 0) return 1;
    access = flags & O_ACCMODE;
    if(access != 2) return 1;

    return 0;
}

int main() {
    if(real()) return 1;
    if(emulated()) return 1;
}