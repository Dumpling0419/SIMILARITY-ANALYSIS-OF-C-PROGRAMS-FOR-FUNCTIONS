int assist(int x) {
    return x + 3;
}

int execute(int a) {
    int temp = assist(a);
    return assist(temp);
}

int main() {
    int n = execute(2);
    return 0;
} 