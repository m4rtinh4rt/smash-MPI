#ifndef CALLOUT_H
#define CALLOUT_H

#define NCALL 10 /* FIXME: Change size */
#define SMASH_CLOCK 100000

#include <mpi.h>

struct mpi_send_args {
	void *buf;
	MPI_Comm comm;
	MPI_Datatype datatype;
	int count, dest, tag;
};

struct callo {
	/* FIXME: change c_time to a bigger type. */
	int c_time;                       /* incremental time */
	int c_arg;                        /* argument to routine */
	int (*c_func)();                  /* routine */
	struct mpi_send_args c_send_args; /* args for MPI_Send */
};

void smash_print_callout(void);

void smash_timeout(int (*func)(), int arg, int time,
                   struct mpi_send_args *args);

void smash_clock(void);

#endif /* CALLOUT_H */
