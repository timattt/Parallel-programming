#include <mpi.h>
#include <stdio.h>

#define EPSILON 0.01

int main(int argc, char *argv[]) {
        int commsize = 0;
        int my_rank = 0;
        int data = 0;
        int i = 0;

        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &commsize);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        printf("Process %d started. Total %d processes.\n", my_rank, commsize);

                long double sum = 0;
                long double val = EPSILON + 1;

        for (i = 1 + my_rank;; i += commsize) {
                        val = 1.0 / (long double)(i);
                        if (val < EPSILON) {
                                break;
                        }
                        sum += val;
                }

                MPI_Send(&sum, 1, MPI_LONG_DOUBLE, 0, 0, MPI_COMM_WORLD);
                printf("Process %d PARTSUM: %LF\n", my_rank, sum);

                if (my_rank == 0) {
                        sum = 0;
                        for (i = 0; i < commsize; i++) {
                                long double partSum = 0;
                                MPI_Recv(&partSum, 1, MPI_LONG_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                                sum += partSum;
                        }
                        printf("SUM: %LF\n", sum);
                }


        MPI_Finalize();
        return 0;
}
