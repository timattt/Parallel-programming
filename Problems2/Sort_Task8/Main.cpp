#include <io.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define TOTAL_THREADS 5
#define ARRAY_MAX_SIZE 10000

int arr[ARRAY_MAX_SIZE] = {0};
int N = 0;

int partition(int *a, int p, int r) {
	int lt[r - p] = { 0 };
	int gt[r - p] = { 0 };
	int i = 0;
	int j = 0;
	int key = a[r];
	int lt_n = 0;
	int gt_n = 0;

	for (i = p; i < r; i++) {
		if (a[i] < a[r]) {
			lt[lt_n++] = a[i];
		} else {
			gt[gt_n++] = a[i];
		}
	}

	for (i = 0; i < lt_n; i++) {
		a[p + i] = lt[i];
	}

	a[p + lt_n] = key;

	for (j = 0; j < gt_n; j++) {
		a[p + lt_n + j + 1] = gt[j];
	}

	return p + lt_n;
}

#define TASK_SIZE 100

void quicksort(int *a, int p, int r) {
	int div = 0;

	if (p < r) {
		div = partition(a, p, r);
#pragma omp task shared(a) if(r - p > TASK_SIZE)
		quicksort(a, p, div - 1);
#pragma omp task shared(a) if(r - p > TASK_SIZE)
		quicksort(a, div + 1, r);
	}
}

void printArray(int arr[], int size) {
	for (int i = 0; i < size; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");
}

void readFromFile(char * name) {
	FILE* file = fopen(name, "r");
	fscanf(file, "%d\n", &N);
	if (N >= ARRAY_MAX_SIZE) {
		printf("Array in the file is too big!\n");
		exit(-1);
	}
	for (int i = 0; i < N; i++) {
		int val = 0;
		fscanf(file, "%d\n", &val);
		arr[i] = val;
	}
	fclose(file);
}

void readFromConsole() {
	std::cin >> N;
	if (N >= ARRAY_MAX_SIZE) {
		printf("Array in the file is too big!\n");
		exit(-1);
	}
	for (int i = 0; i < N; i++) {
		int val = 0;
		std::cin >> val;
		arr[i] = val;
	}
}

bool cheat1() {
	return N == 1;
}

bool cheat2() {
	for (int i = 0; i < N-1; i++) {
		if (arr[i] > arr[i+1]) {
			return false;
		}
	}
	return true;
}

int main(int argc, char * argv[]) {
	if (argc == 2) {
		char * fileName = argv[1];
		if (access(fileName, F_OK) == 0) {
			readFromFile(fileName);
		} else {
			printf("Bad file given!\n");
			exit(-1);
		}
	} else {
		readFromConsole();
	}

	if (!cheat1() && !cheat2()) {
		omp_set_num_threads(TOTAL_THREADS);
		quicksort(arr, 0, N-1);
	}

	printArray(arr, N);
	return 0;
}
