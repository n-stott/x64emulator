#include "kernel/fs/localsocket.h"
#include "verify.h"
#include <fmt/ranges.h>
#include <sys/socket.h>

namespace kernel {

    LocalSocket::LocalSocket(FS* fs, int fd, int domain, int type, int protocol) : Socket(fs, fd, domain, type, protocol) { }

    std::unique_ptr<LocalSocket> LocalSocket::tryCreate(FS* fs, int domain, int type, int protocol) {
        verify(domain == AF_LOCAL);
        int fd = ::socket(domain, type, protocol);
        if(fd < 0) return {};
        return std::unique_ptr<LocalSocket>(new LocalSocket(fs, fd, domain, type, protocol));
    }

    ssize_t LocalSocket::recvmsg(int flags, Message* message) const {
        verify(!!message);
        // Check that the ancillary message is not SCM_RIGHTS
        // This would mean sending/receiving fds, which we are not ready for.
        if(message->msg_control.size() > 0) {
            // struct cmsghdr {
            //     size_t cmsg_len;
            //     int cmsg_level;
            //     int cmsg_type;  <-- HERE
            //     unsigned char __cmsg_data[];
            // };
            struct ControlMessageHeader {
                size_t len;
                int level;
                int type;
            };
            Buffer& control = message->msg_control;
            verify(control.size() >= sizeof(ControlMessageHeader));
            ControlMessageHeader header;
            memcpy(&header, control.data(), sizeof(header));
            if(header.type == SCM_RIGHTS) {
                warn(fmt::format("Getting rights with LocalSocket::recvmsg not supported. Overwriting with 0x11111111..."));
                memset(control.data() + sizeof(ControlMessageHeader), 0x33, control.size() - sizeof(ControlMessageHeader));
            }
        }
        ssize_t nbytes = Socket::recvmsg(flags, message);
        if(nbytes >= 0 && message->msg_control.size() > 0) {
            // struct cmsghdr {
            //     size_t cmsg_len;
            //     int cmsg_level;
            //     int cmsg_type;  <-- HERE
            //     unsigned char __cmsg_data[];
            // };
            struct ControlMessageHeader {
                size_t len;
                int level;
                int type;
            };
            Buffer& control = message->msg_control;
            verify(control.size() >= sizeof(ControlMessageHeader));
            ControlMessageHeader header;
            memcpy(&header, control.data(), sizeof(header));
            if(header.type == SCM_RIGHTS) {
                warn(fmt::format("Getting rights with LocalSocket::recvmsg not supported. Overwriting with 0x11111111..."));
                memset(control.data() + sizeof(ControlMessageHeader), 0x33, control.size() - sizeof(ControlMessageHeader));
            }
        }
        return nbytes;
    }

    ssize_t LocalSocket::sendmsg(int flags, const Message& message) const {
        // Check that the ancillary message is not SCM_RIGHTS
        // This would mean sending/receiving fds, which we are not ready for.
        const Buffer& control = message.msg_control;
        if(control.size() > 0) {
            // struct cmsghdr {
            //     size_t cmsg_len;
            //     int cmsg_level;
            //     int cmsg_type;  <-- HERE
            //     unsigned char __cmsg_data[];
            // };
            struct ControlMessageHeader {
                size_t len;
                int level;
                int type;
            };
            verify(control.size() >= sizeof(ControlMessageHeader));
            ControlMessageHeader header;
            memcpy(&header, control.data(), sizeof(header));
            if(header.type == SCM_RIGHTS) {
                warn("Passing rights in LocalSocket::sendmsg not supported");
                return -ENOTSUP;
            }
        }
        return Socket::sendmsg(flags, message);
    }

}