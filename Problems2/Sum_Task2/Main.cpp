#include <omp.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <windows.h>

#define TOTAL_THREADS 5

int main(int argc, char * argv[]) {
	omp_set_num_threads(TOTAL_THREADS);

	int N = 0;

	if (argc != 2 || !(N = atoi(argv[1]))) {
		printf("Bad arguments!");
		exit(-1);
	}

	int i = 0;
	int sum = 0;

#pragma omp parallel for schedule(dynamic) reduction(+:sum)
	for (i = 0; i < N + 1; i++) {
		sum += i;
	}

	printf("sum: %d\n", sum);

	return 0;
}
