
/* PDC Compiler - A Freely Distributable C Compiler for the Amiga
 *                Based upon prior work by Matthew Brandt and Jeff Lydiatt.
 *
 * PDC Compiler release 3.3 Copyright (C) 1989 Paul Petersen and Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 *
 * This code is freely redistributable upon the conditions that this 
 * notice remains intact and that modified versions of this file not be 
 * distributed as part of the PDC Software Distribution without the express
 * consent of the copyright holders.
 *
 *------------------------------------------------------------------
 *
 * $Log:	GetSym.c,v $
 * Revision 3.33  90/04/05  22:34:48  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:24:30  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * GetSym.c
 * 
 * A hodgepodge of routines for advanced input, error handling, and init's.
 * Oh yeah, it also contains the lexical analyzer.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <ctype.h>
#include    <assert.h>

#include    "host_posix.h"

#define GENERATE_TIME

#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

#define LINDEPTH    20

extern char    *itoa();
extern SYM     *search();
extern char    *prepdefine();
extern char    *litlate();
extern char    *fgets();

extern TABLE             tagtable;
extern unsigned char    *lptr;           /* shared with preproc */
extern FILE             *inclfile[10];   /* shared with preproc */
extern int               inclline[10];
extern char             *inclname[10];

extern int      incldepth;  /* shared with preproc */
extern int      inpreproc;  /* shared with preproc */
extern int      oneline;    /* shared with preproc */
extern enum e_pm premode;   /* shared with preproc */
extern enum e_ps prestat;   /* shared with preproc */

extern char     __linebuf[];
extern char     __filebuf[];
extern char     __datebuf[];
extern char     __funcbuf[];
extern char     __timebuf[];

#define MAX_ERRORS  80

int             fatal = FALSE;
static int      errno[MAX_ERRORS];
static char    *errxx[MAX_ERRORS];

static int      numerrs;
char            in_line[1024];
int             in_line_used;
int             in_comment = FALSE;
/*
 * SAS/C COMMENTNEST: when non-zero, block comments nest.  Default 0 so
 * existing comments that mention globs like bootstrap/ac/*.s still end
 * at the first star-slash (C89 / GCC).  Enable with -Wcommentnest.
 */
int             comment_nesting = 0;
int             total_errors = 0;
int             total_warnings = 0;
int             join_line = FALSE;
int             lstackptr = 0;  /* substitution stack pointer   */
int             dbxlnum = 0;    /* Line number for DBX      */
int             current_column = 1;  /* Current column position */
unsigned char  *linstack[LINDEPTH]; /* stack for substitutions      */
unsigned char   chstack[LINDEPTH];  /* place to save lastch     */

#define MAXERR  30

static char    *errmsg[] =
{
    "Syntax",
    "Bad Character",
    "Floating Point Error",
    "Bad Type",
    "Undefined Symbol",
    "Duplicate Symbol",
    "Punctuation",
    "Identifier expected",
    "Not initialized",
    "Incomplete",
    "Bad initialization",
    "Init size",
    "Bad Class statement",
    "Bad Block",
    "No Pointer",
    "No Function",
    "No Member",
    "L-Value expected",
    "Dereferencing error",
    "Mismatch",
    "Expression expected",
    "While expected",
    "No case",
    "Duplicate case",
    "Label error",
    "Preprocessor error",
    "Include file error",
    "Can't open include file",
    "Define error",
    "Prototype mismatch",
    "static assertion failed",
};

#ifdef GENERATE_TIME

void
time_and_date()
{
    /*
     * Use compiler builtins, not struct tm: SDK time.h is often incomplete
     * when ac-self compiles this file (tm_mon etc. -> No Member).
     */
    strcpy(__datebuf, "\"" __DATE__ "\"");
    strcpy(__timebuf, "\"" __TIME__ "\"");
}

#endif

