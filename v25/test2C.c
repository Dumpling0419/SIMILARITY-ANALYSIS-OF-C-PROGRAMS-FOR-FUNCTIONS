#include <stdio.h>

int f1(int s1, int s2, int s3) {
    int s1 = s2+s3;
    int m = s1 * s2;
    return m;
}

int g1(int r1, int r2) {
    int r3 = f1(2,3,2);
    return r1+r2+r3;
}

int main() {
    int y=2;
    int z=3;
    int k = g1(4, z+y);
    int l = f1(1, 2, 3);
    int m = f1(g1(4, 5), 6, 7);
    return k;
}

