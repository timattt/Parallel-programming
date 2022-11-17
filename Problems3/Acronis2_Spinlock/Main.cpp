#include <thread>
#include <assert.h>
#include <atomic>
#include <thread>

#define TOTAL_CYCLE 1000000
#define MAX_THREADS 8

int count = 0;

// SLEEPS
inline void pause_asm() {
    __asm volatile("pause" :::);
}

inline void pause_asm_mem() {
    __asm volatile("pause" ::: "memory");
}

inline void pause_thread() {
    std::this_thread::yield();
}

// TAS = test and set
//==============================================================================
std::atomic<int> tas_flag = { 0 };

void tas_init() {
	tas_flag = {0};
}

void tas_lock() {
	int expected = 0;
	// если флаг равен expected, т.е. нулю, то кладем в него 1 и возвращается true. И происходит выход из цикла
	// иначе приравниваем expected к 1 и возвращаем false. Цикл продолжается.
	// параметр memory_order_relaxed гарантирует, что нет никакого ограничения на порядок, в котором потоки работают с переменной.
	while (!tas_flag.compare_exchange_weak(expected, 1, std::memory_order_relaxed)) {
		pause_thread();
		expected = 0;
	}
}

void tas_unlock() {
	tas_flag.store(0, std::memory_order_relaxed);
}
//==============================================================================

// TTAS
//==============================================================================
std::atomic<int> ttas_flag = {0};

void ttas_init() {
	ttas_flag = {0};
}

void ttas_lock() {
	int expected = 0;
	while (!ttas_flag.compare_exchange_weak(expected, 1, std::memory_order_relaxed)) {
		expected = 0;

		// операция атомарного обмена требует записи в кещ, но одновременно может туда писать только один поток.
		// А вот читать могут несколько одновременно. Поэтому мы сначала дождемся, когда флаг освободится, а уже потом полезем его перезаписывать.
		while (ttas_flag.load(std::memory_order_relaxed)) {
			pause_thread();
		}
	}
}

void ttas_unlock() {
	ttas_flag.store(0, std::memory_order_relaxed);
}
//==============================================================================

// Ticket lock
//==============================================================================
std::atomic<int> cur = {0}, last = {0};

void tl_init() {
	// это номер клиента, которого обслуживают в данный момент
	cur = {0};
	// это минимальный свободный номерок для нового клиента
	last = {0};
}

void tl_lock() {
	// получаем номерок.
	// эта штука сначала возвращает старое значение, а потом уже прибавляет.
	int index = last.fetch_add(1, std::memory_order_relaxed);
	// ждем до тех пока, не подойдет очередь нашего номерка 
	while (cur.load(std::memory_order_relaxed) != index) {
		pause_asm_mem();
	}
}

void tl_unlock() {
	// высчитываем следующий номерок
	int next_index = cur.load(std::memory_order_relaxed) + 1;
	// говорим, что теперь используется он
	cur.store(next_index, std::memory_order_relaxed);
}
//==============================================================================

void worker(void (*flock)(), void (*funlock)()) {
	for (int i = 0; i < TOTAL_CYCLE; i++) {
		(*flock)();
		count++;
		(*funlock)();
	}
}

void test(int numThreads, void (*init)(), void (* lock)(), void (*unlock)()) {
    assert(numThreads < MAX_THREADS);

    std::thread* threads[MAX_THREADS] = {0};

    count = 0;

    init();

    for (int i = 0; i < numThreads; i++) {
    	threads[i] = new std::thread(worker, lock, unlock);
    }

    for (int i = 0; i < numThreads; i++) {
        threads[i]->join();
    }

    for (int i = 0; i < numThreads; i++) {
        delete threads[i];
    }

}

long calcTime(int numThreads, void (*init)(), void (*lock)(), void (*unlock)()) {
	auto time_begin = std::chrono::high_resolution_clock::now();
	test(numThreads, init, lock, unlock);
	auto time_end = std::chrono::high_resolution_clock::now();

	auto dtime = time_end - time_begin;
	long dtime_ms = std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();
	return dtime_ms;
}

void groupTest(void (*init)(), void (*lock)(), void (*unlock)()) {
	for (int numThreads = 1; numThreads < MAX_THREADS; numThreads++) {
		long dt = calcTime(numThreads, init, lock, unlock);
		printf("%d %ld\n", numThreads, dt);
		fflush(stdout);
	}
}

int main(int argc, char * argv[]) {
	groupTest(tas_init, tas_lock, tas_unlock);
	groupTest(ttas_init, ttas_lock, ttas_unlock);
	groupTest(tl_init, tl_lock, tl_unlock);
	return 0;
}
