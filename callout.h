#ifndef CALLOUT_H
#define CALLOUT_H

#define NCALL 10 /* Obviously a troll */

struct callo {
	int c_time;         /* incremental time */
	int c_arg;          /* argument to routine */
	int (*c_func)();    /* routine */
};

void smash_print_callout(void);

void smash_timeout(int (*func)(), int arg, int time);

void smash_clock(void);

#endif /* CALLOUT_H */