void
install_defines()
{
    SYM *sp;

    strcpy(__linebuf, itoa(dbxlnum));
    padstr(__filebuf, curfile);

    padstr(__funcbuf, "**PDC**");

#ifdef GENERATE_TIME
    time_and_date();
#endif

    setdefine("__LINE__", __linebuf);
    setdefine("__FILE__", __filebuf);
    setdefine("__DATE__", __datebuf);
    setdefine("__TIME__", __timebuf);
    setdefine("__FUNC__", __funcbuf);
    setdefine("__STDC__", " 1 ");
    setdefine("__STDC_VERSION__", " 199409L ");
    /* Freestanding / unsupported C11+ features on Amiga AC */
    setdefine("__STDC_NO_ATOMICS__", " 1 ");
    setdefine("__STDC_NO_THREADS__", " 1 ");
    setdefine("__STDC_NO_COMPLEX__", " 1 ");
    setdefine("__STDC_NO_VLA__", " 1 ");
    setdefine("__PDC__", " 1 ");
    setdefine("pdc", " 1 ");
    setdefine("amiga", " 1 ");
    setdefine("__AMIGA__", " 1 ");
    setdefine("__amigaos__", " 1 ");
    setdefine("__SASC", " 1 ");
    setdefine("_SASC", " 1 ");
    setdefine("__SASC__", " 1 ");


    if (Options.Builtin) {
        setdefine("strcmp", "__BUILTIN_strcmp");
        setdefine("strcpy", "__BUILTIN_strcpy");
        setdefine("strlen", "__BUILTIN_strlen");
        setdefine("strcat", "__BUILTIN_strcat");
        setdefine("bcopy", "__BUILTIN_bcopy");
        setdefine("bzero", "__BUILTIN_bzero");
    }
    for (sp = cmd_defsyms.head; sp != NULL; sp = sp->next) {
        if (search(sp->name, defsyms.head) != NULL) 
            remove_symbol( sp->name, &defsyms );
        if (sp->value.s != NULL)
            setdefine( sp->name, sp->value.s );
        else
            setdefine( sp->name, " 1 " );
    }
    for (sp = cmd_undefsyms.head; sp != NULL; sp = sp->next) 
        remove_symbol( sp->name, &defsyms );
}

void
initsym()
{
    defsyms.head = defsyms.tail = NULL;
    tagtable.head = tagtable.tail = NULL;
    lsyms.head = lsyms.tail = NULL;
    gsyms.head = gsyms.tail = NULL;
    cmd_local.head = cmd_local.tail = NULL;

    in_line[0] = '\0';
    lptr = (unsigned char *) in_line;
    nextlabel = 1;
    numerrs = 0;
    total_errors = 0;
    lineno = 0;
    dbxlnum = 0;
    fatal = FALSE;
    libpragma = NULL;
}

int
ac_getline(listflag)
    int             listflag;
{
    int             err, i;
    char            rv, *s, *data;

    do {
        if (Options.List && listflag && lineno > 0) {
            fprintf(list, "%-6d\t%s", lineno, in_line);
        }

        if (numerrs) {

            /*
             * Print where any error(s) was (were) found:
             */
            if (!(Options.List && listflag)) {
                if (numerrs == 1)
                    fprintf(AC_DIAG_STREAM, "\nError in " );
                else
                    fprintf(AC_DIAG_STREAM, "\nErrors in " );
                if (lineno > 0) {
                    fprintf(AC_DIAG_STREAM, "line %d of ", lineno);
                }
                fprintf(AC_DIAG_STREAM, "%s:\n%s", curfile, in_line);
            }

            /*
             * Describe the errors:
             */
            for (i = 0; numerrs--; ++i) {
                err = errno[i];
                s = (char *) " *** error ";
                fprintf(AC_DIAG_STREAM, "%s%d", s, err);
                if (err <= MAXERR) {
                    fprintf(AC_DIAG_STREAM, " %s", errmsg[err]);
                    if (errxx[i] != NULL) {
                        fprintf(AC_DIAG_STREAM, " : %s", errxx[i]);
                    }
                }
                else {
                    if (Options.List && listflag && lineno > 0) {
                        fprintf(list, "%s%d\n", s, err);
                        if (errxx[i] != NULL) {
                            fprintf(list, " : %s", errxx[i]);
                        }
                        fprintf( list, "\n" );
                    }
                }
                fprintf(AC_DIAG_STREAM, "\n" );
            }
        }

        numerrs = 0;
        ++lineno;
        current_column = 1;  /* Reset column position at start of new line */

        data = fgets(in_line, sizeof(in_line) - 1, input);

        in_line_used = 0;
        rv = feof(input);
        if (data == NULL && rv && incldepth > 0) {
            fclose(input);
            input = inclfile[--incldepth];
            lineno = dbxlnum = inclline[incldepth];
            curfile = inclname[incldepth];
            padstr( __filebuf, curfile );
            strcpy( __linebuf, itoa(dbxlnum));
            return ac_getline(0);
        }

        if (data == NULL && rv)
            return 1;

        ++dbxlnum;
        strcpy(__linebuf, itoa(dbxlnum));

        lptr = (unsigned char *) in_line;
        if (in_line[0] == '#' && !in_comment)
            return preprocess();
    } while (prestat == ps_ignore);
    return 0;
}

