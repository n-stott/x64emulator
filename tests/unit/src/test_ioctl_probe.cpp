#include "kernel/host.h"
#include "utils.h"
#include <array>
#include <sys/ioctl.h>
#include <unistd.h>

int main() {
    std::array<u8, 36> buf;
    std::fill(buf.begin(), buf.end(), 0);
    std::optional<ssize_t> size = kernel::Host::tryGuessIoctlBufferSize(kernel::Host::FD{STDIN_FILENO}, TIOCGWINSZ, buf.data(), buf.size());

    if(!size) {
        printf("Unable to find size\n");
        return 1;
    }

    if(size < 0) {
        printf("Size must be nonnegative but got %ld\n", size.value());
        return 1;
    }

    if(size != sizeof(winsize)) {
        printf("Expected size %lu but found %ld\n", sizeof(winsize), size.value());
        return 1;
    }
    
    return 0;
}