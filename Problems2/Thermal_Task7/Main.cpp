#include <omp.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <windows.h>

#define TOTAL_THREADS 5

#define N 100
#define M 100
#define START_BORDER 10.0
#define TOTAL_TIME_STEPS 100
#define COURANT 0.01

double layer1[N][M] = {{0}};
double layer2[N][M] = {{0}};

int main(int argc, char * argv[]) {
	omp_set_num_threads(TOTAL_THREADS);

	// init conditions
	for (int i = 0; i < N; i++) {
		layer1[i][0] = START_BORDER;
	}
	for (int j = 0; j < M; j++) {
		layer1[0][j] = layer1[N - 1][j] = START_BORDER;
	}

	int j = 0;

#define TOP ((n % 2 == 0) ? layer2 : layer1)
#define BOT ((n % 2 == 0) ? layer1 : layer2)

	for (int n = 0; n < TOTAL_TIME_STEPS; n++) {
		for (int i = 0; i < N; i++) {
			TOP[i][0] = BOT[i][0];
			TOP[i][M-1] = BOT[i][M-1];
		}
		for (int j = 0; j < M; j++) {
			TOP[0][j] = BOT[0][j];
			TOP[N-1][j] = BOT[N-1][j];
		}

#pragma omp parallel for private(j)
		for (int i = 1; i < N - 1; i++) {
			for (j = 1; j < M - 1; j++) {
				TOP[i][j] = COURANT * (BOT[i+1][j] + BOT[i-1][j] + BOT[i][j+1] + BOT[i][j-1] - 4 * BOT[i][j]) + BOT[i][j];
			}
		}
#pragma omp barrier
	}

	// average temperature after TAU time
	double aver = 0;
	for (int i = 0; i < N; i++) {
		for (int j = 0; j < M; j++) {
			aver += (TOTAL_TIME_STEPS % 2 == 0) ? layer1[i][j] : layer2[i][j];
		}
	}
	aver /= (double)(N * M);

	printf("average temperature after some time: %f\n", aver);

	return 0;
}
