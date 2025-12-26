#include "kernel/linux/fs/shadowsymlink.h"
#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"

namespace kernel::gnulinux {

    std::unique_ptr<ShadowSymlink> ShadowSymlink::tryCreate(const Path& path, const std::string& link) {
        return std::unique_ptr<ShadowSymlink>(new ShadowSymlink(path.last(), link));
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