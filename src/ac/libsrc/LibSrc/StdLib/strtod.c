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

/* strtod.c - converts ascii to a double */

#include <stddef.h>
#include <ctype.h>

extern double strtosd(char *, char **, double);
extern double strtosud(char *, char **, double);

double strtod(string, ptr)
char  *string;
char **ptr;
{
    double retval = 0.0;
    double sign = 1.0;
    char *strptr = string;
    char *scan_index = string;
    int  exp = 0;
    int  valid = 0;              /* Advances when a value is established */

    retval = strtosd(strptr, &scan_index, 10.0);
    if (strptr != scan_index) {
        valid++;
        strptr = scan_index;
        if (retval < 0.0) {
            sign = -1.0;
            retval *= -1.0;
            }
        }
    else {
        while (isspace(*strptr))
            strptr++;
        if (*strptr == '-') {
            sign = -1.0;
            strptr++;
            }
        }

    if (*strptr == '.')  {
        strptr++;
        retval += strtosud(strptr, &scan_index, .1);
        if (strptr != scan_index) {
            valid++;
            strptr = scan_index;
            }
        }

    if ((valid > 0) && (tolower(*strptr) == 'e'))  {
        strptr++;
        exp = strtol(strptr, &scan_index, 10);
        strptr = scan_index;
        if (exp > 0)  {
            while (exp-- > 0)
                retval *= 10.0;
            }
        else if (exp < 0)  {
            while (exp++ < 0)
                retval /= 10.0;
            }
        }

    if (ptr != NULL) {
        if (valid > 0)
            *ptr = strptr;
        else
            *ptr = string;
        }

    return(retval * sign);
}
