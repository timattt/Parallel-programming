#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("Bad args count!\n");
		exit(-1);
	}
	int isize = atoi(argv[1]);
	int jsize = atoi(argv[2]);

	double ** a = (double**)calloc(isize, sizeof(double*));
	for(int i = 0; i < isize; i++) {
		a[i] = (double*) calloc(jsize, sizeof(double));
	}

	int i, j;
	FILE *ff;
	for (i = 0; i < isize; i++) {
		for (j = 0; j < jsize; j++) {
			a[i][j] = 10 * i + j;
		}
	}
	for (i = 8; i < isize; i++) {
		for (j = 0; j < jsize - 3; j++) {
			a[i][j] = sin(0.00001 * a[i-8][j + 3]);
		}
	}
	remove("RAW.txt");
	ff = fopen("RAW.txt", "w");
	for (i = 0; i < isize; i++) {
		for (j = 0; j < jsize; j++) {
			fprintf(ff, "%d", (int)a[i][j]);
			if (j + 1 < jsize) {
				fprintf(ff, " ");
			}
		}
		fprintf(ff, "\n");
	}
	fclose(ff);

	for (int i = 0; i < isize; i++) {
		free(a[i]);
	}
	free(a);
}
