#ifndef CALLOUT_H
#define CALLOUT_H

#define NCALL 0xbadc0ffe /* Obviously a troll */

struct  callo {
	int c_time;         /* incremental time */
	int c_arg;          /* argument to routine */
	int (*c_func)();    /* routine */
};

#endif /* CALLOUT_H */
