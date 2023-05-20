#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>


#define MASTER 0

int main(int argc, char **argv)
{
	int my_rank, nb_proc, i;
	MPI_Status status;
	int long t = 1;
	double t1, t2;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	if (my_rank != MASTER) {
		MPI_Send(&t, 1, MPI_LONG, MASTER, 1, MPI_COMM_WORLD);
		MPI_Send(&t, 1, MPI_LONG, MASTER, 2, MPI_COMM_WORLD);
		MPI_Send(&t, 1, MPI_LONG, MASTER, 3, MPI_COMM_WORLD);
		MPI_Send(&t, 1, MPI_LONG, MASTER, 4, MPI_COMM_WORLD);
	} else {
		t1 = MPI_Wtime();
		for (i = 0; i < (nb_proc - 1) * 4; i++) {
			MPI_Recv(&t, 1, MPI_LONG, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			t2 = MPI_Wtime();
			printf("delay before first MPI_Recv = %f from %d\n", t2 - t1, status.MPI_SOURCE);
		}
	}

	MPI_Finalize();
	return 0;
}
