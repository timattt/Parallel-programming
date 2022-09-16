#include <io.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#define TOTAL_THREADS 5
#define ARRAY_MAX_SIZE 10

int arr[ARRAY_MAX_SIZE] = {0};
int N = 0;

void quickSort(int *a, int n) {
	int i = 0;
	int j = n - 1;
	int pivot = a[n / 2];
	do {
		while (a[i] < pivot) {
			i++;
		}
		while (a[j] > pivot) {
			j--;
		}
		if (i <= j) {
			std::swap(a[i], a[j]);
			i++;
			j--;
		}
	} while (i <= j);
#pragma omp task shared(a)
	{
		if (j > 0) {
			quickSort(a, j + 1);
		}
	}
#pragma omp task shared(a)
	{
		if (n > i) {
			quickSort(a + i, n - i);
		}
	}
#pragma omp taskwait
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

#pragma omp parallel
		{
//section of code that must be run by a single available thread
#pragma omp single nowait
			quickSort(arr, N - 1);
		}
	}

	printArray(arr, N);
	return 0;
}
