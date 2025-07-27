#include "kernel/linux/fs/shadowfile.h"
#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/openfiledescription.h"
#include "kernel/linux/fs/path.h"
#include "host/host.h"
#include "scopeguard.h"
#include "verify.h"
#include <fmt/core.h>
#include <fmt/color.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

namespace kernel::gnulinux {

    struct ShadowFileHostData {
        struct stat st;
    };

    ShadowFile* ShadowFile::tryCreateAndAdd(FS* fs, Directory* parent, const std::string& name, bool create) {
        std::string pathname;
        if(!parent || parent == fs->root()) {
            pathname = name;
        } else {
            pathname = (parent->path() + "/" + name);
        }
        int fd = ::openat(AT_FDCWD, pathname.c_str(), O_RDONLY | O_CLOEXEC);
        
        ScopeGuard guard([=]() {
            if(fd >= 0) ::close(fd);
        });

        std::string absolutePathname = fs->toAbsolutePathname(pathname);
        auto path = Path::tryCreate(absolutePathname);
        verify(!!path, "Unable to create path");
        Directory* containingDirectory = fs->ensurePathExceptLast(*path);

        if(fd < 0) {
            if(!create) return {};
            std::vector<u8> data;
            auto shadowFile = std::unique_ptr<ShadowFile>(new ShadowFile(fs, containingDirectory, path->last(), std::move(data)));
            return containingDirectory->addFile(std::move(shadowFile));
        } else {
            // figure out size
            struct stat st;
            if(::fstat(fd, &st) < 0) return {};
            
            mode_t fileType = (st.st_mode & S_IFMT);
            if (fileType != S_IFREG && fileType != S_IFLNK) {
                // not a regular file or a symbolic link
                return {};
            }
            verify(fileType != S_IFLNK, "Support for shadow symlinks needed");
            
            // create data vector
            std::vector<u8> data((size_t)st.st_size, 0x0);

            ssize_t nread = ::read(fd, data.data(), data.size());
            if(nread < 0) return {};
            if((size_t)nread != data.size()) return {};

            auto hostData = std::make_unique<ShadowFileHostData>();
            hostData->st = st;

            auto shadowFile = std::unique_ptr<ShadowFile>(new ShadowFile(fs, containingDirectory, path->last(), std::move(data)));
            shadowFile->hostData_ = std::move(hostData);
            return containingDirectory->addFile(std::move(shadowFile));
        }
    }

    std::unique_ptr<ShadowFile> ShadowFile::tryCreate(FS* fs, const std::string& name) {
        std::vector<u8> data;
        auto shadowFile = std::unique_ptr<ShadowFile>(new ShadowFile(fs, nullptr, name, std::move(data)));
        return shadowFile;
    }

    ShadowFile::ShadowFile(FS* fs, Directory* parent, std::string name, std::vector<u8> data) : RegularFile(fs, parent, std::move(name)), data_(std::move(data)) { }

    ShadowFile::~ShadowFile() = default;

    void ShadowFile::close() {
        
    }

    void ShadowFile::truncate(size_t length) {
        data_.resize(length, 0x0);
    }

    ErrnoOrBuffer ShadowFile::read(OpenFileDescription& openFileDescription, size_t count) {
        if(!isReadable()) return ErrnoOrBuffer{-EINVAL};
        off_t offset = openFileDescription.offset();
        if(offset < 0) return ErrnoOrBuffer{-EINVAL};
        size_t bytesRead = (size_t)offset < data_.size() ? std::min(data_.size() - (size_t)offset, count) : 0;
        const u8* beginRead = data_.data() + offset;
        const u8* endRead = beginRead + bytesRead;
        std::vector<u8> buffer(beginRead, endRead);
        return ErrnoOrBuffer(Buffer{std::move(buffer)});
    }

    ssize_t ShadowFile::write(OpenFileDescription& openFileDescription, const u8* buf, size_t count) {
        if(!isWritable()) return -EINVAL;
        if(openFileDescription.statusFlags().test(FS::StatusFlags::APPEND)) openFileDescription.lseek(0, SEEK_END);
        off_t offset = openFileDescription.offset();
        if(offset < 0) return -EINVAL;
        if((size_t)offset + count > data_.size()) {
            data_.resize((size_t)offset + count);
        }
        size_t bytesWritten = count;
        std::memcpy(data_.data() + offset, buf, bytesWritten);
        return (ssize_t)bytesWritten;
    }

