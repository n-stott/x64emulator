#include "kernel/fs/stream.h"
#include "verify.h"
#include <fmt/color.h>
#include <asm/termbits.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
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

    ErrnoOrBuffer Stream::ioctl(unsigned long request, const Buffer& buffer) {
        switch(request) {
            case TCGETS: {
                struct termios ts;
                int ret = ::ioctl((int)type_, TCGETS, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                std::vector<u8> buffer;
                buffer.resize(sizeof(ts), 0x0);
                std::memcpy(buffer.data(), &ts, sizeof(ts));
                return ErrnoOrBuffer(Buffer{std::move(buffer)});
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
                struct winsize ws;
                int ret = ::ioctl((int)type_, TIOCGWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                std::vector<u8> buffer;
                buffer.resize(sizeof(ws), 0x0);
                std::memcpy(buffer.data(), &ws, sizeof(ws));
                return ErrnoOrBuffer(Buffer{std::move(buffer)});
            }
            case TIOCSWINSZ: {
                struct winsize ws;
                std::memcpy(&ws, buffer.data(), sizeof(ws));
                int ret = ::ioctl((int)type_, TIOCSWINSZ, &ws);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            case TCSETSW: {
                struct termios ts;
                std::memcpy(&ts, buffer.data(), sizeof(ts));
                int ret = ::ioctl((int)type_, TCSETSW, &ts);
                if(ret < 0) return ErrnoOrBuffer(-errno);
                return ErrnoOrBuffer(Buffer{});
            }
            default: break;
        }
        warn(fmt::format("ioctl({:#x}) not implemented", request));
        return ErrnoOrBuffer(-ENOTSUP);
    }

}