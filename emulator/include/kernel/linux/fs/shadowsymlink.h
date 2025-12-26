#ifndef SHADOWSYMLINK_H
#define SHADOWSYMLINK_H

#include "kernel/linux/fs/symlink.h"
#include <fmt/format.h>

namespace kernel::gnulinux {

    class Path;

    class ShadowSymlink : public Symlink {
    public:
        static std::unique_ptr<ShadowSymlink> tryCreate(const Path& path, const std::string& link);

        bool isShadow() const override { return true; }

        void close() override;
        bool keepAfterClose() const override;
        std::optional<int> hostFileDescriptor() const override;

        ErrnoOrBuffer statx(unsigned int) override;

        std::string className() const override {
            return fmt::format("ShadowSymlink(link={})", link());
        }

    private:
        ShadowSymlink(std::string name, std::string link) : Symlink(std::move(name), std::move(link)) { }
    };

}

#endif