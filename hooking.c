#include <dlfcn.h>
#include <err.h>
#include <gnu/lib-names.h>
#include <mpi.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>

#include "callout.h"
#include "hooking.h"
#include "parser.h"

#define SMASH_GRAPH 0x1234

timer_t smash_timer_id;
unsigned int smash_my_rank;
int smash_dead, smash_world_size, smash_alarm;

struct cfg_delays *smash_delays;
struct cfg_failures *smash_failures;

struct smash_graph_msg {
	int src, dst;
};

struct smash_graph_msgs {
	size_t i;
	struct smash_graph_msg msgs[4096];
} smash_graph_msgs;

static int master_done = 0;

int
smash_failure(void)
{
	int buf;
	MPI_Status status;
	size_t recv = 0;
	int (*f)();

	smash_dead = 1;
	f = smash_get_lib_func(LIBMPI, "MPI_Recv");
	while (recv != smash_world_size - smash_failures->size) {
		f(&buf, 1, MPI_INT, MPI_ANY_SOURCE, 0xdead, MPI_COMM_WORLD, &status);
		recv++;
	}
	MPI_Finalize();
	exit(0);
}

int
MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) {
	int (*f)(), res;

	f = smash_get_lib_func(LIBMPI, "MPI_Recv");

	while (1) {
		res = f(buf, count, datatype, source, tag, comm, status);
		if (status->MPI_TAG != 0xdead || status->MPI_TAG != SMASH_GRAPH)
			break;
		bzero(status, sizeof(MPI_Status));
		master_done = status->MPI_TAG == SMASH_GRAPH;
	}

	smash_graph_msgs.msgs[smash_graph_msgs.i].src = status->MPI_SOURCE;
	smash_graph_msgs.msgs[smash_graph_msgs.i].dst = smash_my_rank;
	smash_graph_msgs.i++;

	return res;
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

	if (smash_parse_cfg(CFG_DELAY, (void **)&smash_delays) < 0)
		errx(EXIT_FAILURE, "error in CFG_DELAY\n");

	if (smash_parse_cfg(CFG_FAILURE, (void **)&smash_failures) < 0)
		errx(EXIT_FAILURE, "error in CFG_FAILURE\n");

	f = smash_get_lib_func(LIBSTD, "__libc_start_main");
	smash_alarm = 0;
	smash_dead = 0;
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

	smash_graph_msgs.i = 0;

	f = smash_get_lib_func(LIBMPI, "MPI_Init");
	res = f(argc, argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	smash_my_rank = rank;
	MPI_Comm_size(MPI_COMM_WORLD, &smash_world_size);

	if (smash_failures != NULL) {
		for (i = 0; i < smash_failures->size; ++i) {
			if (smash_failures->failures[i].node == smash_my_rank) {
				smash_timeout(smash_failure, 0, smash_failures->failures[i].time, NULL);
			}
		}
	}
	return res;
}

int
MPI_Finalize(void)
{
	int (*f)(void);
	size_t i, j;
	int (*ssend)();
	int (*recv)();

	recv = smash_get_lib_func(LIBMPI, "MPI_Recv");
	ssend = smash_get_lib_func(LIBMPI, "MPI_Ssend");

	if (smash_failures != NULL) {
		if (!smash_dead) {
			for (i = 0; i < smash_failures->size; i++)
				ssend(&smash_world_size, 1, MPI_INT, smash_failures->failures[i].node, 0xdead, MPI_COMM_WORLD);
		}
	}

	int done;
	if (smash_my_rank == 0) {
		struct smash_graph_msgs tmp = {0};
		MPI_Status status;
		for (i = 1; i < (unsigned int)smash_world_size; ++i) {
			done = 1;
			ssend(&done, 1, MPI_INT, i, SMASH_GRAPH, MPI_COMM_WORLD, &status);
			recv(&tmp, sizeof(struct smash_graph_msgs), MPI_CHAR,
			     i, SMASH_GRAPH, MPI_COMM_WORLD,
			     &status);

			for (j = 0; j < tmp.i; ++j) {
				smash_graph_msgs.msgs[smash_graph_msgs.i].src = tmp.msgs[j].src;
				smash_graph_msgs.msgs[smash_graph_msgs.i].dst = tmp.msgs[j].dst;
				smash_graph_msgs.i++;
			}
		}
		/* Output graph */
		printf("digraph SMASH_MPI {\n layout=twopi\n ranksep=3;\n ratio=auto;\n");
		for (i = 0; i < smash_graph_msgs.i; ++i) {
			printf("\"p%d\" -> \"p%d\" [ color=\"purple\" ];\n",
			       smash_graph_msgs.msgs[i].src,
			       smash_graph_msgs.msgs[i].dst);
		}
		puts("}");
		fflush(stdout);
	} else {
		if (!master_done)
			recv(&done, 1, MPI_INT, 0, SMASH_GRAPH, MPI_COMM_WORLD);
		ssend(&smash_graph_msgs, sizeof(struct smash_graph_msgs),
		     MPI_CHAR, 0, SMASH_GRAPH, MPI_COMM_WORLD);
	}

	free(smash_delays);
	free(smash_failures);
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
		.count = count,
		.datatype = datatype,
		.dest = dest,
		.tag = tag,
		.comm = comm,
	};
	args.buf = malloc(sizeof(buf) * count);
	memcpy(args.buf, buf, sizeof(buf) * count);

	f = smash_get_lib_func(LIBMPI, "MPI_Ssend");

	for (i = 0; i < smash_delays->size; ++i) {
		/* If a delay in the config file matches our rank and the target rank, inject it in the callout struct. */
                if (smash_delays->delays[i].dst == (unsigned int)dest &&
                    smash_delays->delays[i].src == smash_my_rank &&
		    (smash_delays->delays[i].msg > 0 ||
		     smash_delays->delays[i].msg == -1)) {
                        sem_wait(smash_timeout(f, 6, smash_delays->delays[i].delay, &args));
			smash_delays->delays[i].msg -= 1 * (smash_delays->delays[i].msg != -1);
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
		.count = count,
		.datatype = datatype,
		.dest = dest,
		.tag = tag,
		.comm = comm,
	};
	args.buf = malloc(sizeof(buf) * count);
	memcpy(args.buf, buf, sizeof(buf) * count);

	f = smash_get_lib_func(LIBMPI, "MPI_Send");

	for (i = 0; i < smash_delays->size; ++i) {
		/* If a delay in the config file matches our rank and the target rank, inject it in the callout struct. */
                if (smash_delays->delays[i].dst == (unsigned int)dest &&
                    smash_delays->delays[i].src == smash_my_rank &&
		    (smash_delays->delays[i].msg > 0 ||
		     smash_delays->delays[i].msg == -1)) {
                        smash_timeout(f, 6, smash_delays->delays[i].delay, &args);
			smash_delays->delays[i].msg -= 1 * (smash_delays->delays[i].msg != -1);
			return 0;
                }
        }
	/* If there is no delay to apply, call MPI_Send directly. */
	return f(buf, count, datatype, dest, tag, comm);
}
