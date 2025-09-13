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

/* fwrite - writes to an output stream */

#include <stdio.h>

int fwrite(ptr, element_size, count, stream)
char *ptr;
int   element_size;
int   count;
FILE *stream;
{
    int  max_bytes, bytes_left;

    max_bytes = bytes_left = count * element_size;

    while (bytes_left--) {
        if (putc(*ptr++,stream) == EOF)
            break;
    }

    bytes_left++;    /* uh-oh; put it back the way it was! */

    return( (element_size != 0) ? ((max_bytes-bytes_left)/element_size) : element_size);
}
