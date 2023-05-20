#include <mpi.h>
#include <stdio.h>
#include <string.h>

#define MASTER 0
#define SIZE   128

int main(int argc, char **argv)
{
	int my_rank, nb_proc, i;
	char message[4][SIZE];
	MPI_Status status;
	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &nb_proc);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	if (my_rank != MASTER) {
		for (i = 0; i < 4; i++) {
			sprintf(message[i], "Hello from %d [%d]", my_rank, i);
		}
		MPI_Send(message[0], strlen(message[0]) + 1, MPI_CHAR, MASTER, 1, MPI_COMM_WORLD);
		MPI_Send(message[1], strlen(message[0]) + 1, MPI_CHAR, MASTER, 2, MPI_COMM_WORLD);
		MPI_Send(message[2], strlen(message[0]) + 1, MPI_CHAR, MASTER, 3, MPI_COMM_WORLD);
		MPI_Send(message[3], strlen(message[0]) + 1, MPI_CHAR, MASTER, 4, MPI_COMM_WORLD);
	} else {
		for (i = 0; i < (nb_proc - 1) * 4; i++) {
			MPI_Recv(message[0], SIZE, MPI_CHAR, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			printf("%s\n", message[0]);
		}
	}

	MPI_Finalize();
	return 0;
}
