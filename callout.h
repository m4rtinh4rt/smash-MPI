#ifndef CALLOUT_H
#define CALLOUT_H

#define NCALL 256
#define SMASH_CLOCK 100000 /* TODO: remove interval, jump to next timer directly */

#include <mpi.h>
#include <semaphore.h>

struct mpi_send_args {
	void *buf;
	MPI_Comm comm;
	MPI_Datatype datatype;
	int count, dest, tag;
};

struct callo {
	long long c_time;                 /* incremental time */
	int c_arg;                        /* argument to routine */
	int (*c_func)();                  /* routine */
	sem_t c_lock;			  /* lock to preserve locking semantic */
	struct mpi_send_args c_send_args; /* args for MPI_Send */
};

void smash_print_callout(void);

sem_t *smash_timeout(int (*func)(), int arg, int time,
                     struct mpi_send_args *args);

void smash_clock(void);

#endif /* CALLOUT_H */
