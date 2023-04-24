#include "callout.h"

#include <stdio.h>
#include <string.h>
#include <semaphore.h>

struct callo callout[NCALL] = { 0 };

inline void
smash_print_callo(struct callo *c)
{
	printf("c_time: %lli\nc_arg: %i\nc_func @ %p\n\n", c->c_time, c->c_arg,
	       (void *)&c->c_func);
}

void
smash_print_callout(void)
{
	size_t i;

	for (i = 0; callout[i].c_func != NULL; ++i)
		smash_print_callo(&callout[i]);
}

sem_t *
smash_timeout(int (*func)(), int arg, int time, struct mpi_send_args *args)
{
        struct callo *p1, *p2;
        int t;

        t = time;
        p1 = &callout[0];
        while (p1->c_func != 0 && p1->c_time <= t) {
                t -= p1->c_time;
                p1++;
        }
        p1->c_time -= t;
        p2 = p1;
        while (p2->c_func != 0)
                p2++;
        while (p2 >= p1) {
                (p2+1)->c_time = p2->c_time;
                (p2+1)->c_func = p2->c_func;
                (p2+1)->c_arg = p2->c_arg;
                (p2+1)->c_send_args = p2->c_send_args;
                p2--;
        }
        p1->c_time = t;
        p1->c_func = func;
        p1->c_arg = arg;
	if (args != NULL)
		memcpy(&p1->c_send_args, args, sizeof(struct mpi_send_args));
	return &p1->c_lock;
}

void
smash_clock(void)
{
	struct callo *p1;

	if (callout[0].c_func != 0) {
		p1 = &callout[0];
		/* Ignore tasks that have already run. */
		while (p1->c_time == -0xdead && p1->c_func != 0)
			p1++;

		p1->c_time = ((p1->c_time - SMASH_CLOCK) < 0) ? 0 : p1->c_time - SMASH_CLOCK;
		if (p1->c_time == 0 && p1->c_func != 0) {
			/* This is called by multiple processes */
			switch (p1->c_arg) {
			case 6:
				p1->c_func(p1->c_send_args.buf, p1->c_send_args.count,
					   p1->c_send_args.datatype, p1->c_send_args.dest,
					   p1->c_send_args.tag, p1->c_send_args.comm);
				sem_post(&p1->c_lock);
				break;
			case 0:
				p1->c_func();
				break;
			}
			/* Then ignore for the next round, by setting a negative timeout */
                        p1->c_time = -0xdead;
		}
	}
}
