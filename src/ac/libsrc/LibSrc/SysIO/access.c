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

/* access.c -
 *     access(char *name, int mode)     - test accessability of a file/dir
 */

#include <errno.h>
#include <exec/types.h>
#include <functions.h>
#include <libraries/dosextens.h>

#define MAX_MODES 5
static int mask_table[MAX_MODES] = {
    0L,
    FIBF_EXECUTE,
    FIBF_WRITE,
    -1L,
    FIBF_READ
    };

int access(filename, mode)
char *filename;
int mode;
{
    struct FileLock *fl;
    struct FileInfoBlock fib;
    int retval;

    if (mode > MAX_MODES)  {
        errno = EINVAL;
        return(0);
        }

    fl = Lock(filename, ACCESS_READ);
    if (fl == 0)  {
        errno = ENOENT;
        return(0);
        }

    if (Examine(fl, &fib) == DOSFALSE)  {
        errno = EIO;
        retval = 0;
        }
    else  {
        retval = !(fib.fib_Protection & mask_table[mode]);
        }

    UnLock(fl);
    return(retval);
}
