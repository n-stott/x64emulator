#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

// Replay of test_sdl_sound

int main(int argc, char* argv[]) {
    // socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0) = 6
    int fd = ::socket(AF_UNIX, SOCK_STREAM|SOCK_CLOEXEC, 0);
    if(fd < 0) {
        perror("socket");
        return 1;
    }

    // fcntl(6, F_GETFD)                       = 0x1 (flags FD_CLOEXEC)
    if(::fcntl(fd, F_GETFD) != FD_CLOEXEC) {
        perror("fcntl(F_GETFD)");
        return 1;
    }

    // setsockopt(6, SOL_SOCKET, SO_PRIORITY, [6], 4) = 0
    int priority = 6;
    if(::setsockopt(fd, SOL_SOCKET, SO_PRIORITY, &priority, sizeof(priority)) < 0) {
        perror("setsockopt");
        return 1;
    }

    // fcntl(6, F_GETFL)                       = 0x2 (flags O_RDWR)
    if(::fcntl(fd, F_GETFL) != O_RDWR) {
        perror("fcntl(F_GETFL)");
        return 1;
    }

    // fcntl(6, F_SETFL, O_RDWR|O_NONBLOCK)    = 0
    if(::fcntl(fd, F_SETFL, O_RDWR|O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL)");
        return 1;
    }

    // connect(6, {sa_family=AF_UNIX, sun_path="/run/user/1000/pulse/native"}, 110) = 0
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    const char* path = "/run/user/1000/pulse/native";
    ::memset(addr.sun_path, 0, sizeof(addr.sun_path));
    ::memcpy(addr.sun_path, path, strlen(path));
    if(::connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("connect");
        return 1;
    }

    // getsockopt(6, SOL_SOCKET, SO_ERROR, [0], [4]) = 0
    int error = 0;
    socklen_t error_size = sizeof(error);
    if(::getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &error_size) < 0) {
        perror("getsockopt");
        return 1;
    }

    // fcntl(6, F_GETFL)                       = 0x802 (flags O_RDWR|O_NONBLOCK)
    if(fcntl(fd, F_GETFL) != (O_RDWR|O_NONBLOCK)) {
        perror("fcntl(F_GETFL)");
        return 1;
    }

    // setsockopt(6, SOL_SOCKET, SO_RCVBUF, [65472], 4) = 0 // unsupported for unix domain sockets, so ignore
    // setsockopt(6, SOL_SOCKET, SO_SNDBUF, [65472], 4) = 0 // unsupported for unix domain sockets, so ignore

    // getsockname(6, {sa_family=AF_UNIX}, [256->2]) = 0
    // struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    ::memset(addr.sun_path, 0, sizeof(addr.sun_path));
    socklen_t addrlen = sizeof(addr);
    if(::getsockname(fd, (sockaddr*)&addr, &addrlen) < 0) {
        perror("getsockname");
        return 1;
    }

    // setsockopt(6, SOL_SOCKET, SO_PASSCRED, [1], 4) = 0
    int passcred = 1;
    if(::setsockopt(fd, SOL_SOCKET, SO_PASSCRED, &passcred, sizeof(passcred)) < 0) {
        perror("setsockopt");
        return 1;
    }

    // sendmsg(6, 
    //    {
    //        msg_name=NULL, 
    //        msg_namelen=0, 
    //        msg_iov=[{iov_base="\0\0\1\24\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0", iov_len=20}],
    //        msg_iovlen=1,
    //        msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=157465, uid=1000, gid=1000}}],
    //        msg_controllen=32,
    //        msg_flags=0
    //    }, 
    //    MSG_NOSIGNAL) = 20
    msghdr sendmsg1_msg;
    sendmsg1_msg.msg_name = nullptr;
    sendmsg1_msg.msg_namelen = 0;
    iovec sendmsg1_iov;
    uint8_t sendmsg1_data[20] = {
        0x00, 0x00, 0x01, 0x14, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    sendmsg1_iov.iov_base = &sendmsg1_data;
    sendmsg1_iov.iov_len = 20;
    sendmsg1_msg.msg_iov = &sendmsg1_iov;
    sendmsg1_msg.msg_iovlen = 1;
    alignas(void*) char sendmsg1_control_buf[32];
    struct cmsghdr* sendmsg1_control_ptr = reinterpret_cast<struct cmsghdr*>(&sendmsg1_control_buf);
    sendmsg1_control_ptr->cmsg_len = 28;
    sendmsg1_control_ptr->cmsg_level = SOL_SOCKET;
    sendmsg1_control_ptr->cmsg_type = SCM_CREDENTIALS;
    struct ucred sendmsg1_creds;
    sendmsg1_creds.pid = ::getpid();
    sendmsg1_creds.uid = ::getuid();
    sendmsg1_creds.gid = ::getgid();
    memcpy(sendmsg1_control_ptr->__cmsg_data, &sendmsg1_creds, sizeof(sendmsg1_creds));
    sendmsg1_msg.msg_control = sendmsg1_control_ptr;
    sendmsg1_msg.msg_controllen = sizeof(sendmsg1_control_buf);
    sendmsg1_msg.msg_flags = 0;
    if(::sendmsg(fd, &sendmsg1_msg, MSG_NOSIGNAL) != 20) {
        perror("sendmsg");
        return 1;
    }

    // ppoll([{fd=6, events=POLLIN|POLLOUT}], 1, {tv_sec=29, tv_nsec=999825000}, NULL, 8) = 1 ([{fd=6, revents=POLLOUT}])
    struct pollfd ppoll1_pfd;
    ppoll1_pfd.fd = fd;
    ppoll1_pfd.events = POLLIN|POLLOUT;
    ppoll1_pfd.revents = 0;
    struct timespec ppoll1_timeout;
    ppoll1_timeout.tv_sec = 30;
    ppoll1_timeout.tv_nsec = 0;
    if(::ppoll(&ppoll1_pfd, 1, &ppoll1_timeout, nullptr) != 1) {
        perror("ppoll");
        return 1;
    } else {
        assert(ppoll1_pfd.revents == POLLOUT);
    }

    // sendto(6, "L\0\0\0\10L\0\0\0\0L\300\0\0\"x\0\0\1\0\213\257j\226I>\243gx\244\36v"..., 276, MSG_NOSIGNAL, NULL, 0) = 276
    uint8_t sendto1_data[276] = {
        0x4c, 0x00, 0x00, 0x00, 0x08, 0x4c, 0x00, 0x00,
        0x00, 0x00, 0x4c, 0xc0, 0x00, 0x00, 0x22, 0x78,
        0x00, 0x00, 0x01, 0x00, 0x8b, 0xaf, 0x6a, 0x96,
        0x49, 0x3e, 0xa3, 0x67, 0x78, 0xa4, 0x1e, 0x76,
        0xc2, 0x97, 0x6e, 0x06, 0xe8, 0x77, 0x22, 0x28,
        0x73, 0x4d, 0x66, 0x1a, 0x47, 0x2d, 0xaf, 0x16,
        0xb8, 0xa1, 0x10, 0x39, 0xa2, 0x84, 0x05, 0x37,
        0xe1, 0xf4, 0x7b, 0x92, 0xc5, 0x77, 0x3a, 0x73,
        0x3e, 0x12, 0x75, 0x67, 0x0b, 0x63, 0x51, 0xc4,
        0xdf, 0x02, 0x48, 0x20, 0x72, 0x3f, 0x3d, 0x4a,
        0xee, 0xb5, 0xb2, 0xdb, 0xf7, 0x22, 0xac, 0xa7,
        0xa2, 0x78, 0xfa, 0xc7, 0x4d, 0x7e, 0xf5, 0x8d,
        0x1c, 0x4c, 0x6d, 0xe1, 0x84, 0x02, 0x5a, 0x42,
        0x09, 0x04, 0xb1, 0x22, 0x5b, 0x98, 0x7d, 0x92,
        0x6f, 0xbe, 0xa3, 0x31, 0x9e, 0xf0, 0x60, 0xf3,
        0x7e, 0x75, 0x75, 0xcc, 0xc6, 0x11, 0x02, 0x90,
        0xe3, 0x1a, 0xbf, 0x01, 0x40, 0xd7, 0xfb, 0x92,
        0xdf, 0x6e, 0xa7, 0x49, 0xa0, 0x07, 0x09, 0xa9,
        0x7c, 0x5f, 0x0c, 0x31, 0x56, 0x31, 0xc8, 0x6a,
        0xbc, 0x0c, 0xb6, 0x33, 0xbc, 0xd6, 0xee, 0x3d,
        0x6b, 0x00, 0x9f, 0xe2, 0x7c, 0xd8, 0x15, 0x5a,
        0xd0, 0xf9, 0xcd, 0x0e, 0x67, 0x08, 0x95, 0xa8,
        0xe9, 0x66, 0x70, 0x54, 0xc3, 0x46, 0xb4, 0xd5,
        0x41, 0x5f, 0x1b, 0xe1, 0x87, 0x6a, 0x9f, 0x58,
        0x5c, 0x78, 0x66, 0xc2, 0x5f, 0x91, 0x08, 0xbc,
        0x49, 0xfe, 0xf5, 0x61, 0xec, 0x7b, 0xdd, 0x8a,
        0x05, 0xc4, 0xc1, 0xe0, 0x63, 0xda, 0xde, 0xa6,
        0x28, 0xaf, 0x94, 0x29, 0xc9, 0x63, 0x18, 0xe3,
        0xf8, 0x8a, 0x6b, 0x3a, 0x1f, 0xb4, 0x97, 0xaa,
        0x1a, 0x27, 0x1c, 0x18, 0x1b, 0x86, 0xf4, 0x38,
        0xb2, 0x41, 0xb0, 0xac, 0x25, 0x5d, 0x57, 0xdb,
        0xe4, 0x1b, 0x3c, 0xfb, 0x4c, 0xc8, 0x50, 0xac,
        0x5d, 0xb3, 0xf7, 0x9e, 0x17, 0xa4, 0x8a, 0x05,
        0x19, 0x8c, 0xd2, 0x33, 0xf8, 0x3b, 0x4f, 0x7f,
        0xf2, 0xc6, 0xec, 0x37
    };
    if(::sendto(fd, &sendto1_data, 276, MSG_NOSIGNAL, NULL, 0) != 276) {
        perror("sendto");
        return 1;
    }

    // ppoll([{fd=6, events=POLLIN}], 1, {tv_sec=29, tv_nsec=999825000}, NULL, 8) = 1 ([{fd=6, revents=POLLIN}])
    struct pollfd ppoll2_pfd;
    ppoll2_pfd.fd = fd;
    ppoll2_pfd.events = POLLIN;
    ppoll2_pfd.revents = 0;
    struct timespec ppoll2_timeout;
    ppoll2_timeout.tv_sec = 30;
    ppoll2_timeout.tv_nsec = 0;
    if(::ppoll(&ppoll2_pfd, 1, &ppoll2_timeout, nullptr) != 1) {
        perror("ppoll");
        return 1;
    } else {
        assert(ppoll2_pfd.revents == POLLIN);
    }

    // recvmsg(6,
    //    {
    //        msg_name=NULL, 
    //        msg_namelen=0, 
    //        msg_iov=[{iov_base="\0\0\0\17\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0", iov_len=20}], 
    //        msg_iovlen=1, 
    //        msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], 
    //        msg_controllen=32, 
    //        msg_flags=0
    //    }
    //    , 0) = 20
    msghdr recvmsg1_msg;
    recvmsg1_msg.msg_name = nullptr;
    recvmsg1_msg.msg_namelen = 0;
    iovec recvmsg1_iov;
    uint8_t recvmsg1_data[20] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    recvmsg1_iov.iov_base = &recvmsg1_data;
    recvmsg1_iov.iov_len = 20;
    recvmsg1_msg.msg_iov = &recvmsg1_iov;
    recvmsg1_msg.msg_iovlen = 1;
    alignas(void*) char recvmsg1_control_buf[32];
    struct cmsghdr* recvmsg1_control_ptr = reinterpret_cast<struct cmsghdr*>(&recvmsg1_control_buf);
    recvmsg1_control_ptr->cmsg_len = 28;
    recvmsg1_control_ptr->cmsg_level = SOL_SOCKET;
    recvmsg1_control_ptr->cmsg_type = SCM_CREDENTIALS;
    struct ucred recvmsg1_creds;
    recvmsg1_creds.pid = ::getpid();
    recvmsg1_creds.uid = ::getuid();
    recvmsg1_creds.gid = ::getgid();
    memcpy(recvmsg1_control_ptr->__cmsg_data, &recvmsg1_creds, sizeof(recvmsg1_creds));
    recvmsg1_msg.msg_control = recvmsg1_control_ptr;
    recvmsg1_msg.msg_controllen = sizeof(recvmsg1_control_buf);
    recvmsg1_msg.msg_flags = 0;
    if(::recvmsg(fd, &recvmsg1_msg, 0) != 20) {
        perror("recvmsg1");
        return 1;
    }

    // ppoll([{fd=6, events=POLLIN}], 1, {tv_sec=29, tv_nsec=999825000}, NULL, 8) = 1 ([{fd=6, revents=POLLIN}])
    struct pollfd ppoll3_pfd;
    ppoll3_pfd.fd = fd;
    ppoll3_pfd.events = POLLIN;
    ppoll3_pfd.revents = 0;
    struct timespec ppoll3_timeout;
    ppoll3_timeout.tv_sec = 30;
    ppoll3_timeout.tv_nsec = 0;
    if(::ppoll(&ppoll3_pfd, 1, &ppoll3_timeout, nullptr) != 1) {
        perror("ppoll");
        return 1;
    } else {
        assert(ppoll3_pfd.revents == POLLIN);
    }

    // recvmsg(6, 
    //    {
    //        msg_name=NULL, 
    //        msg_namelen=0, 
    //        msg_iov=[{iov_base="L\0\0\0\2L\0\0\0\0L\300\0\0\"", iov_len=15}], 
    //        msg_iovlen=1, 
    //        msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], 
    //        msg_controllen=32, 
    //        msg_flags=0
    //    }
    //    , 0) = 15
    msghdr recvmsg2_msg;
    recvmsg2_msg.msg_name = nullptr;
    recvmsg2_msg.msg_namelen = 0;
    iovec recvmsg2_iov;
    uint8_t recvmsg2_data[15] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    };
    recvmsg2_iov.iov_base = &recvmsg2_data;
    recvmsg2_iov.iov_len = 15;
    recvmsg2_msg.msg_iov = &recvmsg2_iov;
    recvmsg2_msg.msg_iovlen = 1;
    alignas(void*) char recvmsg2_control_buf[32];
    struct cmsghdr* recvmsg2_control_ptr = reinterpret_cast<struct cmsghdr*>(&recvmsg2_control_buf);
    recvmsg2_control_ptr->cmsg_len = 28;
    recvmsg2_control_ptr->cmsg_level = SOL_SOCKET;
    recvmsg2_control_ptr->cmsg_type = SCM_CREDENTIALS;
    struct ucred recvmsg2_creds;
    recvmsg2_creds.pid = ::getpid();
    recvmsg2_creds.uid = ::getuid();
    recvmsg2_creds.gid = ::getgid();
    memcpy(recvmsg2_control_ptr->__cmsg_data, &recvmsg2_creds, sizeof(recvmsg2_creds));
    recvmsg2_msg.msg_control = recvmsg2_control_ptr;
    recvmsg2_msg.msg_controllen = sizeof(recvmsg2_control_buf);
    recvmsg2_msg.msg_flags = 0;
    if(::recvmsg(fd, &recvmsg2_msg, 0) != 15) {
        perror("recvmsg2");
        return 1;
    }

    // The next call passes another file descriptor.
    // We want to forbid this !
    int fds[2];
    if(::pipe2(fds, O_NONBLOCK | O_CLOEXEC) < 0) {
        perror("pipe2");
        return 1;
    }

    if(argc == 1) return 0;

    // sendmsg(6,
    //    {
    //        msg_name=NULL, 
    //        msg_namelen=0, 
    //        msg_iov=[{iov_base="\0\0\0\17\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0L\0\0\0gL\377\377\377\377L|"..., iov_len=35}],
    //        msg_iovlen=1, 
    //        msg_control=[{cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, cmsg_data=[5]}], 
    //        msg_controllen=24,
    //        msg_flags=0
    //    }
    //    , MSG_NOSIGNAL) = 35
    msghdr sendmsg2_msg;
    sendmsg2_msg.msg_name = nullptr;
    sendmsg2_msg.msg_namelen = 0;
    iovec sendmsg2_iov;
    uint8_t sendmsg2_data[35] = {
        0x00, 0x00, 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x4c, 0x00, 0x00, 0x00,
        0x67, 0x4c, 0xff, 0xff, 0xff, 0xff, 0x4c, 0x99,
        0xd4, 0x73, 0x48
    };
    sendmsg2_iov.iov_base = &sendmsg2_data;
    sendmsg2_iov.iov_len = 35;
    sendmsg2_msg.msg_iov = &sendmsg2_iov;
    sendmsg2_msg.msg_iovlen = 1;
    alignas(void*) char sendmsg2_control_buf[24];
    struct cmsghdr* sendmsg2_control_ptr = reinterpret_cast<struct cmsghdr*>(&sendmsg2_control_buf);
    sendmsg2_control_ptr->cmsg_len = 20;
    sendmsg2_control_ptr->cmsg_level = SOL_SOCKET;
    sendmsg2_control_ptr->cmsg_type = SCM_RIGHTS;
    int passedFd = fds[0];
    memcpy(sendmsg2_control_ptr->__cmsg_data, &passedFd, sizeof(passedFd));
    sendmsg2_msg.msg_control = sendmsg2_control_ptr;
    sendmsg2_msg.msg_controllen = sizeof(sendmsg2_control_buf);
    sendmsg2_msg.msg_flags = 0;
    if(::sendmsg(fd, &sendmsg2_msg, MSG_NOSIGNAL) != 35) {
        perror("sendmsg2");
        return 1;
    }



// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="\0\0\0\17\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0", iov_len=20}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}, {cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, cmsg_data=[5]}], msg_controllen=56, msg_flags=0}, 0) = 20
// sendto(6, "\0\0\1{\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0", 20, MSG_NOSIGNAL, NULL, 0) = 20
// sendto(6, "L\0\0\0\tL\0\0\0\1Ptapplication.process."..., 379, MSG_NOSIGNAL, NULL, 0) = 379
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="L\0\0\0gL\377\377\377\377L/\370\251\373", iov_len=15}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], msg_controllen=32, msg_flags=0}, 0) = 15
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="\0\0\0\17\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0", iov_len=20}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}, {cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, cmsg_data=[5]}], msg_controllen=56, msg_flags=0}, 0) = 20
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="L\0\0\0gL\377\377\377\377Lj\265m\356", iov_len=15}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], msg_controllen=32, msg_flags=0}, 0) = 15
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="\0\0\0\n\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0", iov_len=20}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}, {cmsg_len=24, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, cmsg_data=[5, 7]}], msg_controllen=56, msg_flags=0}, 0) = 20
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="L\0\0\0eL\317:\1\340", iov_len=10}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], msg_controllen=32, msg_flags=0}, 0) = 10
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="\0\0\0\20\0\0\0\0\0\0\0\0\0\0\0\0\240\200\0\0", iov_len=20}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], msg_controllen=32, msg_flags=0}, 0) = 20
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="\0\0/\200j\265m\356\0\0\0@\0\0\377\300", iov_len=16}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], msg_controllen=32, msg_flags=0}, 0) = 16
// sendto(6, "\0\0\0\n\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0L\0\0\0eL\317:\1\340", 30, MSG_NOSIGNAL, NULL, 0) = 30
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="\0\0\0\17\377\377\377\377\0\0\0\0\0\0\0\0\0\0\0\0", iov_len=20}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], msg_controllen=32, msg_flags=0}, 0) = 20
// recvmsg(6, {msg_name=NULL, msg_namelen=0, msg_iov=[{iov_base="L\0\0\0\2L\0\0\0\1L\0\0\0\367", iov_len=15}], msg_iovlen=1, msg_control=[{cmsg_len=28, cmsg_level=SOL_SOCKET, cmsg_type=SCM_CREDENTIALS, cmsg_data={pid=2819, uid=1000, gid=1000}}], msg_controllen=32, msg_flags=0}, 0) = 15



}