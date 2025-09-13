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
	fprintf.c - Routines for formatted output

	sprintf(buf, fmt, args)			Print to a memory buffer
	    char           *buf, *fmt;
	    void           *args;

	fprintf(fp, fmt, args)			Print to a file pointer
	    FILE           *fp;
	    char           *fmt;
	    void           *args;

	printf(fmt, args)			Print to stdout
	    char           *fmt;
	    void           *args;
*/

#include <stdio.h>
#include <math.h>

#define isupper(x)  ((x) >= 'A' && (x) <= 'Z')
#define islower(x)  ((x) >= 'a' && (x) <= 'z')
#define toupper(x)  ((x) - 'a' + 'A')
#define tolower(x)  ((x) - 'A' + 'a')
#define isdigit(x)  ((x) >= '0' && (x) <= '9')

static char    *_hexdigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char     _doprntbuf[1024];

#define TRUE    1
#define FALSE   0

#define va_arg(var,mode)    ((mode)(*(mode *)var))

static char     buffer[128];

char           *
_itoa(num, base)
    int             num;
    unsigned int    base;
{
    char           *ptr;
    int             negative, digit;
    unsigned int    un;

    ptr = &buffer[sizeof(buffer) - 1];
    *ptr = '\0';

    if (num == 0x80000000) {
        switch (base) {
        case 8:
            return ("20000000000");
            break;
        case 10:
            return ("-2147483648");
            break;
        case 16:
            return ("80000000");
            break;
        default:
            return ("-OOPS-");
        }
    }

    if (negative = (num < 0))
        un = (unsigned int) -num;
    else
        un = (unsigned int)  num;

    do {
        *(--ptr) = _hexdigits[un % base];
        un /= base;
    } while (un && ptr > &buffer[1]);

    if (negative)
        *(--ptr) = '-';

    return (ptr);
}

char           *
_ftoa(num, precision)
    double          num;
    int             precision;
{
    char           *ptr;
    int             i, negative, digit;
    double          val, frac, intp, temp;

    if (precision > 32)
        precision = 6;

    if (negative = (num < 0.0))
        num = -num;

    num += .5 / pow( (double) 10.0, (double) precision);
    intp = floor(num);
    frac = num - intp;

    ptr = buffer;
    for (i = 0; i < precision; i++) {
        frac *= 10.0;
        digit = floor(frac);
        *ptr++ = (digit + '0');
        frac -= digit;
    }
    *ptr++ = '\0';

    ptr = &buffer[63 - precision];
    strcpy(ptr, buffer);

    *(--ptr) = '.';

    do {
        temp = floor(intp / 10.0);
        digit = intp - (temp * 10.0);
        *(--ptr) = (digit + '0');
        intp = temp;
    } while (intp > 0.0);

    if (negative)
        *(--ptr) = '-';

    return (ptr);
}

char           *
_etoa(num, precision)
    double          num;
    int             precision;
{
    char           *ptr;
    int             i, negative, digit, exp, ival;
    double          frac, intp, temp;

    if (precision > 32)
        precision = 6;

    if (negative = (num < 0.0))
        num = -num;

    exp = 0;

    if (num != 0.0) {
        while (num >= 10.0) {
            num /= 10.0;
            exp++;
        }

        while (num < 1.0) {
            num *= 10.0;
            exp--;
        }
    }

    num += .5 / pow( (double) 10.0, (double) precision);
    intp = floor(num);
    frac = num - intp;

    ptr = buffer;
    for (i = 0; i < precision; i++) {
        frac *= 10.0;
        digit = floor(frac);
        *ptr++ = (digit + '0');
        frac -= digit;
    }

    *ptr++ = 'E';
    if (exp >= 0)
        *ptr++ = '+';
    else {
        *ptr++ = '-';
        exp = -exp;
    }
    for (i = 2; i >= 0; i--) {
        digit = exp % 10;
        ptr[i] = (digit + '0');
        exp /= 10;
    }
    ptr[3] = '\0';

    ptr = &buffer[63 - (precision + 6)];
    strcpy(ptr, buffer);
    *(--ptr) = '.';

    do {
        temp = floor(intp / 10.0);
        digit = intp - (temp * 10.0);
        *(--ptr) = (digit + '0');
        intp = temp;
    } while (intp > 0.0);

    if (negative)
        *(--ptr) = '-';

    return (ptr);
}

char           *
_gtoa(num, precision)
    double          num;
    int             precision;
{
    char           *ptr;
    double          fsiz;

    fsiz = (num < 0.0 ? -num : num);

    if (fsiz > 1.0E+20 || fsiz < 1.0E-20)
        return _etoa(num, precision);

    ptr = _ftoa(num, precision);
    strcpy(buffer, ptr);

    if (precision > 0) {
        ptr = buffer;
        while (*ptr)
            ptr++;
        --ptr;
        while (*ptr == '0')
            *ptr-- = '\0';
    }
    return buffer;
}

