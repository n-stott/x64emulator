#ifndef FILE_H
#define FILE_H

#include "kernel/fs/fsobject.h"
#include "kernel/utils/buffer.h"
#include "kernel/utils/erroror.h"
#include <sys/types.h>
#include <string>

namespace kernel {

    class Directory;

    class File : public FsObject {
    public:
        explicit File(FS* fs) : FsObject(fs), parent_(nullptr), name_("_anonymous_file_") { }
        explicit File(FS* fs, Directory* parent, std::string name) : FsObject(fs), parent_(parent), name_(std::move(name)) { }

        virtual bool isShadow() const { return false; }

        virtual std::string path() const;
        virtual std::string name() const { return name_; }

        virtual void open() { }

        virtual bool isReadable() const = 0;
        virtual bool isWritable() const = 0;

        virtual bool canRead() const = 0;
        virtual bool canWrite() const = 0;

        virtual ErrnoOrBuffer read(size_t count, off_t offset) = 0;
        virtual ssize_t write(const u8* buf, size_t count, off_t offset) = 0;

        virtual off_t lseek(off_t offset, int whence) = 0;

        enum class Mode : u32 {
            ISUID = 04000, // set-user-ID bit (see execve(2))
            ISGID = 02000, // set-group-ID bit (see below)
            ISVTX = 01000, // sticky bit (see below)

            IRWXU = 00700, // owner has read, write, and execute permission
            IRUSR = 00400, // owner has read permission
            IWUSR = 00200, // owner has write permission
            IXUSR = 00100, // owner has execute permission

            IRWXG = 00070, // group has read, write, and execute permission
            IRGRP = 00040, // group has read permission
            IWGRP = 00020, // group has write permission
            IXGRP = 00010, // group has execute permission

            IRWXO = 00007, // others  (not  in group) have read, write, and execute permission
            IROTH = 00004, // others have read permission
            IWOTH = 00002, // others have write permission
            IXOTH = 00001, // others have execute permission
        };

        enum class Type : u32 {
            IFMT   =  0170000, // bit mask for the file type bit field
            IFSOCK =  0140000, // socket
            IFLNK  =  0120000, // symbolic link
            IFREG  =  0100000, // regular file
            IFBLK  =  0060000, // block device
            IFDIR  =  0040000, // directory
            IFCHR  =  0020000, // character device
            IFIFO  =  0010000, // FIFO
        };

        virtual ErrnoOrBuffer stat() = 0;
        virtual ErrnoOrBuffer statfs() = 0;
        
        virtual ErrnoOrBuffer getdents64(size_t count) = 0;

        virtual int fcntl(int cmd, int arg) = 0;
        virtual ErrnoOrBuffer ioctl(unsigned long request, const Buffer& buffer) = 0;
        virtual ErrnoOrBuffer ioctlWithBufferSizeGuess(unsigned long request, const Buffer& buffer);

        virtual std::string className() const = 0;

    private:
        Directory* parent_ { nullptr };
        std::string name_;
    };

}

#endif