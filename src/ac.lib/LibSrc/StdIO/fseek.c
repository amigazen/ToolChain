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

/* fseek - seeks relative some part of an I/O stream */

#include <stdio.h>

extern int _doflush();
extern long	lseek();

fseek( fp, position, mode )
FILE *fp;
long position;
int  mode;
{
	int i;
	long lseek();

	fp->_fileflag &= ~_FILEATEOF;
	if ( fp->_fileflag & _FILEISDIRTY ) {
		if (_doflush(fp, -1))
			return EOF;
	}
	else if (mode == 1 && fp->_filecpos)
		position -= (long)(fp->_fileend - fp->_filecpos);
	fp->_filecpos = NULL;
	fp->_fileend = NULL;

	if ( lseek((int)fp->_fileunit, position, mode) < 0)
		return EOF;
	return 0;
}

   long
ftell(fp)
FILE *fp;
{
	long position;

	position = lseek((int)fp->_fileunit, 0L, 1);

	if ( fp->_fileflag & _FILEISDIRTY)
		position += (long)(fp->_filecpos - fp->_filebufp);
	else if ( fp->_filecpos )
		position -= (long)(fp->_fileend - fp->_filecpos);

	return position;
}
