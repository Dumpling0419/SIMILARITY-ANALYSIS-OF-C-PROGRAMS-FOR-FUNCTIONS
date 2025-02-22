#include <stdio.h>

int add(int a, int b) {
    int c = 2;
    a=5;
    b=3;
    return a + b - c;
}

int main() {
    int result = add(5, 3);
    printf("Result is: %d\n", result);
    return result;
}