/*
 * getch - basic get character routine.
 */

int
getch()
{
    while ((lastch = *lptr++) == '\0') {
        if (lstackptr > 0) {
            lptr = linstack[--lstackptr];
            lastch = chstack[lstackptr];
            /* Update column position */
            if (lastch == '\n') {
                current_column = 1;
            } else if (lastch != '\r') {
                current_column++;
            }
            return lastch;
        }
        if (oneline)
            return lastch = -1;
        if (ac_getline(incldepth == 0))
            return lastch = -1;
    }
    
    /* Update column position */
    if (lastch == '\n') {
        current_column = 1;
    } else if (lastch != '\r') {
        current_column++;
    }
    
    return lastch;
}

/*
 * joinch - basic get character routine, will NOT obey oneline.
 */

int
joinch()
{
    while ((lastch = *lptr++) == '\0') {
        if (lstackptr > 0) {
            lptr = linstack[--lstackptr];
            lastch = chstack[lstackptr];
            return lastch;
        }
        if (ac_getline(incldepth == 0))
            return lastch = -1;
    }
    return lastch;
}

/*
 * error - print error information
 */

void
fatal_error(n, msg)
    int             n;
    char           *msg;
{
    fatal = TRUE;

    if (numerrs < MAX_ERRORS) {
        errno[numerrs] = n;
        errxx[numerrs] = msg;
        ++numerrs;
        ++total_errors;
    }
}

void
error(n, msg)
    int             n;
    char           *msg;
{
    if (numerrs < MAX_ERRORS) {
        errno[numerrs] = n;
        errxx[numerrs] = msg;
        ++numerrs;
        ++total_errors;
    }
    /* Also print the error immediately using the new format */
    error_at_line(n, msg, NULL, 0, 0);
}

void
warning(n, msg)
    int             n;
    char           *msg;
{
    /* Print warning immediately using the new format */
    warning_at_line(n, msg, NULL, 0, 0);
}

void
error_at_line(n, msg, filename, line, column)
    int             n;
    char           *msg;
    char           *filename;
    int             line;
    int             column;
{
    /* Print error in selected format */
    if (filename == NULL) filename = curfile;
    if (line <= 0) line = lineno;
    if (column <= 0) column = current_column;
    
    switch (Options.OutputFormat) {
    case 0: /* GCC format */
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, "%s:%d:%d: error: %s", filename, line, column, msg);
        } else {
            fprintf(AC_DIAG_STREAM, "%s:%d: error: %s", filename, line, msg);
        }
        if (n <= MAXERR) {
            fprintf(AC_DIAG_STREAM, " (%s)", errmsg[n]);
        }
        fprintf(AC_DIAG_STREAM, "\n");
        break;
    case 1: /* SASC format */
        fprintf(AC_DIAG_STREAM, "%s(%d", filename, line);
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, ",%d", column);
        }
        fprintf(AC_DIAG_STREAM, ") : error: %s", msg);
        if (n <= MAXERR) {
            fprintf(AC_DIAG_STREAM, " (%s)", errmsg[n]);
        }
        fprintf(AC_DIAG_STREAM, "\n");
        break;
    case 2: /* PDC format */
    default:
        fprintf(AC_DIAG_STREAM, "Error in %s:%d: %s", filename, line, msg);
        if (n <= MAXERR) {
            fprintf(AC_DIAG_STREAM, " (%s)", errmsg[n]);
        }
        fprintf(AC_DIAG_STREAM, "\n");
        break;
    }
}

