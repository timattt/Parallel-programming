#include <omp.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <windows.h>
#include <time.h>
#include <algorithm>
#include <dir.h>

#define TOTAL_THREADS 5

#define START_BORDER 10.0
#define MAX_TIME_STEPS 1000
#define COURANT 0.01

double ** layer1 = NULL;
double ** layer2 = NULL;

void saveLayer(double ** layer, int n, int m, int index) {
	char destName[200] = { 0 };
	mkdir("frames");
	sprintf(destName, "frames/frame%d.txt", index);

	FILE * dest = fopen(destName, "w");
	if (dest == NULL) {
		return;
	}
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			fprintf(dest, "%f", layer[i][j]);
			if (j + 1 < m) {
				fprintf(dest, " ");
			}
		}
		fprintf(dest, "\n");
	}

	fclose(dest);
}

int main(int argc, char *argv[]) {
	double epsilon = 0;
	time_t start_t = 0, end_t = 0;
	double diff_t = 0;
	int n = 0;
	int m = 0;

	// args
	if (argc != 4) {
		printf("Bad args count!");
		exit(0);
	}
	epsilon = std::atof(argv[1]);
	n = std::atoi(argv[2]);
	m = std::atoi(argv[3]);

	if (n == 0 || m == 0 || epsilon == 0) {
		printf("Bad args!");
		exit(0);
	}

	omp_set_num_threads(TOTAL_THREADS);
	time(&start_t);

	// init arrays
	layer1 = (double**)calloc(n, sizeof(double*));
	layer2 = (double**)calloc(n, sizeof(double*));
	for (int i = 0; i < n; i++) {
		layer1[i] = (double*)calloc(m, sizeof(double));
		layer2[i] = (double*)calloc(m, sizeof(double));
	}

	// init conditions
	for (int i = 0; i < n; i++) {
		layer1[i][0] = START_BORDER;
	}

	int j = 0;

#define TOP ((k % 2 == 0) ? layer2 : layer1)
#define BOT ((k % 2 == 0) ? layer1 : layer2)

	int k = 0;
	double D = 0;

	for (k = 0; k < MAX_TIME_STEPS; k++) {
		for (int i = 0; i < n; i++) {
			TOP[i][0] = BOT[i][0];
			TOP[i][m-1] = BOT[i][m-1];
		}
		for (int j = 0; j < m; j++) {
			TOP[0][j] = BOT[0][j];
			TOP[n-1][j] = BOT[n-1][j];
		}

#pragma omp parallel for private(j)
		for (int i = 1; i < n - 1; i++) {
			for (j = 1; j < m - 1; j++) {
				TOP[i][j] = COURANT * (BOT[i+1][j] + BOT[i-1][j] + BOT[i][j+1] + BOT[i][j-1] - 4 * BOT[i][j]) + BOT[i][j];
			}
		}
#pragma omp barrier

		saveLayer(TOP, n, m, k);

		D = 0;
		for (int i = 0; i < n; i++) {
			for (int j = 0; j < m; j++) {
				D = std::max(D, abs(TOP[i][j] - BOT[i][j]));
			}
		}

		if (D < epsilon) {
			break;
		}
	}

	// average temperature after TAU time
	double aver = 0;
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < m; j++) {
			aver += (k % 2 == 0) ? layer1[i][j] : layer2[i][j];
		}
	}
	aver /= (double)(n * m);

	time(&end_t);
	diff_t = difftime(end_t, start_t);

	if (D > epsilon) {
		printf("Warning! The required temperature has not been reached\n");
	}

	printf("Average temperature after %d iterations: [%f], deltaTime=[%d] seconds\n", n, aver, (int)diff_t);

	return 0;
}
