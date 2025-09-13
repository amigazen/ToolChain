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

/* ctype.h - very efficient macros for classifying characters */

extern char _ctype[256];

#define _UPPER       (1)
#define _LOWER       (1 << 1)
#define _HEXIT       (1 << 2)
#define _DIGIT       (1 << 3)
#define _SPACE       (1 << 4)
#define _CNTRL       (1 << 5)
#define _PUNCT       (1 << 6)
#define _OCTIT       (1 << 7)

#define isupper(x)   (_ctype[(x)&0xff] & _UPPER)
#define islower(x)   (_ctype[(x)&0xff & _LOWER)
#define	isxdigit(x)  (_ctype[(x)&0xff] & _HEXIT)
#define	isdigit(x)   (_ctype[(x)&0xff] & _DIGIT)
#define isspace(x)   (_ctype[(x)&0xff] & _SPACE)
#define iswhite(x)   (_ctype[(x)&0xff] & _SPACE)
#define iscntrl(x)   (_ctype[(x)&0xff] & _CNTRL)
#define ispunct(x)   (_ctype[(x)&0xff] & _PUNCT)
#define isodigit(x)  (_ctype[(x)&0xff] & _OCTIT)

#define isalpha(x)   (_ctype[(x)&0xff] & (_UPPER | _LOWER))
#define isalnum(x)   (_ctype[(x)&0xff] & (_UPPER | _LOWER | _DIGIT))

#define isprint(x)   (!(_ctype[(x)&0xff] & _CNTRL))
#define isgraph(x)   ((!(_ctype[(x)&0xff] & _CNTRL)) & ((x) != ' '))
#define isascii(x)   ((x) >= 0 && (x) < 128)
#define iscsym(x)    ((_ctype[(x)&0xff] & (_UPPER | _LOWER | _DIGIT)) || ((x) == '_') || ((x) == '$'))

#define tolower(x)  ((x) | 32)
#define toupper(x)  ((x) & ~32)
#define tocntrl(x)  (((((x)+1)&~96)-1)&127)
#define toascii(x)  ((x) & 127)
#define toint(x)    ((int)((_ctype[(x)&0xff]&_DIGIT)?((x)-'0'):(((x)|32)-'a'+10)))
