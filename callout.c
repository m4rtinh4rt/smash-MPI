#include "callout.h"

#include <stdio.h>

/* TODO: Add a NULL member struct to the end of the callout struct in the parsing */
struct callo callout[NCALL] = { 0 };

void
print_callo(struct callo *c)
{
	printf("c_time: %i\nc_arg: %i\nc_func @ %p\n\n", c->c_time, c->c_arg,
	       (void *)&c->c_func);
}

void
print_callout(void)
{
	size_t i;

	for (i = 0; callout[i].c_func != NULL; ++i)
		print_callo(&callout[i]);
}

void
timeout(int (*func)(), int arg, int time)
{
        struct callo *p1, *p2;
        int t;

        t = time;
        p1 = &callout[0];
        while(p1->c_func != 0 && p1->c_time <= t) {
                t -= p1->c_time;
                p1++;
        }
        p1->c_time -= t;
        p2 = p1;
        while(p2->c_func != 0)
                p2++;
        while(p2 >= p1) {
                (p2+1)->c_time = p2->c_time;
                (p2+1)->c_func = p2->c_func;
                (p2+1)->c_arg = p2->c_arg;
                p2--;
        }
        p1->c_time = t;
        p1->c_func = func;
        p1->c_arg = arg;
}

void clock(void)
{
	extern int iaflags, idleflag;
	register struct callo *p1;
	register int *pc;

	if (callout[0].c_func != 0) {
		p1 = &callout[0];
		while (p1->c_time < 0 && p1->c_func != 0)
			p1++;
		p1->c_time--;
		if (p1->c_time == 0 && p1->c_func != 0) {
			p1->c_func();
			p1->c_time--;
		}
	}
}
