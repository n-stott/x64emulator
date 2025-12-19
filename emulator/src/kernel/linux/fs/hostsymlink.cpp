#include "kernel/linux/fs/hostsymlink.h"
#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/path.h"
#include "scopeguard.h"
#include "verify.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel::gnulinux {

    File* HostSymlink::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name) {
        std::string pathname;
        if(!parent || parent == fs->root()) {
            pathname = name;
        } else {
            pathname = (parent->path().absolute() + "/" + name);
        }

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

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        char linkbuf[256];
        ssize_t len = ::readlink(absolutePathname.c_str(), linkbuf, sizeof(linkbuf));
        if(len < 0) {
            warn("Unable to read link");
            return {};
        }
        std::string link(linkbuf, linkbuf+len);

        auto hostSymlink = std::unique_ptr<HostSymlink>(new HostSymlink(fs, containingDirectory, path->last(), link));
        return containingDirectory->addFile(std::move(hostSymlink));
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