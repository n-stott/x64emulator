#include "fs/fs.h"
#include "fs/file.h"
#include "fs/hostfile.h"
#include "interpreter/kernel.h"
#include "interpreter/verify.h"

namespace kernel {

    FS::FS(Kernel& kernel) : kernel_(kernel) { }

    FS::~FS() = default;

    FS::OpenFlags FS::fromFlags(int flags) {
        return OpenFlags {
            Host::Open::isReadable(flags),
            Host::Open::isWritable(flags),
            Host::Open::isAppending(flags),
            Host::Open::isTruncating(flags),
            Host::Open::isCreatable(flags),
            Host::Open::isCloseOnExec(flags),
        };
    }

    FS::Permissions FS::fromMode(unsigned int mode) {
        return Permissions {
            Host::Mode::isUserReadable(mode),
            Host::Mode::isUserWritable(mode),
            Host::Mode::isUserExecutable(mode),
        };
    }


    File* FS::open(const std::string& path, OpenFlags flags, Permissions permissions) {
        (void)permissions;
        x64::verify(!path.empty(), "FS::open: empty path");
        x64::verify(path[0] == '/', "FS::open: non absolute path not supported");
        bool canUseHostFile = true;
        if(flags.append || flags.create || flags.truncate || flags.write) canUseHostFile = false;
        
        x64::verify(canUseHostFile, "only host-backed files are supported for now");

        Host::FD fd = kernel_.host().openat(Host::cwdfd(), path, 0, 0);
        auto hostBackedFile = std::make_unique<HostFile>(this, fd.fd);
        File* file = hostBackedFile.get();
        Node node {
            path,
            std::move(hostBackedFile),
            Permissions { true, false, false },
        };
        files_.push_back(std::move(node));
        return file;
    }

}