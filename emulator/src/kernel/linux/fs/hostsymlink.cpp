#include "kernel/linux/fs/hostsymlink.h"
#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/path.h"
#include "scopeguard.h"
#include "verify.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel::gnulinux {

    std::unique_ptr<HostSymlink> HostSymlink::tryCreate(const Path& path) {
        std::string pathname = path.absolute();

        // check that the file is a regular file
        struct stat s;
        if(::lstat(pathname.c_str(), &s) < 0) {
            return {};
        }
        
        mode_t fileType = (s.st_mode & S_IFMT);
        if (fileType != S_IFLNK) {
            // not a symbolic link
            return {};
        }

        char linkbuf[256];
        ssize_t len = ::readlink(pathname.c_str(), linkbuf, sizeof(linkbuf));
        if(len < 0) {
            warn("Unable to read link");
            return {};
        }
        std::string link(linkbuf, linkbuf+len);

        return std::unique_ptr<HostSymlink>(new HostSymlink(path.last(), link));
    }

    void HostSymlink::close() {
        verify(false, "HostSymlink::close not implemented");
    }

    bool HostSymlink::keepAfterClose() const {
        return true;
    }

    std::optional<int> HostSymlink::hostFileDescriptor() const {
        verify(false, "HostSymlink::hostFileDescriptor not implemented");
        return {};
    }

}