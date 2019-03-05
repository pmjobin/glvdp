#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	FILE *infp = stdin;
	FILE *outfp = stdout;
	int result = EXIT_FAILURE;
	int c = '\0';

	if (argc >= 2) {
		infp = fopen(argv[1], "rb");
		if (!infp) {
			perror("Unable to open IN file\n");
			goto end;
		}
	}
	if (argc >= 3) {
		outfp = fopen(argv[2], "wt");
		if (!outfp) {
			perror("Unable to open OUT file\n");
			goto end;
		}
	}
	while ((c = fgetc(infp)) != EOF) {
		fprintf(outfp, "'\\x%X',", (unsigned)c);
	}
	fprintf(outfp, "'\\0'");
	result = EXIT_SUCCESS;

end:
	fclose(infp);
	fclose(outfp);
	return result;
}
