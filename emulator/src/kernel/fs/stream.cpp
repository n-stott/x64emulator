#include "kernel/fs/stream.h"
#include "verify.h"
#include <fmt/color.h>
#include <asm/termbits.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <unistd.h>

namespace kernel {

    bool Stream::isReadable() const {
        return type_ == TYPE::IN;
    }

    bool Stream::isWritable() const {
        return type_ == TYPE::OUT || type_ == TYPE::ERR;
    }

    void Stream::close() {
        
    }

    bool Stream::canRead() const {
        if(!isPollable()) return false;
        struct pollfd pfd;
        pfd.fd = (int)type_;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int timeout = 0; // return immediately
        int ret = ::poll(&pfd, 1, timeout);
        if(ret < 0) return false;
        return !!(pfd.revents & POLLIN);
    }

    ErrnoOrBuffer Stream::read(size_t count, off_t) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        std::vector<u8> buffer;
        buffer.resize(count, 0x0);
        ssize_t nbytes = ::read((int)type_, buffer.data(), count);
        if(nbytes < 0) return ErrnoOrBuffer(-errno);
        buffer.resize((size_t)nbytes);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
        
    }

    ssize_t Stream::write(const u8* buf, size_t count, off_t) {
        if(!isWritable()) return -EINVAL;
        return ::write((int)type_, buf, count);
    }

    ErrnoOrBuffer Stream::stat() {
        if(type_ != TYPE::IN
        && type_ != TYPE::OUT
        && type_ != TYPE::ERR) return ErrnoOrBuffer(-EINVAL);
        struct stat st;
        int rc = ::fstat((int)type_, &st);
        if(rc < 0) return ErrnoOrBuffer(-errno);
        std::vector<u8> buf(sizeof(st), 0x0);
        std::memcpy(buf.data(), &st, sizeof(st));
        return ErrnoOrBuffer(Buffer{std::move(buf)});
    }
    
    ErrnoOrBuffer Stream::statfs() {
        verify(false, "Stream::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    off_t Stream::lseek(off_t, int) {
        return -ESPIPE;
    }
    
    ErrnoOrBuffer Stream::getdents64(size_t) {
        return ErrnoOrBuffer(-EINVAL);
    }

    int Stream::fcntl(int cmd, int arg) {
        switch(cmd) {
            case F_GETFD:
            case F_GETFL: {
                int ret = ::fcntl((int)type_, cmd, arg);
                if(ret < 0) return -errno;
                return ret;
            }
            case F_SETFD:
            case F_SETFL: {
                return 0;
            }
            default: break;
        }
        warn(fmt::format("implement missing fcntl {} on HostFile", cmd));
        return -ENOTSUP;
    }

    ErrnoOrBuffer Stream::ioctl(unsigned long request, const Buffer& buf) {
        Buffer buffer(buf);
        switch(request) {
            case TCGETS: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl((int)type_, TCGETS, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TCSETS: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl((int)type_, TCSETS, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case FIOCLEX: {
                int ret = ::ioctl((int)type_, FIOCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case FIONCLEX: {
                int ret = ::ioctl((int)type_, FIONCLEX, nullptr);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case TIOCGWINSZ: {
                verify(buffer.size() == sizeof(struct winsize));
                int ret = ::ioctl((int)type_, TIOCGWINSZ, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TIOCSWINSZ: {
                verify(buffer.size() == sizeof(struct winsize));
                int ret = ::ioctl((int)type_, TIOCSWINSZ, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TCSETSW: {
                verify(buffer.size() == sizeof(struct termios));
                int ret = ::ioctl((int)type_, TCSETSW, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            case TIOCGPGRP: {
                verify(buffer.size() == sizeof(pid_t));
                int ret = ::ioctl((int)type_, TIOCGPGRP, buffer.data());
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(std::move(buffer));
            }
            default: break;
        }
        warn(fmt::format("Stream::ioctl({:#x}) not implemented", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}