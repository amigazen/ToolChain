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

/* errno.h - ANSI C error numbers */

#ifndef ERRNO_H
#define ERRNO_H

/* ANSI C required error numbers */
#define EDOM    1       /* Domain error */
#define ERANGE  2       /* Range error */

/* Additional error numbers for compatibility */
#define EOK     0       /* No error */
#define ENOMEM  3       /* Out of memory */
#define EINVAL  4       /* Invalid argument */
#define EBADF   5       /* Bad file descriptor */
#define EEXIST  6       /* File exists */
#define EMFILE  7       /* Too many open files */
#define ENOENT  8       /* No such file or directory */
#define EIO     9       /* I/O error */

/* External error indicator */
extern int errno;

/* Non-ANSI extensions */
extern char *sys_errlist[];
extern int sys_nerr;

#endif /* ERRNO_H */