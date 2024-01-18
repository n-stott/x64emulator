#ifndef HOST
#define HOST

#include "utils/utils.h"
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

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

    static std::optional<std::vector<u8>> read(FD, size_t count);
    static ssize_t write(FD, const u8* data, size_t count);
    static int close(FD);

    static std::optional<std::vector<u8>> stat(const std::string& path);
    static std::optional<std::vector<u8>> fstat(FD fd);
    static off_t lseek(FD fd, off_t offset, int whence);
    static FD openat(FD dirfd, const std::string& pathname, int flags, mode_t mode);
    static int access(const std::string& path, int mode);

    static std::optional<std::vector<u8>> readlink(const std::string& path, size_t count);
    static std::optional<std::vector<u8>> uname();

    static std::optional<std::vector<u8>> sysinfo();
    
    static std::optional<std::vector<u8>> getcwd(size_t size);

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