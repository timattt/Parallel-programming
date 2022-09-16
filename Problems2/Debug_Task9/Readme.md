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

### II

Внимание вопрос к составителям этого задания. Вот как можно найти баг в программе, если не написано, что эта программа должна делать?
Дичь, короче.   
В общем здесь проблема явно в переменной **total**. Тут нужен либо **lastprivate**, либо как я сделал - переписать цикл, чтобы он считал человеческую сумму.
Используем директиву **reduction**.

```
/******************************************************************************
* ЗАДАНИЕ: bugged2.c
* ОПИСАНИЕ:
*   Еще одна программа на OpenMP с багом.
******************************************************************************/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    int nthreads, i, tid;
    float total;

    #pragma omp parallel
    {
        tid = omp_get_thread_num();
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }
        printf("Thread %d is starting...\n", tid);

        #pragma omp barrier

        total = 0.0;
#pragma omp for schedule(dynamic, 10) reduction(+:total)
        for (i = 0; i < 1000000; i++) {
            total = total + i*1.0;
        }

        printf ("Thread %d is done! Total= %e\n", tid, total);
    }
}

```

