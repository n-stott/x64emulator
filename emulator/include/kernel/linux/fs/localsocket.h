#ifndef LOCALSOCKET_H
#define LOCALSOCKET_H

#include "kernel/linux/fs/socket.h"
#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include <fmt/core.h>
#include <memory> 

namespace kernel {

    class LocalSocket : public Socket {
    public:
        static std::unique_ptr<LocalSocket> tryCreate(FS* fs, int domain, int type, int protocol);

        ssize_t recvmsg(int flags, Message* message) const override;
        ssize_t sendmsg(int flags, const Message& message) const override;

        std::string className() const override {
            return fmt::format("LocalSocket(realfd={})", hostFd_);
        }

    private:
        LocalSocket(FS* fs, int fd, int domain, int type, int protocol);
    };

}


#endif