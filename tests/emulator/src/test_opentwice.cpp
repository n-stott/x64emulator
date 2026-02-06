#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main() {

    int fd0 = ::open("opentwice_testfile.txt", O_RDONLY, 0777);
    
    if(fd0 < 0) {
        perror("open 0");
        return 1;
    }

    int fd1 = ::open("opentwice_testfile.txt", O_RDONLY, 0777);
    
    if(fd1 < 0) {
        perror("open 1");
        return 1;
    }

    auto on_close = [&]() {
        ::close(fd0);
        ::close(fd1);
    };
    
    int ret = 0;

    const size_t wbuf_size = 0x100;
    char wbuf[wbuf_size];
    ::memset(wbuf, 0xff, wbuf_size);
    ret = ::read(fd0, wbuf, wbuf_size);
    if(ret < 0) {
        perror("read(fd0, wbuf, wbuf_size)");
        on_close();
        return 1;
    }
    if(ret != 0x100) {
        printf("Should have read 0x100 bytes but read %d\n", ret);
        on_close();
        return 1;
    }
    {
        off_t off0 = ::lseek(fd0, 0, SEEK_CUR);
        off_t off1 = ::lseek(fd1, 0, SEEK_CUR);
        printf("off0 = %d, off1 = %d\n", (int)off0, (int)off1);
        if(off0 != 0x100) return 1;
        if(off1 != 0) return 1;
    }

    const size_t rbuf_size = 0x100;
    char rbuf[rbuf_size];
    ::memset(rbuf, 0xff, rbuf_size);
    ret = ::read(fd1, rbuf, rbuf_size);
    if(ret < 0) {
        perror("read");
        on_close();
        return 1;
    }
    if(ret != 0x100) {
        printf("Should have read 0x100 bytes but read %d\n", ret);
        on_close();
        return 1;
    }
    {
        off_t off0 = ::lseek(fd0, 0, SEEK_CUR);
        off_t off1 = ::lseek(fd1, 0, SEEK_CUR);
        printf("off0 = %d, off1 = %d\n", (int)off0, (int)off1);
        if(off0 != 0x100) return 1;
        if(off1 != 0x100) return 1;
    }

    on_close();
    return 0;
}