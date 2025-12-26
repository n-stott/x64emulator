#ifndef DEVICE_H
#define DEVICE_H

#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/file.h"
#include "utils.h"
#include <verify.h>
#include <cassert>
#include <sys/types.h>

namespace kernel::gnulinux {

    class Device : public File {
    public:
        explicit Device(std::string name) : File(std::move(name)) { }

        bool isDevice() const override final { return true; }

        ErrnoOrBuffer getdents64(size_t) override final {
            verify(false, "Device::getdents64 not implemented");
            return ErrnoOrBuffer(-ENOTSUP);
        }
    };

}

#endif