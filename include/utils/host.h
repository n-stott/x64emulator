#ifndef HOST_H
#define HOST_H

#include "utils/utils.h"
#include <cstring>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

class Buffer {
public:
    explicit Buffer(std::vector<u8> buf) : data_(std::move(buf)) { }

    template<typename T>
    explicit Buffer(const T& val) {
        data_.resize(sizeof(T));
        std::memcpy(data_.data(), &val, sizeof(T));
    }

    size_t size() const { return data_.size(); }

    const u8* data() const { return data_.data(); }
    u8* data() { return data_.data(); }

private:
    std::vector<u8> data_;
};

template<typename Value>
class ErrnoOr {
public:
    explicit ErrnoOr(int err) : data_(err) { }
    explicit ErrnoOr(Value val) : data_(std::move(val)) { }

    bool isError() const {
        return std::holds_alternative<int>(data_);
    }

    int errorOr(int value) const {
        if(isError()) return std::get<0>(data_);
        return value;
    }

    template<typename T, typename Func>
    T errorOrWith(Func func) {
        static_assert(std::is_integral_v<T>);
        if(isError()) return (T)std::get<0>(data_);
        const Value& value = std::get<Value>(data_);
        return func(value);
    }

private:
    std::variant<int, Value> data_;
};

using ErrnoOrBuffer = ErrnoOr<Buffer>;

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

    static ErrnoOrBuffer read(FD, size_t count);
    static ErrnoOrBuffer pread64(FD, size_t count, off_t offset);
    static ssize_t write(FD, const u8* data, size_t count);
    static int close(FD);
    static FD dup(FD);

    static ErrnoOrBuffer stat(const std::string& path);
    static ErrnoOrBuffer fstat(FD fd);
    static ErrnoOrBuffer lstat(const std::string& path);
    static off_t lseek(FD fd, off_t offset, int whence);
    static FD openat(FD dirfd, const std::string& pathname, int flags, mode_t mode);
    static int access(const std::string& path, int mode);

    static ErrnoOrBuffer statfs(const std::string& path);
    static ErrnoOrBuffer statx(FD dirfd, const std::string& path, int flags, unsigned int mask);

    static ErrnoOrBuffer getxattr(const std::string& path, const std::string& name, size_t size);
    static ErrnoOrBuffer lgetxattr(const std::string& path, const std::string& name, size_t size);

    static int getfd(FD fd);
    static int setfd(FD fd, int flag);

    static FD socket(int domain, int type, int protocol);

    static ErrnoOrBuffer readlink(const std::string& path, size_t count);
    static ErrnoOrBuffer uname();

    static ErrnoOrBuffer tcgetattr(FD fd);
    static ErrnoOrBuffer tiocgwinsz(FD fd);

    static ErrnoOrBuffer sysinfo();
    static int getuid();
    static int getgid();
    static int geteuid();
    static int getegid();
    
    static ErrnoOrBuffer getcwd(size_t size);
    static ErrnoOrBuffer getdents64(FD fd, size_t count);
    static int chdir(const std::string& path);

    static ErrnoOrBuffer clock_gettime(clockid_t clockid);
    static time_t time();
    static ErrnoOr<std::pair<Buffer, Buffer>> gettimeofday();

    static std::vector<u8> readFromFile(FD fd, size_t length, off_t offset);

    static int prlimit64(pid_t pid, int resource, const std::vector<u8>* new_limit, std::vector<u8>* old_limit);

    static int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timeval* timeout);
    static int pselect6(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, timespec* timeout, const sigset_t* sigmask);

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
    Host();
    std::unordered_map<int, std::string> openFiles_;

    static bool isStdout(FD fd);
    static bool isStderr(FD fd);
};

#endif