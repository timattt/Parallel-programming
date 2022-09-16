#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

#define MAX_SIZE 1000
#define TOTAL_THREADS 5
#define MAX_ELEM_VAL 100

int A[MAX_SIZE][MAX_SIZE] = {0};
int B[MAX_SIZE][MAX_SIZE] = {0};
int C[MAX_SIZE][MAX_SIZE] = {0};

int N = 0;

int main(int argc, char * argv[]) {
	int i = 0;
	int j = 0;
	int k = 0;
	int dot = 0;
	struct timeval tv1 = { 0 }, tv2 = { 0 };
	struct timezone tz = { 0 };
	double elapsed = 0;

	if (argc != 3 || atoi(argv[1]) == 0 || atoi(argv[2]) == 0) {
		printf("Bad arguments!\n");
		exit(-1);
	}

	N = atoi(argv[1]);

	omp_set_num_threads(atoi(argv[2]));

	for (i = 0; i < MAX_SIZE; i++)
		for (j = 0; j < MAX_SIZE; j++) {
			A[i][j] = rand() % MAX_ELEM_VAL;
			B[i][j] = rand() % MAX_ELEM_VAL;
		}

	gettimeofday(&tv1, &tz);

#pragma omp parallel for private(j, k, dot) shared(A, B, C)
	for (i = 0; i < N; ++i) {
		for (j = 0; j < N; ++j) {
			dot = 0;
			for (k = 0; k < N; ++k) {
				dot += A[i][k] * B[k][j];
			}
			C[i][j] = dot;
		}
	}

	gettimeofday(&tv2, &tz);
	elapsed = (double) (tv2.tv_sec - tv1.tv_sec)
			+ (double) (tv2.tv_usec - tv1.tv_usec) * 1.e-6;
	printf("%f\n", elapsed);

}
