#include <stdio.h>
#include <math.h>
#include "pml.h"
#include "cordic.h"

static char     funcname[] = "atanh";

extern CORDIC_Table CORDIC_Table1[1];

double
atanh(x)
    double          x;
{
    double          x0, y0, z0;
    double          M, X;
    int             E;
    struct exception xcpt;

    if (x > MAXDOUBLE || x < -MAXDOUBLE) {
        xcpt.type = UNDERFLOW;
        xcpt.name = funcname;
        xcpt.arg1 = x;
        if (!matherr(&xcpt)) {
            fprintf(stderr, "%s: UNDERFLOW error\n", funcname);
            errno = EDOM;
            xcpt.retval = 0.0;
        }
    }
    else {
        X = 1 - x;
        M = frexp(X, &E);

        x0 = 1.0;
        y0 = (2.0 - M - X) / (2.0 + M - X);
        z0 = -E * LN2 * 0.5;

        CORDIC_rotate1(0, CORDIC_Table1, &x0, &y0, &z0);

        xcpt.retval = z0;
    }
    return (xcpt.retval);
}
