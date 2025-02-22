#include <stdio.h>

int add(int x, int y) {
    return x + y;
}

int multiply(int a, int b) {
    return a * b;
}

int main() {
    int result1 = add(3, 5);
    int result2 = multiply(2, 4);

    printf("Results: %d, %d\n", result1, result2);
    return 0;
}

