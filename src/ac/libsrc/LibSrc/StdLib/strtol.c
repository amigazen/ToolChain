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

/* strtol.c - converts ascii to a long */

#include <stddef.h>
#include <ctype.h>

long strtol(string, ptr, base)
char *string;
char **ptr;
int base;
{
    char *strptr = string;
    int retval = 0;
    int sign;
    char c;
    int i = 0;

    while (isspace(c = *strptr))
        strptr++;

    if (c == '-') {
        sign = -1;
        c = *(++strptr);
        }
    else
        sign = 1;

    if (c == '+')
        c = *(++strptr);

    if (base == 0) {
        if (c == '0') {
            if ((c = tolower(*(++strptr))) == 'x')
                base = 16;
            else
                base = 8;
            }
        else
            base = 10;
        }

    do {
        if (isalnum(c))
            retval = (retval*base) + toint(c);
        else
            break;
        i++;
        } while ((c = *(++strptr)) != 0);

    if (ptr != NULL) {
        if (i > 0)
            *ptr = strptr;
        else
            *ptr = string;
        }

    return(retval * sign);
}
