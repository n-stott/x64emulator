#ifndef HOST_H
#define HOST_H

#include "utils/utils.h"
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class BufferOrErrno {
public:
    explicit BufferOrErrno(int err);
    explicit BufferOrErrno(std::vector<u8> buf);

    bool isError() const;
    bool isBuffer() const;

    int errorOr(int value) const;

    template<typename T, typename Func>
    T errorOrWith(Func func) {
        if(isError()) return (T)std::get<int>(data_);
        const auto& buffer = std::get<std::vector<u8>>(data_);
        return func(buffer);
    }

private:
    std::variant<int, std::vector<u8>> data_;
};

class Host {
public:
    // math
    static f80 round(f80);

    // cpu
    struct CPUID {
        u32 a, b, c, d;
    };
    static CPUID cpuid(u32 a);

    struct XGETBV {
        u32 a, d;
    };
    static XGETBV xgetbv(u32 c);

    // syscalls
    struct FD {
        int fd;
    };

    static BufferOrErrno read(FD, size_t count);
    static ssize_t write(FD, const u8* data, size_t count);
    static int close(FD);
    static FD dup(FD);

    static BufferOrErrno stat(const std::string& path);
    static BufferOrErrno fstat(FD fd);
    static BufferOrErrno lstat(const std::string& path);
    static off_t lseek(FD fd, off_t offset, int whence);
    static FD openat(FD dirfd, const std::string& pathname, int flags, mode_t mode);
    static int access(const std::string& path, int mode);

    static int getfd(FD fd);
    static int setfd(FD fd, int flag);

    static BufferOrErrno readlink(const std::string& path, size_t count);
    static BufferOrErrno uname();

    static BufferOrErrno tcgetattr(FD fd);

    static BufferOrErrno sysinfo();
    static int getuid();
    static int getgid();
    static int geteuid();
    static int getegid();
    
    static BufferOrErrno getcwd(size_t size);
    static BufferOrErrno getdents64(FD fd, size_t count);

    static BufferOrErrno clock_gettime(clockid_t clockid);
    static time_t time();

    static std::vector<u8> readFromFile(FD fd, size_t length, off_t offset);

    static int prlimit64(pid_t pid, int resource, const std::vector<u8>* new_limit, std::vector<u8>* old_limit);

    enum class AUX_TYPE {
        NIL,
        ENTRYPOINT,
        PROGRAM_HEADERS,
        PROGRAM_HEADER_ENTRY_SIZE,
        PROGRAM_HEADER_COUNT,
        RANDOM_VALUE_ADDRESS,
        PLATFORM_STRING_ADDRESS,
        VDSO_ADDRESS,
        EXEC_FILE_DESCRIPTOR,
        EXEC_PATH_NAME,
        UID,
        GID,
        EUID,
        EGID,
    };

    struct AuxVal {
        u64 type;
        u64 value;
    };

    static std::optional<AuxVal> getauxval(AUX_TYPE type);

    // singleton
    static Host& the();
    
    std::optional<std::string> filename(FD fd) const;

private:
    std::unordered_map<int, std::string> openFiles_;
};

#endif