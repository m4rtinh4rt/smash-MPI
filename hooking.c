#include <dlfcn.h>
#include <err.h>
#include <gnu/lib-names.h>
#include <mpi.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "callout.h"
#include "hooking.h"
#include "parser.h"

struct cfg_delays *smash_delays;

void *
smash_get_lib_func(const char *lname, const char *fname)
{
	void *lib, *p;

	if (!(lib = dlopen(lname, RTLD_LAZY)))
		errx(EXIT_FAILURE, "%s", dlerror());

	if (!(p = dlsym(lib, fname)))
		errx(EXIT_FAILURE, "%s", dlerror());

	dlclose(lib);
	return p;
}

static void
smash_handler(__attribute__((unused)) int signum)
{
	smash_clock();
	alarm(1);
}

void
smash_setup_alarm(void)
{
	struct sigaction sa;

	sa.sa_handler = smash_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	sigaction(SIGALRM, &sa, NULL);
	alarm(1);
}

int
__libc_start_main(
	int (*main)(int, char **, char **),
	int argc,
	char **argv,
	int (*init)(int, char **, char **),
	void (*fini)(void),
	void (*rtld_fini)(void),
	void *stack_end)
{
	int (*f)();

	if (!(smash_delays = smash_parse_cfg(CFG_DELAY)))
		errx(EXIT_FAILURE, "error in CFG_DELAY\n");

	f = smash_get_lib_func(LIBSTD, "__libc_start_main");
	smash_setup_alarm();
	return f(main, argc, argv, init, fini, rtld_fini, stack_end);
}

int
MPI_Finalize(void)
{
	int (*f)(void);

	free(smash_delays);
	f = smash_get_lib_func(LIBMPI, "MPI_Finalize");
	return f();
}

int
MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm)
{
	int (*f)(), my_rank;
	unsigned int i;
	struct mpi_send_args args = {
		.buf = (void *)buf,
		.count = count,
		.datatype = datatype,
		.dest = dest,
		.tag = tag,
		.comm = comm,
	};

	f = smash_get_lib_func(LIBMPI, "MPI_Send");
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

	for (i = 0; i < smash_delays->size; ++i) {
		/* If a delay in the config file matches our rank and the target rank, inject it in the callout struct. */
		if (smash_delays->delays[i].dst == (unsigned int)dest && smash_delays->delays[i].src == (unsigned int)my_rank) {
			smash_timeout(f, 6, smash_delays->delays[i].delay, &args);
			return 0;
		}
	}
	/* If there is no delay to apply, call MPI_Send directly. */
	return f(buf, count, datatype, dest, tag, comm);
}
