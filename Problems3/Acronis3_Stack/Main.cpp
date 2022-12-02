#include <thread>
#include <assert.h>
#include <atomic>
#include <thread>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TOTAL_CYCLE 1000000
#define MAX_THREADS 8

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

// STRUCTURES
//================================================
typedef struct expBackoff {
    int nInitial;
    int nStep;
    int nThreshold;
    int nCurrent;
} expBackoff;

void initBackOff(struct expBackoff * bo) {
	memset((void*) bo, 0, sizeof(struct expBackoff));
	bo->nCurrent = bo->nInitial = 10;
	bo->nStep = 2;
	bo->nThreshold = 8000;
}

void backOff(struct expBackoff *bo) {
	for (int k = 0; k < bo->nCurrent; ++k) {
		__asm ("nop");
	}
	bo->nCurrent *= bo->nStep;

	if (bo->nCurrent > bo->nThreshold)
		bo->nCurrent = bo->nThreshold;
}
//================================================

class LockFreeStack {
public:

	struct Node {
		std::atomic<Node*> next;
	};

	class TaggedPointer {
	public:
		TaggedPointer() :
				m_node(nullptr), m_counter(0) {
		}

		Node* GetNode() {
			return m_node.load(std::memory_order_acquire);
		}

		uint64_t GetCounter() {
			return m_counter.load(std::memory_order_acquire);
		}

		bool CompareAndSwap(Node *oldNode, uint64_t oldCounter, Node *newNode, uint64_t newCounter) {
			bool cas_result = 0;
			__asm__ __volatile__
			(
					"lock cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
					"setz       %3;"// if ZF set, set cas_result to 1

					: "+m" (*this), "+a" (oldNode), "+d" (oldCounter), "=q" (cas_result)
					: "b" (newNode), "c" (newCounter)
					: "cc", "memory"
			);
			return cas_result;
		}

private:

		std::atomic<Node*> m_node;
		std::atomic<uint64_t> m_counter;
	}

	// у нас cas на 16 байт
	__attribute__((aligned(16)));

	bool TryPushStack(Node *entry) {
		Node *oldHead = NULL;
		uint64_t oldCounter = 0;

		oldHead = head.GetNode();
		oldCounter = head.GetCounter();
		entry->next.store(oldHead, std::memory_order_relaxed);
		return head.CompareAndSwap(oldHead, oldCounter, entry, oldCounter + 1);
	}

	bool TryPopStack(Node *&oldHead, int threadId) {
		oldHead = head.GetNode();
		uint64_t oldCounter = head.GetCounter();
		if (oldHead == nullptr) {
			return true;
		}
		hazards[threadId].store(oldHead, std::memory_order_seq_cst);
		if (head.GetNode() != oldHead) {
			return false;
		}
		return head.CompareAndSwap(oldHead, oldCounter, oldHead->next.load(std::memory_order_acquire), oldCounter + 1);
	}

	void Push(Node *entry, expBackoff *bo) {
		initBackOff(bo);
		while (true) {
			if (TryPushStack(entry)) {
				return;
			}
			backOff(bo);
		}
	}

	Node* Pop(int threadId, expBackoff *bo) {
		initBackOff(bo);
		Node *res = NULL;
		while (true) {
			if (TryPopStack(res, threadId)) {
				while (res) {
					bool ex = 1;
					for (int i = 0; i < MAX_THREADS; i++) {
						if (res == hazards[i].load() && i != threadId) {
							backOff(bo);
							ex = 0;
						}
					}
					if (ex) {
						break;
					}
				}
				hazards[threadId] = nullptr;
				return res;
			}
			backOff(bo);
		}
	}

private:

	TaggedPointer head;
	std::atomic<Node*> hazards[MAX_THREADS];
};


void worker(LockFreeStack * st, int (*token)(int index), int index) {
	expBackoff bo = {0};
	initBackOff(&bo);

	for (int i = 0; i < TOTAL_CYCLE; i++) {
		int d = token(i);
		switch (d) {
		case 0://push
			st->Push(new LockFreeStack::Node, &bo);
			break;
		case 1://pop
			delete st->Pop(index, &bo);
			break;
		}
	}
}

void test(int numThreads, int (*token)(int index)) {
	assert(numThreads < MAX_THREADS);

	std::thread *threads[MAX_THREADS] = { 0 };

	LockFreeStack *st = new LockFreeStack();

	for (int i = 0; i < numThreads; i++) {
		threads[i] = new std::thread(worker, st, token, i);
	}

	for (int i = 0; i < numThreads; i++) {
		threads[i]->join();
	}

	for (int i = 0; i < numThreads; i++) {
		delete threads[i];
	}
}

long calcTime(int numThreads, int (*token)(int index)) {
	auto time_begin = std::chrono::high_resolution_clock::now();
	test(numThreads, token);
	auto time_end = std::chrono::high_resolution_clock::now();

	auto dtime = time_end - time_begin;
	long dtime_ms = std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();
	return dtime_ms;
}

void groupTest(int (*token)(int index)) {
	for (int numThreads = 1; numThreads < MAX_THREADS; numThreads++) {
		long dt = calcTime(numThreads, token);
		printf("%d %ld\n", numThreads, dt);
		fflush(stdout);
	}
}

int simpleToken(int v) {
	return v % 2;
}

int balancedToken(int v) {
	return rand() % 2;
}

int unbalancedToken(int v) {
	return (int)(v / 1000) % 2;
}

int main(int argc, char *argv[]) {
	groupTest(&balancedToken);
	groupTest(&unbalancedToken);
	return 0;
}
