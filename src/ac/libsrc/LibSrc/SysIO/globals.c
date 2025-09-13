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

/* globals.c
 * These are the globals brought into a program that uses the PDC scanned
 * libraries and runtime environment.
 */

#include <stdio.h>
#include <fcntl.h>

void *IconBase = 0;

/* Ptrs to a function to close open files and one to free memory upon exit().
 */

void (*_fcloseall)();		/* Function to close all open files */
void (*_freeall)();		/* Function to free all (m)alloc'd memory */

int _argc = 0;
int _argBufSize = 0;
char **_argv = 0;
char *_argBuf = 0;

int errno = 0;

short _numdev = OPEN_MAX;	/* The number of entries in _devtab */
struct _device *_devtab = NULL;	/* The device table must be malloc'd! */

FILE *stdin;
FILE *stdout;
FILE *stderr;

FILE _fdevtab[4] = {
	{ &_fdevtab[1], 0L, NULL, NULL, 0L, NULL, 0, 0, 0 },
	{ &_fdevtab[2], 0L, NULL, NULL, 0L, NULL, 0, 0, 0 },
	{ &_fdevtab[3], 0L, NULL, NULL, 0L, NULL, 0, 0, 0 },
	{         NULL, 0L, NULL, NULL, 0L, NULL, 0, 0, 0 }
};
