#include <stdio.h>
#include <math.h>
#include "pml.h"
#include "cordic.h"

static char     funcname[] = "cos";

extern CORDIC_Table CORDIC_Table0[1];

double
cos(x)
    double          x;
{
    int             Q;
    double          x0, y0, z0;
    double          D, t, temp;
    struct exception xcpt;

    t = x / HALFPI;
    D = modf(t, &temp);
    Q = temp;
    if (Q < 0)
        Q = -Q;

    y0 = 0.0;
    z0 = D * HALFPI;

    if (Q & 1) {
        x0 = (Q & 2 ? 1.0 : -1.0);
        CORDIC_rotate0(1, CORDIC_Table0, &x0, &y0, &z0);
        xcpt.retval = y0 / CORDIC_Table0->kval;
    }
    else {
        x0 = (Q & 2 ? -1.0 : 1.0);
        CORDIC_rotate0(1, CORDIC_Table0, &x0, &y0, &z0);
        xcpt.retval = x0 / CORDIC_Table0->kval;
    }
    return (xcpt.retval);
}
