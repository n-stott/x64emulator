int* initPointer() {
    return new int(42);
}

static int* ptr1 = initPointer();
static int* ptr2 = initPointer();

int test1() {
    return *ptr1 + *ptr2;
}

int main() {
    test1();
}