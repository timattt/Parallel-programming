#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <thread>
#include <omp.h>

void consistent(double ** a, int n, int m) {
	for (int i = 8; i < n; i++) {
		for (int j = 0; j < m - 3; j++) {
			a[i][j] = sin(0.00001 * a[i - 8][j + 3]);
		}
	}
}

void paral(double ** a, int n, int m) {
	for (int i = 8; i < n; i++) {
#pragma omp parallel for
		for (int j = 0; j < m - 3; j++) {
			a[i][j] = sin(0.00001 * a[i - 8][j + 3]);
		}
	}
}

void init(double ** a, int n, int m) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			a[i][j] = 10 * i + j;
		}
	}
}

void save(double ** a, int n, int m) {
	FILE *ff = NULL;
	ff = fopen("result.txt", "w");
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			fprintf(ff, "%f ", a[i][j]);
		}
		fprintf(ff, "\n");
	}
	fclose(ff);
}

int compare(double ** a, double ** b, int n, int m) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			if (a[i][j] != b[i][j]) {
				return 0;
			}
		}
	}

	return 1;
}

double** allocArr(int n, int m) {
	double ** res = (double**)calloc(n, sizeof(double*));
	for (int i = 0; i < n; i++) {
		res[i] = (double*)calloc(m, sizeof(double));
	}
	return res;
}

void freeArr(double ** arr, int n, int m) {
	for (int i = 0; i < n; i++) {
		free(arr[i]);
	}
	free(arr);
}

int main(int argc, char **argv) {
	int n = atoi(argv[1]);
	int m = atoi(argv[2]);
	int q = atoi(argv[3]);

	omp_set_num_threads(q);

	double ** a = allocArr(n, m);
	double ** b = allocArr(n, m);

	init(a, n, m);
	init(b, n, m);

	// cons
	auto time_begin = std::chrono::high_resolution_clock::now();
	consistent(a, n, m);
	auto time_end = std::chrono::high_resolution_clock::now();

	auto dtime = time_end - time_begin;
	long dt_cons = std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();

	// paral
	time_begin = std::chrono::high_resolution_clock::now();
	paral(b, n, m);
	time_end = std::chrono::high_resolution_clock::now();

	dtime = time_end - time_begin;
	long dt_paral = std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();

	if (compare(a, b, n, m) == 0) {
		printf("ERROR!\n");
	} else {
		printf("%f %f\n", dt_cons/1000000.0, dt_paral/1000000.0);
	}

	freeArr(a, n, m);
	freeArr(b, n, m);
}
