#include <thread>
#include <assert.h>
#include <atomic>
#include <thread>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <mutex>
#include <stack>

#define TOTAL_CYCLE 100000
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
		__asm__ __volatile__ ("nop");
	}
	bo->nCurrent *= bo->nStep;

	if (bo->nCurrent > bo->nThreshold)
		bo->nCurrent = bo->nThreshold;
}
//================================================
class LockedStack
{
public:
    void Push(int entry)
    {
        m_mutex.lock();
        m_stack.push(entry);
        m_mutex.unlock();
    }

    int Pop()
    {
    	m_mutex.lock();
        if(m_stack.empty())
        {
            return -1;
        }
        int ret = m_stack.top();
        m_stack.pop();
        m_mutex.unlock();
        return ret;
    }

private:
    std::stack<int> m_stack;
    std::mutex m_mutex;
};


#define FREELIST_SIZE 40

class LockFreeStack {
public:

	struct Node {
		std::atomic<Node*> next;
	};

	class TaggedPointer {
	public:
		TaggedPointer() :
				node(nullptr), counter(0) {
		}

		Node* GetNode() {
			return node.load(std::memory_order_acquire);
		}

		uint64_t GetCounter() {
			return counter.load(std::memory_order_acquire);
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

		std::atomic<Node*> node;
		std::atomic<uint64_t> counter;
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

	void Push(int threadId, Node *entry, expBackoff *bo) {
		initBackOff(bo);
		wantPush(threadId);
		while (true) {
			if (eliminatePush(threadId)) {
				return;
			}
			if (TryPushStack(entry)) {
				dontWantPush(threadId);
				return;
			}
			backOff(bo);
		}
	}

	bool canBeFreed(Node * nd) {
		for (int i = 0; i < MAX_THREADS; i++) {
			if (hazards[i].compare_exchange_weak(nd, nd, std::memory_order_relaxed)) {
				return 0;
			}
		}
		return 1;
	}

	void addToFreeList(Node * nd) {
		for (int i = 0; i < FREELIST_SIZE; i++) {
			Node * tmp = nullptr;
			if (freelist[i].compare_exchange_weak(tmp, nd, std::memory_order_relaxed)) {
				return;
			}
		}

		// may clean the list
		for (int i = 0; i < FREELIST_SIZE; i++) {
			delete freelist[i].exchange(nullptr, std::memory_order_relaxed);
		}
	}

	void wantPop(int threadId) {
		pops[threadId].store(1);
	}

	void wantPush(int threadId) {
		pushs[threadId].store(0);
	}

	void dontWantPop(int threadId) {
		pops[threadId].store(0);
	}

	void dontWantPush(int threadId) {
		pushs[threadId].store(0);
	}

	bool eliminatePop(int threadId) {
		bool v = 0;
		if (pops[threadId].compare_exchange_weak(v, v)) {
			return 1;
		}
		for (int i = 0; i < MAX_THREADS; i++) {
			v = 1;
			if (pushs[threadId].compare_exchange_weak(v, 0)) {
				dontWantPop(threadId);
				return 1;
			}
		}
		return 0;
	}

	bool eliminatePush(int threadId) {
		bool v = 0;
		if (pushs[threadId].compare_exchange_weak(v, v)) {
			return 1;
		}
		for (int i = 0; i < MAX_THREADS; i++) {
			v = 1;
			if (pops[threadId].compare_exchange_weak(v, 0)) {
				dontWantPush(threadId);
				return 1;
			}
		}
		return 0;
	}

	void Pop(int threadId, expBackoff *bo) {
		initBackOff(bo);
		Node *res = NULL;
		wantPop(threadId);
		while (true) {
			if (eliminatePop(threadId)) {
				return;
			}
			if (TryPopStack(res, threadId)) {
				dontWantPop(threadId);
				hazards[threadId].store(nullptr);
				if (canBeFreed(res)) {
					delete res;
				} else {
					addToFreeList(res);
				}
				return;
			}
			backOff(bo);
		}
	}

private:

	TaggedPointer head;
	std::atomic<Node*> hazards[MAX_THREADS] = {};
	std::atomic<Node*> freelist[FREELIST_SIZE];

	std::atomic<bool> pops[MAX_THREADS] = {};
	std::atomic<bool> pushs[MAX_THREADS] = {};
};


void worker(LockFreeStack * st, int (*token)(int index, int id), int index) {
	expBackoff bo = {0};
	initBackOff(&bo);

	for (int i = 0; i < TOTAL_CYCLE; i++) {
		int d = token(i, index);
		switch (d) {
		case 0://push
			st->Push(index, new LockFreeStack::Node, &bo);
			break;
		case 1://pop
			st->Pop(index, &bo);
			break;
		}
	}
}

void workerLocked(LockedStack * st, int (*token)(int index, int id), int index) {
	expBackoff bo = {0};
	initBackOff(&bo);

	for (int i = 0; i < TOTAL_CYCLE; i++) {
		int d = token(i, index);
		switch (d) {
		case 0://push
			st->Push(rand());
			break;
		case 1://pop
			st->Pop();
			break;
		}
	}
}

void test(int numThreads, int (*token)(int index, int id)) {
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

void testLocked(int numThreads, int (*token)(int index, int id)) {
	assert(numThreads < MAX_THREADS);

	std::thread *threads[MAX_THREADS] = { 0 };

	LockedStack *st = new LockedStack();

	for (int i = 0; i < numThreads; i++) {
		threads[i] = new std::thread(workerLocked, st, token, i);
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

long calcTimeLocked(int numThreads, int (*token)(int index, int id)) {
	auto time_begin = std::chrono::high_resolution_clock::now();
	testLocked(numThreads, token);
	auto time_end = std::chrono::high_resolution_clock::now();

	auto dtime = time_end - time_begin;
	long dtime_ms = std::chrono::duration_cast<std::chrono::microseconds>(dtime).count();
	return dtime_ms;
}

void groupTest(int (*token)(int index, int id)) {
	for (int numThreads = 1; numThreads < MAX_THREADS; numThreads++) {
		long dt = calcTime(numThreads, token);
		printf("%d %ld\n", numThreads, dt);
		fflush(stdout);
	}
}

void groupTestLocked(int (*token)(int index, int id)) {
	for (int numThreads = 1; numThreads < MAX_THREADS; numThreads++) {
		long dt = calcTimeLocked(numThreads, token);
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
	return (id+(int)(v / 1000)) % 2;
}

int main(int argc, char *argv[]) {
	groupTest(&balancedToken);
	groupTest(&unbalancedToken);
	groupTestLocked(&simpleToken);
	return 0;
}
