#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>

#include "Core/SkipList.h"
#include "Core/SkipNode.h"
#include "RCU/RCUManager.h"

#define TOTAL_CYCLE 100000
#define MAX_THREADS 10

int totalThreads = 0;

void worker(SkipList * st, int (*token)(int index, int id), int index, RCUManager * rcu) {
	for (int i = 0; i < TOTAL_CYCLE; i++) {
		int d = token(i, index);

		int val = rand() % 10;

		SkipNode* nd = nullptr;
		switch (d) {
		case 0://insert
			rcu->startReading(index);
			st->insert(val, val*val);
			rcu->endReading(index);
			break;
		case 1://erase
			nd = st->remove(val);
			if (!nd) {
				continue;
			}
			rcu->startDeleting(index);
			st->find_with_gc(nd->key, nullptr, nullptr);
			//delete nd;
			break;

		case 2://contains
			rcu->startReading(index);
			st->find_with_gc(val, nullptr, nullptr);
			rcu->endReading(index);
			break;
		}

	}

}

void test(int numThreads, int (*token)(int index, int id)) {
	if (!(numThreads < MAX_THREADS)) {
		printf("error!\n");exit(-1);
	}

	std::thread *threads[MAX_THREADS] = { 0 };

	SkipList *st = new SkipList();
	RCUManager * rcu = new RCUManager(numThreads);

	for (int i = 0; i < numThreads; i++) {
		threads[i] = new std::thread(worker, st, token, i, rcu);
	}

	for (int i = 0; i < numThreads; i++) {
		threads[i]->join();
	}

	for (int i = 0; i < numThreads; i++) {
		delete threads[i];
	}
}

long calcTime(int numThreads, int (*token)(int index, int id)) {
	auto time_begin = std::chrono::high_resolution_clock::now();
	test(numThreads, token);
	auto time_end = std::chrono::high_resolution_clock::now();

	auto dtime = time_end - time_begin;
	long dtime_ms = std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();
	return dtime_ms;
}

void groupTest(int (*token)(int index, int id)) {
	for (int numThreads = 1; numThreads < MAX_THREADS; numThreads++) {
		totalThreads = numThreads;
		long dt = calcTime(numThreads, token);
		printf("%d %ld\n", numThreads, dt);
		fflush(stdout);
	}
}

int simpleToken(int v, int id) {
	return v % 2;
}

int balancedToken(int v, int id) {
	return rand() % 2;
}

int unbalancedToken(int v, int id) {
	if (v < 100) {
		return 0;
	}
	int t = rand() % 10;
	if (t > 1) {
		return 2;
	}
	return t;
}

int main(int argc, char *argv[]) {

	//=======================
	// RANDOMIZED TEST
	//=======================

#define RAND_TEST
#ifdef RAND_TEST

#define SZ 100

	SkipList *st = new SkipList();

	int bitmap[SZ] = { 0 };

	int ins = 0;
	int er = 0;

	for (int j = 0; j < 100; j++) {
		int i = rand() % SZ;
		switch (rand() % 3) {
		case 0:
			ins++;
			st->insert(i, i*i);
			bitmap[i] = 1;
			break;
		case 1:
			er++;
			st->remove(i);
			bitmap[i] = 0;
			break;
		case 2:
			if (st->find_with_gc(i, nullptr, nullptr) != (bitmap[i] > 0)) {
				printf("err: %d\n", j);
				printf("total ins=%d, total erases=%d\n", ins, er);
				for (int k = 0; k < SZ; k++) {
					printf("%d, bitmap=%d, cont=%d\n", k, bitmap[k],
							st->find_with_gc(k, nullptr, nullptr));
				}
				return 0;
			}
			break;
		}
	}

	st->dump_graph(nullptr, nullptr);

	//printf("total ins=%d, total erases=%d\n", ins, er);
	//printf("OK\n");
	//fflush(stdout);

#endif

	//=======================
	// MULTITHREAD TEST
	//=======================

	groupTest(&balancedToken);
	groupTest(&unbalancedToken);

	return 0;
}
