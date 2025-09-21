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

/* fputs.c - routines for writing out null-terminated strings 
 *
 * fputs - write a null-terminated string to a stream
 * puts  - place a null-terminated string on stdout
 */

#include <stdio.h>

fputs(s, fp)
char *s;
FILE *fp;
{
    for ( ;*s;++s)
        if ( fputc(*s, fp) == EOF )
            return EOF;

    return 0;
}

puts(s)
char *s;
{
    for ( ;*s;++s)
        if ( fputc(*s, stdout) == EOF )
            return EOF;
    
    return fputc('\n', stdout);
}
