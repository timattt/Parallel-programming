#include <mpi.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
        int commsize = 0;
        int my_rank = 0;
        int data = 0;

        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &commsize);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        if (my_rank == 0) {
                MPI_Send(&data, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
                printf("Initial process[%d] sent data[%d]\n", my_rank, data);
        }

        MPI_Recv(&data, 1, MPI_INT, my_rank - 1, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Process[%d] received data[%d]\n", my_rank, data);

        data++;
        if (my_rank == commsize - 1) {
                MPI_Send(&data, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
                printf("Last process[%d] sent data[%d]\n", my_rank, data);
        }
        if (my_rank > 0 && my_rank < commsize - 1) {
                MPI_Send(&data, 1, MPI_INT, my_rank + 1, 0, MPI_COMM_WORLD);
                printf("Process[%d] sent data[%d]\n", my_rank, data);
        }

        MPI_Finalize();
        return 0;
}
