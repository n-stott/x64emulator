extern "C" {

    __attribute__((noinline))
    void intrinsic$putchar(char) { }

    int fake$puts(const char* s) {
        while(*s) {
            intrinsic$putchar(*s);
            ++s;
        }
        return 0;
    }

}