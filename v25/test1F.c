#include <stdio.h>

int add(int a, int b) {
    int c = a + b;
    return c;
}

int multiplication(int a, int b) {
    int c = a * b;
    return c;
}

int main() {
    int result = add(5,3) + multiplication(2,4);
    return result;
}

