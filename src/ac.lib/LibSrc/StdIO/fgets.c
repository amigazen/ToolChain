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

/* fgets.c
 *
 * fgets(linbuf, maxchars, fp)  -   Reads line from file into buffer
 */

#include <stdio.h>

char *fgets(linbuf, maxchars, fp)
char *linbuf;
int   maxchars;
FILE *fp;
{
    char c;
    char *s;

    for ( s = &linbuf[0]; --maxchars > 0 && (c = getc(fp)) != EOF; ) {
        *s++ = c;
        if ( c == '\n')
            break;
    }
    *s = '\0';
    if ( c == EOF && s == &linbuf[0])
        return NULL;
    return (&linbuf[0]);
}
