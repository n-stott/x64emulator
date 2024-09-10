int f(int x) {
    return x + 1;
}

int test1() {
    int x = f(2);
    return x;
}

int main() {
    test1();
}