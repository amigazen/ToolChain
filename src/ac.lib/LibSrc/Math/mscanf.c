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

/*
	fscanf.c - Routines for formatted Input

	sscanf(buf, fmt, args)			Scan from a memory buffer
	    char           *buf, *fmt;
	    void           *args;

	fscanf(fp, fmt, args)			Scan from a file pointer
	    FILE           *fp;
	    char           *fmt;
	    void           *args;

	scanf(fmt, args)			Scan from stdin
	    char           *fmt;
	    void           *args;
*/

#include <stdio.h>
#include <varargs.h>
#include <ctype.h>

#define TRUE    1
#define FALSE   0

_scanchar(fp, buffer)
    FILE           *fp;
    char          **buffer;
{
    char           *ptr;

    if (fp != NULL)
        return (fgetc(fp));
    else if (*buffer != NULL) {
        ptr = *buffer;
        if (*ptr == '\0')
            return (EOF);
        else {
            *buffer += 1;
            return (*ptr);
        }
    }
    return (EOF);
}


_unscanc(fp, buffer, ch)
    FILE           *fp;
    char          **buffer, ch;
{
    char           *ptr;

    if (fp != NULL)
        ungetc(ch, fp);
    else if (*buffer != NULL) {
        *buffer -= 1;
        **buffer = ch;
    }
}


_scanstr(fp, buffer, str, maxlen)
    FILE           *fp;
    char          **buffer, *str;
    int             maxlen;
{
    int             i, ch;

    do {
        ch = _scanchar(fp, buffer);
        if (ch == EOF)
            return (0);
    } while (isspace(ch));

    _unscanc(fp, buffer, ch);

    for (i = 0; i < maxlen; i++) {
        ch = _scanchar(fp, buffer);
        if (isspace(ch) || ch == EOF)
            goto done;
        *str++ = ch;
    }
done:
    *str = '\0';
    return (i != 0);
}



_scanint(fp, buffer, result, base, maxlen)
    FILE           *fp;
    char          **buffer;
    int            *result, base, maxlen;
{
    int             digit, value = 0;
    int             negative, ch, i;

    *result = value;

    do {
        ch = _scanchar(fp, buffer);
        if (ch == EOF)
            return (0);
    } while (isspace(ch));

    if (!(negative = (ch == '-')))
        _unscanc(fp, buffer, ch);

    for (i = 0; i < maxlen; i++) {
        ch = _scanchar(fp, buffer);
        if (ch == EOF)
            goto done;
        if (base == 16) {
            if (isdigit(ch))
                value = value * base + ch - '0';
            else if (ch >= 'a' && ch <= 'f')
                value = value * base + ch - 'a' + 10;
            else if (ch >= 'A' && ch <= 'F')
                value = value * base + ch - 'A' + 10;
            else if (!(i == 1 && (ch == 'X' || ch == 'x'))) {
                _unscanc(fp, buffer, ch);
                goto done;
            }
        }
        else {
            digit = ch - '0';
            if ((digit >= 0) && (digit < base))
                value = value * base + digit;
            else {
                _unscanc(fp, buffer, ch);
                goto done;
            }
        }
    }
done:
    if (negative)
        *result = -value;
    else
        *result = value;
    return (i != 0);
}


_scandouble(fp, buffer, result, maxlen)
    FILE           *fp;
    char          **buffer;
    int             maxlen;
    double         *result;
{
    double          value = 0.0;
    double          intp, frac;
    int             state;
    int             digit, exp;
    int             negative, ch, i, nexp;

    *result = value;
    exp = 0;

    do {
        ch = _scanchar(fp, buffer);
        if (ch == EOF)
            return (0);
    } while (isspace(ch));

    if (!(negative = (ch == '-')))
        _unscanc(fp, buffer, ch);

    for (i = 0; i < maxlen; i++) {
        ch = _scanchar(fp, buffer);
        if (ch == EOF)
            goto done;
        digit = ch - '0';
        if ((digit >= 0) && (digit < 10))
            value = value * 10 + digit;
        else
            break;
    }
    if (ch == '.') {
        frac = 1.0;
        for (; i < maxlen; i++) {
            ch = _scanchar(fp, buffer);
            if (ch == EOF)
                goto done;
            digit = ch - '0';
            if (digit < 0 || digit > 9)
                break;
            else {
                frac /= 10.0;
                value += frac * digit;
            }
        }
    }
    if (ch == 'E' || ch == 'e') {
        ch = _scanchar(fp, buffer);
        nexp = 0;
        if (ch == '-' || ch == '+')
            nexp = (ch == '-');
        else
            _unscanc(fp, buffer, ch);
        exp = 0;
        for (; i < maxlen; i++) {
            ch = _scanchar(fp, buffer);
            if (ch == EOF)
                goto done;
            digit = ch - '0';
            if (digit < 0 || digit > 9)
                break;
            else
                exp = exp * 10 + digit;
        }
        if (nexp) {
            while (exp > 0) {
                value /= 10.0;
                --exp;
            }
        }
        else {
            while (exp > 0) {
                value *= 10.0;
                --exp;
            }
        }
    }
    _unscanc(fp, buffer, ch);

done:
    if (negative)
        *result = -value;
    else
        *result = value;
    return (i != 0);
}


