#include <omp.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <windows.h>
#include <cmath>
#include <algorithm>

const int SQRT_MAXN = 100000; // корень из максимального значения N
const int S = 10000;
bool nprime[SQRT_MAXN] = {0}, bl[S] = {0};
int primes[SQRT_MAXN] = {0}, cnt = {0};

#define TOTAL_THREADS 5

int main(int argc, char *argv[]) {
	omp_set_num_threads(TOTAL_THREADS);

	int N = 0;

	if (argc != 2 || !(N = atoi(argv[1]))) {
		printf("Bad arguments!");
		exit(-1);
	}

	int nsqrt = (int) sqrt(N + .0);
	for (int i = 2; i <= nsqrt; ++i)
		if (!nprime[i]) {
			primes[cnt++] = i;
			if (i * 1ll * i <= nsqrt)
				for (int j = i * i; j <= nsqrt; j += i)
					nprime[j] = true;
		}

	int result = 0;
	int maxk = N / S;
	int start = 0;
	int i = 0;

#pragma omp parallel for private(start, i)
	for (int k = 0; k <= maxk; ++k) {
		memset(bl, 0, sizeof bl);
		start = k * S;
		for (i = 0; i < cnt; i++) {
			int start_idx = (start + primes[i] - 1) / primes[i];
			int j = std::max(start_idx, 2) * primes[i] - start;
			for (; j < S; j += primes[i])
				bl[j] = true;
		}
		if (k == 0)
			bl[0] = bl[1] = true;
		for (int i = 0; i < S && start + i <= N; i++)
			if (!bl[i])
				++result;
	}


	printf("%d\n", result);

}
