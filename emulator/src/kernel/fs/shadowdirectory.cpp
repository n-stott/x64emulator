#include "kernel/fs/shadowdirectory.h"
#include "kernel/fs/fs.h"
#include "kernel/fs/path.h"
#include "kernel/kernel.h"
#include "verify.h"

namespace kernel {

    std::unique_ptr<ShadowDirectory> ShadowDirectory::tryCreate(FS* fs, Directory* parent, std::string name) {
        std::string pathname;
        if(!parent) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        return std::unique_ptr<ShadowDirectory>(new ShadowDirectory(fs, containingDirectory, path->last()));
    }

    ErrnoOrBuffer ShadowDirectory::getdents64(size_t) {
        verify(false, "implement getdents on shadow directory");
        return ErrnoOrBuffer(-ENOTSUP);
    }

}