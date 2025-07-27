#ifndef HOSTSYMLINK_H
#define HOSTSYMLINK_H

#include "kernel/linux/fs/symlink.h"
#include <fmt/format.h>

namespace kernel::gnulinux {

    class HostSymlink : public Symlink {
    public:
        static File* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& pathname);

        void close() override;
        bool keepAfterClose() const override;
        std::optional<int> hostFileDescriptor() const override;

        std::string className() const override {
            return fmt::format("HostSymlink(link={})", link());
        }

    private:
        HostSymlink(FS* fs, Directory* dir, std::string name, std::string link) : Symlink(fs, dir, std::move(name), std::move(link)) { }
    };

}

#endif