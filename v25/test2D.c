#include <stdio.h>

int f(int x, int y, int z)
{
    int a1 = 5;
    int k1 = a1+1;
	return x+k1*z/2;
}


int main()
{
	int k=3, n=7, m=5;
	int d=f(k,m,n);
}



