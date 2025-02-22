#include <stdio.h>

int add(int a, int b) {
    int c = 2;
    int d = 5;
    int e = d - c;
    int g = e;
    int f = a + b - g;
    return f;
}

int main() {
    int result = add(5, 3);
    return result;
}

