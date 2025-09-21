#include <stdio.h>
#include <math.h>
#include "pml.h"
#include "cordic.h"

static char     funcname[] = "sinh";

extern CORDIC_Table CORDIC_Table1[1];

double
sinh(x)
    double          x;
{
    int             Q;
    double          x0, y0, z0;
    double          D, t, temp, value;
    struct exception xcpt;

    t = x / LN2;
    D = modf(t, &temp);
    Q = temp;

    x0 = 1.0;
    y0 = 0.0;
    z0 = D * LN2;

    CORDIC_rotate1(1, CORDIC_Table1, &x0, &y0, &z0);

    x0 /= CORDIC_Table1->kval;  /* cosh(D)   */
    y0 /= CORDIC_Table1->kval;  /* sinh(D)   */

    z0 = ldexp(x0 + y0 - ldexp(x0 - y0, -2 * Q), Q - 1);

    return (z0);
}
