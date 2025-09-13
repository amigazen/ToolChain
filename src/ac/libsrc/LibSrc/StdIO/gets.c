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

/* gets.c - The ubiquitous gets() function, present solely for compatibility
 *
 *     gets()  Read from stdin to buffer until EOF or newline
 *
 * NOTE: This function is one big bug.  Since it doesn't hold any regard for
 *       the size of the buffer it is copying into, the USER of a program
 *       containing gets() can cause crashes and other security breaches by
 *       typing past the end of your buffer.  It is provided here for
 *       compatibility purposes only.  DO NOT USE THIS FUNCTION IN THE CODE
 *       THAT YOU WRITE.  Although it is provided here in source form, it
 *       is not present in PDC.lib.
 */

#include <stdio.h>

char *gets( linbuf )
char *linbuf;
{
    char *s;
    int      c;

    s = &linbuf[0];
    while ( (c = getchar()) != EOF && c != '\n' )
        *s++ = c;

    *s = '\0';
    if ( c == EOF && s == &linbuf[0] )
        return NULL;
    return (&linbuf[0]);
}