char           *
_leftjust(str, width, pad)
    char           *str;
    int             width, pad;
{
    char           *ptr;
    int             len;

    len = 0;
    ptr = buffer;
    for (ptr = buffer; *str; len++)
        *ptr++ = *str++;
    while (len < width) {
        *ptr++ = pad;
        len++;
    }
    *ptr = '\0';
    return (buffer);
}

char           *
_rightjust(str, width, pad)
    char           *str;
    int             width, pad;
{
    char           *ptr;
    int             len;

    len = strlen(str);
    ptr = buffer;
    while (len++ < width)
        *ptr++ = pad;
    strcpy(ptr, str);

    return (buffer);
}

char           *
_justify(str, width, pad)
    char           *str;
    int             width;
    int             pad;
{
    if (width > 0)
        str = _rightjust(str, width, pad);
    else if (width < 0)
        str = _leftjust(str, -width, pad);
    return (str);
}


_doprnt(fmt, args, buffer)
    char           *fmt, *buffer;
    void           *args;
{
    int             ch, width, just, pad, maxlen;
    char           *aptr = (char *) args;
    char           *fptr = fmt;
    char           *cptr, *tptr;
    double         *dptr;
    char            local[2];

    while (*fptr) {
        switch (*fptr) {
        case '%':
            fptr++;
            width = 0;
            maxlen = 1024;
            if (just = ((*fptr) == '-'))
                fptr++;
            if ((pad = (*fptr)) != '0')
                pad = ' ';
            if ((*fptr) == '*') {
                fptr++;
                width = va_arg(aptr, int);
                aptr += sizeof(int);
            }
            else {
                while (isdigit(*fptr)) {
                    width = width * 10 + (*fptr) - '0';
                    fptr++;
                }
            }
            if (just)
                width = -width;
            if ((*fptr) == '.') {
                fptr++;
                if ((*fptr) == '*') {
                    fptr++;
                    maxlen = va_arg(aptr, int);
                    aptr += sizeof(int);
                }
                else {
                    maxlen = 0;
                    while (isdigit(*fptr)) {
                        maxlen = maxlen * 10 + (*fptr) - '0';
                        fptr++;
                    }
                }
            }
    doagain:
            switch (*fptr) {
            case '%':
                local[0] = '%';
                local[1] = '\0';
                cptr = local;
                break;
            case 'c':
                local[0] = va_arg(aptr, int);
                local[1] = '\0';
                cptr = local;
                aptr += sizeof(int);
                break;
            case 'd':
                cptr = _itoa(va_arg(aptr, int), 10);
                aptr += sizeof(int);
                break;
            case 'e':
                cptr = _etoa(va_arg(aptr, double), maxlen);
                aptr += sizeof(double);
                maxlen = 1024;
                break;
            case 'f':
                cptr = _ftoa(va_arg(aptr, double), maxlen);
                aptr += sizeof(double);
                maxlen = 1024;
                break;
            case 'g':
                cptr = _gtoa(va_arg(aptr, double), maxlen);
                aptr += sizeof(double);
                maxlen = 1024;
                break;
            case 'l':
                fptr++;
                goto doagain;
                break;
            case 'o':
                cptr = _itoa(va_arg(aptr, int), 8);
                aptr += sizeof(int);
                break;
            case 's':
                cptr = va_arg(aptr, char *);
                aptr += sizeof(char *);
                if (cptr == NULL)
                    cptr = "(null)";
                break;
            case 'u':
                cptr = _itoa(va_arg(aptr, unsigned), 10);
                aptr += sizeof(unsigned);
                break;
            case 'x':
                tptr = cptr = _itoa(va_arg(aptr, int), 16);
                while (*tptr) {
                    if (isupper(*tptr))
                        *tptr = tolower(*tptr);
                    tptr++;
                }
                aptr += sizeof(int);
                break;
            case 'X':
                cptr = _itoa(va_arg(aptr, int), 16);
                aptr += sizeof(int);
                break;
            default:
                cptr = "-OOPS-";
                break;
            }
            if (strlen(cptr) > maxlen)
                cptr[maxlen] = '\0';
            cptr = _justify(cptr, width, pad);
            while (*cptr)
                *buffer++ = *cptr++;
            break;
        default:
            *buffer++ = *fptr;
            break;
        }
        fptr++;
    }
    *buffer = '\0';
}

char           *
sprintf(buf, fmt, args)
    char           *buf, *fmt;
    void           *args;
{
    _doprnt(fmt, &args, buf);
    return (buf);
}

fprintf(fp, fmt, args)
    FILE           *fp;
    char           *fmt;
    void           *args;
{
    char           *buffer = _doprntbuf;

    _doprnt(fmt, &args, buffer);
    fwrite(buffer, 1, strlen(buffer), fp);
}

printf(fmt, args)
    char           *fmt;
    void           *args;
{
    char           *buffer = _doprntbuf;

    _doprnt(fmt, &args, buffer);
    fwrite(buffer, 1, strlen(buffer), stdout);
}
