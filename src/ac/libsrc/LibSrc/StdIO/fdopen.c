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

/* fdopen -  applies buffering to an open file desciptor */

#include <stdio.h>

extern FILE *addStream();

FILE *fdopen(fd, mode)
int   fd;
char *mode;
{
    FILE *fp;

    if ((fp = addStream()) == NULL )
        return fp;
    fp->_fileunit = fd;
    fp->_fileflag = _FILEACTIVE;
    if (isatty(fp))
        fp->_fileflag |= _FILEISTTY;
    return fp;
}
