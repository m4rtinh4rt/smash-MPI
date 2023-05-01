#include <dlfcn.h>
#include <err.h>
#include <gnu/lib-names.h>
#include <mpi.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "callout.h"
#include "hooking.h"
#include "parser.h"

int smash_alarm;
timer_t smash_timer_id;
unsigned int smash_my_rank;
struct cfg_delays *smash_delays;
struct cfg_failures *smash_failures;

int
smash_failure(void)
{
	int buf, world;
	MPI_Status status;
	size_t recv = 0;

	MPI_Comm_size(MPI_COMM_WORLD, &world);

	while (recv != world - smash_failures->size) {
		MPI_Recv(&buf, 1, MPI_INT, MPI_ANY_SOURCE, 0xdead, MPI_COMM_WORLD, &status);
		recv++;
	}
	return 0;
}

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
}

timer_t
smash_setup_alarm(void)
{
	timer_t timerid;
	struct sigaction sa;
	struct sigevent sev;

	sa.sa_handler = smash_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sigaction(SIGALRM, &sa, NULL);

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) < 0)
		errx(1, "timer_create");

	return timerid;
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

	if (!(smash_failures = smash_parse_cfg(CFG_FAILURE)))
		errx(EXIT_FAILURE, "error in CFG_FAILURE\n");

	f = smash_get_lib_func(LIBSTD, "__libc_start_main");
	smash_alarm = 0;
	return f(main, argc, argv, init, fini, rtld_fini, stack_end);
}

int
MPI_Init(int *argc, char ***argv)
{
	unsigned int i;
	int (*f)(int *, char ***), res, rank;

        if (!smash_alarm) {
		smash_timer_id = smash_setup_alarm();
		smash_alarm = 1;
	}

	f = smash_get_lib_func(LIBMPI, "MPI_Init");
	res = f(argc, argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	smash_my_rank = rank;

	for (i = 0; i < smash_failures->size; ++i) {
                if (smash_failures->failures[i].node == smash_my_rank)
			smash_timeout(smash_failure, 0, smash_failures->failures[i].time, NULL);
        }
	return res;
}

int
MPI_Finalize(void)
{
	int (*f)(void);
	int world;
	size_t i;

	MPI_Comm_size(MPI_COMM_WORLD, &world);
	for (i = 0; i < smash_failures->size; i++)
		MPI_Send(&world, 1, MPI_INT, smash_failures->failures[i].node, 0xdead, MPI_COMM_WORLD);

	free(smash_delays);
	f = smash_get_lib_func(LIBMPI, "MPI_Finalize");
	return f();
}

int
MPI_Ssend(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm)
{
	int (*f)();
	unsigned int i;
	struct mpi_send_args args = {
		.buf = (void *)buf,
		.count = count,
		.datatype = datatype,
		.dest = dest,
		.tag = tag,
		.comm = comm,
	};

	f = smash_get_lib_func(LIBMPI, "MPI_Ssend");

	for (i = 0; i < smash_delays->size; ++i) {
		/* If a delay in the config file matches our rank and the target rank, inject it in the callout struct. */
                if (smash_delays->delays[i].dst == (unsigned int)dest &&
                    smash_delays->delays[i].src == smash_my_rank) {
                        sem_wait(smash_timeout(f, 6, smash_delays->delays[i].delay, &args));
			return 0;
                }
        }
	/* If there is no delay to apply, call MPI_Ssend directly. */
	return f(buf, count, datatype, dest, tag, comm);
}

int
MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest,
             int tag, MPI_Comm comm)
{
	int (*f)();
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

	for (i = 0; i < smash_delays->size; ++i) {
		/* If a delay in the config file matches our rank and the target rank, inject it in the callout struct. */
                if (smash_delays->delays[i].dst == (unsigned int)dest &&
                    smash_delays->delays[i].src ==smash_my_rank) {
                        smash_timeout(f, 6, smash_delays->delays[i].delay, &args);
			return 0;
                }
        }
	/* If there is no delay to apply, call MPI_Send directly. */
	return f(buf, count, datatype, dest, tag, comm);
}
