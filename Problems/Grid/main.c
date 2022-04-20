#include "count.h"

int main(int argc, char** argv) {
	check_number_arguments(argc);
	int M_step, T_step;	//number of step
	convert_to_step(argv, &M_step, &T_step);
	printf("%d %d\n", M_step, T_step);
	double m = ( end_x - begin_x ) / M_step;
	double t = ( end_t - begin_t ) / T_step;
	int commsize, rank;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	Data grid;
	init_grid(&grid, M_step, T_step);
	init_conditions(&grid, m, t);
	double begin_time = MPI_Wtime();
	if ( commsize == 1 ) {
		solution(&grid, m, t);
	}
	else {
		multi_solution(&grid, m, t, rank, commsize, status);
	}
	//if ( rank == 0 ) {		//IF you want to see result, clear this commentary
	//	print_grid(&grid, rank);
	//}
	double end_time = MPI_Wtime();
	printf("Time = %lf sec\n", end_time - begin_time);
	clear_grid(&grid);
	MPI_Finalize();
	return 0;
}

