int process(int x, int y) {
    int result = x + y;
    int result = result * 2;
    return result;
}

int execute(int a, int b) {
    int sum = a + b;
    int sum = sum * 3;
    return sum;
}

int complete(int a, int b) {
    int c = process(a, b) + execute(a, b);
    return c;
}

int main() {
    int num1 = 10;
    int num2 = 20;
    int output1 = process(num1, num2);
    int output2 = execute(num1, num2);
    int add = complete(output1, output2);
    return add;
} 