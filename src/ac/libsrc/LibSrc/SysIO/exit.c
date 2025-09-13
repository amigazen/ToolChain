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

/*    exit.c
 *
 *    exit(returnCode)    - Cleans up PDC's I/O
 *    int returnCode;     - The return code passed back to the startup module
 */

#include <fcntl.h>

extern void _exit(), close();
extern void (*_fcloseall)(), (*_freeall)();

extern short _numdev;
extern struct _device *_devtab;

exit(returnCode)
int returnCode;        
{
    int fd;

    if ( _fcloseall != 0 )
        (*_fcloseall)();

    if (_devtab != 0) {
        for (fd = 0 ; fd < _numdev ; ++fd)
            close( fd );
        free(_devtab);
    }

    if ( _freeall != 0 )
        (*_freeall)(); /* Free any malloc()ed memory */    

    _exit ((long)returnCode);
}
