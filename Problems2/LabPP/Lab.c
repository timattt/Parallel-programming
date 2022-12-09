#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <string.h>

int trueISIZE = 0;
int trueJSIZE = 0;
#define ISHIFT 8
#define JSHIFT 3

typedef struct block {
	double arr[ISHIFT][JSHIFT];
} block;

block** allocBlocks(int x, int y) {
	block ** res = (block**) calloc(x, sizeof(block*));
	for (int i = 0; i < x; i++) {
		res[i] = (block*) calloc(y, sizeof(block));
	}
	return res;
}

void clearBlocks(block ** blocks, int x, int y) {
	for (int i = 0; i < x; i++) {
		free(blocks[i]);
	}
	free(blocks);
}

void initBlocks(block ** blocks, int szi, int szj, int rank, int size) {
	for (int i = 0; i < szi; i++) {
		if (i == 0) {
			for (int j = 0; j < szj; j++) {
				for (int bi = 0; bi < ISHIFT; bi++) {
					for (int bj = 0; bj < JSHIFT; bj++) {
						blocks[i][j].arr[bi][bj] = 10 * (i*ISHIFT + bi) + (j+szj*rank)*JSHIFT + bj;
					}
				}
			}
		} else if (rank + 1 == size) {
			int j = szj-1;
			for (int bi = 0; bi < ISHIFT; bi++) {
				for (int bj = 0; bj < JSHIFT; bj++) {
					blocks[i][j].arr[bi][bj] = 10 * (i*ISHIFT + bi) + (j+szj*rank)*JSHIFT + bj;
				}
			}
		}
	}
}

void fillZeros(block ** blocks, int szi, int szj) {
	for (int i = 0; i < szi; i++) {
		for (int j = 0; j < szj; j++) {
			memset(blocks[i][j].arr, 0, sizeof(block));
		}
	}
}

void passBlock(block * from, block * to) {
	for (int bi = 0; bi < ISHIFT; bi++) {
		for (int bj = 0; bj < JSHIFT; bj++) {
			to->arr[bi][bj] = sin(0.00001 * from->arr[bi][bj]);
		}
	}
}

void sendBlock(block toSend, int rank) {
	for (int i = 0; i < ISHIFT; i++) {
		for (int j = 0; j < JSHIFT; j++) {
			toSend.arr[i][j] = sin(0.00001 * toSend.arr[i][j]);
		}
	}
	MPI_Send(&toSend, sizeof(block) / sizeof(int), MPI_INT, rank, 0, MPI_COMM_WORLD);
}

