#ifndef HOSTSYMLINK_H
#define HOSTSYMLINK_H

#include "kernel/linux/fs/symlink.h"
#include <fmt/format.h>

namespace kernel::gnulinux {

    class HostSymlink : public Symlink {
    public:
        static std::unique_ptr<HostSymlink> tryCreate(const Path& path);

        void close() override;
        bool keepAfterClose() const override;
        std::optional<int> hostFileDescriptor() const override;

        std::string className() const override {
            return fmt::format("HostSymlink(link={})", link());
        }

    private:
        HostSymlink(std::string name, std::string link) : Symlink(std::move(name), std::move(link)) { }
    };

}

#endif