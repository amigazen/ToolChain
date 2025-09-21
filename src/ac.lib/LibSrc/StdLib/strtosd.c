/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 * PDC I/O Library (C) 1987 by J.A. Lydiatt.
 *
 * This code is freely redistributable upon the conditions that this 
 * notice remains intact and that modified versions of this file not
 * be included as part of the PDC Software Distribution without the
 * express consent of the copyright holders.  No warrantee of any
 * kind is provided with this code.  For further information, contact:
 *
 *  PDC Software Distribution    Internet:                     BIX:
 *  P.O. Box 4006             or hummel@cs.uiuc.edu            lhummel
 *  Urbana, IL  61801-8801       petersen@uicsrd.csrd.uiuc.edu
 */

/* strtosd - parses a scaled double from an ascii string */

#include <stddef.h>
#include <ctype.h>

/* If (base < 1), strtosd returns the number as being right of the base
 * point.  IE, 123456 is returned as 0.123456.  The only function crazier
 * than this is its unsigned counterpart, strtosud().  It is something of
 * a kludge, designed to simplify the code to collect the fractional part
 * of an FP number.  What else is it good for?  Well, you can always use
 * it to read binary floating point (eg, 01101.1101)!
 */

double strtosd(string, ptr, base)
char *string;
char **ptr;
double base;
{
    char *strptr = string;
    double retval = 0.0;
    double sign;
    double obase = base;
    int i = 0;
    char c;

    obase = base;

    while (isspace(c = *strptr))
        strptr++;

    if (c == '-') {
        sign = -1.0;
        c = *(++strptr);
        }
    else
        sign = 1.0;

    if (c == '+')
        c = *(++strptr);

    while ((c = *strptr) != 0) {
        if (isdigit(c)) {
            i++;
            strptr++;
            if (obase > 1)
                retval = (retval*base) + (c-'0');
            else {
                retval += base * (c-'0');
                base *= obase;
                }
            }
        else
            break;
        }

    if (ptr != NULL) {
        if (i > 0)
            *ptr = strptr;
        else
            *ptr = string;
        }

    return(retval * sign);
}
