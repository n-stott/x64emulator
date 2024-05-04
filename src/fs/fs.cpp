#include "fs/fs.h"
#include "fs/file.h"
#include "fs/hostfile.h"
#include "host/host.h"
#include "interpreter/verify.h"

namespace kernel {

    FS::FS(Host* host) : host_(host) { }

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

/*
    FS::FD FS::open([[maybe_unused]] FD dirfd, const std::string& path, OpenFlags flags, Permissions permissions) {
        x64::verify(!path.empty(), "FS::open: empty path");
        x64::verify(path[0] == '/', "FS::open: non absolute path not supported");
        bool canUseHostFile = true;
        if(flags.append || flags.create || flags.truncate || flags.write) canUseHostFile = false;
        x64::verify(canUseHostFile, "unable to use host file without mutating it");

        if(canUseHostFile) {
            Host::FD fd = host_->openat(Host::FD{dirfd.fd}, path, flags, mode);
        } else {
            Host::FD fd = host_->openat(Host::FD{dirfd.fd}, path, flags, mode);
            if(fd.fd < 0) {
                
            }
        }
    }
*/
}