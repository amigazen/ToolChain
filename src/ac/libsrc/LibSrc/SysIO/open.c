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

/* open.c - 
 *	creat(char *name, int pmode)	- creates a new file
 *	open(char *name, int mode)	- opens a file
 */

#include	<errno.h>
#include	<fcntl.h>
#include	<exec/types.h>
#include        <functions.h>
#include	<libraries/dos.h>

/*
 * creates a new file with the requested file modes
 */
creat(name, pmode)
char *name;
int   pmode;
{
	return( open(name, O_WRONLY|O_TRUNC|O_CREAT, pmode));
}

/*
 * returns the max. number of entries available in the file descrptor table
 */

int getdtablesize()
{
    extern int _numdev;

    return(_numdev);
}

/*
 open a file for unbuffered I/O.

  open(path, flags, mode)
  char *path;
  int   flags;
  int   mode;

 The file mode flag is presently ignored - This may not be the case in
 the future, so it is recommended that you use the value DEF_PMODE defined
 in fcntl.h if you have the O_CREAT flag set.
 */

open(path, flags, mode)
char *path;
int   flags;
int   mode;
{
	struct _device *p;
	int fd, err;
	struct FileHandle *fh;
	long lib;
	extern void Chk_Abort();
	extern struct _device *_devtab;

	Chk_Abort();

	/*
	 * Find the first empty entry in the device table.
	 */

	p = &_devtab[0];

	for ( fd=0; fd < _numdev; ++fd )
		if ( !p[fd]._fileHandle ) break;

	if ( fd >= _numdev ){
		errno = EMFILE;
		return(-1);
	}

	if (flags & O_TRUNC) {
		if (fh = Lock(path, ACCESS_WRITE)) {
			UnLock(fh);
			if ( !DeleteFile(path) ){
	 			errno = IoErr();
				return(-1);
			}
		}
	}

	fh = Open(path, MODE_OLDFILE);
	if ( !fh ) {
		if ( !(flags&O_CREAT)) {
			errno = ENOENT;
			return(-1);
		}
		if ((fh = Open(path, MODE_NEWFILE)) == 0) {
			errno = IoErr();
			return(-1);
		}
	}
	else if ((flags&(O_CREAT|O_EXCL)) == (O_CREAT|O_EXCL)) {
		Close(fh);
		errno = EEXIST;
		return(-1);
	}
	p[fd]._fileHandle = fh;
	p[fd]._mode = flags;
	if (flags & O_APPEND)
		Seek(fh, 0L, OFFSET_END);
	return(fd);

}
