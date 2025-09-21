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

/*  fopen.c - assigns buffered I/O streams to files
 *
 *  fopen - opens the given file name for buffered I/O
 *  freopen - changes the mode on a buffered I/O stream  
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

extern int errno;
extern void fclose();
extern int  open(), strcmp();
extern FILE *addStream();

static struct modeType {
    char *requested;
    int   openMode;
    } modeTable[9] = {
        {"r",   O_RDONLY},
        {"r+",  O_RDWR},
        {"w",  (O_WRONLY|O_CREAT|O_TRUNC)},
        {"w+", (O_RDWR|  O_CREAT|O_TRUNC)},
        {"a",  (O_WRONLY|O_CREAT|O_APPEND)},
        {"a+", (O_RDWR|  O_CREAT|O_APPEND)},
        {"x",  (O_WRONLY|O_CREAT|O_EXCL)},
        {"x+", (O_RDWR|  O_CREAT|O_EXCL)},
        {"",    0L}
        };

FILE *
freopen(name, mode, fp)
char *name;
char *mode;
FILE *fp;
{
    struct modeType *mp;
    int     fd;

    fclose(fp);
    /*
     * Set mp to point to the correct table entry.
     */
    for (mp = &modeTable[0];; ++mp) {
        if ( !*mp->requested ) {
            errno = EINVAL;
            return NULL;
        }
        if ( strcmp(mp->requested, mode) == 0)
            break;
    }
    /*
     * Open it again with the correct attributes.
     */
    if (( fd = open(name, mp->openMode)) == -1)
        return NULL;
    fp->_fileunit = fd;
    fp->_fileflag = _FILEACTIVE;
    return fp;
}

FILE *
fopen(name, mode)
char *name;
char *mode;
{
    FILE *fp;

    if ((fp = addStream()) == NULL )
        return NULL;
    return freopen( name, mode, fp );
}

