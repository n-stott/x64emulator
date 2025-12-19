#include "kernel/linux/fs/file.h"
#include "kernel/linux/fs/directory.h"
#include "verify.h"
#include <sstream>
#include <string>
#include <vector>

namespace kernel::gnulinux {

    Path File::path() const {
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
        auto path = Path::tryCreate(ss.str());
        verify(!!path, "Unable to create path");
        return *path;
    }

    void File::advanceInternalOffset(off_t offset) {
        verify(false, fmt::format("File::advanceInternalOffset(offset={}) not implemented for file type {}\n", offset, className()));
    }

}