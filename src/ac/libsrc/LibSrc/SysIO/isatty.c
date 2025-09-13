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

/* isatty - is an fd is attached to a pokey "user"? */

#include <fcntl.h>
#include <errno.h>

extern struct _device *_devtab;

int
isatty(fd)
int fd;
{
	extern long IsInteractive();
	struct _device *fp;

	fp = &_devtab[fd];
	if ( !(0 <= fd && fd <= _numdev) || !fp->_fileHandle ) {
		errno = EBADF;
		return -1;
	}

	return ( IsInteractive(fp->_fileHandle) != 0 );
}
