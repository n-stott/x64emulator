#include <sys/stat.h>
#include <cstddef>
#include <cstdio>

int main() {
    using Stat = struct stat;
    Stat s;
    printf("sizeof(stat) : %d\n", sizeof(s));
    printf("st_dev     : %d  %d\n", sizeof(s.st_dev), offsetof(Stat, st_dev));
    printf("st_ino     : %d  %d\n", sizeof(s.st_ino), offsetof(Stat, st_ino));
    printf("st_nlink   : %d  %d\n", sizeof(s.st_nlink), offsetof(Stat, st_nlink));
    printf("st_mode    : %d  %d\n", sizeof(s.st_mode), offsetof(Stat, st_mode));
    printf("st_uid     : %d  %d\n", sizeof(s.st_uid), offsetof(Stat, st_uid));
    printf("st_gid     : %d  %d\n", sizeof(s.st_gid), offsetof(Stat, st_gid));
    printf("st_rdev    : %d  %d\n", sizeof(s.st_rdev), offsetof(Stat, st_rdev));
    printf("st_size    : %d  %d\n", sizeof(s.st_size), offsetof(Stat, st_size));
    printf("st_blksize : %d  %d\n", sizeof(s.st_blksize), offsetof(Stat, st_blksize));
    printf("st_blocks  : %d  %d\n", sizeof(s.st_blocks), offsetof(Stat, st_blocks));
    printf("st_atim    : %d  %d\n", sizeof(s.st_atim), offsetof(Stat, st_atim));
    printf("st_mtim    : %d  %d\n", sizeof(s.st_mtim), offsetof(Stat, st_mtim));
    printf("st_ctim    : %d  %d\n", sizeof(s.st_ctim), offsetof(Stat, st_ctim));
}