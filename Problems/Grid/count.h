#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>

#define ERROR(string) fprintf(stderr, "Error = %s\t line = %d, file = %s\n", string, __LINE__, __FILE__)

double begin_x = 0.0;
double end_x = 1.0;
double begin_t = 0.0;
double end_t = 1.0;

double a = 2.0;
//equation dU/dt + 2dU/dx = x + t

//initial conditions dU/dx(x = 0, t) = e ^ (-t), dU/dt(x, t = 0) = cos(pi * x)


typedef struct _Data {
	int M;
	int T;
	double** arr;
} Data;

double function(double x, double t) {
	return x + t;
}

double initial_t(double t) {
	return exp(-1 * t);
}

double initial_x(double x) {
	return cos(M_PI * x);
}

void init_conditions(Data* t, double m_step, double t_step) {
	int i, j;
	for(i = 0; i < t->M; ++i) {
		for(j = 0; j < t->T; ++j) {
			if ( i == t->M - 1 ) {
				t->arr[i][j] = initial_t(t_step * j);
			}
			else if ( j == 0 ) {
				t->arr[i][j] = initial_x(m_step * i);
			}
			else {
				t->arr[i][j] = 0;
			}
		}
	}
}


void check_number_arguments(int argc) {
	if ( argc != 3 ) {
		ERROR("Error in input");
		exit(1);
	}
}

void convert_to_step(char** argv, int* M, int* H) {
	(*M) = atoi(argv[1]);
	(*H) = atoi(argv[2]);
}

void init_grid(Data* t, int M, int T) {
	int i;
	t->M = M;
	t->T = T;
	t->arr = (double**) malloc(t->M * sizeof(double*));
	if ( t->arr == NULL ) {
		ERROR("ERROR IN MALLOC");
		exit(1);
	}
	for(i = 0; i < t->M; ++i) {
		t->arr[i] = (double*) malloc(t->T * sizeof(double));
		if ( t->arr[i] == NULL ) { 
			ERROR("ERROR IN MALLOC");
			exit(2);
		}
	}
}

void print_grid(Data* t, int rank) {
	int i, j;
	//ERROR("print grid");
	for(i = 0; i < t->M; ++i) {
		for(j = 0; j < t->T; ++j) {
			printf("(rg= %d, i = %d, j = %d) %lf ", rank, i, j, t->arr[i][j]);
		}
		printf("\n");
	}
}

void clear_grid(Data* t) {
	int i;
	for(i = 0; i < t->M; ++i) {
		free(t->arr[i]);
	}
	free(t->arr);
}

double calc_r_up(double l_down, double l_up, double r_down, double h, double tau, int i, int j);	//definition

void solution(Data* t, double h, double tau) {		//calculate right_up
	int i, j;
	double l_up, l_down, r_down;
	for(i = t->M - 2; i >= 0; i--) {
		for(j = 1; j < t->T; ++j) {
			//printf("i = %d, j = %d\n", i, j);
			l_up = t->arr[i][j - 1];
			l_down = t->arr[i + 1][j - 1];
			r_down = t->arr[i + 1][j];
			t->arr[i][j] = calc_r_up(l_down, l_up, r_down, h, tau, i, j);		}
	}
}

void union_result(Data* t, int rank) {
	int i, j;
	double reply;
	for( i = t->M - 2; i >= 0; i--) {
		for(j = 1; j < t->T; ++j) {
			MPI_Reduce(&(t->arr[i][j]), &(reply), 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
			if ( rank == 0 ) {
				t->arr[i][j] = reply;
			}
		}
	}
}

double calc_r_up(double l_down, double l_up, double r_down, double h, double tau, int i, int j) {
	return ( 2 * h * tau ) / ( h + a * tau ) * ( function((i - 0.5) * tau, (j - 0.5) * h) ) - ( r_down - l_down - l_up ) / ( 2 * tau ) - a * ( l_up - l_down - r_down ) / ( 2 * h);
} 

void multi_solution(Data* t, double h, double tau, int rank, int commsize, MPI_Status status) {
	int i, j;
	double l_up, l_down, r_down, r_up;
	int prev_proc = rank - 1;
	int next_proc = rank + 1;
	if ( rank == 0 ) {
		prev_proc = commsize - 1;
	}
	else if ( rank == commsize - 1 ) {
		next_proc = 0;
	}
	//printf("prev= %d, my = %d, next = %d\n", prev_proc, rank, next_proc);
	for(i = t->M - 2 - rank; i >= 0; i = i - commsize) {
		for(j = 1; j < t->T; ++j) {
			if ( i == t->M - 2 ) {	
				l_up = t->arr[i][j - 1];
                        	l_down = t->arr[i + 1][j - 1];
                        	r_down = t->arr[i + 1][j];
				t->arr[i][j] = calc_r_up(l_down, l_up, r_down, h, tau, i, j);
				r_up = t->arr[i][j];
				//MPI_Send(&(t->arr[i][j]), 1, MPI_FLOAT, next_proc, j, MPI_COMM_WORLD);		//send r_up
			}
			else {
				//printf("WAIT\n");
				MPI_Recv(&(r_down), 1, MPI_DOUBLE, prev_proc, j, MPI_COMM_WORLD, &status);		//recv r_down	
				l_up = t->arr[i][j - 1];
                                if ( j == 1 ) {
					l_down = t->arr[i + 1][j - 1];
				}	
				t->arr[i][j] = calc_r_up(l_down, l_up, r_down, h, tau, i, j);
				r_up = t->arr[i][j];
				l_down = r_down; 		//IMPORTANT ACTION
			}
			MPI_Send(&(r_up), 1, MPI_DOUBLE, next_proc, j, MPI_COMM_WORLD);
		}
	}
	union_result(t, rank);
}

