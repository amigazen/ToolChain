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

/* Chk_Abort - calls _abort() if a break has been received */


#include <libraries/dos.h>

#define ABORTSTATE (SIGBREAKF_CTRL_C | SIGBREAKF_CTRL_D)

int Enable_Abort = 1;

long
Chk_Abort()
{
    long status;
    extern long SetSignal();
    extern void _abort();

    if ( ((status = SetSignal(0L, ABORTSTATE)) & ABORTSTATE) && Enable_Abort )
        _abort();

    return( status );
}
