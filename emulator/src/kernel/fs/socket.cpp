#include "kernel/fs/socket.h"
#include "verify.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>

namespace kernel {

    Socket::Socket(FS* fs, int fd, int domain, int type, int protocol) :
            File(fs),
            hostFd_(fd),
            domain_(domain),
            type_(type),
            protocol_(protocol) {
        (void)domain_;
        (void)type_;
        (void)protocol_;
    }

    std::unique_ptr<Socket> Socket::tryCreate(FS* fs, int domain, int type, int protocol) {
        auto validateDomain = [](int domain) -> bool {
            if(domain == AF_LOCAL) return true;
            if(domain == AF_NETLINK) return true;
            return false;
        };
        if(!validateDomain(domain)) {
            verify(false, [&]() {
                fmt::print("Unsupported socket domain {}\n", domain);
            });
            return {};
        }
        int fd = ::socket(domain, type, protocol);
        if(fd < 0) return {};
        return std::unique_ptr<Socket>(new Socket(fs, fd, domain, type, protocol));
    }

    void Socket::close() {
        if(refCount_ > 0) return;
        int rc = ::close(hostFd_);
        verify(rc == 0);
    }

    bool Socket::canRead() const {
        struct pollfd pfd;
        pfd.fd = hostFd_;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int timeout = 0; // return immediately
        int ret = ::poll(&pfd, 1, timeout);
        if(ret < 0) return false;
        return !!(pfd.revents & POLLIN);
    }

    bool Socket::canWrite() const {
        struct pollfd pfd;
        pfd.fd = hostFd_;
        pfd.events = POLLOUT;
        pfd.revents = 0;
        int timeout = 0; // return immediately
        int ret = ::poll(&pfd, 1, timeout);
        if(ret < 0) return false;
        return !!(pfd.revents & POLLOUT);
    }

    int Socket::connect(const Buffer& buffer) const {
        int ret = ::connect(hostFd_, (const struct sockaddr*)buffer.data(), (socklen_t)buffer.size());
        if(ret < 0) return -errno;
        return ret;
    }

    int Socket::fcntl(int cmd, int arg) {
        switch(cmd) {
            case F_GETFD:
            case F_SETFD:
            case F_GETFL:
            case F_SETFL: {
                int ret = ::fcntl(hostFd_, cmd, arg);
                if(ret < 0) return -errno;
                return ret;
            }
            default: break;
        }
        return -ENOTSUP;
    }

    int Socket::shutdown(int how) const {
        int ret = ::shutdown(hostFd_, how);
        if(ret < 0) return -errno;
        return ret;
    }

    int Socket::bind(const Buffer& name) const {
        int ret = ::bind(hostFd_, (const struct sockaddr*)name.data(), (socklen_t)name.size());
        if(ret < 0) return -errno;
        return ret;
    }

    ErrnoOrBuffer Socket::getpeername(u32 buffersize) const {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        std::vector<u8> buffer;
        buffer.resize(buffersize, 0x0);
        int ret = ::getpeername(hostFd_, (sockaddr*)buffer.data(), &buffersize);
        if(ret < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)buffersize);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ErrnoOrBuffer Socket::getsockname(u32 buffersize) const {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        std::vector<u8> buffer;
        buffer.resize(buffersize, 0x0);
        int ret = ::getsockname(hostFd_, (sockaddr*)buffer.data(), &buffersize);
        if(ret < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)buffersize);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ErrnoOrBuffer Socket::read(size_t count, off_t) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read(hostFd_, buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t Socket::write(const u8* buf, size_t count, off_t) {
        if(!isWritable()) return -EINVAL;
        ssize_t nbytes = ::write(hostFd_, buf, count);
        if(nbytes < 0) return -errno;
        return nbytes;
    }

    off_t Socket::lseek(off_t, int) {
        return -ESPIPE;
    }

    ErrnoOr<std::pair<Buffer, Buffer>> Socket::recvfrom(size_t len, int flags, bool requireSrcAddress) const {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        if(requireSrcAddress) {
            return ErrnoOr<std::pair<Buffer, Buffer>>(-ENOTSUP);
        }
        std::vector<u8> buffer;
        buffer.resize(len, 0);
        ssize_t ret = ::recvfrom(hostFd_, buffer.data(), len, flags, nullptr, nullptr);
        if(ret < 0) return ErrnoOr<std::pair<Buffer, Buffer>>(-errno);
        buffer.resize((size_t)ret);
        return ErrnoOr<std::pair<Buffer, Buffer>>(std::make_pair(Buffer{std::move(buffer)}, Buffer{}));
    }

    ssize_t Socket::recvmsg(int flags, Buffer* msg_name, std::vector<Buffer>* msg_iov, Buffer* msg_control, int* msg_flags) const {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header;
        header.msg_name = msg_name->data();
        header.msg_namelen = (socklen_t)msg_name->size();
        std::vector<iovec> iovs;
        for(auto& buf : *msg_iov) {
            iovec iov;
            iov.iov_base = buf.data();
            iov.iov_len = buf.size();
            iovs.push_back(iov);
        }
        header.msg_iov = iovs.data();
        header.msg_iovlen = iovs.size();
        header.msg_control = msg_control->data();
        header.msg_controllen = msg_control->size();
        header.msg_flags = 0;
        ssize_t ret = ::recvmsg(hostFd_, &header, flags);
        *msg_flags = header.msg_flags;
        if(ret < 0) return -errno;
        return ret;
    }

    ssize_t Socket::send(const Buffer& buffer, int flags) const {
        ssize_t ret = ::send(hostFd_, buffer.data(), buffer.size(), flags);
        if(ret < 0) return -errno;
        return ret;
    }

    ssize_t Socket::sendmsg(int flags, const Buffer& msg_name, const std::vector<Buffer>& msg_iov, const Buffer& msg_control, int msg_flags) const {
        // struct msghdr {
        //     void*         msg_name;       /* Optional address */
        //     socklen_t     msg_namelen;    /* Size of address */
        //     struct iovec* msg_iov;        /* Scatter/gather array */
        //     size_t        msg_iovlen;     /* # elements in msg_iov */
        //     void*         msg_control;    /* Ancillary data, see below */
        //     size_t        msg_controllen; /* Ancillary data buffer len */
        //     int           msg_flags;      /* Flags on received message */
        // };
        msghdr header;
        header.msg_name = (void*)msg_name.data();
        header.msg_namelen = (socklen_t)msg_name.size();
        std::vector<iovec> iovs;
        for(const auto& buf : msg_iov) {
            iovec iov;
            iov.iov_base = (void*)buf.data();
            iov.iov_len = buf.size();
            iovs.push_back(iov);
        }
        header.msg_iov = iovs.data();
        header.msg_iovlen = iovs.size();
        header.msg_control = (void*)msg_control.data();
        header.msg_controllen = msg_control.size();
        header.msg_flags = msg_flags;
        ssize_t ret = ::sendmsg(hostFd_, &header, flags);
        if(ret < 0) return -errno;
        return ret;
    }

    ErrnoOrBuffer Socket::stat() {
        struct stat st;
        int rc = ::fstat(hostFd_, &st);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        std::vector<u8> buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(Buffer{std::move(buf)});
    }

    ErrnoOrBuffer Socket::ioctl(unsigned long request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on socket", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ErrnoOrBuffer Socket::getdents64(size_t) {
        return ErrnoOrBuffer(-ENOTDIR);
    }

}