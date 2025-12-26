#include "kernel/linux/fs/openfiledescription.h"

namespace kernel::gnulinux {

    ErrnoOrBuffer OpenFileDescription::ioctl(Ioctl request, const Buffer& buffer) {
        if(request == Ioctl::fionbio) {
            int setNonBlocking = 0;
            verify(buffer.size() == sizeof(int));
            memcpy(&setNonBlocking, buffer.data(), buffer.size());
            if(setNonBlocking) {
                statusFlags_.add(StatusFlags::NONBLOCK);
            } else {
                statusFlags_.remove(StatusFlags::NONBLOCK);
            }
            return ErrnoOrBuffer(0);
        }
        return file_->ioctl(*this, request, buffer);
    }

}