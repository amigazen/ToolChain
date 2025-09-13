#include <stdio.h>
#include <math.h>
#include "pml.h"
#include "cordic.h"

static char     funcname[] = "atan";

extern CORDIC_Table CORDIC_Table0[1];

double
atan(x)
    double          x;
{
    double          x0, y0, z0;
    struct exception xcpt;

    if (x < 0.0)
        xcpt.retval = -(atan(-x));
    else if (x > 1.0) {
        if (x < MAXDOUBLE && x > -MAXDOUBLE)
            xcpt.retval = HALFPI - atan(1.0 / x);
        else {
            xcpt.type = UNDERFLOW;
            xcpt.name = funcname;
            xcpt.arg1 = x;
            if (!matherr(&xcpt)) {
                fprintf(stderr, "%s: UNDERFLOW error\n", funcname);
                errno = EDOM;
                xcpt.retval = 0.0;
            }
        }
    }
    else {
        x0 = 1.0;
        y0 = x;
        z0 = 0.0;
        CORDIC_rotate0(0, CORDIC_Table0, &x0, &y0, &z0);
        xcpt.retval = z0;
    }
    return (xcpt.retval);
}
