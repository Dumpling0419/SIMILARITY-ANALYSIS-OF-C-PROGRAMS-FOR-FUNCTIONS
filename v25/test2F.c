#include <stdio.h>

int sum1(int a1, int b1) {
    int c1 = a1 + b1;
    return c1;
}

int product1(int a1, int b1) {
    int c1 = a1 * b1;
    return c1;
}

int main() {
    int result1 = sum1(5,3) + product1(2,4);
    return result1;
}

