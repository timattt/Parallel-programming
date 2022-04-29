#include "count.h"

int main(int argc, char** argv) {
	// ARGS
	check_number_arguments(argc);
	
	int M_step, T_step;	//number of step
	convert_to_step(argv, &M_step, &T_step);
	
	printf("%d %d\n", M_step, T_step);
	
	// STEPS
	double m = ( end_x - begin_x ) / M_step;
	double t = ( end_t - begin_t ) / T_step;
	
	// MPI INIT
	int commsize, rank;
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &commsize);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	
	// DATA
	Data grid;
	init_grid(&grid, M_step, T_step);
	init_conditions(&grid, m, t);
	
	double begin_time = MPI_Wtime();
	
	// IF ONLY ONE WORKER THEN NO MPI IS USED
	if ( commsize == 1 ) {
		solution(&grid, m, t);
	}
	// ELSE USE MPI
	else {
		multi_solution(&grid, m, t, rank, commsize, status);
	}
	if ( rank == 0 ) {
		print_grid(&grid, rank);
	}
	
	// TIME
	double end_time = MPI_Wtime();
	printf("Time = %lf sec\n", end_time - begin_time);
	
	// CLEANUP
	clear_grid(&grid);
	MPI_Finalize();
	
	return 0;
}

