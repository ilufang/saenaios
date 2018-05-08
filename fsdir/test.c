#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int globalrw;

int main(int argc, char *argv[]) {
	char *name;
	int number, i;
	char *literal = "RODATA";

	for (i = 0; i < argc; i++) {
		printf("Argv[%d]=%s\n", i, argv[i]);
	}


	name = malloc(50 << 20);

	printf("Hi.\n\n");

	printf("What's your name?");
	fflush(stdout);
	scanf("%s", name);
	printf("Hello %s, what's your favorite number?", name);
	fflush(stdout);
	scanf("%d", &number);
	printf("Here's what your favorite number looks like in memory: %08x\n", number);

	printf("TEXT   = %p\n", &main);
	printf("STACK  = %p\n", &number);
	printf("RWDATA = %p\n", &globalrw);
	printf("RODATA = %p\n", literal);
	printf("HEAP   = %p\n", name);
	printf("BREAK  = %p\n", sbrk(0));

	return 0;
}
