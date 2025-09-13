/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
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

/*  limits.h - ANSI-compatible numerical limits (dpANSI 2.2.4.2)
 */

#define	BITSPERBYTE	8
#define	CHAR_BIT	BITSPERBYTE
#define	SCHAR_MIN	(-128)
#define	SCHAR_MAX	(127)
#define	UCHAR_MAX	(~((unsigned char) 0))
#define	CHAR_MIN	SCHAR_MIN
#define	CHAR_MAX	SCHAR_MAX
#define MB_LEN_MAX	1
#define	SHRT_MIN	(-32768)
#define	SHRT_MAX	(32767)
#define	USHRT_MAX	(~((unsigned short) 0))
#define	INT_MIN		(-2147483648)
#define	INT_MAX		(2147483647)
#define	UINT_MAX	(~((unsigned int) 0))
#define	LONG_MIN	INT_MIN
#define	LONG_MAX	INT_MAX
#define	ULONG_MAX	(~((unsigned long) 0))

#define	PATHSIZE	(1024L)
#define	MAXPATH		PATHSIZE
#define MAXSTRING	(1024L)
