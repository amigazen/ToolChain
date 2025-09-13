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

/*	lseek.c
 *
 *	long lseek(fd, offset, origin)	- move read/write file pointer
 *	int fd;
 *	long offset;
 *	int origin;
 */

#include	<errno.h>
#include	<fcntl.h>

extern long IoErr(), Seek();
extern void Chk_Abort();

long
lseek(fd, offset, origin)
int fd;
long offset;
int origin;
{
	struct _device *p;
	extern struct _device *_devtab;

	Chk_Abort();
	p = &_devtab[fd];
	if ( !(0 < fd && fd < _numdev) || !p->_fileHandle) {
		errno = EBADF;
		return(-1);
	}
	/*
	 * Use AmigaDos Seek to move to the required offset.
	 */

	if ( Seek(p->_fileHandle, offset, (long)(origin-1)) == -1) {
		errno = IoErr();
		return(-1);
	}

	/*
	 * Return the new position in the file in bytes from the start.
	 */

	return( Seek(p->_fileHandle, 0L, 0L) );
}
