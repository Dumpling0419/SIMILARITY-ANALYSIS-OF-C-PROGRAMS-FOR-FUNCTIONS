int calculate(int x, int y) {
    int temp = x * 2;
    int temp = temp + y;
    return temp + 3;
}

int compute(int a, int b) {
    int val = a * 3;
    int val = val + b;
    return val;
}

int main() {
    int input1 = 5;
    int input2 = 15;
    int final = calculate(input1, input2);
    int final2 = compute(input1, input2);
    return final + final2;
} 