void receiveBlock(block * dest, int rank) {
	MPI_Recv(dest, sizeof(block) / sizeof(int), MPI_INT, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void writeBlocks(block ** blocks, int szi, int szj, int rank) {
	char name[BUFSIZ] = {0};
	sprintf(name, "result%d.txt", rank);
	remove(name);
	FILE *ff = fopen(name, "a+");

	for (int i = 0; i < szi * ISHIFT; i++) {
		for (int j = 0; j < szj * JSHIFT; j++) {
			int blI = i / ISHIFT;
			int blJ = j / JSHIFT;

			int locI = i % ISHIFT;
			int locJ = j % JSHIFT;

			fprintf(ff, "%d", (int)blocks[blI][blJ].arr[locI][locJ]);

			if (j + 1 < szj * JSHIFT) {
				fprintf(ff, " ");
			}
		}
		fprintf(ff, "\n");
	}

	fclose(ff);
}

void uniteFiles(int size) {
	remove("LAB.txt");
	FILE * res = fopen("LAB.txt", "a+");

	FILE ** fs = calloc(size, sizeof(FILE*));

	for (int rank = 0; rank < size; rank++) {
		char name[BUFSIZ] = {0};
		sprintf(name, "result%d.txt", rank);

		fs[rank] = fopen(name, "a+");
	}

	for (int i = 0; i < trueISIZE; i++) {
		int j = 0;
		for (int rank = 0; rank < size; rank++) {
			char * line = NULL;
			size_t len = 0;
			int read = getline(&line, &len, fs[rank]);

			if (read == -1) {
				printf("ERR\n");
				exit(-1);
			}

			line[read - 1] = 0;
			char * cur = line;
			while (*cur) {
				if (j >= trueJSIZE-JSHIFT) {
					*cur = 0;
					break;
				}
				while (*cur != ' ' && *cur != 0){
					cur++;
				}
				j++;
				if (j >= trueJSIZE-JSHIFT) {
					*cur = 0;
					break;
				}
				cur++;
			}

			fprintf(res, "%s", line);

			if (j >= trueJSIZE-JSHIFT) {
				if (j > 0) {
					fprintf(res, " ");
				}
				for (;j < trueJSIZE; j++) {
					fprintf(res, "%d", 10 * i + j);
					if (j + 1 < trueJSIZE) {
						fprintf(res, " ");
					}
				}
				break;
			}

			if (rank + 1 < size) {
				fprintf(res, " ");
			}
		}

		fprintf(res, "\n");
	}

	for (int rank = 0; rank < size; rank++) {
		fclose(fs[rank]);
		char name[BUFSIZ] = {0};
		sprintf(name, "result%d.txt", rank);
		remove(name);
	}

	fclose(res);
}

void sendByte(int rank) {
	int dat = 1;
	MPI_Send(&dat, 1, MPI_INT, rank, 0, MPI_COMM_WORLD);
}

void receiveByte(int rank) {
	int dat = 1;
	MPI_Recv(&dat, 1, MPI_INT, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void print(block ** blocks, int szi, int szj) {
	for (int i = 0; i < szi * ISHIFT; i++) {
		for (int j = 0; j < szj * JSHIFT; j++) {
			int blI = i / ISHIFT;
			int blJ = j / JSHIFT;

			int locI = i % ISHIFT;
			int locJ = j % JSHIFT;

			printf("%d", (int)blocks[blI][blJ].arr[locI][locJ]);

			if (j + 1 < szj * JSHIFT) {
				printf(" ");
			}

			if ((j + 1) % JSHIFT == 0) {
				printf(" | ");
			}

		}
		printf("\n");
		if ((i+1)%ISHIFT == 0) {
			for (int k = 0; k < 100; k++){
				printf("-");
			}
			printf("\n");
		}
	}

}

int main(int argc, char** argv){
    MPI_Init(&argc, &argv);

    int rank = 0;
	int size = 0;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

	trueISIZE = atoi(argv[1]);
	trueJSIZE = atoi(argv[2]);

    int isize = trueISIZE;
    int jsize = trueJSIZE;

	if (isize % ISHIFT != 0) {
		isize += (ISHIFT - isize % ISHIFT);
	}
	if (jsize % (JSHIFT * size) != 0) {
		jsize += ((JSHIFT * size) - jsize % (JSHIFT * size));
	}
	
	//printf("di=%d, dj=%d\n", isize - trueISIZE, jsize-trueJSIZE);

    int blocksI = isize / ISHIFT;
    int blocksJ = jsize / (JSHIFT * size);
    int blockOffsetJ = blocksJ * rank;

    block ** blocks = allocBlocks(blocksI, blocksJ);

    initBlocks(blocks, blocksI, blocksJ, rank, size);


    double T_st = MPI_Wtime();

    for (int bi = 0; bi < blocksI-1; bi++) {
    	for (int bj = 0; bj < blocksJ; bj++) {
    		// lower block send
    		if (bj == 0 && blockOffsetJ > 0) {
    			sendBlock(blocks[bi][bj], rank-1);
    		}

    		// top block receive
    		if (bi > 0 && bj == blocksJ - 1 && rank < size - 1) {
    			receiveBlock(&blocks[bi][bj], rank+1);
    		}

    		if (bj > 0) {
    			passBlock(&blocks[bi][bj], &blocks[bi+1][bj-1]);
    		}
    	}
    }

    // last layer only receive
    int bi = blocksI - 1;
	for (int bj = 0; bj < blocksJ; bj++) {
		// top block receive
		if (bi > 0 && bj == blocksJ - 1 && rank < size - 1) {
			receiveBlock(&blocks[bi][bj], rank+1);
		}
	}

	initBlocks(blocks, blocksI, blocksJ, rank, size);

	if (rank == 0) {
        // send time
        double T_work = MPI_Wtime() - T_st;
        printf("%f\n", T_work);
	}

    // synchronized write
    if (rank > 0) {
    	receiveByte(rank-1);
    }

    writeBlocks(blocks, blocksI, blocksJ, rank);

    if (rank < size - 1) {
    	sendByte(rank+1);
    } else {
    	uniteFiles(size);
    }

    clearBlocks(blocks, blocksI, blocksJ);

    MPI_Finalize();
    return 0;
}
