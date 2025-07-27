#include "kernel/linux/fs/file.h"
#include "kernel/linux/fs/directory.h"
#include "verify.h"
#include <sstream>
#include <string>
#include <vector>

namespace kernel {

    std::string File::path() const {
        std::vector<const Directory*> rdir;
        const Directory* dir = parent_;
        while(dir != nullptr) {
            rdir.push_back(dir);
            dir = dir->parent_;
        }
        std::stringstream ss;
        for(auto it = rdir.rbegin(); it != rdir.rend(); ++it) {
            const auto& name = (*it)->name();
            if(name.empty()) continue;
            ss << '/' << name;
        }
        ss << '/' << name_;
        return ss.str();
    }

    void File::advanceInternalOffset(off_t offset) {
        verify(false, fmt::format("File::advanceInternalOffset(offset={}) not implemented for file type {}\n", offset, className()));
    }

    ErrnoOrBuffer File::statx(unsigned int mask) {
        verify(false, fmt::format("File::statx(mask={:#x}) not implemented for file type {}\n", mask, className()));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}