#include <stdio.h>

int main() {
	int c;
	while ((c = fgetc(stdin)) != EOF) {
		printf("'\\x%X',", (unsigned)c);
	}
	printf("'\\0'");
}
