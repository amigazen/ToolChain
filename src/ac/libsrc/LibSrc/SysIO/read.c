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

/* read.c - read from file
 *
 * int read(fd, buf, bufsize)
 * int fd;
 * char *buf;
 * int bufsize;
 *
 * Returns number of bytes read
 *         0 if at EOF
 *        -1 if there was a DOS error
 */

#include    <errno.h>
#include    <fcntl.h>

extern long IoErr(), Read();
extern void Chk_Abort();

int read(fd, buf, bufsize)
char *buf;
int fd;
int bufsize;
{
    struct _device *p;
    extern struct _device *_devtab;
    int bytes_read;

    Chk_Abort();
    p = &_devtab[fd];
    if ( !( 0 <= fd  && fd < _numdev ) || !p->_fileHandle) {
        errno = EBADF;
        return -1;
    }

    if ((p->_mode & 0x03) == O_WRONLY) {
        errno = EINVAL;
        return -1;
    }

    if ((bytes_read = Read(p->_fileHandle, buf, (long)bufsize)) == -1) {
        errno = IoErr();
        return -1;
    }

    Chk_Abort();

    return bytes_read;
}
