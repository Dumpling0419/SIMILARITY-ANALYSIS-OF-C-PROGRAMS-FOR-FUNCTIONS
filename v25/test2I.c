int calculate1(int p1, int p2) {
    int result = p1 * 2;
    int result = result + p2;
    return result;
}

int compute1(int x, int y) {
    int sum = x * 3;
    int sum = sum + y;
    return sum + 8;
}

int main() {
    int num1 = 8;
    int num2 = 12;
    int output1 = calculate1(num1, num2);
    int output2 = compute1(num1, num2);
    return output1 + output2;
} 