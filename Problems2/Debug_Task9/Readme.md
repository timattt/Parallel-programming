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

### III

Я постоянно наблюдаю к этих кодах неинициализированные переменные. Знаете, чем они заполнены? Правильно - мусором.
А знаете, что самое веселое? Это то, что этот мусор в **95%** случаев равен нулю. Поэтому вы можете таким манером легко получить SIGSEGV.
Но, который вылетает один раз из двадцати. Дебаг потом займет много дней. Поэтому, давайте будет инициализировать переменные сразу.      

Тут просто зачем-то стоит лишний барьер в самом конце.

```
/******************************************************************************
* ЗАДАНИЕ: bugged3.c
* ОПИСАНИЕ:
*   Ошибка времени выполнения.
******************************************************************************/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N 50

int main (int argc, char **argv)
{
    int i, nthreads, tid, section;
    float a[N], b[N], c[N];
    void print_results(float array[N], int tid, int section);

    for (i = 0; i < N; i++)
        a[i] = b[i] = i * 1.0;

    #pragma omp parallel private(c, i, tid, section)
    {
        tid = omp_get_thread_num();
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }

        #pragma omp barrier
        printf("Thread %d starting...\n", tid);
        #pragma omp barrier

        #pragma omp sections nowait
        {
            #pragma omp section
            {
                section = 1;
                for (i = 0; i < N; i++)
                    c[i] = a[i] * b[i];
                print_results(c, tid, section);
            }

            #pragma omp section
            {
            section = 2;
            for (i = 0; i < N; i++)
                c[i] = a[i] + b[i];
            print_results(c, tid, section);
            }
        }

        #pragma omp barrier
        printf("Thread %d exiting...\n",tid);
    }
}

void print_results(float array[N], int tid, int section)
{
    int i, j;

    j = 1;
    #pragma omp critical
    {
        printf("\nThread %d did section %d. The results are:\n", tid, section);
        for (i = 0; i < N; i++)
        {
            printf("%e  ", array[i]);
            j++;
            if (j == 6)
            {
                printf("\n");
                j = 1;
            }
        }
        printf("\n");
    }

    //#pragma omp barrier
    printf("Thread %d done and synchronized.\n", tid);
}
```

### IV

Ну, тут у меня сегфалт, потому что на стеке выделяется 4 мегабайта. А в win64 ограничение на стек в размере 1 МБ.
Поэтому, нам надо выделять этот массив либо как-то иначе, либо уменьшить размер.

```
/******************************************************************************
* ЗАДАНИЕ: bugged4.c
* ОПИСАНИЕ:
*   Очень простая программа с segmentation fault.
******************************************************************************/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N 10

int main (int argc, char *argv[])
{
    int nthreads, tid, i, j;
    double a[N][N];

    #pragma omp parallel shared(nthreads) private(i, j, tid, a)
    {
        tid = omp_get_thread_num();
        if (tid == 0)
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }
        printf("Thread %d starting...\n", tid);

        for (i = 0; i < N; i++)
            for (j = 0; j < N; j++)
                a[i][j] = tid + i + j;

        printf("Thread %d done. Last element= %f\n", tid, a[N-1][N-1]);
    }
}
```

### V

Тут все просто. После блокирования не происходит разблокирование. Надо немного вызовы местами поменять, чтобы, когда мы работали с массивом А
блокировался ключ А. И аналогично и с Б.

```
/******************************************************************************
* ЗАДАНИЕ: bugged5.c
* ОПИСАНИЕ:
*   Используя секции, два потока инициализируют свои личные массивы
*   и затем добавляют свой массив в массивы соседа.
*   Почему-то происходит deadlock...
******************************************************************************/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define N     1000000
#define PI    3.1415926535
#define DELTA .01415926535

int main (int argc, char *argv[]) 
{
    int nthreads, tid, i;
    float a[N], b[N];
    omp_lock_t locka, lockb;

    omp_init_lock(&locka);
    omp_init_lock(&lockb);

    for (i = 0; i < N; i++) {
        a[i]=0;
        b[i]=0;
    }

    #pragma omp parallel shared(a, b, nthreads, locka, lockb) private(tid, i)
    {
        tid = omp_get_thread_num();
        #pragma omp master
        {
            nthreads = omp_get_num_threads();
            printf("Number of threads = %d\n", nthreads);
        }
        printf("Thread %d starting...\n", tid);
        #pragma omp barrier

        #pragma omp sections nowait
        {
            #pragma omp section
            {
                omp_set_lock(&locka);
                printf("Thread %d updating a[]\n", tid);
                for (i = 0; i < N; i++)
                    a[i] += DELTA * i;
		omp_unset_lock(&locka);
		
                omp_set_lock(&lockb);
                printf("Thread %d updating b[]\n", tid);
                for (i = 0; i < N; i++)
                    b[i] += DELTA + i;
                omp_unset_lock(&lockb);
            }

            #pragma omp section
            {
                omp_set_lock(&lockb);
                printf("Thread %d updating b[]\n", tid);
                for (i = 0; i < N; i++)
                    b[i] += PI * i;
		omp_unset_lock(&lockb);   
		 
                omp_set_lock(&locka);
                printf("Thread %d adding b[] to a[]\n", tid);
                for (i = 0; i < N; i++)
                    a[i] += PI + i;
                omp_unset_lock(&locka);
            }
        }
    }
    printf("Sample results: %f %f %f %f\n", a[0], b[0], a[999999], b[999999]);
}
```

### VI

Тут надо просто везде натыкать **reduction**.

```
/******************************************************************************
* ЗАДАНИЕ: bugged6.c
* ОПИСАНИЕ:
*   Множественные ошибки компиляции
******************************************************************************/

#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

#define VECLEN 100

float a[VECLEN], b[VECLEN];

float dotprod()
{
    int i, tid;
    float sum = 0;

    tid = omp_get_thread_num();
    #pragma omp parallel for reduction(+:sum)
    for (i = 0; i < VECLEN; i++)
    {
        sum = sum + (a[i] * b[i]);
        printf("  tid= %d i=%d\n", tid, i);
    }

    return sum;
}


int main (int argc, char *argv[])
{
    int i;
    float sum;

    for (i = 0; i < VECLEN; i++)
        a[i] = b[i] = 1.0 * i;
    sum = 0.0;

    #pragma omp parallel reduction(+:sum)
    {
    	sum+=dotprod();
    }

    printf("Sum = %f\n",sum);

    return 0;
}



```
