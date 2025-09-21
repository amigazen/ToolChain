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

/* setjmp.h - ANSI C non-local jumps */

#ifndef SETJMP_H
#define SETJMP_H

#include <stddef.h>

/* Jump buffer type - contains enough information to restore calling environment */
typedef struct {
    unsigned long regs[16];  /* General purpose registers */
    unsigned long sp;        /* Stack pointer */
    unsigned long pc;        /* Program counter */
    unsigned long flags;     /* Status flags */
} jmp_buf;

/* ANSI C required functions */
int setjmp(jmp_buf env);
void longjmp(jmp_buf env, int val);

#endif /* SETJMP_H */
