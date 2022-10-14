#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <cstdio>
#include <cstring>
#include <cstdio>
#include <cwchar>
#include <thread>
#include <vector>

int** alloc_mat(int n) {
	int **mat = (int**) malloc(n * sizeof(int*));
	for (int i = 0; i < n; i++)
		mat[i] = (int*) malloc(n * sizeof(int));
	return mat;
}

void free_mat(int ** mat, int n) {
	for (int i = 0; i < n; i++) {
		free(mat[i]);
	}
	free(mat);
}

void fill_mat(int ** mat, int n) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			mat[i][j] = 0;
			if (i == j) {
				mat[i][j] = 1;
			}
			//mat[i][j] = rand() % 10;
		}
	}
}

void zero_mat(int ** mat, int n) {
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			mat[i][j] = 0;
		}
	}
}

void noCache_oneThread_mul(int n) {
	int ** a = alloc_mat(n);
	int ** b = alloc_mat(n);
	int ** c = alloc_mat(n);

	fill_mat(a, n);
	fill_mat(b, n);
	zero_mat(c, n);

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			for (int k = 0; k < n; ++k) {
				c[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}

void cache_oneThread(int n) {
	int ** a = alloc_mat(n);
	int ** b = alloc_mat(n);
	int ** c = alloc_mat(n);

	fill_mat(a, n);
	fill_mat(b, n);
	zero_mat(c, n);

    int bi=0;
    int bj=0;
    int bk=0;
    int i=0;
    int j=0;
    int k=0;
    int blockSize=32;//L1

	for (bi = 0; bi < n; bi += blockSize)
		for (bj = 0; bj < n; bj += blockSize)
			for (bk = 0; bk < n; bk += blockSize)
				for (i = 0; i < blockSize; i++)
					for (j = 0; j < blockSize; j++)
						for (k = 0; k < blockSize; k++)
							if (bi + i < n && bj + j < n && bk + k < n)
								c[bi + i][bj + j] += a[bi + i][bk + k] * b[bk + k][bj + j];
}

void worker_noCache_multiThread_mul(int ** a, int ** b, int ** c, int n, int from, int to) {
	for (int i = from; i < to; ++i) {
		for (int j = 0; j < n; ++j) {
			for (int k = 0; k < n; ++k) {
				c[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}

int min(int a, int b) {
	return a > b ? b : a;
}

void noCache_multiThread_mul(int n) {
	// n - threads number
	int N = 2048;
	int blockSize = N / n;
	//int blocksQuantity = N / n + (N % n == 0 ? 0 : 1);

	int ** a = alloc_mat(N);
	int ** b = alloc_mat(N);
	int ** c = alloc_mat(N);

	fill_mat(a, N);
	fill_mat(b, N);
	zero_mat(c, N);

	std::vector<std::thread*> threads;

	for (int i = 0; i < N; i += blockSize) {
		int from = i;
		int to = min(N, i + blockSize);

		std::thread * th = new std::thread(worker_noCache_multiThread_mul, a, b, c, N, from, to);
		threads.push_back(th);
	}

	for (std::thread * th : threads) {
		th->join();
	}

	return;

	// check
	int err = 0;

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			if (i != j) {
				if (c[i][j] != 0) {
					err++;
				}
			} else {
				if (c[i][j] != 1) {
					err++;
				}
			}
		}
	}

	printf("%d\n", err);
}

void worker_cache_multiThread_mul(int ** a, int ** b, int ** c, int n, int index, int total) {
    int bi=0;
    int bj=0;
    int bk=0;
    int i=0;
    int j=0;
    int k=0;
    int blockSize=32;//L1

    int blocks_per_thread = n / blockSize / total;

	for (bi = index * n / total; bi < (index+1) * n / total; bi += blockSize)
		for (bj = 0; bj < n; bj += blockSize)
			for (bk = 0; bk < n; bk += blockSize)
				for (i = 0; i < blockSize; i++)
					for (j = 0; j < blockSize; j++)
						for (k = 0; k < blockSize; k++)
							if (bi + i < n && bj + j < n && bk + k < n)
								c[bi + i][bj + j] += a[bi + i][bk + k] * b[bk + k][bj + j];
}

void cache_multiThread_mul(int n) {
	// n - threads number
	int N = 2048;

	int ** a = alloc_mat(N);
	int ** b = alloc_mat(N);
	int ** c = alloc_mat(N);

	fill_mat(a, N);
	fill_mat(b, N);
	zero_mat(c, N);

	std::vector<std::thread*> threads;

	for (int i = 0; i < n; i++) {
		std::thread * th = new std::thread(worker_cache_multiThread_mul, a, b, c, N, i, n);
		threads.push_back(th);
	}

	for (std::thread * th : threads) {
		th->join();
	}

	return;

	// check
	int err = 0;

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			if (i != j) {
				if (c[i][j] != 0) {
					err++;
				}
			} else {
				if (c[i][j] != 1) {
					err++;
				}
			}
		}
	}

	printf("errors:  %d\n", err);
}

int main(int argc, char * argv[]) {
	int mode = atoi(argv[1]);

	switch (mode) {
		case 1: noCache_oneThread_mul	(atoi(argv[2]));break;
		case 2: cache_oneThread			(atoi(argv[2]));break;
		case 3: noCache_multiThread_mul	(atoi(argv[2]));break;
		case 4: cache_multiThread_mul	(atoi(argv[2]));break;
	}
}
