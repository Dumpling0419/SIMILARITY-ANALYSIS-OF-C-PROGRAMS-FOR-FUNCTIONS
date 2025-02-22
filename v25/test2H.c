int process(int p1, int p2) {
    int value = p1 + p2;
    int value = value * 2;
    return value;
}

int execute(int x, int y) {
    int total = x + y;
    int total = total * 3;
    return total + 10;
}

int total(int a, int b) {
    int t = execute(a, b) + execute(a, b);
    return t;
}

int main() {
    int a = 15;
    int b = 25;
    int result1 = process(a, b);
    int result2 = execute(a, b);
    int sum = complete(result1, result2);
    return sum;
} 