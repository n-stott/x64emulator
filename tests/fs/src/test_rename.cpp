#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/path.h"
#include <fcntl.h>

using namespace kernel;
using namespace kernel::gnulinux;

int main() {
    FS fs;
    
    {
        Path path("tmp", "testfile");
        BitFlags<FS::AccessMode> accessMode(FS::AccessMode::READ, FS::AccessMode::WRITE);
        BitFlags<FS::CreationFlags> createFlags(FS::CreationFlags::CREAT);
        BitFlags<FS::StatusFlags> statusFlags(FS::StatusFlags::RDWR);
        FS::Permissions permissions { true, true, true };
        FS::FD fd = fs.open(path, accessMode, createFlags, statusFlags, permissions);
        if(fd.fd < 0) return 1;
        
        if(fs.close(fd) < 0) {
            return 1;
        }
    }

    {
        Path oldPath("tmp", "testfile");
        Path newPath("home", "myfile");
        if(fs.rename(oldPath, newPath) < 0) {
            return 1;
        }
    }

    {
        Path path("home", "myfile");
        BitFlags<FS::AccessMode> accessMode(FS::AccessMode::READ, FS::AccessMode::WRITE);
        BitFlags<FS::CreationFlags> createFlags;
        BitFlags<FS::StatusFlags> statusFlags(FS::StatusFlags::RDWR);
        FS::Permissions permissions { true, true, true };
        FS::FD fd = fs.open(path, accessMode, createFlags, statusFlags, permissions);
        if(fd.fd < 0) return 1;
        
        if(fs.close(fd) < 0) {
            return 1;
        }
    }


    return 0;
}