void
warning_at_line(n, msg, filename, line, column)
    int             n;
    char           *msg;
    char           *filename;
    int             line;
    int             column;
{
    if (filename == NULL) filename = curfile;
    if (line <= 0) line = lineno;
    if (column <= 0) column = current_column;
    
    switch (Options.OutputFormat) {
    case 0: /* GCC format */
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, "%s:%d:%d: warning: %s", filename, line, column, msg);
        } else {
            fprintf(AC_DIAG_STREAM, "%s:%d: warning: %s", filename, line, msg);
        }
        if (n <= MAXERR) {
            fprintf(AC_DIAG_STREAM, " (%s)", errmsg[n]);
        }
        fprintf(AC_DIAG_STREAM, "\n");
        break;
    case 1: /* SASC format */
        fprintf(AC_DIAG_STREAM, "%s(%d", filename, line);
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, ",%d", column);
        }
        fprintf(AC_DIAG_STREAM, ") : warning: %s", msg);
        if (n <= MAXERR) {
            fprintf(AC_DIAG_STREAM, " (%s)", errmsg[n]);
        }
        fprintf(AC_DIAG_STREAM, "\n");
        break;
    case 2: /* PDC format */
    default:
        fprintf(AC_DIAG_STREAM, "Warning in %s:%d: %s", filename, line, msg);
        if (n <= MAXERR) {
            fprintf(AC_DIAG_STREAM, " (%s)", errmsg[n]);
        }
        fprintf(AC_DIAG_STREAM, "\n");
        break;
    }
    
    ++total_warnings;
    
    /* Treat warnings as errors if requested */
    if (Options.WarningsAsErrors) {
        ++total_errors;
    }
}

#ifndef isidch
int
isidch( x )
    int x;
{
    return(x == '$' || x == '_' || isalpha(x) || isdigit(x));
}
#endif

/*
 * getid - get an identifier.
 * 
 * identifiers are any isidch conglomerate that doesn't start with a numeric
 * character. this set INCLUDES keywords.
 */

void 
getid()
{
    int             i = 0;

    lastst = id;
    while (isidch(lastch)) {
        if (i < MAX_ID)
            lastid[i++] = lastch;
        getch();
    }
    lastid[i] = '\0';
}

int
gethex_ch()
{
    int     j, v = 0;

    for (j = 0; j < 2; ++j) {
        if (lastch <= '9' && lastch >= '0')
            v = (v << 4) + lastch - '0';
        else if (lastch <= 'f' && lastch >= 'a')
            v = (v << 4) + lastch - 'a' + 10;
        else if (lastch <= 'F' && lastch >= 'A')
            v = (v << 4) + lastch - 'A' + 10;
        else
            break;
        getch();
    }
    return (v);
}

int
getoct_ch()
{
    int     j, v = 0;

    for (j = 0; j < 3; ++j) {
        if (lastch == 'x' && v == 0) {
            getch();
            return( gethex_ch() );
        }
        if (lastch > '7' || lastch < '0')
            break;
        v = (v << 3) + lastch - '0';
        getch();
    }
    return( v );
}

/*
 * getsch - get a character in a quoted string.
 * 
 * this routine handles all of the escape mechanisms for characters in strings
 * and character constants.
 */
