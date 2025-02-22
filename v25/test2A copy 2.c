#include <stdio.h>

int sum(int x, int y) {
    int z = 2;
    int k = 5;
    int l = k-z;
    int m = x + y - l;
    return m;
}

int main() {
    int total = sum(5, 3);
    return total;
}

