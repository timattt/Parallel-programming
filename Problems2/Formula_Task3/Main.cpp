#include <omp.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <windows.h>

#define TOTAL_THREADS 3
#define N 100000
#define BLOCK_SIZE ((N) / (TOTAL_THREADS))

long long a[N] = {0};
long long b[N] = {0};

long long prevs[TOTAL_THREADS] = {0};
long long lasts[TOTAL_THREADS] = {0};

int main() {
	omp_set_num_threads(TOTAL_THREADS);

	// init
	for (int i = 0; i < N; i++) {
		a[i] = i+1;
	}

	// получим запланированный результат, чтобы потом сравнить.
	for (int i = 0; i < N; i++) {
		b[i] = ((i == 0) ? 1 : a[i-1]) * a[i] * ((i + 1 == N) ? 1 : a[i+1]);
	}

	long long old = 0;
	int i = 0;

	/*
	 * Заполним крайние значения для блоков. Причем обязательно дождемся, чтобы это сделали все.
	 */
#pragma omp parallel
	{
		int index = omp_get_thread_num() * BLOCK_SIZE;
		prevs[omp_get_thread_num()] = (index == 0) ? 1 : a[index - 1];
		lasts[omp_get_thread_num()] = (index + BLOCK_SIZE >= N) ? 1 : a[index + BLOCK_SIZE];
#pragma omp barrier
	}

	/*
	 * Если N не кратно кол. потоков, то остаточек посчитаем в конце отдельно.
	 * Очевидно, что кол. итераций при подсчете отсаточка меньше BLOCK_SIZE.
	 * Значит, итоговая ассимптотика не меняется, т.е. будет O(BLOCK_SIZE)
	 */
	int rare_start = TOTAL_THREADS * BLOCK_SIZE;
	long long rare_old = (rare_start > 0) ? a[rare_start-1] : 0;

	/*
	 * Теперь каждый блок пусть считает.
	 */
#pragma omp parallel for schedule(static, N / TOTAL_THREADS) private(old)
	for (i = 0; i < TOTAL_THREADS * BLOCK_SIZE; i++) {
		old = a[i];
		a[i] = prevs[omp_get_thread_num()] * a[i] * ((i + 1 == omp_get_thread_num() * BLOCK_SIZE + BLOCK_SIZE) ? lasts[omp_get_thread_num()] : a[i+1]);
		prevs[omp_get_thread_num()] = old;
	}

	// остаточек
	long long prev = rare_old;
	for (int i = rare_start; i < N; i++) {
		rare_old = a[i];
		a[i] = prev * a[i] * ((i + 1 == N) ? 1 : a[i+1]);
		prev = rare_old;
	}

	// Теперь смотрим, сколько у нас сошлось с запланированным результатом.
	int bad = 0;

	for (int i = 0; i < N; i++) {
		if (a[i] != b[i]) {
			bad++;
			printf("%d\n", i);
		}
	}

	printf("%d/%d\n", N-bad, N);

	return 0;
}
