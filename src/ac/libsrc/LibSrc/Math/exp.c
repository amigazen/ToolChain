#include <stdio.h>
#include <math.h>
#include "pml.h"
#include "cordic.h"

static char     funcname[] = "exp";

extern CORDIC_Table CORDIC_Table0[];
extern CORDIC_Table CORDIC_Table1[];

double
exp(x)
    double          x;
{
    double          t, y, z;
    double          x0, y0, z0;
    double          D, temp;
    int             Q;
    struct exception xcpt;

    if (x > LOGE_MAXDOUBLE) {
        xcpt.type = OVERFLOW;
        xcpt.name = funcname;
        xcpt.arg1 = x;
        if (!matherr(&xcpt)) {
            fprintf(stderr, "%s: OVERFLOW error\n", funcname);
            errno = ERANGE;
            xcpt.retval = MAXDOUBLE;
        }
    }
    else if (x <= LOGE_MINDOUBLE) {
        xcpt.type = UNDERFLOW;
        xcpt.name = funcname;
        xcpt.arg1 = x;
        if (!matherr(&xcpt)) {
            fprintf(stderr, "%s: OVERFLOW error\n", funcname);
            errno = ERANGE;
            xcpt.retval = 0.0;
        }
    }
    else {
        t = x / LN2;
        D = modf(t, &temp);
        Q = temp;
        x0 = y0 = 1.0;
        z0 = D * LN2;
        CORDIC_rotate1(1, CORDIC_Table1, &x0, &y0, &z0);
        xcpt.retval = ldexp(x0 / CORDIC_Table1->kval, Q);
    }
    return (xcpt.retval);
}
