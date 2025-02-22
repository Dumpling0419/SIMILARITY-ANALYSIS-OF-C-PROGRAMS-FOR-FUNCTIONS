#include <stdio.h>

int sum(int p, int q) {
    return p + q;
}

int product(int m, int n) {
    return m * n;
}

int main() {
    int res1 = sum(3, 5);
    int res2 = product(2, 4);

    printf("Results: %d, %d\n", res1, res2);
    return 0;
}
