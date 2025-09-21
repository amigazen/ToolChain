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

/* write - writes to a file */

#include    <errno.h>
#include    <fcntl.h>
#include    <exec/types.h>
#include    <libraries/dos.h>
#include    <functions.h>

extern void Chk_Abort();

write(fd, buf, bufsize)
int   fd;
char *buf;
int   bufsize;
{
    struct _device *p;
    extern struct _device *_devtab;

    long err;

    Chk_Abort();

    p = &_devtab[fd];
    if ( !( 0 <= fd && fd < _numdev) || !p->_fileHandle) {
        errno = EBADF;
        return -1;
    }

    if ( (p->_mode & 0x03) == O_RDONLY) {
        errno = EINVAL;
        return -1;
    }

    if ((err = Write((struct FileHandle *) p->_fileHandle, buf, (long)bufsize)) == -1 ) {
        errno = IoErr();
        return -1;
    }

    return (int)err;
}
