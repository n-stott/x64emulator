#include "kernel/fs/file.h"
#include "kernel/fs/directory.h"
#include <sstream>

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

}