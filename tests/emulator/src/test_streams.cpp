#include <stdio.h>
#include <unistd.h>

int main() {
    char message[] = "Hello there !\n";
    if(::write(STDIN_FILENO, message, sizeof(message)) < 0) {
        perror("write");
        return 1;
    }
    if(::write(STDOUT_FILENO, message, sizeof(message)) < 0) {
        perror("write");
        return 1;
    }
    if(::write(STDERR_FILENO, message, sizeof(message)) < 0) {
        perror("write");
        return 1;
    }
    if(fwrite(message, 1, sizeof(message), stdin) > 0) {
        return 1;
    }
    if(fwrite(message, 1, sizeof(message), stdout) == 0) {
        return 1;
    }
    if(fwrite(message, 1, sizeof(message), stderr) == 0) {
        return 1;
    }
    return 0;
}