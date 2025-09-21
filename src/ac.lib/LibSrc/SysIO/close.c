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

/* close - closes an open file and free's its fd */

#include <errno.h>
#include <fcntl.h>

extern void Close();

close(fd)
int fd;		/* The file descriptor. */
{
	struct _device *p;
	extern struct _device *_devtab;

	p = &_devtab[fd];
	if ( !(0 <= fd && fd < _numdev) || p->_fileHandle == 0) {
		errno = EBADF;
		return(-1);
	}

	if ( !(p->_mode & O_STDIO) ) {
		Close( p->_fileHandle );
	}

	p->_mode = 0;
	p->_fileHandle = 0;
	return 0;
}
