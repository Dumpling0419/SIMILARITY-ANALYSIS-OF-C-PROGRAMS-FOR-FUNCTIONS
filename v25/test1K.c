int helper(int x) {
    return x + 3;
}

int process(int a) {
    return helper(helper(a));
}

int main() {
    int m = process(2);
    return 0;
}
