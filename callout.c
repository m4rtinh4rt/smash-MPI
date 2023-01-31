#include "callout.h"

struct callo callout[NCALL];

void
timeout(int (*func)(), int arg, int time)
{
        struct callo *p1, *p2;
        int t;
        int s;

        t = time;
        // s = PS->integ;
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
        // PS->integ = s;
}
