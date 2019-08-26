#include <stdio.h>
#include <stdlib.h>

int abc(void)
{
	int ccc = 0;

	printf("ccc = %d\n", ccc);
}

int main (void)
{
	int ddd = abc();
	int i;

	for (i = 0; i < 10; i++)
		ddd ++;

	printf("ddd = %d\n", ddd);

	return 0;
}