    ErrnoOrBuffer ShadowFile::stat() {
        if(!!hostData_) {
            struct stat st = hostData_->st;
            st.st_size = (off_t)data_.size();
            Buffer buf(st);
            return ErrnoOrBuffer(std::move(buf));
        } else {
            struct stat st;
            st.st_dev = 0xcafe; // dummy value
            st.st_ino = 0xbabe; // dummy value
            st.st_mode = (u32)File::Type::IFREG
                    | (u32)File::Mode::IRWXU
                    | (u32)File::Mode::IRWXG
                    | (u32)File::Mode::IRWXO;
            st.st_nlink = 0;
            st.st_uid = (uid_t)Host::getuid();
            st.st_gid = (gid_t)Host::getgid();
            st.st_rdev = 0; // dummy value
            st.st_size = (off_t)data_.size();
            st.st_blksize = 0x200; // dummy value
            st.st_blocks = (__blkcnt_t)((data_.size() + 0x200 - 1) / 0x200);

            Buffer buf(st);
            return ErrnoOrBuffer(std::move(buf));
        }
    }

    ErrnoOrBuffer ShadowFile::statfs() {
        warn("ShadowFile::statfs not implemented");
        return ErrnoOrBuffer(-ENOTSUP);
    }

    ErrnoOrBuffer ShadowFile::statx(unsigned int mask) {
        warn(fmt::format("ShadowFile::statx(path={}, mask={:#x}) not implemented", path(), mask));
        return ErrnoOrBuffer(-ENOTSUP);
    }

    void ShadowFile::advanceInternalOffset(off_t) {
        // nothing to do here
    }

    off_t ShadowFile::lseek(OpenFileDescription& openFileDescription, off_t offset, int whence) {
        off_t baseOffset = 0;
        if(Host::Lseek::isSeekSet(whence)) {
            baseOffset = 0;
        } else if(Host::Lseek::isSeekCur(whence)) {
            baseOffset = openFileDescription.offset();
        } else if(Host::Lseek::isSeekEnd(whence)) {
            baseOffset = (off_t)data_.size();
        } else {
            return -EINVAL;
        }
        if(baseOffset + offset < 0) return -EINVAL;
        return baseOffset + offset;
    }

    ErrnoOrBuffer ShadowFile::getdents64(size_t) {
        verify(false, "implement getdents64 on ShadowFile");
        return ErrnoOrBuffer(-EINVAL);
    }

    std::optional<int> ShadowFile::fcntl(int cmd, int arg) {
        if(cmd == F_DUPFD || cmd == F_DUPFD_CLOEXEC) return {}; // nothing to do here
        if(cmd == F_SETLK) {
            warn(fmt::format("ShadowFile::fcntl(F_SETLK, {}) not implemented", arg));
            return 0;
        }
        if(cmd == F_GETFL) {
            warn(fmt::format("ShadowFile::fcntl(F_GETFL, {}) not implemented", arg));
            return {}; // trust the open file description
        }
        if(cmd == F_ADD_SEALS) {
            warn(fmt::format("ShadowFile::fcntl(F_ADD_SEALS, {}) not implemented", arg));
            return 0;
        }
        verify(false, [&]() { fmt::print("ShadowFile::fcntl({}, {}) not implemented\n", cmd, arg); });
        return -EINVAL;
    }

    ErrnoOrBuffer ShadowFile::ioctl(OpenFileDescription&, Ioctl request, const Buffer&) {
        verify(false, [&]() { fmt::print("ShadowFile::ioctl({:#x}) not implemented", (int)request); });
        return ErrnoOrBuffer(-ENOTSUP);
    }

    int ShadowFile::fallocate(int mode, off_t offset, off_t len) {
        verify(!Host::FallocateMode::isKeepSize(mode), "ShadowFile::fallocate with mode = KeepSize not supported");
        verify(!Host::FallocateMode::isPunchHole(mode), "ShadowFile::fallocate with mode = PunchHole not supported");
        verify(!Host::FallocateMode::isNoHidestale(mode), "ShadowFile::fallocate with mode = NoHidestale not supported");
        verify(!Host::FallocateMode::isCollapseRange(mode), "ShadowFile::fallocate with mode = CollapseRange not supported");
        verify(!Host::FallocateMode::isZeroRange(mode), "ShadowFile::fallocate with mode = ZeroRange not supported");
        verify(!Host::FallocateMode::isInsertRange(mode), "ShadowFile::fallocate with mode = InsertRange not supported");
        verify(!Host::FallocateMode::isUnshareRange(mode), "ShadowFile::fallocate with mode = UnshareRange not supported");
        data_.resize(std::max(data_.size(), (size_t)(offset+len)));
        (void)offset;
        (void)len;
        return 0;
    }

}