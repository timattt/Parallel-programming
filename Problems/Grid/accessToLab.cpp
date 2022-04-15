#include "helper.h"

#define num_circles 10000

int main (int argc , char** argv)
{
    int size = 0 , rank = 0 , num = 0;

    act (MPI_Init (NULL , NULL));
    act (MPI_Comm_size (MPI_COMM_WORLD , &size));
    act (MPI_Comm_rank (MPI_COMM_WORLD , &rank));

    auto start_time = MPI_Wtime ();
    for (int i = 0; i < num_circles; ++i)
    {
        if (rank != mpi::rank_main)
        {
            act (MPI_Recv (&num , /*size*/ 1 , MPI_INT , rank - 1 , 0 , MPI_COMM_WORLD , MPI_STATUS_IGNORE));
        }
        if (rank != size - 1)
            act (MPI_Send (&num , /*size*/ 1 , MPI_INT , rank + 1 , 0 , MPI_COMM_WORLD));
        else // last
            act (MPI_Send (&num , /*size*/ 1 , MPI_INT , /*rank*/ 0 , 0 , MPI_COMM_WORLD));

        if (rank == mpi::rank_main)
        {
            act (MPI_Recv (&num , /*size*/ 1 , MPI_INT , /*rank*/ size - 1 , 0 , MPI_COMM_WORLD , MPI_STATUS_IGNORE));
        }
    }
    auto end_time = MPI_Wtime();
    if (rank == mpi::rank_main)
        std::cout << "\n\n" << (end_time - start_time) / num_circles;

    quit ();
    return 0;
}
