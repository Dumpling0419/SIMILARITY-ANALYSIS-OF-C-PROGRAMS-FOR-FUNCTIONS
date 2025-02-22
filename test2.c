#include <stdio.h>

int sum(int x, int y) {
    int z = 2;
    x=3;
    y=5;
    return x + y - z;
}

int main() {
    int total = sum(5, 3);
    printf("Total is: %d\n", total);
    return total;
}

