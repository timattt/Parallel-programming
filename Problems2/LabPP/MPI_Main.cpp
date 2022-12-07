#include <iostream>
#include <cstdlib>
#include <cmath>
#include <mpi.h>

#define ISIZE 5000
#define JSIZE 5000

double** allocate(int i, int j){
    double ** a = (double**)calloc(i, sizeof(double*));
    for (int iter = 0; iter < i; iter++){
        a[iter] = (double*)calloc(j, sizeof(double));
    }
    return a;
}

int main(int argc, char** argv){
    MPI_Init(&argc, &argv);

    int rank = 0;
	int	size = 0;
	int send_flag = 0;
    double send_data = 0;
	double rcv_data = 0;
    
	MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int jstart = (JSIZE / size) * rank;
	int jend = (JSIZE / size) * (rank + 1); // Делим массив на участки для каждого процесса

    if (rank + 1 == size){
        jend = JSIZE;
    }

    double ** a = allocate(ISIZE, jend - jstart));

    for (int i = 0; i < ISIZE; i++){
        for (int j = 0; j < jend - jstart; j++){
            a[i][j] = 10 * i + j + jstart;
        }
    }

    double start = MPI_Wtime();
    if (rank + 1 == size) {
        for(int i = 2; i < ISIZE; i++) {
            for(int j = 0; j < jend - jstart - 3; j++) {
                if(j <= 8) {
                    send_data = a[i-8][j];
                    MPI_Send(&send_data, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD);
                }
                a[i][j]=sin(0.00001 * a[i - 8][j + 3]);
            }
        }
    } else {
        for (int i = 8; i < ISIZE; i++){
            for (int j = 0; j < jend - jstart; j++){
                if((rank != 0) && (j <= 8)) {
                    send_data = a[i-8][j];
                    MPI_Send(&send_data, 1, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD);
                }
            
                if (j + 3 >= (jend - jstart)) {
                    MPI_Recv(&rcv_data, 1, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    a[i][j]=sin(0.00001 * rcv_data);
                }
                else {
                    a[i][j]=sin(0.00001 * a[i - 2][j + 3]);
                }
            }
        }
    }
    std::cout << MPI_Wtime() - start << "\n";

    FILE *ff = NULL;
    for (int i = 0; i < ISIZE; i++){
        if (rank == 0 && i == 0){
            ff = fopen("MPI_result.txt", "w");
        }
        else {
            if(rank == 0) {
                MPI_Recv(&send_flag, 1, MPI_INT, size - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            else {
                MPI_Recv(&send_flag, 1, MPI_INT, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            ff = fopen("MPI_result.txt", "a");
        }
        
        for (int j = 0; j < jend - jstart; j++){
            fprintf(ff, "%f ", a[i][j]);
        }
        
        if(rank + 1 == size) {
            fprintf(ff, "\n");
        }
        
        fclose(ff);
        if(rank + 1 == size) {
            if(i + 1 != ISIZE) {
                MPI_Send(&send_flag, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            }
        } else {
            MPI_Send(&send_flag, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        }
    }

    MPI_Finalize();
    return 0;
}
