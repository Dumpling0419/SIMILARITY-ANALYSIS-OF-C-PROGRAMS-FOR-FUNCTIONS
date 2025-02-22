int add(int x, int y) {
    int result = x + y;
    return result;
}

int calculate(int a, int b) {
    int w = add(a, b);
    int z = add(w, 10);
    int f = w - z;
    return f;
}

int main() {
    int m = calculate(3, 4);
    return 0;
} 