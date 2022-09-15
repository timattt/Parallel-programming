#include <omp.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <windows.h>

#define TOTAL_THREADS 5

int main() {
	omp_set_num_threads(TOTAL_THREADS);

	int current = TOTAL_THREADS - 1;

#pragma omp parallel shared(current)
	{
		int self = omp_get_thread_num();

		while (current != self) {
			Sleep(1);
		}
		current--;
		printf("Hello world from %d thread!\n", self);
	}

	return 0;
}
