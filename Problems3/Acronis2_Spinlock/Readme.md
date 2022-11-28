# Спинлоки

Мы хотим сделать критическую секцию.
Делаем с помощью циклов, чтобы пока один поток находится в критической секции - другие катались в цикле.

## Полезные функции

* compare_exchange_weak - true, если в this находится значение nExpected (в этом случае оно меняется на nNew), false в случае неудачи (тогда nExpected будет содержать текущее значение переменной по адресу pAddr).
* store - кладем в атомарную переменную что-нибудь - одна операция записи.
* load - читаем значение атомарной переменной, соответственно одна операция чтения.
* fetch_add - сначала возвращает старое значение, а уже потом прибавляет.

## TAS

Test and store

Самый простой вариант блокировки. У нас есть атомарная переменная. Поток смотрит - она единица или нет.
Если ноль, то поток идет дальше и ставит ее в 1. Если 1, то ждет в цикле.
При разблокировке поток ставит ее значение в 0.

Алгоритм не wait free - порядок поток может нарушиться. И возможно, что какой-то поток будет ждать долго.

## TTAS

test and test and set

Все аналогично предыдущему случаю, но мы знаем, что операция compare_exchange требует синхронизации для записи, что тратит лишнее время.
А вот операция load - требует только чтения. Поэтому мы будет ждать в цикле, который только читает.

Алгоритм не wait free - порядок поток может нарушиться. И возможно, что какой-то поток будет ждать долго.

## Ticket lock

Нам выдают номерки. Как в гос учреждении. Есть две атомарные переменные - текущий номерок и последний выданный номерок.
Поток блокирует - ему выдают номерок равный последнему выданному. Он ждет, пока текущий не равен его номерку. И потом идет дальше.
При разблокировке мы текущий номер увеличиваем на 1.

![image](https://user-images.githubusercontent.com/25401699/202441339-47960e2c-ca94-4f85-9a5d-9c07c8fd0ce7.png)

Это честный алгоритм - он соблюдает порядок. Никакой поток слишком долго ждать не будет.

## Полезности

* [статья про кэши в многопроцессорных системах](https://habr.com/ru/post/183834/)

## Заметки

* Каждый поток имеет свой приватный кэш, в котором хранятся копии необходимых линий, а некоторые из них при этом локально модифицированы.
* Протокол когерентности обеспечивает единое пространство памяти для потоков - чтобы в их локальных кешах были одинаковые значения.
* Протокол MESI (modified, exclusive, shared, invalid) - пример протокола когерентности. Далее диаграмма изменения состояния кэш-линий. 

![image](https://user-images.githubusercontent.com/25401699/203031375-47b51cbb-228f-4286-817a-f6ec3d4d6249.png)

* Классические ReadModifyWrite операции: CompareAndSwap, FetchAndAdd, TestAndSet
* Использование CAS влечет проблему ABA. Один поток считал значение из разделяемой памяти A. Потом другой записал сначала туда же B и потом сразу A.
* Но первый сравнит данные и ничего не заметит.
* От спинлока мьютекс отличается передачей управления планировщику для переключения потоков при невозможности захвата мьютекса.
* Барьеры компилятора (директива memory) требует от компилятора не смешивать код до и код после.
* lock free - без классических инструментов блокировки (семафоры и тд)
* wait free - lock free, но без ожидания
* Слабая версия CAS может быть неудачной, то есть возвратить false, даже в том случае, если текущее значение переменной равно ожидаемому.

## Модели памяти

### Sequential consistency (последовательная согласованность)

Все операции с атомарными переменными выполняются строго в указанном программистом порядке.

### Acquire/release семантика

Можно задать барьер, в котором ЦП не будет неретасовывать команды для работы с памятью. 

### Relaxed memory ordering (слабый порядок)

Процессор делает почти, что хочет.

## Store buffer

Когда мы хотим записать что-то в кещ линию, мы должны уведомить другие процессы об этом. Инвалиировать ее у них. Это требует времени.
Поэтому можно завести промежуточный буффер, чтобы сначала записать туда и потом, когда другие будут готовы записать уже окончательно.

## Store forwarding

каждая операция записи может прямо передаваться следующей операции чтения через store buffer, без обращения к кэшу.

## Простои при последовательности записей

Если store buffer мал, а записей много, то придется ждать, пока все синхронизируются, чтобы эту проблему решить вводим invalidate queue.

## Invalidate queue

Если мы получаем сообцение invalidate, то мы можем не обрабатывать его сразу, а поместить в очередь. И сразу ответить acknowledge.
Помещение элемента в invalidate queue – это по сути обещание процессора обработать данное invalidate-сообщение перед тем, как отправлять любой сигнал протокола MESI,
относящийся к данной кэш-линии.

Поэтому барьеры памяти взаимодействуют invalidate queue - когда процессор выполняет барьер памяти, он должен помечать все элементы в своей invalidate-очереди и
притормозить все последующие чтения до тех пор, пока invalidate-очередь не будет им полностью обработана.