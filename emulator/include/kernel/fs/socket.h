#ifndef SOCKET_H
#define SOCKET_H

#include "kernel/fs/fsobject.h"
#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include <memory> 

namespace kernel {

    class Socket : public FsObject {
    public:
        static std::unique_ptr<Socket> tryCreate(FS* fs, int domain, int type, int protocol);

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isSocket() const override { return true; }

        bool isPollable() const override { return true; }

        int connect(const Buffer& buffer);
        int fcntl(int cmd, int arg);
        int shutdown(int how);

        int bind(const Buffer& name);

        ErrnoOrBuffer getpeername(u32 buffersize);
        ErrnoOrBuffer getsockname(u32 buffersize);

        ErrnoOr<std::pair<Buffer, Buffer>> recvfrom(size_t len, int flags, bool requireSrcAddress);
        ssize_t recvmsg(int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags);
        ssize_t sendmsg(int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags);

        bool isReadable() const override { return true; }
        bool isWritable() const override { return true; }

        ErrnoOrBuffer read(size_t count) override;
        ssize_t write(const u8* buf, size_t count) override;

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

    private:
        Socket(FS* fs, int fd, int domain, int type, int protocol);

        int hostFd_ { -1 };
        int domain_ { 0 };
        int type_ { 0 };
        int protocol_ { 0 };
    };

}


#endif