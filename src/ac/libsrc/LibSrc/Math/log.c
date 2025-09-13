#include <stdio.h>
#include <math.h>
#include "pml.h"
#include "cordic.h"

static char     funcname[] = "log";

extern CORDIC_Table CORDIC_Table1[1];

double
log(x)
    double          x;
{
    double          y, z;
    int             k;
    double          x0, y0, z0;
    struct exception xcpt;

    if (x == 0.0) {
        xcpt.type = SING;
        xcpt.name = funcname;
        xcpt.arg1 = x;
        if (!matherr(&xcpt)) {
            fprintf(stderr, "%s: SINGULARITY error\n", funcname);
            errno = EDOM;
            xcpt.retval = -MAXDOUBLE;
        }
    }
    else if (x < 0.0) {
        xcpt.type = DOMAIN;
        xcpt.name = funcname;
        xcpt.arg1 = x;
        if (!matherr(&xcpt)) {
            fprintf(stderr, "%s: DOMAIN error\n", funcname);
            errno = EDOM;
            xcpt.retval = -MAXDOUBLE;
        }
    }
    else {
        y = frexp(x, &k);
        x0 = y + 1.0;
        y0 = y - 1.0;
        z0 = 0.0;
        CORDIC_rotate1(0, CORDIC_Table1, &x0, &y0, &z0);
        xcpt.retval = k * LN2 + 2.0 * z0;
    }
    return (xcpt.retval);
}
