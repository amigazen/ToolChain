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

/* stdlib.h - Declarations for functions in the standard C library
 */

extern void          abort(void);
extern float         atof(char *);
extern int           atoi(char *);
extern long          atol(char *);
extern void         *calloc(int, int);
extern void          exit(int);
extern void          free(void *);
extern char         *getenv(char *);
extern void         *malloc(int);
extern double        strtod(char *, char **);
extern long          strtol(char *, char **, int);
extern unsigned long strtoul(char *, char **, int);
extern int           system(char *);

#if 0                    /* Not yet implemented */
typedef struct {
    int quot;
    int rem;
    } div_t;

typedef struct {
    long quot;
    long rem;
    } ldiv_t;

extern int           abs(int);
extern char         *bsearch(char *, char *, unsigned, int, int (*compar)());
extern div_t         div(int, int);
extern long          labs(long);
extern ldiv_t        ldiv(long, long);
extern void          qsort(char *, unsigned, int, int (*compar)());
#endif
