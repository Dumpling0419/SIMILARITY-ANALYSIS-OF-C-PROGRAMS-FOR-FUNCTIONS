#include <stdio.h>

int sum(int x, int y) {
    int z = 3;
    int k = 8;
    int l = k+z;
    int m = x + y - l;
    return m;
}

int main() {
    int total = sum(5, 3);
    return total;
}

