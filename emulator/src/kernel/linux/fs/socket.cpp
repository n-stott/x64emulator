#include "kernel/linux/fs/socket.h"
#include "kernel/linux/fs/localsocket.h"
#include "host/host.h"
#include "verify.h"
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/stat.h>

namespace kernel::gnulinux {

    Socket::Socket(int fd, int domain, int type, int protocol) :
            hostFd_(fd),
            domain_(domain),
            type_(type),
            protocol_(protocol) {
        (void)domain_;
        (void)type_;
        (void)protocol_;
    }

    std::unique_ptr<Socket> Socket::tryCreate(int domain, int type, int protocol) {
        if(domain == AF_LOCAL) {
            return LocalSocket::tryCreate(domain, type, protocol);
        }
        auto validateDomain = [](int domain) -> bool {
            return domain == AF_NETLINK; // add other valid domains here
        };
        if(!validateDomain(domain)) {
            warn(fmt::format("Unsupported socket domain {}", domain));
            return {};
        }
        auto validateType = [](int type) -> bool {
            if((type & 0xF) == SOCK_RAW) return false;
            return true;
        };
        if(!validateType(type)) {
            warn(fmt::format("Unsupported socket type {:o}", type));
            return {};
        }
        int fd = ::socket(domain, type, protocol);
        if(fd < 0) return {};
        return std::unique_ptr<Socket>(new Socket(fd, domain, type, protocol));
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

    std::optional<int> Socket::fcntl(int cmd, int arg) {
        return Host::fcntl(Host::FD{hostFd_}, cmd, arg);
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
        Buffer buffer(buffersize, 0x0);
        int ret = ::getpeername(hostFd_, (sockaddr*)buffer.data(), &buffersize);
        if(ret < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)buffersize);
        return ErrnoOrBuffer(std::move(buffer));
    }

    ErrnoOrBuffer Socket::getsockname(u32 buffersize) const {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        Buffer buffer(buffersize, 0x0);
        int ret = ::getsockname(hostFd_, (sockaddr*)buffer.data(), &buffersize);
        if(ret < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)buffersize);
        return ErrnoOrBuffer(std::move(buffer));
    }

    ErrnoOrBuffer Socket::getsockopt(int level, int optname, const Buffer& buffer) const {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        Buffer buf = buffer;
        socklen_t bufsize = (socklen_t)buf.size();
        int ret = ::getsockopt(hostFd_, level, optname, buf.data(), &bufsize);
        if(ret < 0) return ErrnoOrBuffer(-errno);
        buf.shrink((size_t)bufsize);
        return ErrnoOrBuffer(std::move(buf));
    }

    int Socket::setsockopt(int level, int optname, const Buffer& buffer) const {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        return ::setsockopt(hostFd_, level, optname, buffer.data(), (socklen_t)buffer.size());
    }

    ErrnoOrBuffer Socket::read(OpenFileDescription&, size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        Buffer buffer(count, 0x0);
        ssize_t nbytes = ::read(hostFd_, buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.shrink((size_t)nbytes);
        return ErrnoOrBuffer(std::move(buffer));
    }

    ssize_t Socket::write(OpenFileDescription&, const u8* buf, size_t count) {
        if(!isWritable()) return -EINVAL;
        ssize_t nbytes = ::write(hostFd_, buf, count);
        if(nbytes < 0) return -errno;
        return nbytes;
    }

    void Socket::advanceInternalOffset(off_t) {
        // nothing to do
    }

    off_t Socket::lseek(OpenFileDescription&, off_t, int) {
        return -ESPIPE;
    }

    ErrnoOr<std::pair<Buffer, Buffer>> Socket::recvfrom(size_t len, int flags, bool requireSrcAddress) const {
        static_assert(sizeof(socklen_t) == sizeof(u32));
        if(requireSrcAddress) {
            return ErrnoOr<std::pair<Buffer, Buffer>>(-ENOTSUP);
        }
        Buffer buffer(len, 0);
        ssize_t ret = ::recvfrom(hostFd_, buffer.data(), len, flags, nullptr, nullptr);
        if(ret < 0) return ErrnoOr<std::pair<Buffer, Buffer>>(-errno);
        buffer.shrink((size_t)ret);
        return ErrnoOr<std::pair<Buffer, Buffer>>(std::make_pair(std::move(buffer), Buffer{}));
    }

    ssize_t Socket::recvmsg(int flags, Socket::Message* message) const {
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
        header.msg_name = message->msg_name.data();
        header.msg_namelen = (socklen_t)message->msg_name.size();
        std::vector<iovec> iovs;
        for(auto& buf : message->msg_iov) {
            iovec iov;
            iov.iov_base = buf.data();
            iov.iov_len = buf.size();
            iovs.push_back(iov);
        }
        header.msg_iov = iovs.data();
        header.msg_iovlen = iovs.size();
        header.msg_control = message->msg_control.data();
        header.msg_controllen = message->msg_control.size();
        header.msg_flags = 0;
        ssize_t ret = ::recvmsg(hostFd_, &header, flags);
        if(header.msg_controllen != 0) {
            Buffer buffer(header.msg_controllen, 0x0);
            ::memcpy(buffer.data(), (const u8*)header.msg_control, header.msg_controllen);
            message->msg_control = std::move(buffer);
        } else {
            message->msg_control = {};
        }
        message->msg_flags = header.msg_flags;
        if(ret < 0) return -errno;
        return ret;
    }

    ssize_t Socket::send(const Buffer& buffer, int flags) const {
        ssize_t ret = ::send(hostFd_, buffer.data(), buffer.size(), flags);
        if(ret < 0) return -errno;
        return ret;
    }

    ssize_t Socket::sendmsg(int flags, const Socket::Message& message) const {
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
        header.msg_name = (void*)message.msg_name.data();
        header.msg_namelen = (socklen_t)message.msg_name.size();
        std::vector<iovec> iovs;
        for(const auto& buf : message.msg_iov) {
            iovec iov;
            iov.iov_base = (void*)buf.data();
            iov.iov_len = buf.size();
            iovs.push_back(iov);
        }
        header.msg_iov = iovs.data();
        header.msg_iovlen = iovs.size();
        header.msg_control = (void*)message.msg_control.data();
        header.msg_controllen = message.msg_control.size();
        header.msg_flags = message.msg_flags;
        ssize_t ret = ::sendmsg(hostFd_, &header, flags);
        if(ret < 0) return -errno;
        return ret;
    }

    ErrnoOrBuffer Socket::stat() {
        struct stat st;
        int rc = ::fstat(hostFd_, &st);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        Buffer buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(std::move(buf));
    }

    ErrnoOrBuffer Socket::statfs() {
        verify(false, "statfs not implemented on socket");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer Socket::statx(unsigned int) {
        verify(false, "statx not implemented on socket");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer Socket::ioctl(OpenFileDescription&, Ioctl request, const Buffer&) {
        verify(false, fmt::format("ioctl(request={}) not implemented on socket", (int)request));
        return ErrnoOrBuffer(-ENOTSUP);
    }
    
    ErrnoOrBuffer Socket::getdents64(size_t) {
        return ErrnoOrBuffer(-ENOTDIR);
    }

}