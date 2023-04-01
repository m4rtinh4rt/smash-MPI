#ifndef CALLOUT_H
#define CALLOUT_H

#define NCALL 10 /* Obviously a troll */

struct callo {
	int c_time;         /* incremental time */
	int c_arg;          /* argument to routine */
	int (*c_func)();    /* routine */
};

void print_callout(void);

void timeout(int (*func)(), int arg, int time);

void clock(void);

#endif /* CALLOUT_H */
