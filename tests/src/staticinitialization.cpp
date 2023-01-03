int* initPointer() {
    return new int(42);
}

static int* ptr1 = initPointer();
static int* ptr2 = initPointer();

int main() {
    return *ptr1 + *ptr2;
}