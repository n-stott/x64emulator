#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

int main() {

    int fd = ::open("lseek_test_file.txt", O_CREAT | O_TRUNC | O_RDWR, 0777);
    
    if(fd < 0) {
        perror("open");
        return 1;
    }

    auto on_close = [&]() {
        ::close(fd);
        ::unlink("lseek_test_file.txt");
    };
    
    int ret = 0;
    ret = ::lseek(fd, 0x1000, SEEK_SET);
    if(ret < 0) {
        perror("lseek(fd, 0x1000, SEEK_SET)");
        on_close();
        return 1;
    }

    const size_t buf_size = 0x100;
    char buf[buf_size];
    ::memset(buf, 0xff, buf_size);
    ret = ::read(fd, buf, buf_size);
    if(ret < 0) {
        perror("read");
        on_close();
        return 1;
    }
    if(ret > 0) {
        printf("Should have read 0 bytes but read %d\n", ret);
        on_close();
        return 1;
    }

    ret = ::lseek(fd, 0, SEEK_CUR);
    if(ret < 0) {
        perror("lseek(fd, 0, SEEK_CUR)");
        on_close();
        return 1;
    }
    if(ret != 0x1000) {
        printf("Should have advanced 0x1000 bytes but only advanced  %d\n", ret);
        on_close();
        return 1;
    }

    ret = ::ftruncate(fd, 0x1100);
    if(ret < 0) {
        perror("ftruncate");
        on_close();
        return 1;
    }

    ret = ::read(fd, buf, buf_size);
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

    ret = ::ftruncate(fd, 0);
    if(ret < 0) {
        perror("ftruncate");
        on_close();
        return 1;
    }
    ret = ::lseek(fd, 0, SEEK_END);
    if(ret < 0) {
        perror("lseek(fd, 0, SEEK_END)");
        on_close();
        return 1;
    }
    if(ret != 0) {
        printf("Should have advanced to 0 bytes but advanced to %d\n", ret);
        on_close();
        return 1;
    }

    ret = ::lseek(fd, 0x1000, SEEK_SET);
    if(ret < 0) {
        perror("lseek(fd, 0, SEEK_SET)");
        on_close();
        return 1;
    }
    if(ret != 0x1000) {
        printf("Should have advanced to 0x1000 bytes but advanced to %d\n", ret);
        on_close();
        return 1;
    }

    char msg[] = "hello there";
    ret = ::write(fd, msg, sizeof(msg));
    if(ret < 0) {
        perror("write");
        on_close();
        return 1;
    }
    if(ret != sizeof(msg)) {
        printf("Should have written %d bytes but wrote %d\n", sizeof(msg), ret);
        on_close();
        return 1;
    }

    ret = ::lseek(fd, 0, SEEK_CUR);
    if(ret < 0) {
        perror("lseek(fd, 0, SEEK_CUR)");
        on_close();
        return 1;
    }
    if(ret != 0x1000 + sizeof(msg)) {
        printf("Should have advanced 0x100c bytes but only advanced  %d\n", ret);
        on_close();
        return 1;
    }

    ret = ::lseek(fd, 0x1000, SEEK_SET);
    if(ret < 0) {
        perror("lseek(fd, 0, SEEK_SET)");
        on_close();
        return 1;
    }
    if(ret != 0x1000) {
        printf("Should have advanced to 0x1000 bytes but advanced to %d\n", ret);
        on_close();
        return 1;
    }

    char rbuf[0x100];
    ::memset(rbuf, 0, sizeof(rbuf));
    ret = ::read(fd, rbuf, sizeof(rbuf));
    if(ret < 0) {
        perror("read");
        on_close();
        return 1;
    }
    printf("Read %d bytes\n", ret);
    printf("strlen = %d\n", strlen(rbuf));
    puts(rbuf);

    on_close();
    return 0;
}