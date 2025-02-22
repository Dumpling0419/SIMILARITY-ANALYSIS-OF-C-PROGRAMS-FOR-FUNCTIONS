#include <stdio.h>

int f(int p1, int p2, int p3) {
    p1 = p2+p3;
    return p1 * p2;
}

int g(int q1, int q2) {
    int q3 = f(2,3,2);
    k = q1+q2;
    return k;
}

int main() {
    int x = 2;
    int t = 3;
    int b = g(4, x+t);
    int a = f(1, 2, 3);
    int c = f(g(4, 5), 6, 7);
    return b;
}

