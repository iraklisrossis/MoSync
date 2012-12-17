#include "core/Core.h"
#include <stdio.h>

int main(int argc, const char** argv)
{
	printf("Hello World!\n");
	FILE* log = fopen("logs/log.txt", "w");
	fprintf(log, "Hello World!\n");
	fclose(log);
	return 0;
}
