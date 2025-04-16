#ifndef SHADOWSYMLINK_H
#define SHADOWSYMLINK_H

#include "kernel/fs/symlink.h"
#include <fmt/format.h>

namespace kernel {

    class ShadowSymlink : public Symlink {
    public:
        static File* tryCreateAndAdd(FS* fs, Directory* parent, const std::string& pathname, const std::string& link);

        bool isShadow() const override { return true; }

        void close() override;
        bool keepAfterClose() const override;
        std::optional<int> hostFileDescriptor() const override;

        std::string className() const override {
            return fmt::format("ShadowSymlink(link={})", link());
        }

    private:
        ShadowSymlink(FS* fs, Directory* dir, std::string name, std::string link) : Symlink(fs, dir, std::move(name), std::move(link)) { }
    };

}

#endif