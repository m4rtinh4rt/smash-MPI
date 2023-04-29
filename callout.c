#include "callout.h"

#include <err.h>
#include <stdio.h>
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

sem_t *
smash_timeout(int (*func)(), int arg, int time, struct mpi_send_args *args)
{
	struct itimerspec its;
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

	int s = t / 1000000;
	int ns = t % 1000000;

	if (s == 0 && ns == 0)
		goto end;

	/* TODO: remove debug */
	printf("%d %d\n", s, ns);
	its.it_value.tv_sec = s;
	its.it_value.tv_nsec = ns;

	its.it_interval.tv_sec = 0;
	its.it_interval.tv_nsec = 0;

	if (timer_settime(smash_timer_id, 0, &its, NULL) < 0)
		err(1, "timer_settime");

end:
	return &p1->c_lock;
}

void
smash_clock(void)
{
	struct callo *p1;

	if (callout[0].c_func != 0) {
		p1 = &callout[0];
		/* TODO: remove debug */
		smash_print_callout();

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

		/* Remove the task. */
		while ((p1+1)->c_func != NULL) {
			p1->c_time = (p1+1)->c_time;
			p1->c_func = (p1+1)->c_func;
			p1->c_arg = (p1+1)->c_arg;
			p1->c_send_args = (p1+1)->c_send_args;
			p1++;
		}
		p1->c_func = NULL;
	}
}
