/*
 * PDC I/O Library Copyright (C) 1987 by J.A. Lydiatt.
 * Modifications for PDC release 3.3 Copyright (C) 1988 Lionel D. Hummel.
 * PDC Software Distribution Copyright (C) 1988 Lionel Hummel and Paul Petersen.
 *
 * This code is freely redistributable upon the conditions that this notice
 * remains intact and that modified versions of this file not be included
 * as part of the PDC Software Distribution without the express consent of
 * the copyright holders.
 */

#include <math.h>

/*    round.c - performs rounding
 */

/* Returns an integer rounded from a double.  In case of fraction exactly
 * midway, round chooses nearest even value.
 */
double rint(num)
double num;
{
    double ipart;
    int    ival;
    double frac;

    frac = modf(num, &ipart);

    ival = ipart;
    if ( (frac > 0.5) || ((frac == 0.5) && (ival%2)) )
        ival++;

    return((double) ival);
}
