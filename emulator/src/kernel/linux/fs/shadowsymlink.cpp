#include "kernel/linux/fs/shadowsymlink.h"
#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"

namespace kernel::gnulinux {

    File* ShadowSymlink::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name, const std::string& link) {
        std::string pathname;
        if(!parent || parent == fs->root()) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        auto shadowSymlink = std::unique_ptr<ShadowSymlink>(new ShadowSymlink(fs, containingDirectory, path->last(), link));
        return containingDirectory->addFile(std::move(shadowSymlink));
    }

    void ShadowSymlink::close() {
        verify(false, "ShadowSymlink::close not implemented");
    }

    bool ShadowSymlink::keepAfterClose() const {
        return true;
    }

    std::optional<int> ShadowSymlink::hostFileDescriptor() const {
        return {};
    }

    ErrnoOrBuffer ShadowSymlink::statx(unsigned int mask) {
        (void)mask;
        return ErrnoOrBuffer(-ENOTSUP);
    }

}