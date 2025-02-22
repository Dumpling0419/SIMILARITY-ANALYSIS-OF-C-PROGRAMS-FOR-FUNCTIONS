#include <stdio.h>

int g(int x, int y)
{
	return x*x+y;
}

int f(int x, int y, int z)
{
	int w=g(x,y)+g(y,z)-z/2;
	return w;
}


int main()
{

	int a=3,b=5,c=7;
	int d=f(a,b,c);
	return d;
}