int
getsch()
{               /* return an in-quote character */
    register int    i;

    if (lastch == '\n')
        return -1;
    if (lastch != '\\') {
        i = lastch;
        getch();
        return i;
    }

    getch();        /* get an escaped character */

    if (lastch == 'x' || isdigit(lastch)) 
        return( getoct_ch() );

    i = lastch;
    getch();
    if (i == '\n')
        return getsch();

    switch (i) {
    case 'a':
        return '\a';
    case 'b':
        return '\b';
    case 'f':
        return '\f';
    case 'n':
        return '\n';
    case 'r':
        return '\r';
    case 't':
        return '\t';
    case 'v':
        return '\v';
    default:
        return i;
    }
}

int
radix36(c)
    char            c;
{
    if (isdigit(c))
        return c - '0';
    if (c >= 'a' && c <= 'z')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'Z')
        return c - 'A' + 10;
    return -1;
}

/*
 * getbase - get an integer in any base.
 *
 * Accumulates into the 64-bit pair (ival_hi, ival) using only 32-bit
 * arithmetic so SAS/C / Amiga hosts without long long still work.
 */

void
getbase(b)
    char            b;
{
    register long   j;
    unsigned long   base;
    unsigned long   lo, hi;
    unsigned long   a0, a1, b0, b1, p0, p1, p2, c;

    base = (unsigned long) b;
    lo = 0;
    hi = 0;
    while (isalnum(lastch) || lastch == '\'') {
        /* C23 digit separators: skip ' between digits (1'000, 0xFF'FF). */
        if (lastch == '\'') {
            getch();
            if (!isalnum(lastch))
                break;
            continue;
        }
        if ((j = radix36(lastch)) < b) {
            /*
             * (hi:lo) = (hi:lo) * base + digit  via 16-bit partial products.
             */
            a0 = lo & 0xffffUL;
            a1 = (lo >> 16) & 0xffffUL;
            b0 = base & 0xffffUL;
            b1 = (base >> 16) & 0xffffUL;
            p0 = a0 * b0;
            p1 = a0 * b1 + a1 * b0;
            p2 = a1 * b1;
            c = (p0 >> 16) + (p1 & 0xffffUL);
            lo = (p0 & 0xffffUL) | ((c & 0xffffUL) << 16);
            c = (c >> 16) + (p1 >> 16) + p2;
            hi = hi * base + c;
            lo += (unsigned long) j;
            if (lo < (unsigned long) j)
                hi++;
            getch();
        }
        else
            break;
    }
    ival = (long) lo;
    ival_hi = (long) hi;
    lastst = iconst;
}

/*
 * getfrac - get fraction part of a floating number.
 *
 * Digits and the 10^n scale are gathered as integers so this function's
 * own source has no floating literals (1.0 / 10.0).  Compiling GetSym.c
 * with ac-self used to re-enter getfrac while lexing those literals and
 * die mid-_getfrac; casts only need .Fl2d/.FDdiv/.FDadd at runtime.
 */

void
getfrac()
{
    long            digs;
    long            scale;
    int             n, maxn;

    digs = 0;
    n = 0;
    maxn = 9;       /* fit in signed 32-bit with room */
    while (lastch >= '0' && lastch <= '9') {
        if (n < maxn)
            digs = digs * 10 + (lastch - '0');
        n++;
        getch();
    }
    if (n == 0)
        return;
    if (n > maxn)
        n = maxn;
    scale = 1;
    while (n-- > 0)
        scale = scale * 10;
    rval = rval + ((double) digs) / ((double) scale);
}

/*
 * getexp - get exponent part of floating number.
 *
 * Same rule as getfrac: no floating literals in this source.  Exponent
 * magnitude is capped at 310; scale with square-and-multiply on doubles
 * built only from integer casts.
 */

