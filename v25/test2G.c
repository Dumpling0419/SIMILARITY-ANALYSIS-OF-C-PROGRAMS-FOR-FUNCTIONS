int sum(int p, int q) {
    int total = p + q;
    return total;
}

int process(int x, int y) {
    int v = sum(x, y);
    int r = sum(v, 5);
    int g = v - r;
    return g;
}

int main() {
    int n = process(3, 4);
    return 0;
} 