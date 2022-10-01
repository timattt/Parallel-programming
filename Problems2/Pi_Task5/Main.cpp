#include <omp.h>
#include <cstdio>
#include <cstdlib>

#define TOTAL_THREADS 5

int main(int argc, char * argv[]) {
	omp_set_num_threads(TOTAL_THREADS);

	int N = 0;

	if (argc != 2 || !(N = atoi(argv[1]))) {
		printf("Bad arguments!");
		exit(-1);
	}

	int i = 0;
	long double sum = 0;

#pragma omp parallel for schedule(dynamic) reduction(+:sum)
	for (i = 0; i < N; i++) {
		sum += ((i % 2 == 1) ? -1.0 : 1.0) / (2.0 * (long double)(i) + 1);
	}

	sum *= 4.0;

	printf("sum: %Lf\n", sum);

	return 0;
}