_doscan(fmt, args, fp, buffer)
    char           *fmt, *buffer;
    FILE           *fp;
    void           *args;
{
    char            local[128];
    int             ch, maxlen, skip, check;
    int             match = 0;
    char           *aptr = (char *) args;
    char           *fptr = fmt;
    char           *cptr;
    int            *iptr;
    short          *sptr;
    unsigned int   *uptr;
    float          *fdptr;
    double         *dptr, dval;

    while (*fptr) {
        check = 0;
        switch (*fptr) {
        case ' ':
        case '\t':
        case '\n':
            check = ' ';
            break;
        case '%':
            fptr++;
            if ((check = (*fptr)) == '%')
                break;
            check = 0;
            maxlen = 1024;
            if (skip = ((*fptr) == '*'))
                fptr++;
            if (isdigit(*fptr)) {
                maxlen = 0;
                while (isdigit(*fptr)) {
                    maxlen = maxlen * 10 + (*fptr) - '0';
                    fptr++;
                }
            }
            switch (*fptr) {
            case 'c':
                ch = _scanchar(fp, &buffer);
                if (ch == EOF)
                    return (match);
                if (!skip) {
                    cptr = va_arg(aptr, char *);
                    *cptr = ch;
                    match++;
                }
                break;
            case 'd':
            case 'D':
                if (!_scanint(fp, &buffer, &ch, 10, maxlen))
                    return (match);
                if (!skip) {
                    iptr = va_arg(aptr, int *);
                    *iptr = ch;
                    match++;
                }
                break;
            case 'e':
            case 'f':
            case 'g':
                if (!_scandouble(fp, &buffer, &dval, maxlen))
                    return (match);
                if (!skip) {
                    fdptr = va_arg(aptr, float *);
                    *fdptr = dval;
                    match++;
                }
                break;
            case 'E':
            case 'F':
            case 'G':
                if (!_scandouble(fp, &buffer, &dval, maxlen))
                    return (match);
                if (!skip) {
                    dptr = va_arg(aptr, double *);
                    *dptr = dval;
                    match++;
                }
                break;
            case 'h':
                if (!_scanint(fp, &buffer, &ch, 10, maxlen))
                    return (match);
                if (!skip) {
                    sptr = va_arg(aptr, short *);
                    *sptr = ch;
                    match++;
                }
                break;
            case 'l':
            case 'L':
                ++fptr;
                switch (*fptr) {
                case 'd':
                case 'D':
                    if (!_scanint(fp, &buffer, &ch, 10, maxlen))
                        return (match);
                    if (!skip) {
                        iptr = va_arg(aptr, int *);
                        *iptr = ch;
                        match++;
                    }
                    break;
                case 'e':
                case 'E':
                case 'f':
                case 'F':
                case 'g':
                case 'G':
                    if (!_scandouble(fp, &buffer, &dval, maxlen))
                        return (match);
                    if (!skip) {
                        dptr = va_arg(aptr, double *);
                        *dptr = dval;
                        match++;
                    }
                    break;
                }
                break;
            case 'o':
                if (!_scanint(fp, &buffer, &ch, 8, maxlen))
                    return (match);
                if (!skip) {
                    iptr = va_arg(aptr, int *);
                    *iptr = ch;
                    match++;
                }
                break;
            case 's':
                if (skip) {
                    if (!_scanstr(fp, &buffer, local, maxlen))
                        return (match);
                }
                else {
                    cptr = va_arg(aptr, char *);
                    if (!_scanstr(fp, &buffer, cptr, maxlen))
                        return (match);
                    match++;
                }
                break;
            case 'u':
                if (!_scanint(fp, &buffer, &ch, 10, maxlen))
                    return (match);
                if (!skip) {
                    uptr = va_arg(aptr, unsigned *);
                    *uptr = ch;
                    match++;
                }
                break;
            case 'x':
                if (!_scanint(fp, &buffer, &ch, 16, maxlen))
                    return (match);
                if (!skip) {
                    iptr = va_arg(aptr, int *);
                    *iptr = ch;
                    match++;
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        fptr++;
        if (check > 0 && !isspace(check)) {
            ch = _scanchar(fp, &buffer);
            if (ch != check)
                return (match);
        }
    }
    return (match);
}

sscanf(buf, fmt, args)
    char           *buf, *fmt;
    void           *args;
{
    FILE            fp;

    return (_doscan(fmt, &args, NULL, buf));
}

fscanf(fp, fmt, args)
    FILE           *fp;
    char           *fmt;
    void           *args;
{
    return (_doscan(fmt, &args, fp, NULL));
}

scanf(fmt, args)
    char           *fmt;
    void           *args;
{
    return (_doscan(fmt, &args, stdin, NULL));
}
