#include <stdio.h>
#include <math.h>
#include "pml.h"
#include "cordic.h"

static char     funcname[] = "sin";

extern CORDIC_Table CORDIC_Table0[1];

double
sin(x)
    double          x;
{
    double          x0, y0, z0;
    double          D, t, temp;
    int             Q;
    struct exception xcpt;

    t = x / HALFPI;
    D = modf(t, &temp);
    Q = temp;

    if (Q < 0)
        Q = -Q;

    x0 = ((Q & 2) ? -1.0 : 1.0);
    y0 = 0.0;
    z0 = D * HALFPI;

    if (Q & 1) {
        CORDIC_rotate0(1, CORDIC_Table0, &x0, &y0, &z0);
        xcpt.retval = x0 / CORDIC_Table0->kval;
    }
    else {
        x0 = (Q & 2 ? -1.0 : 1.0);
        CORDIC_rotate0(1, CORDIC_Table0, &x0, &y0, &z0);
        xcpt.retval = y0 / CORDIC_Table0->kval;
    }
    return (xcpt.retval);
}
