#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/hostdirectory.h"
#include "kernel/linux/fs/shadowdirectory.h"
#include "verify.h"
#include <algorithm>
#include <stack>
#include <dirent.h>

namespace kernel::gnulinux {

    File* Directory::tryGetEntry(const std::string& name) {
        for(auto& entry : entries_) {
            if(entry->name() != name) continue;
            return entry.get();
        }
        return nullptr;
    }

    std::unique_ptr<File> Directory::tryTakeEntry(const std::string& name) {
        auto it = std::find_if(entries_.begin(), entries_.end(), [&](const auto& entry) {
            return entry->name() == name;
        });
        if(it == entries_.end()) return nullptr;
        auto file = std::move(*it);
        entries_.erase(it);
        return file;
    }

    Directory* Directory::tryGetSubDirectory(const std::string& name) {
        for(auto& entry : entries_) {
            if(entry->name() != name) continue;
            if(!entry->isDirectory()) return nullptr;
            return static_cast<Directory*>(entry.get());
        }
        return nullptr;
    }

    Directory* Directory::tryAddHostDirectory(const std::string& name) {
        for(auto& entry : entries_) {
            if(entry->name() == name) return nullptr;
        }
        auto dir = HostDirectory::tryCreate(fs_, this, name);
        if(!dir) return nullptr;
        Directory* dirPtr = dir.get();
        entries_.push_back(std::move(dir));
        return dirPtr;
    }

    Directory* Directory::tryAddShadowDirectory(const std::string& name) {
        for(auto& entry : entries_) {
            if(entry->name() == name) return nullptr;
        }
        auto dir = ShadowDirectory::tryCreate(fs_, this, name);
        if(!dir) return nullptr;
        this->setTaintedByShadow();
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
            fmt::print("{:{}} \"{}\"\n", "", depth, d->name());
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

    ErrnoOrBuffer Directory::read(OpenFileDescription&, size_t) {
        verify(false, "Cannot read from directory");
        return ErrnoOrBuffer(-EINVAL);
    }

    ssize_t Directory::write(OpenFileDescription&, const u8*, size_t) {
        verify(false, "Cannot write to directory");
        return -EINVAL;
    }

    off_t Directory::lseek(OpenFileDescription&, off_t, int) {
        verify(false, "Cannot seek in directory");
        return -EINVAL;
    }

    std::optional<int> Directory::fcntl(int cmd, int arg) {
        verify(false, fmt::format("fcntl(cmd={}, arg={}) not implemented on directory", cmd, arg));
        return -ENOTSUP;
    }

    ErrnoOrBuffer Directory::ioctl(OpenFileDescription&, Ioctl request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on directory", (int)request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}