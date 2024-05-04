#ifndef HOSTFILE_H
#define HOSTFILE_H

#include "fs/file.h"
#include "fs/fs.h"

namespace x64 {

    class HostFile : public File {
    public:
        HostFile(FS* fs, int hostFd) : File(fs), hostFd_(hostFd) { }

        ssize_t read(u8* buf, size_t count) const override;
        ssize_t write(const u8* buf, size_t count) override;

    private:
        int hostFd_ { -1 };
    };

}

#endif