void
getexp()
{
    int             neg;
    long            e;
    double          base, acc;

    if (lastst != rconst)
        rval = (double) ival;
    neg = 0;
    if (lastch == '+' || lastch == '-') {
        neg = (lastch == '-');
        getch();
    }
    getbase(10);

    if (ival > 310)
        error(ERR_FPCON, NULL);
    else {
        e = ival;
        if (e < 0)
            e = -e;
        base = (double) 10;
        if (neg)
            base = ((double) 1) / base;
        acc = (double) 1;
        while (e > 0) {
            if (e & 1)
                acc = acc * base;
            base = base * base;
            e >>= 1;
        }
        rval = rval * acc;
    }
    lastst = rconst;
}

/*
 * Floating-constant suffixes (C99/C23): f/F → float, l/L → long double.
 * Amiga maps long double to double; value stays in rval as double.
 */
static void
getfloatsuffix()
{
    rval_float_suffix = 0;
    if (lastch == 'f' || lastch == 'F') {
        rval_float_suffix = 1;
        getch();
    }
    else if (lastch == 'l' || lastch == 'L') {
        rval_float_suffix = 2;
        getch();
    }
}

/*
 * getnumber - get a number from input.
 * 
 * getnumber handles all of the numeric input. it accepts decimal, octal,
 * hexadecimal, binary (0b/0B), and floating point numbers.  Digit
 * separators (') are accepted inside integer digit runs.
 */

void
getnumber()
{
    ival_hi = 0;
    rval_float_suffix = 0;
    if (lastch != '0')
        getbase(10);
    else {
        getch();
        if (lastch == 'x' || lastch == 'X') {
            getch();
            getbase(16);
        }
        else if (lastch == 'b' || lastch == 'B') {
            getch();
            getbase(2);
        }
        else if (lastch == '.')
            ival = 0;
        else
            getbase(8);
    }

    if (lastch == '.') {
        getch();
        rval = ival;    /* float the integer part */
        getfrac();  /* add the fractional part */
        lastst = rconst;
    }
    if (lastch == 'e' || lastch == 'E') {
        getch();
        getexp();   /* get the exponent */
    }

    /*
     * Floating suffixes first (1.0f / 1e0F / 1.0L).  Then C89/C99 integer
     * suffixes in any order: U/u with L/l or LL/ll.
     */
    if (lastst == rconst) {
        getfloatsuffix();
        ival_unsigned = 0;
    }
    else if (lastst == iconst) {
        int             have_u;
        int             nlong;
        int             more;

        have_u = 0;
        nlong = 0;
        more = 1;
        ival_unsigned = 0;
        while (more) {
            more = 0;
            if (!have_u && (lastch == 'u' || lastch == 'U')) {
                have_u = 1;
                getch();
                more = 1;
            } else if (nlong < 2 && (lastch == 'l' || lastch == 'L')) {
                getch();
                nlong++;
                more = 1;
            }
        }
        ival_unsigned = have_u;
        if (nlong >= 2)
            lastst = llconst;
        else if (nlong == 1)
            lastst = lconst;
    } else
        ival_unsigned = 0;
}

void
getdotnumber()
{
    /* No "0.0" literal — see getfrac comment (ac-self compiling GetSym). */
    rval = (double) 0;
    getfrac();      /* add the fractional part */
    lastst = rconst;

    if (lastch == 'e' || lastch == 'E') {
        getch();
        getexp();   /* get the exponent */
    }
    getfloatsuffix();
    ival_unsigned = 0;
}
 
static char *
charmsg( ch )
    int ch;
{
    char    *ptr, buffer[80];

    strcpy( buffer, "unknown character (" );
    strcat( buffer, itoa(ch));

    for (ptr = buffer; *ptr; ++ptr);

    *ptr++ = ')';
    *ptr++ = ' ';
    *ptr++ = '\'';

    if (ch >= 32)
        *ptr++ = ch;
    else {
        *ptr++ = '^';
        *ptr++ = (ch + '@');
    }
    *ptr++ = '\'';
    *ptr = '\0';
    return( litlate(buffer) );
}

