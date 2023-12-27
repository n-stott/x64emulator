struct S {
    S() {
        int a = 0xbeef;
    }

    ~S() {
        int a = 0xf00d;
    }
};

thread_local S s;

int main() {
    
}