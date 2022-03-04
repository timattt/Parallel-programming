#include <mpi.h>
#include <stdio.h>

client_loop: send disconnect: Connection reset

C:\Users\timat>gc, char *argv[]) {
        int commsize = 0;
        int my_rank = 0;
        long long i = 0;

        MPI_Init(&argc, &argv);
        MPI_Comm_size(MPI_COMM_WORLD, &commsize);
        MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

        printf("Process %d started. Total %d processes.\n", my_rank, commsize);

        long long startN = my_rank * N / commsize;
        long long endN = (my_rank + 1) * N / commsize;

        long double res = 0;
        long long fac = 1;

        for (i = startN; i < endN; i++) {
                printf("Process %d fac: %d\n", my_rank, fac);
                if (i != 0) {
                        fac *= i;
                }
                res += (1.0 / (long double)(fac));
        }
        printf("Process %d final fac: %d\n", my_rank, fac);

        long double prevRes = 0;
        long long prevFac = 1;

        if (my_rank != 0) {
                MPI_Recv(&prevRes, 1, MPI_LONG_DOUBLE, my_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&prevFac, 1, MPI_LONG_LONG, my_rank - 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                printf("Process %d received prevFact = %d\n", my_rank, prevFac);
        }

        res /= (long double) prevFac;
        fac *= prevFac;
        res += prevRes;

        if (my_rank != commsize - 1) {
                MPI_Send(&res, 1, MPI_LONG_DOUBLE, my_rank + 1, 0, MPI_COMM_WORLD);
                MPI_Send(&fac, 1, MPI_LONG_LONG, my_rank + 1, 1, MPI_COMM_WORLD);
                printf("Process %d is sending fact = %d\n", my_rank, fac);
        } else {
                printf("Result: %LF\n", res);
        }

        MPI_Finalize();
        return 0;
}
