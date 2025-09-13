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

/*  format.c - routines for generating formatted strings
 *
 *  format - formats a string and outputs character
 */

#include <limits.h>

static unsigned int base;
static int minwidth, precision, padchar, rjust, nsent;
static int (*put)();

/*
 * putint: convert long to ascii. Return a pointer to start of string.
 */
static char digit[] = "0123456789ABCDEF";

static char *
putint( str, n)
char *str;
long    n;
{
    char *s;
    int is_negative;
    unsigned int un;

    if (n == 0x80000000) {		/* special case */
        switch (base) {
        case 8:
            s = "20000000000";
            break;
        case 10:
            s = "-2147483648";
            break;
        case 16:
            s = "80000000";
            break;
        default:
            s = "-OOPS-";
        }
        return s;
    }

    is_negative = ( base==10 && n<0 );
    if ( is_negative )
        n = -n;

    un = (unsigned int) n;

    s = &str[MAXSTRING];
    *--s = '\0';
    do {
        *--s = digit[ un % base ];
        un /= base;
    }
        while ( un && s > &str[1] );

    if ( is_negative )
        *--s = '-';

    return s;
}

/*
 * prt: Does the justification and fill if required.
 * Side Effects: updates nsent, destroys minwidth.
 */

static int 
prt( strbuf )
char *strbuf;
{

    extern int  strlen();
    int i; 
    int c;
    int len = 1;
    int strsize = strlen( strbuf );

    if ( strsize > precision )
        strsize = precision;

    if ( rjust ) {
        if ( padchar != ' ' && (c = *strbuf) == '-' || c == '+' ) {
            if ( (*put)(c) == -1 )
                return -1;
            ++len;
            ++strbuf;
            --strsize;
        }

        while ( --minwidth >= strsize ) {
            if ( (*put)(padchar) == -1 )
                return -1;
            ++len;
        }

    }

    for ( i=0; i < precision && (c = *strbuf++); ++i ){
        if ( (*put)(c) == -1 )
                return -1;
    }
    len += i;

    /*
     * Pad with blanks, if necessary.
     */

    while ( len < minwidth ) {
        if ( (*put)(' ') == -1 )
           return -1;
        ++len;
    }

    nsent += len;
    return len;
}

format( psub, ctl, argptr )
int (*psub)();        /* Address of a routine to put a character */
char *ctl;            /* The control string. */
char *argptr;         /* pointer to arguments */
{
    union {
        short *hptr;
        int  *iptr;
        long *lptr;
        char *cptr;
        char **cptr2;
    }  args;

    unsigned int argsize, c;
    long l;
    char *sptr;
    char strbuf[MAXSTRING];

    put = psub;
    args.cptr = argptr;
    nsent = 0;
    while ( c = *ctl )    {

        /*
         * Just print any non '%' characters in control string.
         */

        if ( c != '%' )    {
            if ( (*psub)(c) == -1 )
                return -1;
            ++ctl;
            ++nsent;
            continue;
        }

        /*
         * Or echo a single '%' for a control string of "%%".
         */

        if ( (c = *++ctl) == '%' )    {
            if ( (*psub)(c) == -1 )
                return -1;
            ++ctl;
            ++nsent;
            continue;
        }

        /*
         * Start of a format specifier: "%-010.5ld" for example.
         */

        rjust = 1;
        argsize = sizeof(int);
        minwidth = precision = 0;
        padchar = ' ';

        if ( c == '-' )    {
            rjust = 0;
            c = *++ctl;
        }

        if ( c == '0' )    {
            padchar = '0';
            c = *++ctl;
        }

        if ( c == '*' )    {
            minwidth = *args.iptr++;
            c = *++ctl;
        } else {
            while ( '0' <= c && c <= '9' )    {
                minwidth += minwidth * 10 + c - '0';
                c = *++ctl;
            }
        }

        if ( c == '.' )    {
            c = *++ctl;
            if ( c == '*' ) {
                precision = *args.iptr++;
                c = *++ctl;
            } else {
                while ( '0' <= c && c <= '9' ){
                    precision += precision * 10 + c - '0';
                    c = *++ctl;
                }
            }
        }

        if ( c == 'l' || c == 'L' )    {
            argsize = sizeof(long);
            c = *++ctl;
        }
        else if ( c == 'h' || c == 'H' ){
            argsize = sizeof(short int);
            c = *++ctl;
        }

        /*
         * We should be at a conversion specification character.
         */

        if ( precision >= (MAXSTRING-2)  || precision == 0)
            precision = (MAXSTRING-2);
        base = 0;
        ++ctl;

        switch(c)    {
            case 'x': base += 8;        /* FALL-THROUGH */
            case 'o': base += 8;
            case 'd': if ( !base )
                          base = 10;
                      if ( argsize == 4 )
                          l = *args.lptr++;
                      else
                          l = *args.hptr++;
                      sptr = putint( strbuf, l );
                      if ( prt( sptr ) == -1 )
                          return -1;
                      break;

            case 's': sptr = *args.cptr2++;
                      if (minwidth)
                          precision = minwidth;
                      else
                          precision = 32717;
                      if ( prt( sptr ) == -1 )
                          return -1;
                      break;

            case 'c': c = *args.iptr++;

            default : strbuf[0] = c;
                      strbuf[1] = '\0';
                      if ( prt( strbuf ) == -1 )
                          return -1;
                      break;

        }
    }
}
