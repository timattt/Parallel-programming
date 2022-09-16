# Дебаг

### I

Здесь проблема в директиве for. После нее должен сразу идти цикл, а не что-то другое.
Решение - затащим инициализацию tid в тело цикла.

```
/******************************************************************************
 * ЗАДАНИЕ: bugged1.c
 * ОПИСАНИЕ:
 *   Данная программа демонстрирует использование конструкции 'parallel for'.
 *   Однако, данный код вызывает ошибки компиляции.
 ******************************************************************************/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N           50
#define CHUNKSIZE   5

int main(int argc, char **argv) {
	int i, chunk, tid;
	float a[N], b[N], c[N];

	for (i = 0; i < N; i++)
		a[i] = b[i] = i * 1.0;
	chunk = CHUNKSIZE;

#pragma omp parallel for shared(a, b, c, chunk) private(i, tid) schedule(static, chunk)
	for (i = 0; i < N; i++) {
		tid = omp_get_thread_num();
		c[i] = a[i] + b[i];
		printf("tid= %d i= %d c[i]= %f\n", tid, i, c[i]);
	}
}

```
