#include "kernel/fs/directory.h"
#include "verify.h"
#include <stack>

namespace kernel {

    Directory* Directory::tryGetOrAddSubDirectory(std::string name) {
        for(auto& entry : entries_) {
            if(entry->name() != name) continue;
            if(!entry->isDirectory()) return nullptr;
            return static_cast<Directory*>(entry.get());
        }
        std::unique_ptr<Directory> dir = std::make_unique<Directory>(fs_, this, std::move(name));
        Directory* dirPtr = dir.get();
        entries_.push_back(std::move(dir));
        return dirPtr;
    }

    void Directory::printSubtree() const {
        size_t depth = 1;
        std::stack<std::vector<const Directory*>> s;
        s.push({{this}});
        while(!s.empty()) {
            auto& v = s.top();
            if(v.empty()) {
                s.pop();
                --depth;
                continue;
            }
            const Directory* d = v.back();
            v.pop_back();
            fmt::print("{:{}} {}\n", "", depth, d->name());
            std::vector<const Directory*> newd;
            for(const auto& entry : d->entries_) {
                if(!entry->isDirectory()) continue;
                newd.push_back(static_cast<Directory*>(entry.get()));
            }
            std::sort(newd.begin(), newd.end(), [](const Directory* a, const Directory* b) {
                return a->name() > b->name();
            });
            ++depth;
            s.push(newd);
        }
    }

    void Directory::close() {
        // nothing to do ?
    }

    ErrnoOrBuffer Directory::read(size_t, off_t) {
        verify(false, "Cannot read from directory");
        return ErrnoOrBuffer(-EINVAL);
    }

    ssize_t Directory::write(const u8*, size_t, off_t) {
        verify(false, "Cannot write to directory");
        return -EINVAL;
    }

    off_t Directory::lseek(off_t, int) {
        verify(false, "Cannot seek in directory");
        return -EINVAL;
    }

    ErrnoOrBuffer Directory::stat() {
        verify(false, "stat not implemented on directory");
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ErrnoOrBuffer Directory::getdents64(size_t) {
        verify(false, "getdents64 not implemented on directory");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    int Directory::fcntl(int cmd, int arg) {
        verify(false, fmt::format("fcntl(cmd={}, arg={}) not implemented on directory", cmd, arg));
        return -ENOTSUP;
    }

    ErrnoOrBuffer Directory::ioctl(unsigned long request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on directory", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}