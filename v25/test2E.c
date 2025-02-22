#include <stdio.h>

int g(int x, int y)
{
	int w= x*x+y;
	return w;
}

int f(int x, int y, int z)
{
	int w1=g(x,y);
    int w2=g(y,z);
	int w=w1+w2-z/2;
	return w;
}


int main()
{
	int k=3,m=5,n=7;
	int d=f(k,m,n);
	return d;
}




