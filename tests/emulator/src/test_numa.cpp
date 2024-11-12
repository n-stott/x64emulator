#include <numa.h>
#include <stdio.h>

int main() {
    int nmpn = numa_max_possible_node();
    printf("numa_max_possible_node = %d\n", nmpn);
    return 0;
}
