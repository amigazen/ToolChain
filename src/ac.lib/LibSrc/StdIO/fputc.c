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

/* fputc - writes a char to a buffered output stream */

#include <stdio.h>

extern write();
extern addStreamBuf();
extern int _doflush();

int fputc( c, fp )
unsigned int c;
FILE *fp;
{
    char buf = c;

    int t;

    if ( fp->_filecpos >= fp->_fileend )
        return _doflush( fp, c);

    *fp->_filecpos++ = c;

    if ( fp->_fileflag & _FILEISTTY && ( c == '\n' || c == '\r' ) ) {
        t = _doflush( fp, c);
        --fp->_filecpos;
        return t;
    }

    return c;
}
