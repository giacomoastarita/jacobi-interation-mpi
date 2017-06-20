/*
 ============================================================================
 Name        : Jacobi_v2.c
 Author      : Giacomo Astarita
 Version     :
 Copyright   : Your copyright notice
 Description : Jacobi Laplace  in C
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "mpi.h"
#define MATRIX_SIZE 3000

int main(int argc, char* argv[]){
	int  rank; /* rank of process */
	int  n_processes;       /* number of processes */
	double start, finish;
	double diffnorm, gdiffnorm;
	double **xlocal;
	double **xnew;
	int i, j;
	int send[2];
	/* return status for receive */
	MPI_Status status ;

	/* seed */
	srand(13);
	
	/* start up MPI */
	MPI_Init(&argc, &argv);
	/* find out process rank */
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	/* find out number of processes */
	MPI_Comm_size(MPI_COMM_WORLD, &n_processes);
	/* If the matrix is divisible for the processes */
	if(n_processes!=1){
		if (MATRIX_SIZE%(n_processes-1) != 0){
			printf("The matrix is not divisible for processes' number. Reboot your code and change the processes' number. \n");
			fflush(stdout);
			MPI_Abort( MPI_COMM_WORLD, 1);
		}
	}
	/*Start to estimates of computation time */
	printf("Start of computation of processes %d of %d...\n\n", rank, n_processes);
	fflush(stdout);
	start = MPI_Wtime();
	/* Allocation dynamic matrix */
	xlocal= (double **) malloc(MATRIX_SIZE * sizeof(double *));
	for(i = 0; i< MATRIX_SIZE; i++)
		xlocal[i]= (double *) malloc(MATRIX_SIZE * sizeof(double *));
	xnew= (double **) malloc(MATRIX_SIZE * sizeof(double *));
	for(i = 0; i< MATRIX_SIZE; i++)
		xnew[i]= (double *) malloc(MATRIX_SIZE * sizeof(double *));

	/* Create matrix */
	for (i = 0; i < MATRIX_SIZE; i++) {
		for (j = 0; j < MATRIX_SIZE; j++){
			if(i == 0 || i == MATRIX_SIZE-1)
				xlocal[i][j] = -1; //FIRST ROW e LAST ROW
			else
				xlocal[i][j] = rand() % 10; //OTHER ROWS
		}
	}

	if (rank == 0){
		if(n_processes == 1){
			//sequential
			int iteration = 0;
			do {
				iteration++;
				/*Calculate the difforms and the iterations*/
				diffnorm = 0.0;
				for (i=1; i<MATRIX_SIZE-1; i++){
					for (j=1; j<MATRIX_SIZE-1; j++) {
						xnew[i][j] = (xlocal[i+1][j] + xlocal[i-1][j] + xlocal[i][j+1] + xlocal[i][j-1]) / 4;
						diffnorm += (xnew[i][j] - xlocal[i][j]) * (xnew[i][j] - xlocal[i][j]);
					}
				}
				for (i=1; i<=MATRIX_SIZE-1; i++){
					for (j=1; j<MATRIX_SIZE-1; j++)
						xlocal[i][j] = xnew[i][j];
				}
				diffnorm = sqrt( diffnorm );
			} while (diffnorm > 1.0e-2 && iteration < 100);
			printf( "At iteration n° %d, the difform is %e\n", iteration, diffnorm );
		}else{
			//parallel
			int work = MATRIX_SIZE/(n_processes-1); //work for processes
			int row_start = 0, row_finish=0;
			int index_processes;
			for (index_processes = 1; index_processes <= n_processes-1; index_processes++) {
				if(index_processes == 1){
					row_start = 1;
					row_finish += work;
				}else if(index_processes == n_processes-1){
					row_start = row_finish;
					row_finish = MATRIX_SIZE - 2;
				}else{
					row_start = row_finish;
					row_finish += work;
				}
				send[0] = row_start;
				send[1] = row_finish - 1;
				MPI_Send(send, 2, MPI_INT, index_processes, 99, MPI_COMM_WORLD);
			}
			int iteration=0;
			do{
				iteration++;
				diffnorm=0.0;
				MPI_Allreduce(&diffnorm, &gdiffnorm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
				gdiffnorm=sqrt(gdiffnorm);
			}while(gdiffnorm > 1.0e-2 && iteration < 100);

			printf( "At iteration n. %d, the diffnorm is %e\n", iteration, gdiffnorm);
		}
		/*End to estimates of computation time */
		printf("\n...End of computation time\n");
		fflush(stdout);
		finish = MPI_Wtime();
		printf("Computation time: %f\n", finish - start);
		fflush(stdout);
	}
	else{
		MPI_Recv(&send, 2, MPI_INT, 0, 99, MPI_COMM_WORLD, &status);
		int iteration=0;

		/*Calculate the diffnorms*/
		do{
			iteration++;
			diffnorm = 0.0;
			for (i = send[0]; i <= send[1]; i++) {
				for (j = 1; j < MATRIX_SIZE - 1; j++) {
					xnew[i][j] = (xlocal[i + 1][j] + xlocal[i - 1][j]+ xlocal[i][j + 1] + xlocal[i][j - 1]) / 4;
					diffnorm += (xnew[i][j] - xlocal[i][j])* (xnew[i][j] - xlocal[i][j]);
				}
			}
			for (i = send[0]; i <= send[1]; i++) {
				for (j = 1; j < MATRIX_SIZE - 1; j++) {
					xlocal[i][j] = xnew[i][j];
				}
			}
			MPI_Allreduce(&diffnorm, &gdiffnorm, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
			gdiffnorm = sqrt(gdiffnorm);
		}while(gdiffnorm > 1.0e-2 && iteration < 100);
	}
	/* shut down MPI */
	MPI_Finalize(); 
	return 0;
}
