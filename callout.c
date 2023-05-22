#include "callout.h"

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <time.h> 

struct callo callout[NCALL] = { 0 };
extern timer_t smash_timer_id;

void
smash_print_callout(void)
{
	size_t i;

	for (i = 0; callout[i].c_func != NULL; ++i)
		printf("c_time: %lli\nc_arg: %i\nc_func @ %p\n\n",
		       callout[i].c_time, callout[i].c_arg,
		       (void *)callout[i].c_func);
}

void
smash_set_timer(int time)
{
	int t, s, ns;
	struct itimerspec curr;

	timer_gettime(smash_timer_id, &curr);

	if (time == -1 || (curr.it_value.tv_nsec == 0 && curr.it_value.tv_sec == 0)) {
		t = time == -1 ? callout[0].c_time : time;
		s = t / 1000;
		ns = (t % 1000) * 1000000;

		if (s == 0 && ns == 0)
			return;

		struct itimerspec its;

		its.it_value.tv_sec = s;
		its.it_value.tv_nsec = ns;

		its.it_interval.tv_sec = 0;
		its.it_interval.tv_nsec = 0;
		if (timer_settime(smash_timer_id, 0, &its, NULL) < 0)
			err(1, "timer_settime");
	}
}

sem_t *
smash_timeout(int (*func)(), int arg, int time, struct mpi_send_args *args)
{
	int t;
        struct callo *p1, *p2;


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

	smash_set_timer(t);

	return &p1->c_lock;
}

void
smash_clock(void)
{
	struct callo *p1, *p2;

	if (callout[0].c_func != 0) {
		p1 = p2 = &callout[0];
		p1->c_time = 0;

		while (p1->c_func != 0 && p1->c_time == 0) {
			switch (p1->c_arg) {
			case 6:
				p1->c_func(p1->c_send_args.buf, p1->c_send_args.count,
					   p1->c_send_args.datatype, p1->c_send_args.dest,
					   p1->c_send_args.tag, p1->c_send_args.comm);
				free(p1->c_send_args.buf);
				sem_post(&p1->c_lock);
				break;
			case 0:
				p1->c_func();
				break;
			}
			p1++;
		}

		while ((p2->c_func = p1->c_func)) {
			p2->c_time = p1->c_time;
			p2->c_arg = p1->c_arg;
			p2->c_send_args = p1->c_send_args;
			p1++;
			p2++;
		}
		smash_set_timer(-1);
	}
}
