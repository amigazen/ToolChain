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

/*  fclose.c
 *
 *  Close a stdio stream.
 */

#include <stdio.h>

extern int _doflush();

int fclose( fp )
FILE *fp;
{
    int result;

    result = 0;
    if ( fp == NULL )
        return -1;

    if ( fp->_fileflag ) {
        if ( fp->_fileflag & _FILEISDIRTY )
            result = _doflush( fp, -1 );
        result |= close( (int)fp->_fileunit );
        if ( fp->_fileflag & _FILEISDYNA )
            free( fp->_filebufp );
    }

    fp->_filebufp = NULL;
    fp->_filecpos = NULL;
    fp->_fileend  = NULL;
    fp->_fileflag = 0;

    return result;
}

int fflush( fp )
FILE *fp;
{
    return(_doflush( fp, -1 ));
}
