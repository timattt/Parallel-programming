# Заметки по библиотеке OpenMP

Инклудим библиотеку:
```
#include "omp.h"
```

```
#pragma omp parallel
{
  ...
}
```
Это обозначает парал. участок кода, который будет исполняться несколькими потоками.
Все, что внутри будет выполнено несколькими исполнителями.

### Build

Нужен флаг **-fopenmp**.

Задание колличества потоков - в терминале написать **$set OMP_NUM_THREADS=5**. Потом запускать.


```
omp_get_num_threads()
```
 - возвращает кол. потоков.
Если возвращенное значение = 0, то это мастер поток. Можно определять номер потока как и в MPI.

### условия на переменные

* **shared** - переменная с именем из аргументов дикрективы будет разделенной памятью. Без shared переменные все равно будут shared.
* **private** - переменная будет уникальна для каждого потока. Значение до и после засирается.
* **firstprivate** - как private, только исходное значение переменной сохраняется.
* **lastprivate** - при выходе из региона значение сохранится. Будет запомнено последнее измененное.
* **if** - блок выполняется только, если условие из ифа истинно.
* **default** - все переменные по умолчанию будут такими, как мы зададим в этой директиве.

### директива for

```
#pragma omp for
  for (i = 0; i < 1000; i++)
```

Автоматически распределяет выполнение цикла между исполнителями.
Если исполнителей тут всего 4, то распределение будет такое:

* **thread 0** - i = (0, 249)
* **thread 1** - i = (250, 499)
* **thread 2** - i = (500, 749)
* **thread 3** - i = (750, 999)

### директива reduction (доп. условие к for)

Эта штука выполняет глобальные операции над переменными.
Синтаксис:

```
reduction(ОПЕРАТОР:ПЕРЕМЕННЫЕ)
```
Результат операции будет положен в первую переменную в списке.

### директива типа расписания schedule (доп. условие к for)

По умолчанию:
```
schedule(static, 1)
```

Используя тип **static** можно поставить размер блока, который приходится на один поток в цикле for.
```
schedule(static, 200)
```
Например такая дичь даст каждому потоку 200 итераций в цикле.

Используя тип **dynamic** последовательность блоков выдается сразу, как только поток закончилвыполнять блок цикла. А в режиме **static** все определено заранее.

Тип **guided** будет давать блоки размера больше параметра, который задан.

Тип **runtime** определяет все сам в процессе выполнения программы, причем можно задавать тип через переменную окружения **OMP_SCHEDULE**.

### Секции

```
#pragma omp parallel sections(nowait?)
{
#pragma omp section
{
printf("A");
}
#pragma omp section
{
printf("B");
}
}
```

Только одна секция.

```
#pragma omp single
  printf("C");
```

### Зависимость по данным

* Функции необходимо сделать независимыми от внешних данных, кроме как от значения параметров.
* В функции так же не должно быть статических переменных.
* Циклы, в которых есть выход по условию распараллеливать **нельзя**.

### Критическая секция

В ней есть доступ к данным, которые мы не хотим попортить.
В критическую секцию в один момент может зайти только один поток.
Все безыменные секция считаются одной большой.

```
#pragma omp critical (NAME)
{
...
}
```

### Барьерная синхронизация

```
#pragma omp barrier
```

### Атомарные операции

```
#pragma omp atomic
```

Гарантируется, что такая операция не прервется посередине.

### Порядок

```
#pragma omp ordered
```

Операции выполняются в порядке номеров потоков.

### Сброс

```
#pragma omp flush(var1, ...)
```

Сбрасывает немедленный сброс значений переменных в разделяемую память.
Таким образом гарантируется, что во всех потоках значение переменных одинаковое.

### одна приватная переменная для всех секций

Один раз объявили и во всех блоках для потока она будет.

```
#pragma omp threadprivate(var1, var2...)
```

### mutex (замки)

```
omp_set_lock(omp_lock * lock)
```

Заходя в эту функцию, ждет, пока параметр не освободится.
Аналог критической секции.

### Tasks

Задача (task) помещается внутрь параллельной области, и задает блок операторов, который может выполняться в отдельном потоке. Задача не создает новый поток. Задача (блок операторов) помещается в пул, из которого ее может взять один из свободных потоков для выполнения.

### Tasks - условия

```
#pragma omp task if(clause)
```

Если скалярное выражение принимает false, то:
* Текущее задание приостанавливается
* Данное новое задание начианется немедленно
* Родительское задание продолжается, когда новое задание будет завершено

### Работа со связным списком

```
my_pointer = listhead;
#pragma omp parallel
{
// первый поток распределяет задания другим потокам
// эта секция выолняется ровно одним потоком, все остальные ждут задачи (task)
  #pragma omp single
  {
    while (my_pointer) {
      #pragma omp task firstprivate(my_pointer)
        do_independent_work(my_pointer);
      my_pointer = my_pointer->next;
    }
  }
}
```

### Зависимости

Пример:
```
#pragma omp parallel
#pragma omp single
{

#pragma omp task depend(out: x)
  do1(); // 1
#pragma omp task depend(in: x)
  do2(); // 2
#pragma omp task depend(in: x)
  do3(); // 3
  
}
```
1 должна быть выполнена до начала 2 и 3.
2 и 3 могут быть выполнены параллельно.

Типы:

* in - будет зависеть от предыдущих out, inout
* out/inout - задача будет зависима от всех ранее сгенерированных задач

### Условие final

```
#pragma omp task final(expr)
```

Ограничение на создание задач в зависимости от условия.
Если оно false, то это обычная задача.
Если оно true, то задача будет выполнена в последний раз и больше выполняться вообще никогда не будет.

### Условие mergeable

```
#pragma omp task mergeable
```
Подсказка, которая показывает, что при необходимости можно выполнить слияние данных таких заданий вместе, если:
* задание не вложенное - ```if(false)```
* задание вложенное - ```final(true)```

### Условие taskloop

```
#pragma omp taskloop
```
Разбивание цикла с использованием задач.
Условия:
* ```grainsize(size)``` - отрезки имеют мин размер size и макс размер 2*size
* ```num_tasks(num)``` - распределить итерации цикла на num задач
