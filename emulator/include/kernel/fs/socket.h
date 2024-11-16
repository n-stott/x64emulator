#ifndef SOCKET_H
#define SOCKET_H

#include "kernel/fs/file.h"
#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include <fmt/core.h>
#include <memory> 

namespace kernel {

    class Socket : public File {
    public:
        static std::unique_ptr<Socket> tryCreate(FS* fs, int domain, int type, int protocol);

        void close() override;
        bool keepAfterClose() const override { return false; }

        bool isSocket() const override { return true; }

        bool isPollable() const override { return true; }
        bool canRead() const override;
        bool canWrite() const override;

        int connect(const Buffer& buffer) const;
        std::optional<int> fcntl(int cmd, int arg) override;
        int shutdown(int how) const;

        int bind(const Buffer& name) const;

        ErrnoOrBuffer getpeername(u32 buffersize) const;
        ErrnoOrBuffer getsockname(u32 buffersize) const;
        ErrnoOrBuffer getsockopt(int level, int optname, const Buffer& buffer) const;
        int setsockopt(int level, int optname, const Buffer& buffer) const;

        ErrnoOr<std::pair<Buffer, Buffer>> recvfrom(size_t len, int flags, bool requireSrcAddress) const;
        ssize_t recvmsg(int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags) const;
        ssize_t send(const Buffer& buffer, int flags) const;
        ssize_t sendmsg(int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags) const;

        bool isReadable() const override { return true; }
        bool isWritable() const override { return true; }

        ErrnoOrBuffer read(OpenFileDescription&, size_t) override;
        ssize_t write(OpenFileDescription&, const u8* buf, size_t) override;

        off_t lseek(OpenFileDescription&, off_t offset, int whence) override;

        ErrnoOrBuffer stat() override;
        ErrnoOrBuffer statfs() override;

        ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) override;
        
        ErrnoOrBuffer getdents64(size_t count) override;

        std::optional<int> hostFileDescriptor() const override { return hostFd_; }

        std::string className() const override {
            return fmt::format("Socket(realfd={})", hostFd_);
        }

    private:
        Socket(FS* fs, int fd, int domain, int type, int protocol);

        int hostFd_ { -1 };
        int domain_ { 0 };
        int type_ { 0 };
        int protocol_ { 0 };
    };

}


#endif