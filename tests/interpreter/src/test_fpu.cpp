int testAddFloat() {
    float x = 2.375;
    float y = 4.5;
    float z = x+y;
    if(z != 6.875) return 1;
    return 0;
}

int testAddDouble() {
    double x = 2.375;
    double y = 4.5;
    double z = x+y;
    if(z != 6.875) return 1;
    return 0;
}

int main() {
    testAddFloat();
    testAddDouble();
}