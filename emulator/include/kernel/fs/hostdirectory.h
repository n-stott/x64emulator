#ifndef HOSTDIRECTORY_H
#define HOSTDIRECTORY_H

#include "kernel/fs/directory.h"
#include "kernel/fs/file.h"
#include <memory>
#include <string>

namespace kernel {

    class Directory;
    class FS;

    class HostDirectory : public Directory {
    public:
        static std::unique_ptr<HostDirectory> tryCreateRoot(FS* fs);
        static std::unique_ptr<HostDirectory> tryCreate(FS* fs, Directory* parent, const std::string& pathname);

        void open() override;
        void close() override;

        off_t lseek(off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;

        ErrnoOrBuffer getdents64(size_t count) override;

        int fcntl(int cmd, int arg) override;

        std::string className() const override { return "HostDirectory"; }
    private:
        HostDirectory(FS* fs, Directory* parent, std::string name) : Directory(fs, parent, std::move(name)) { }
        std::optional<int> hostFd_; // the host fd when the folder is opened
    };

}

#endif