/*
 * getsym - get next symbol from input stream.
 * 
 * getsym is the basic lexical analyzer.  It builds basic tokens out of the
 * characters on the input stream and sets the following global variables:
 * 
 *  lastch:    A look behind buffer. 
 *  lastst:    type of last symbol read.
 *  laststr:   last string constant read. 
 *  lastid:    last identifier read. 
 *  ival:      last integer constant read. 
 *  rval:      last real constant read.
 * 
 * getsym should be called for all your input needs...
 */

void
getsym()
{
    register int    i, j;
    unsigned char  *loc;
    SYM            *sp;

restart:            /* we come back here after comments */

    while (premode != pr_asm && isspace(lastch))
        getch();

    if (premode == pr_asm && !oneline) {
        if (lastch == -1) {
            lastst = eof;
            return;
        }
        for (i = 0; i < MAX_STRLEN; ++i) {
            if (lastch == '\n' || lastch == -1 || (j = getsch()) == -1)
                break;
            laststr[i] = j;
        }
        laststr[i] = 0;
        if (lastch == '\n')
            getch();
        else
            error(ERR_SYNTAX, NULL);
        lastst = asmconst;
        return;
    }

    while (isspace(lastch))
        getch();

    if (lastch == -1)
        lastst = eof;
    else if (isdigit(lastch))
        getnumber();
    else if (isidch(lastch)) {
        getid();
        if (!inpreproc && (sp = search(lastid, defsyms.head)) != NULL) {
            loc = (unsigned char *) prepdefine(sp);
            if (loc != NULL) {
                if (lstackptr >= LINDEPTH) {
                    error(ERR_DEFINE, NULL);
                    lstackptr = 0;
                    lptr = linstack[lstackptr];
                    lastch = chstack[lstackptr];
                }
                else {
                    linstack[lstackptr] = lptr;
                    chstack[lstackptr++] = lastch;
                    lptr = loc;
                    getch();
                    goto restart;
                }
            }
        }
    }
    else
        switch (lastch) {
        case '+':
            getch();
            if (lastch == '+') {
                getch();
                lastst = autoinc;
            }
            else if (lastch == '=') {
                getch();
                lastst = asplus;
            }
            else
                lastst = plus;
            break;
        case '-':
            getch();
            if (lastch == '-') {
                getch();
                lastst = autodec;
            }
            else if (lastch == '=') {
                getch();
                lastst = asminus;
            }
            else if (lastch == '>') {
                getch();
                lastst = pointsto;
            }
            else
                lastst = minus;
            break;
        case '*':
            getch();
            if (lastch == '=') {
                getch();
                lastst = astimes;
            }
            else
                lastst = star;
            break;
        case '/':
            getch();
            if (lastch == '=') {
                getch();
                lastst = asdivide;
            }
            else if (lastch == '*') {
                /*
                 * Block comment.  Default: C89/GCC — first star-slash ends
                 * it (so globs like *.s in comments stay safe).  With
                 * -Wcommentnest (SAS/C COMMENTNEST): slash-star raises
                 * depth, star-slash lowers it.
                 */
                {
                    int             depth;

                    in_comment = TRUE;
                    depth = 1;
                    getch();
                    for (;;) {
                        if (lastch == -1) {
                            error(ERR_SYNTAX, "unterminated comment");
                            in_comment = FALSE;
                            lastst = eof;
                            return;
                        }
                        if (comment_nesting && lastch == '/') {
                            getch();
                            if (lastch == '*') {
                                depth++;
                                getch();
                            }
                        }
                        else if (lastch == '*') {
                            getch();
                            if (lastch == '/') {
                                getch();
                                depth--;
                                if (depth <= 0) {
                                    in_comment = FALSE;
                                    goto restart;
                                }
                            }
                        }
                        else
                            getch();
                    }
                }
            }
            else if (lastch == '/') {
                /*
                 * C99 // line comment (GCC -std=c99 / gnu89+).  Runs to
                 * end of line or EOF; does not nest or span lines here
                 * (backslash line splice is handled earlier by input).
                 */
                while (lastch != '\n' && lastch != -1)
                    getch();
                goto restart;
            }
            else
                lastst = divide;
            break;
        case '^':
            getch();
            if (lastch == '=') {
                getch();
                lastst = aseor;
            }
            else
                lastst = uparrow;
            break;
        case ';':
            getch();
            lastst = semicolon;
            break;
        case ':':
            getch();
            lastst = colon;
            break;
        case '=':
            getch();
            if (lastch == '=') {
                getch();
                lastst = eq;
            }
            else
                lastst = assign;
            break;
        case '>':
            getch();
            if (lastch == '=') {
                getch();
                lastst = geq;
            }
            else if (lastch == '>') {
                getch();
                if (lastch == '=') {
                    getch();
                    lastst = asrshift;
                }
                else
                    lastst = rshift;
            }
            else
                lastst = gt;
            break;
        case '<':
            getch();
            if (lastch == '=') {
                getch();
                lastst = leq;
            }
            else if (lastch == '<') {
                getch();
                if (lastch == '=') {
                    getch();
                    lastst = aslshift;
                }
                else
                    lastst = lshift;
            }
            else
                lastst = lt;
            break;
        case '\'':
            getch();
            ival = getsch();    /* get a string char */
            if (lastch != '\'')
                error(ERR_SYNTAX, NULL);
            else
                getch();
            lastst = cconst;
            break;
        case '\"':
            getch();
            i = 0;
            for (;;) {
                if (lastch == '\"')
                    break;
                if ((j = getsch()) == -1)
                    break;
                if (i < MAX_STRLEN) 
                    laststr[i++] = j;
            }
            laststr[i] = 0;
            lastst = sconst;
            if (lastch != '\"')
                error(ERR_SYNTAX, NULL);
            else
                getch();
            break;
        case '!':
            getch();
            if (lastch == '=') {
                getch();
                lastst = neq;
            }
            else
                lastst = sym_not;
            break;
        case '%':
            getch();
            if (lastch == '=') {
                getch();
                lastst = asmodop;
            }
            else
                lastst = modop;
            break;
        case '~':
            getch();
            lastst = sym_compl;
            break;
        case '.':
            getch();
            if (isdigit(lastch)) {
                getdotnumber();
            }
            else if (lastch != '.')
                lastst = dot;
            else {
                getch();
                if (lastch == '.') {
                    getch();
                    lastst = ellipsis;
                }
                else {
                    error(ERR_ILLCHAR, charmsg( lastch ) );
                    goto restart;   /* get a real token */
                }
            }
            break;
        case ',':
            getch();
            lastst = comma;
            break;
        case '&':
            getch();
            if (lastch == '&') {
                lastst = land;
                getch();
            }
            else if (lastch == '=') {
                lastst = asand;
                getch();
            }
            else
                lastst = sym_and;
            break;
        case '|':
            getch();
            if (lastch == '|') {
                lastst = lor;
                getch();
            }
            else if (lastch == '=') {
                lastst = asor;
                getch();
            }
            else
                lastst = sym_or;
            break;
        case '(':
            getch();
            lastst = openpa;
            break;
        case ')':
            getch();
            lastst = closepa;
            break;
        case '[':
            getch();
            lastst = openbr;
            break;
        case ']':
            getch();
            lastst = closebr;
            break;
        case '{':
            getch();
            lastst = begin;
            break;
        case '}':
            getch();
            lastst = end;
            break;
        case '?':
            getch();
            lastst = hook;
            break;
        case '\\':
            getch();
            if (lastch == '\n') {
                joinch();
                goto restart;
            }
        default:
            error(ERR_ILLCHAR, charmsg( lastch ) );
            getch();
            goto restart;   /* get a real token */
        }

    if (lastst == id)
        searchkw();
}

void
needpunc(p)
    enum e_sym      p;
{
    if (lastst == p)
        getsym();
    else {
        error(ERR_PUNCT, NULL);
    }
}
