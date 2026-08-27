
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
 * $Log:	PreProc.c,v $
 * Revision 3.33  90/04/05  22:45:26  lionel
 * Changed default UNIX include path to something more likely.
 * 
 * Revision 3.32  90/02/03  16:25:27  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * PreProc.c Built-in preprocessor
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    <ctype.h>

#ifdef AZTEC_C
#define isidch(x)    ((ctp_[(x)+1] & 0x07) || ((x) == '_') || ((x) == '$'))
#endif

#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    "host_posix.h"
#include    "FrontEnd.h"

extern char    *itoa(int x);

#define LPAR    '('
#define RPAR    ')'
#define QUOT    '"'
#define SQUOT   '\''
#define COMMA   ','
#define BSLASH  '\\'

#ifndef INCLUDE   /* The standard place for include files */
#if defined(AC_HOST_POSIX)
#define INCLUDE "/usr/include/"
#else
#define INCLUDE "AC:include/"
#endif
#endif

/*
 * Include-stack and -I path tables.  Real pointer arrays (not char[40]
 * overlays): type_size() recomputes count*elemsize when a bare count was
 * left in tp->size, so DS.b is 40 for char *[10] under ac-self.
 */
FILE           *inclfile[10];
int             inclline[10];
char           *inclname[10];
char           *incldir[10];

int             incldepth = 0;
int             prepdepth = 0;
int             inpreproc = FALSE;
int             oneline = FALSE;
int             inclnum = 0;
unsigned char  *lptr;
char            prepbuffer[1024];

#if !defined(AC_HOST_POSIX)
/*
 * Default -I list for Amiga self-host.  Nested #include <...> walks this
 * same list (no "dir of parent header" search).
 *
 * Order matters: C library headers (stdio/stdlib) must come from cclib or
 * local include/ — not from NDK include:, which has SAS-isms AC rejects
 * (e.g. "unsigned long int").  NDK is last so dos/exec/proto still resolve.
 */
void
install_bootstrap_includes(void)
{
    if (inclnum == 0) {
        /* Prefer cclib.library headers (shared C lib); fall back to local. */
        incldir[inclnum++] = "///SDK/cclib.library/SDK/Include_H/";
        incldir[inclnum++] = "compinc/";
        incldir[inclnum++] = "include/";
        /* NDK last — dos/, exec/, proto/, … */
        incldir[inclnum++] = "include:";
    }
}
#endif

char            __linebuf[10];  /* buffer for __LINE__  */
char            __filebuf[64];  /* buffer for __FILE__  */
char            __datebuf[64];  /* buffer for __DATE__  */
char            __timebuf[64];  /* buffer for __TIME__  */
char            __funcbuf[64];  /* buffer for __FUNC__  */

/* Token buffer for lookahead */
static char     token_buffer[256];
static int      token_buffered = 0;

enum e_pm       inclprep[32];  /* Increased from 10 to 32 for deep nesting */
enum e_ps       inclstat[32];  /* Increased from 10 to 32 for deep nesting */

enum e_pm       premode = pr_all;
enum e_ps       prestat = ps_do;

void  setdefine();

extern int      in_comment;
extern int      comment_nesting;
extern int      dbxlnum;
extern char    *litlate();
extern char    *xalloc();
extern SYM     *search();
extern enum e_sym getid();
extern struct snode *filestmt();

/*
 * True if name looks like an Amiga volume/assign path (e.g. RAM:foo)
 * rather than a relative path that may contain '/'.
 */
static int
include_name_absolute(name)
    char           *name;
{
    char           *p;

    if (name == NULL || name[0] == '\0')
        return 0;
    if (name[0] == '/')
        return 1;
    for (p = name; *p != '\0'; p++) {
        if (*p == ':')
            return 1;
        if (*p == '/')
            return 0;
    }
    return 0;
}

/*
 * Build "dir-of-curfile" + name into dst for #include "name".
 * Returns 1 if a candidate path was written.
 */
static int
include_beside_caller(dst, name)
    char           *dst;
    char           *name;
{
    char           *p;
    char           *cut;
    char           *src;
    int             n;
    int             i;
    int             namelen;

    dst[0] = '\0';
    if (curfile == NULL || curfile[0] == '\0' || name == NULL)
        return 0;
    if (include_name_absolute(name))
        return 0;

    cut = NULL;
    for (p = curfile; *p != '\0'; p++) {
        if (*p == '/' || *p == ':')
            cut = p;
    }
    if (cut == NULL)
        return 0;

    n = (int) (cut - curfile) + 1;
    namelen = (int) strlen(name);
    if (n + namelen >= (int) sizeof(prepbuffer))
        return 0;

    src = curfile;
    for (i = 0; i < n; i++)
        dst[i] = src[i];
    dst[n] = '\0';
    strcat(dst, name);
    return 1;
}

void
padstr(buf, value)
    char           *buf, *value;
{
    *buf++ = '"';

    if (value != NULL)
        while (*value)
            *buf++ = *value++;

    *buf++ = '"';
    *buf = '\0';
}


void
doinclude()
{
    int             num, method;
    SYM             *sp;
    char           *ptr, filename[80];
    FILE           *fp;

    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    fp = NULL;
    num = 0;
    method = TRUE;  /* <> search; set FALSE for "file" */
    oneline = TRUE;

    while (isspace(lastch)) /* Skip the white space */
        getch();

    if (lastch == QUOT) {   /* #include "file.h"    */
        getsym();
        oneline = FALSE;
        if (lastst == sconst) {
            strcpy( filename, laststr );
            if (search( filename, cmd_include.head) != NULL) {
                ac_getline(incldepth == 0);
                return;
            }
            method = FALSE;
            /*
             * Quoted includes: search beside the including file first
             * (dir of curfile), then as a literal/cwd path, then -I.
             * Angle-bracket includes skip the beside-caller step.
             */
            if (include_beside_caller(prepbuffer, laststr)
                && (fp = fopen(prepbuffer, "r")) != NULL)
                strcpy(laststr, prepbuffer);
            if (fp == NULL && (fp = fopen(laststr, "r")) == NULL) {
                for (num = 0; num < inclnum && fp == NULL; num++) {
                    strcpy(prepbuffer, incldir[num]);
                    strcat(prepbuffer, laststr);
                    if ((fp = fopen(prepbuffer, "r")) != NULL)
                        strcpy(laststr, prepbuffer);
                }
            }
            if (fp == NULL) {
                strcpy(prepbuffer, INCLUDE);
                strcat(prepbuffer, laststr);
                if ((fp = fopen(prepbuffer, "r")) != NULL) 
                    strcpy(laststr, prepbuffer);
            }
        }
    }
    else if (lastch == '<') {   /* #include <file.h>    */
        getch();
        ptr = laststr;
        while (lastch != EOF && lastch != '>') {
            *ptr++ = lastch;
            getch();
        }
        *ptr = '\0';
        lastst = sconst;

        oneline = FALSE;
        strcpy( filename, laststr );
        if (search( filename, cmd_include.head ) != NULL) {
            ac_getline(incldepth == 0);
            return;
        }
        method = TRUE;
        for (num = 0; num < inclnum && fp == NULL; num++) {
            strcpy(prepbuffer, incldir[num]);
            strcat(prepbuffer, laststr);
            if ((fp = fopen(prepbuffer, "r")) != NULL)
                strcpy(laststr, prepbuffer);
        }
        if (fp == NULL) {
            strcpy(prepbuffer, INCLUDE);
            strcat(prepbuffer, laststr);
            if ((fp = fopen(prepbuffer, "r")) != NULL)
                strcpy(laststr, prepbuffer);
        }
    }

    if (lastst != sconst) {
        fatal_error(ERR_INCLFILE, "file name is not a string");
        ac_getline(incldepth == 0);
        return;
    }

    if (fp == NULL) {
        /*
         * Show every candidate tried.  Nested includes use the same -I
         * list; a missing NDK assign shows up here as include: + name.
         */
        fprintf(AC_DIAG_STREAM, "Can't open include file \"%s\"\n", filename);
        if (method == FALSE && include_beside_caller(prepbuffer, filename))
            fprintf(AC_DIAG_STREAM, "  tried: %s\n", prepbuffer);
        fprintf(AC_DIAG_STREAM, "  tried: %s\n", filename);
        for (num = 0; num < inclnum; num++) {
            strcpy(prepbuffer, incldir[num]);
            strcat(prepbuffer, filename);
            fprintf(AC_DIAG_STREAM, "  tried: %s\n", prepbuffer);
        }
        fatal_error(ERR_CANTOPEN, filename);
        ac_getline(incldepth == 0);
    }
    else {
        inclline[incldepth] = dbxlnum;
        inclname[incldepth] = curfile;
        inclfile[incldepth++] = input;  /* push current input file */
        input = fp;
        ++global_flag;
        curfile = litlate(laststr);
        --global_flag;

        padstr(__filebuf, curfile);
        strcpy(__linebuf, "1" );

        ++global_flag;
        sp = (SYM *) xalloc(SZ_SYM);
        sp->value.i = method;
        sp->name = litlate(filename);
        sp->storage_class = sc_library;
        sp->storage_type  = sc_library;
        sp->tp = NULL;
        --global_flag;

        insert(sp, &cmd_local );
        
        ac_getline(incldepth >= 1);

#ifdef NOLISTINC
        lineno = -32767;            /* don't list include files         */
                                    /* >32K lines unlikely, but a flag  */
                                    /* would be better - LDH            */
#else
        lineno = 0;                 /* list include files */
#endif

    }
}

/*
 * Read a function-macro formal into lastid.  getid() only accepts
 * identifier characters, so C99 "..." must be recognized separately.
 */
static void
get_macro_param()
{
    lastid[0] = '\0';
    while (isspace(lastch))
        getch();
    if (lastch == '.') {
        getch();
        if (lastch != '.') {
            error(ERR_DEFINE, "bad ellipsis in macro parameter list");
            return;
        }
        getch();
        if (lastch != '.') {
            error(ERR_DEFINE, "bad ellipsis in macro parameter list");
            return;
        }
        getch();
        strcpy(lastid, "...");
        return;
    }
    getid();
}

static SYM *
make_macro_formal(name, variadic)
    char           *name;
    int             variadic;
{
    SYM            *sp1;

    sp1 = (SYM *) xalloc(SZ_SYM);
    sp1->name = litlate(name);
    sp1->storage_class = sc_define;
    sp1->storage_type = sc_define;
    sp1->value.i = variadic ? 1 : 0;
    sp1->next = NULL;
    return sp1;
}

void
dodefine()
{
    SYM            *sp, *sp1;
    unsigned char  *ptr, *buffer;
    int             valid, in_quote;

    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    inpreproc = TRUE;
    oneline = TRUE;
    getsym();       /* get past #define     */
    oneline = FALSE;
    inpreproc = FALSE;

    if (lastst != id) {
        error(ERR_DEFINE, "token to define is not an identifier");
        ac_getline(incldepth == 0);
        return;
    }

    if (search(lastid, defsyms.head) != NULL) {
        /* Check if it's an identical redefinition */
        SYM *existing = search(lastid, defsyms.head);
        if (existing->tp != NULL && lastch == '(') {
            /* Function macro - allow redefinition and update the value */
            existing->value.s = NULL; /* Will be set later */
        } else if (existing->tp == NULL && lastch != '(') {
            /* Object macro - allow redefinition and update the value */
            existing->value.s = NULL; /* Will be set later */
        } else {
            /* Type mismatch - function vs object macro */
            warning(0, "macro redefinition with different type");
            existing->value.s = NULL; /* Will be set later */
        }
    }

    ++global_flag;      /* always do #define as globals */

    sp = (SYM *) xalloc(SZ_SYM);
    sp->name = litlate(lastid);
    sp->storage_class = sc_define;
    sp->storage_type = sc_define;
    sp->tp = NULL;

    if (lastch == '(' /*-)-*/ ) {   /* Function macro definition    */
        sp->tp = (TYP *) xalloc(SZ_TYP);
        sp->tp->type = bt_func;

        ptr = (unsigned char *) lastid;   /* NULL the parameter name      */
        *ptr = '\0';
        oneline = TRUE;
        getch();    /* Get past the openpa          */
        get_macro_param();
        if (*lastid) {  /* If we have a parameter       */
            /*
             * C99: #define M(...) and #define M(a, ...) both bind
             * the ellipsis formal as __VA_ARGS__.
             */
            if (strcmp(lastid, "...") == 0)
                sp1 = make_macro_formal("__VA_ARGS__", 1);
            else
                sp1 = make_macro_formal(lastid, 0);
            sp->tp->lst.head = sp->tp->lst.tail = sp1;
            while (isspace(lastch)) /* Skip the white space     */
                getch();
            while (lastch == ',' && sp1->value.i == 0) {
                getch();    /* Skip the comma           */
                get_macro_param();
                if (!*lastid)
                    break;
                if (strcmp(lastid, "...") == 0) {
                    sp1 = make_macro_formal("__VA_ARGS__", 1);
                    sp->tp->lst.tail->next = sp1;
                    sp->tp->lst.tail = sp1;
                    break;
                }
                sp1 = make_macro_formal(lastid, 0);
                sp->tp->lst.tail->next = sp1;
                sp->tp->lst.tail = sp1;
                while (isspace(lastch)) /* Skip the white space     */
                    getch();
            }
        }
        while (isspace(lastch)) /* Skip the white space         */
            getch();
        if (lastch != /*-(-*/ ')')
            error(ERR_DEFINE, "missing right parenthesis");
        oneline = FALSE;
    }
    buffer = (unsigned char *) prepbuffer;
    valid = FALSE;

    in_quote = FALSE;
    in_comment = FALSE;
    {
        int             comment_depth;

        comment_depth = 0;
        for (ptr = lptr; *ptr; ptr++) {
            /*
             * C99 // line comments: drop from // to end of logical line.
             * Block comments nest (SAS/C COMMENTNEST) via comment_depth.
             */
            if (!in_quote && comment_depth == 0
                && *ptr == '/' && *(ptr + 1) == '/')
                break;
            if (!isspace(*ptr))
                valid = TRUE;
            if (*ptr == '"')
                in_quote = !in_quote;
            if (!in_quote && *ptr == '/' && *(ptr + 1) == '*') {
                if (comment_nesting || comment_depth == 0) {
                    comment_depth++;
                    in_comment = TRUE;
                }
            }
            else if (!in_quote && comment_depth > 0
                     && *ptr == '*' && *(ptr + 1) == '/') {
                comment_depth--;
                if (comment_depth == 0)
                    in_comment = FALSE;
            }
            if (*ptr == '\\' && *(ptr + 1) == '\n') {
                ac_getline(incldepth == 0);
                ptr = lptr;
            }
            *buffer++ = *ptr;
            if (comment_depth > 0
                && (*(ptr + 1) == '\0' || *(ptr + 1) == '\n')) {
                ac_getline(incldepth == 0);
                ptr = lptr;
            }
        }
    }

    *buffer = '\0';

    in_comment = FALSE;

    if (valid)
        sp->value.s = litlate(prepbuffer);
    else
        sp->value.s = litlate(" ");

    /* Check for identical redefinition and warn if different */
    if (search(lastid, defsyms.head) != NULL) {
        SYM *existing = search(lastid, defsyms.head);
        if (existing->value.s != NULL && sp->value.s != NULL) {
            if (strcmp(existing->value.s, sp->value.s) != 0) {
                warning(0, "macro redefinition with different value");
            }
        }
    }

    insert(sp, &defsyms);
    --global_flag;
    ac_getline(incldepth == 0);
}

void
doundef()
{
    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    inpreproc = TRUE;
    getsym();       /* get past #undef */
    inpreproc = FALSE;

    if (lastst != id) {
        error(ERR_DEFINE, "missing identifier");
        ac_getline(incldepth == 0);
        return;
    }
    remove_symbol(lastid, &defsyms);
    ac_getline(incldepth == 0);
}

void
doendif()
{
    if (prepdepth <= 0)
        error(ERR_DEFINE, "endif without matching if");
    else {
        premode = inclprep[--prepdepth];
        prestat = inclstat[prepdepth];
    }
    ac_getline(incldepth == 0);
}

void
doelse()
{
    if (prestat == ps_ignore) {
        switch (premode) {
        case pr_all:
            break;
        case pr_if:
            premode = pr_else;
            prestat = ps_do;
            break;
        case pr_else:
            error(ERR_DEFINE, "else without matching if");
            break;
        }
    }
    else {          /* ps_do */
        switch (premode) {
        case pr_all:
            error(ERR_DEFINE, "else without matching if");
            break;
        case pr_if:
            premode = pr_else;
            prestat = ps_ignore;
            break;
        case pr_else:
            error(ERR_DEFINE, "else without matching if");
            break;
        }
    }
    ac_getline(incldepth == 0);
}

void
doifdef(mode)
    int             mode;
{
    SYM            *sp;

    if (prestat == ps_ignore) {
        if (prepdepth >= 32) {
            error(ERR_PREPROC, "preprocessor nesting too deep");
            ac_getline(incldepth == 0);
            return;
        }
        inclstat[prepdepth] = prestat;
        inclprep[prepdepth++] = premode;
        prestat = ps_ignore;
        premode = pr_all;
        ac_getline(incldepth == 0);
        return;
    }

    inpreproc = TRUE;
    getsym();       /* get past #ifdef */
    inpreproc = FALSE;

    if (lastst != id) {
        error(ERR_DEFINE, "identifier expected");
        ac_getline(incldepth == 0);
        return;
    }

    if (prepdepth >= 32) {
        error(ERR_PREPROC, "preprocessor nesting too deep");
        ac_getline(incldepth == 0);
        return;
    }
    inclstat[prepdepth] = prestat;
    inclprep[prepdepth++] = premode;

    sp = search(lastid, defsyms.head);

    premode = pr_if;
    prestat = ps_ignore;
    if (mode) {     /* ifdef            */
        if (sp != NULL) /* compile section  */
            prestat = ps_do;
    }
    else {          /* ifndef        */
        if (sp == NULL) /* compile section  */
            prestat = ps_do;
    }
    ac_getline(incldepth == 0);
}

void
doif()
{
    int             value;

    if (prestat == ps_ignore) {
        if (prepdepth >= 32) {
            error(ERR_PREPROC, "preprocessor nesting too deep");
            ac_getline(incldepth == 0);
            return;
        }
        inclstat[prepdepth] = prestat;
        inclprep[prepdepth++] = premode;
        prestat = ps_ignore;
        premode = pr_all;
        ac_getline(incldepth == 0);
        return;
    }

    if (prepdepth >= 32) {
        error(ERR_PREPROC, "preprocessor nesting too deep");
        ac_getline(incldepth == 0);
        return;
    }
    inclstat[prepdepth] = prestat;
    inclprep[prepdepth++] = premode;

    oneline = TRUE;     /* The expresion must be on one line */

    getsym();       /* get past #if */

    value = intexpr();  /* get the expression */

    oneline = FALSE;

    premode = pr_if;
    prestat = ps_ignore;

    if (value != 0)     /* compile section  */
        prestat = ps_do;

    ac_getline(incldepth == 0);
}

char    *
getparm(buffer)
    char    *buffer;
{
    char    *ptr;
    int     paren, state;

    /*
     * Leading/trailing whitespace is not part of a macro argument
     * (C89).  Without this, CAT(a, b) keeps the space after the comma
     * and a##b pastes into "a b" instead of "ab".
     */
    while (isspace(lastch))
        getch();

    ptr = buffer;
    state = paren = 0;

    for (;;) {
        switch (state) {
        case 0:
            if (lastch == EOF)
                goto done;
            else if (lastch == QUOT)
                state = 1;
            else if (lastch == SQUOT)
                state = 2;
            else if (lastch == LPAR)
                ++paren;
            else if (lastch == RPAR) {
                if (paren <= 0)
                    goto done;
                --paren;
            }
            else if (lastch == COMMA) {
                if (paren <= 0) {
                    getch();
                    goto done;
                }
            }
            else if (lastch == BSLASH) {
                *ptr++ = lastch;
                getch();
            }
            break;
        case 1:
            if (lastch == EOF)
                goto done;
            else if (lastch == QUOT)
                state = 0;
            else if (lastch == BSLASH) {
                *ptr++ = lastch;
                getch();
            }
            break;
        case 2:
            if (lastch == EOF)
                goto done;
            else if (lastch == SQUOT)
                state = 0;
            else if (lastch == BSLASH) {
                *ptr++ = lastch;
                getch();
            }
            break;
        }
        if (lastch != EOF) {
            *ptr++ = lastch;
            getch();
        }
    }
done:
    *ptr = '\0';
    while (ptr > buffer && isspace((unsigned char) ptr[-1])) {
        --ptr;
        *ptr = '\0';
    }
    return(litlate(buffer));
}

char    *
getvarargs(buffer)
    char    *buffer;
{
    char    *ptr;
    int     paren, state;

    /* Match getparm: leading spaces are not part of the argument. */
    while (isspace(lastch))
        getch();

    ptr = buffer;
    state = paren = 0;

    /* Collect all remaining arguments until closing parenthesis */
    for (;;) {
        switch (state) {
        case 0:
            if (lastch == EOF)
                goto done;
            else if (lastch == QUOT)
                state = 1;
            else if (lastch == SQUOT)
                state = 2;
            else if (lastch == LPAR)
                ++paren;
            else if (lastch == RPAR) {
                if (paren <= 0)
                    goto done;
                --paren;
            }
            else if (lastch == BSLASH) {
                *ptr++ = lastch;
                getch();
            }
            break;
        case 1:
            if (lastch == EOF)
                goto done;
            else if (lastch == QUOT)
                state = 0;
            else if (lastch == BSLASH) {
                *ptr++ = lastch;
                getch();
            }
            break;
        case 2:
            if (lastch == EOF)
                goto done;
            else if (lastch == SQUOT)
                state = 0;
            else if (lastch == BSLASH) {
                *ptr++ = lastch;
                getch();
            }
            break;
        }
        if (lastch != EOF) {
            *ptr++ = lastch;
            getch();
        }
    }
done:
    *ptr = '\0';
    return(litlate(buffer));
}

char           *
stringify_param(param)
    char           *param;
{
    char           *result, *ptr;
    int             len;
    
    /* Calculate length needed for stringified parameter */
    len = strlen(param) + 2; /* +2 for quotes */
    result = (char *)xalloc(len + 1);
    ptr = result;
    *ptr++ = '"';
    
    while (*param) {
        if (*param == '"' || *param == '\\') {
            *ptr++ = '\\';
        }
        *ptr++ = *param++;
    }
    *ptr++ = '"';
    *ptr = '\0';
    
    return result;
}

char           *
paste_tokens(token1, token2)
    char           *token1, *token2;
{
    char           *result;
    int             len;
    
    len = strlen(token1) + strlen(token2) + 1;
    result = (char *)xalloc(len);
    strcpy(result, token1);
    strcat(result, token2);
    
    return result;
}

static void
reverse_string(char *str)
{
    char *start, *end, temp;
    
    if (str == NULL || *str == '\0')
        return;
        
    start = str;
    end = str + strlen(str) - 1;
    
    while (start < end) {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}

char           *
reverse_args(args)
    char           *args;
{
    char           *reversed;
    int             len;
    int             nregs;
    int             i;

    /*
     * SAS/C mask form: <arg-regs in reverse order><return-reg><arg-count>.
     * gen_libcall2 wants arg regs in stack/parameter order.  Only reverse
     * the register digits; leave return + count nibbles alone.
     * Example: Write "32103" -> "12303".
     */
    if (args == NULL)
        return NULL;

    len = strlen(args);
    reversed = (char *) xalloc(len + 1);
    if (len < 2) {
        strcpy(reversed, args);
        return reversed;
    }

    nregs = len - 2;
    for (i = 0; i < nregs; i++)
        reversed[i] = args[nregs - 1 - i];
    reversed[nregs] = args[nregs];
    reversed[nregs + 1] = args[nregs + 1];
    reversed[len] = '\0';
    return reversed;
}

char           *
prepdefine(sp)
    SYM            *sp;
{
    SYM            *sp1;
    char           *buffer, *pattern, *ptr, *loc;

    if (sp == NULL)
        return (NULL);

    if (sp->tp == NULL)
        return (sp->value.s);

    buffer = prepbuffer;

    sp1 = sp->tp->lst.head;

    /*
     * sp->value.s:                 The macro text 
     * sp->tp->lst.head:            The list of parameters 
     * sp->tp->lst.head->value.s:   The bound parameters
     */

    while (isspace(lastch)) /* Skip the white space */
        getch();

    if (lastch != LPAR) /* Return the parameter name if not a func */
        return (sp->name);

    getch();

    while (sp1 != NULL) {
        if (sp1->value.i == 1) {
            /* Handle __VA_ARGS__ - collect all remaining arguments */
            sp1->value.s = getvarargs(buffer);
        } else {
            sp1->value.s = getparm(buffer);
        }
        sp1 = sp1->next;
    }

    while (isspace(lastch)) /* Skip the white space */
        getch();

    if (lastch != RPAR) {   /* Return NULL  */
        error(ERR_DEFINE, "missing right parenthesis");
        return (NULL);
    }

    getch();        /* Skip the closepa */

    /* Now that we know what the parameters are just subst them */

    pattern = sp->value.s;
    ptr = buffer;
    *ptr++ = '$';

    while (*pattern) {
        if (isspace(*pattern)) {
            *ptr++ = ' ';
            while (isspace(*pattern))
                ++pattern;
        }
        if (isdigit(*pattern)) {
            for (;;) {
                if (!*pattern)
                    break;
                if (isdigit(*pattern))
                    *ptr++ = *pattern++;
                else if (*pattern == 'E' || *pattern == 'e')
                    *ptr++ = *pattern++;
                else if (*pattern == '-' || *pattern == '+')
                    *ptr++ = *pattern++;
                else
                    break;
            }
        }
        else if (*pattern == '#' && *(pattern + 1) == '#') {
            /*
             * Token pasting: left##right.  Must run before single-# stringize.
             * Whitespace around ## is ignored.  Left token was already written
             * into buffer; rewind it, expand the right token, and emit the
             * concatenation.  Do not treat the leading '$' sentinel as part of
             * the left token ($ is isidch for Amiga asm identifiers).
             */
            char            token1[256];
            char            token2[256];
            char           *start;
            char           *t;
            int             ti;
            int             found;

            pattern += 2;
            while (isspace(*pattern))
                ++pattern;

            while (ptr > buffer + 1 && isspace((unsigned char) ptr[-1]))
                --ptr;

            start = ptr;
            if (start > buffer + 1) {
                --start;
                while (start > buffer + 1 &&
                       (isidch(*start) || isdigit((unsigned char) *start)))
                    --start;
                if (!(isidch(*start) || isdigit((unsigned char) *start)))
                    ++start;
            }
            ti = 0;
            for (t = start; t < ptr && ti < 255; ++t)
                token1[ti++] = *t;
            token1[ti] = '\0';
            ptr = start;

            ti = 0;
            token2[0] = '\0';
            if (isidch(*pattern)) {
                while (isidch(*pattern) && ti < 255)
                    token2[ti++] = *pattern++;
                token2[ti] = '\0';
                found = 0;
                for (sp1 = sp->tp->lst.head; sp1 != NULL; sp1 = sp1->next) {
                    if (strcmp(token2, sp1->name) == 0) {
                        if (sp1->value.s != NULL) {
                            t = sp1->value.s;
                            ti = 0;
                            while (*t != '\0' && ti < 255) {
                                token2[ti] = *t;
                                ti++;
                                t++;
                            }
                            token2[ti] = '\0';
                        }
                        found = 1;
                        break;
                    }
                }
                (void) found;
            } else if (isdigit((unsigned char) *pattern)) {
                while (*pattern &&
                       (isdigit((unsigned char) *pattern) ||
                        *pattern == 'E' || *pattern == 'e' ||
                        *pattern == '+' || *pattern == '-') &&
                       ti < 255)
                    token2[ti++] = *pattern++;
                token2[ti] = '\0';
            }

            loc = paste_tokens(token1, token2);
            while (*loc)
                *ptr++ = *loc++;
        }
        else if (*pattern == '#') {
            /* Stringification operator (#param) — not ## */
            pattern++; /* Skip the # */
            if (isidch(*pattern)) {
                loc = laststr;
                while (isidch(*pattern))
                    *loc++ = *pattern++;
                *loc = '\0';
                loc = laststr;
                for (sp1 = sp->tp->lst.head; sp1 != NULL; sp1 = sp1->next) {
                    if (strcmp(loc, sp1->name) == 0) {
                        loc = stringify_param(sp1->value.s);
                        break;
                    }
                }
                while (*loc)
                    *ptr++ = *loc++;
            }
        }
        else if (isidch(*pattern)) {
            loc = laststr;
            while (isidch(*pattern))
                *loc++ = *pattern++;
            *loc = '\0';
            loc = laststr;
            for (sp1 = sp->tp->lst.head; sp1 != NULL; sp1 = sp1->next) {
                if (strcmp(loc, sp1->name) == 0) {
                    loc = sp1->value.s;
                    break;
                }
            }
            while (*loc)
                *ptr++ = *loc++;
        }
        else if (*pattern == '"') {
            *ptr++ = *pattern++;
            while (*pattern && *pattern != '"') {
                if (*pattern == '\\')
                    *ptr++ = *pattern++;
                if (*pattern)
                    *ptr++ = *pattern++;
            }
            if (*pattern)
                *ptr++ = *pattern++;
        }
        else if (*pattern == '\'') {
            *ptr++ = *pattern++;
            while (*pattern && *pattern != '\'') {
                if (*pattern == '\\')
                    *ptr++ = *pattern++;
                if (*pattern)
                    *ptr++ = *pattern++;
            }
            if (*pattern)
                *ptr++ = *pattern++;
        }
        else
            *ptr++ = *pattern++;
    }
    *ptr = '\0';
    return (litlate(buffer + 1));
}

void
doasm()
{
    if (prestat == ps_ignore) {
        if (prepdepth >= 32) {
            error(ERR_PREPROC, "preprocessor nesting too deep");
            ac_getline(incldepth == 0);
            return;
        }
        inclstat[prepdepth] = prestat;
        inclprep[prepdepth++] = premode;
        prestat = ps_ignore;
        premode = pr_all;
        ac_getline(incldepth == 0);
        return;
    }

    inclstat[prepdepth] = prestat;
    inclprep[prepdepth++] = premode;

    premode = pr_asm;
    prestat = ps_do;

    ac_getline(incldepth == 0);
}

void
doendasm()
{
    if (prepdepth <= 0)
        error(ERR_DEFINE, "endasm without matching asm");
    else {
        premode = inclprep[--prepdepth];
        prestat = inclstat[prepdepth];
    }

    ac_getline(incldepth == 0);
}

void
doline()
{
    struct snode   *snp;

    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    oneline = TRUE;     /* must be on one line   */
    getsym();
    oneline = FALSE;
    if (lastst != iconst) {
        error(ERR_PREPROC, "line number missing");
        ac_getline(incldepth == 0);
        return;
    }
    else {
        dbxlnum = ival - 1; /* set the line number  */
        oneline = TRUE; /* must be on one line   */
        getsym();
        strcpy( __linebuf, itoa(dbxlnum));
        oneline = FALSE;
        if (lastst == eof) {
            ac_getline(incldepth == 0);
            return;
        }
        else if (lastst == sconst) {
            ++global_flag;
            curfile = litlate(laststr);
            padstr(__filebuf, curfile);
            snp = filestmt();
            if (snp != NULL)
                addauto(snp);
            --global_flag;
            ac_getline(incldepth == 0);
            return;
        }
        error(ERR_PREPROC, "invalid token");
        ac_getline(incldepth == 0);
    }
}

void
doelif()
{
    int             value;

    /*
     * #elif must not push a nesting level (old code did, and leaked
     * depth until "preprocessor nesting too deep" on cclib headers).
     *
     * Skip/rest state uses pr_all at this depth (so later #elifs stay
     * skipped).  Evaluating a candidate branch must set ps_do first —
     * otherwise the lexer ignores the expression and we hang.
     */
    if (prestat == ps_ignore) {
        switch (premode) {
        case pr_all:
            /* Nested inside a skipped region — skip this line. */
            ac_getline(incldepth == 0);
            return;
        case pr_if:
            /* Prior branch false — will evaluate below. */
            premode = pr_else;
            prestat = ps_do;
            break;
        case pr_else:
            error(ERR_DEFINE, "elif after else");
            ac_getline(incldepth == 0);
            return;
        }
    }
    else {          /* ps_do */
        switch (premode) {
        case pr_all:
            error(ERR_DEFINE, "elif without matching if");
            ac_getline(incldepth == 0);
            return;
        case pr_if:
            /* Prior #if/#elif true — skip remainder of the chain. */
            premode = pr_all;
            prestat = ps_ignore;
            ac_getline(incldepth == 0);
            return;
        case pr_else:
            error(ERR_DEFINE, "elif after else");
            ac_getline(incldepth == 0);
            return;
        }
    }

    oneline = TRUE;
    getsym();       /* get past #elif */
    value = intexpr();
    oneline = FALSE;

    premode = pr_if;
    prestat = ps_ignore;
    if (value != 0)
        prestat = ps_do;

    ac_getline(incldepth == 0);
}

void
doerror()
{
    char buffer[1024];
    char *cp, *endp;
    int fmt;

    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    oneline = TRUE;
    cp = &buffer[0];
    endp = &buffer[1022];
    while (lastch != EOF && lastch != '\n') {
        if (cp < endp)
            *cp++ = lastch;
        getch();
    }
    *cp++ = '\0';
    
    /* Use selected error format; cc front-end forces gcc shape. */
    fmt = Options.OutputFormat;
    if (frontend_mode == FE_CC)
        fmt = 0;
    switch (fmt) {
    case 0: /* GCC format */
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, "%s:%d:%d: error: %s\n", curfile, lineno, current_column, buffer);
        } else {
            fprintf(AC_DIAG_STREAM, "%s:%d: error: %s\n", curfile, lineno, buffer);
        }
        break;
    case 1: /* SASC format */
        fprintf(AC_DIAG_STREAM, "%s(%d", curfile, lineno);
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, ",%d", current_column);
        }
        fprintf(AC_DIAG_STREAM, ") : error: %s\n", buffer);
        break;
    case 2: /* PDC format */
    default:
        fprintf(AC_DIAG_STREAM, "Error in %s:%d: %s\n", curfile, lineno, buffer);
        break;
    }
    oneline = FALSE;

    ac_getline(incldepth == 0);
}

void
dowarning()
{
    char buffer[1024];
    char *cp, *endp;
    int fmt;

    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    oneline = TRUE;
    cp = &buffer[0];
    endp = &buffer[1022];
    while (lastch != EOF && lastch != '\n') {
        if (cp < endp)
            *cp++ = lastch;
        getch();
    }
    *cp++ = '\0';
    
    /* Use selected warning format; cc front-end forces gcc shape. */
    fmt = Options.OutputFormat;
    if (frontend_mode == FE_CC)
        fmt = 0;
    switch (fmt) {
    case 0: /* GCC format */
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, "%s:%d:%d: warning: %s\n", curfile, lineno, current_column, buffer);
        } else {
            fprintf(AC_DIAG_STREAM, "%s:%d: warning: %s\n", curfile, lineno, buffer);
        }
        break;
    case 1: /* SASC format */
        fprintf(AC_DIAG_STREAM, "%s(%d", curfile, lineno);
        if (Options.ShowColumn) {
            fprintf(AC_DIAG_STREAM, ",%d", current_column);
        }
        fprintf(AC_DIAG_STREAM, ") : warning: %s\n", buffer);
        break;
    case 2: /* PDC format */
    default:
        fprintf(AC_DIAG_STREAM, "Warning in %s:%d: %s\n", curfile, lineno, buffer);
        break;
    }
    oneline = FALSE;

    ac_getline(incldepth == 0);
}

void
dopragma_once()
{
    static char **included_files = NULL;
    static int num_included = 0;
    int i;
    
    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    /* Check if current file already included */
    for (i = 0; i < num_included; i++) {
        if (strcmp(included_files[i], curfile) == 0) {
            /* File already included, skip to end of file */
            while (ac_getline(incldepth == 0) == 0) {
                /* Skip lines until end of file */
            }
            return;
        }
    }
    
    /* Add current file to included list */
    if (included_files == NULL) {
        included_files = (char **)xalloc(10 * sizeof(char *));
    } else if (num_included != 0 && safe_lmod(num_included, 10) == 0) {
        included_files = (char **)xalloc((num_included + 10) * sizeof(char *));
    }
    
    included_files[num_included] = (char *)xalloc(strlen(curfile) + 1);
    strcpy(included_files[num_included], curfile);
    num_included++;
    
    ac_getline(incldepth == 0);
}

void
gettoken()
{
    char           *ptr;
    char           *end;

    ptr = laststr;
    end = laststr + (MAX_IDP1 - 1);

    while (lastch != EOF && isspace(lastch))
        getch();

    while (lastch != EOF && !isspace(lastch)) {
        if (ptr >= end)
            break;
        *ptr++ = lastch;
        getch();
    }

    *ptr = '\0';
}

static void
buffer_token()
{
    if (!token_buffered) {
        strcpy(token_buffer, laststr);
        token_buffered = 1;
    }
}

static void
unbuffer_token()
{
    if (token_buffered) {
        strcpy(laststr, token_buffer);
        token_buffered = 0;
    }
}

/*
 * Self-host collapses auto offsets so multiple locals alias one register
 * (bootstrap/self/PreProc.s pushed D3 five times into libentry_store).
 * Keep the in-progress pragma record in a global instead.
 */
static char    *pragma_entry;

/*
 * Fill a 5-pointer libcall-shaped record by byte offset.
 * Layout: next, basename, funcname, args, offset (see Gen.h LIBCALL_*).
 */
static void
libentry_set_field(which, value)
    int             which;
    char           *value;
{
    if (pragma_entry == NULL)
        return;
    if (which == 0)
        LIBCALL_NEXT(pragma_entry) = value;
    else if (which == 1)
        LIBCALL_BASE(pragma_entry) = value;
    else if (which == 2)
        LIBCALL_FUNC(pragma_entry) = value;
    else if (which == 3)
        LIBCALL_ARGS(pragma_entry) = value;
    else if (which == 4)
        LIBCALL_OFF(pragma_entry) = value;
}

static void
dopragma_liblike(prefix, listhead)
    char           *prefix;
    void          **listhead;
{
    ++global_flag;
    pragma_entry = (char *) xalloc(SZ_LIBCALL);
    libentry_set_field(0, (char *) (*listhead));

    gettoken();
    libentry_set_field(1, litlate(laststr));
    gettoken();
    libentry_set_field(2, litlate(laststr));
    gettoken();
    libentry_set_field(4, litlate(laststr));
    gettoken();
    libentry_set_field(3, reverse_args(litlate(laststr)));

    *listhead = (void *) pragma_entry;

    strcpy(laststr, prefix);
    strcat(laststr, LIBCALL_FUNC(pragma_entry));
    setdefine(LIBCALL_FUNC(pragma_entry), litlate(laststr));
    pragma_entry = NULL;
    --global_flag;
}

void
dopragma()
{
    struct msgcall *msg;
    char           *state_str;

    if (prestat == ps_ignore) {
        ac_getline(incldepth == 0);
        return;
    }

    oneline = TRUE;

    gettoken();
    if (strcmp(laststr, "libcall") == 0)
        dopragma_liblike("__LIBCALL_", (void **) &libpragma);
    else if (strcmp(laststr, "flibcall") == 0)
        dopragma_liblike("__FLIBCALL_", (void **) &flibpragma);
    else if (strcmp(laststr, "syscall") == 0)
        dopragma_liblike("__SYSCALL_", (void **) &syspragma);
    else if (strcmp(laststr, "tagcall") == 0)
        dopragma_liblike("__TAGCALL_", (void **) &tagpragma);
    else if (strcmp(laststr, "msg") == 0) {
        ++global_flag;
        msg = (struct msgcall *) xalloc(16);
        
        /* Get message number */
        gettoken();
        msg->msg_num = atoi(laststr);
        
        /* Get state (error/warn/ignore/pop) */
        gettoken();
        state_str = laststr;
        if (strcmp(state_str, "error") == 0 || strcmp(state_str, "err") == 0) {
            msg->msg_state = 0; /* error */
        } else if (strcmp(state_str, "warn") == 0 || strcmp(state_str, "wrn") == 0) {
            msg->msg_state = 1; /* warning */
        } else if (strcmp(state_str, "ignore") == 0 || strcmp(state_str, "ign") == 0) {
            msg->msg_state = 2; /* ignore */
        } else if (strcmp(state_str, "pop") == 0) {
            msg->msg_state = 3; /* pop - restore previous state */
            msg->push_flag = 0;
            msg->next = msgpragma;
            msgpragma = msg;
            --global_flag;
            ac_getline(incldepth == 0);
            return;
        } else {
            error(ERR_PREPROC, "invalid msg state");
            --global_flag;
            ac_getline(incldepth == 0);
            return;
        }
        
        /* Check for push keyword */
        msg->push_flag = 0;
        buffer_token(); /* Save current token */
        gettoken();
        if (strcmp(laststr, "push") == 0) {
            msg->push_flag = 1;
        } else {
            /* Restore the token if it's not push */
            unbuffer_token();
        }

        msg->next = msgpragma;
        msgpragma = msg;
        --global_flag;
    }
    else if (strcmp( laststr, "intmath" ) == 0) {
        gettoken();
        if ( strcmp( laststr, "library" ) == 0)
            Options.MulDiv32 = TRUE;
        else if (strcmp( laststr, "inline" ) == 0)
            Options.MulDiv32 = FALSE;
    }
    else if (strcmp( laststr, "fpmath" ) == 0) {
        gettoken();
        if ( strcmp( laststr, "ieee" ) == 0)
            ;   /*  IEEE format */
        else if ( strcmp( laststr, "fpp" ) == 0)
            ;   /*  FPP format */
    }
    else if (strcmp( laststr, "once" ) == 0) {
        dopragma_once();
        return;
    }

    oneline = FALSE;

    ac_getline(incldepth == 0);
}

int
dodefined()
{
    int             seen;

    /*
     * Check to see if the next token has been defined, return TRUE, or
     * FALSE, the form may be defined(X) or defined X
     */

    while (isspace(lastch)) /* Skip the white space         */
        getch();

    if (seen = (lastch == LPAR)) {  /* SKip the leading paren       */
        getch();
        while (isspace(lastch)) /* Skip the white space         */
            getch();
    }

    while (isspace(lastch)) /* Skip the white space         */
        getch();

    getid();        /* Get the identifier           */

    while (isspace(lastch)) /* Skip the white space         */
        getch();

    if (seen) {     /* Skip the trailing paren      */
        if (lastch == RPAR)
            getch();
        else
            error(ERR_SYNTAX, NULL);
    }

    return (search(lastid, defsyms.head) != NULL);
}

/*
 * Process defines of the type __FILE__, __LINE__, etc... These assume a
 * static buffer to hold the values
 */

void
setdefine(name, value)
    char           *name, *value;
{
    SYM            *sp;

    sp = search(name, defsyms.head);

    if (sp != NULL)
        sp->value.s = value;    /* Update the value      */
    else {
        ++global_flag;  /* always do #define as globals */
        sp = (SYM *) xalloc(SZ_SYM);
        sp->name = name;
        sp->value.s = value;
        --global_flag;
        insert(sp, &defsyms);
    }
}

void 
preprocess()
{
    struct snode   *snp;

    ++lptr;
    lastch = ' ';

    oneline = TRUE;
    getsym();       /* get first word on line   */
    oneline = FALSE;

    if (lastst == kw_if)      /* #if (expr)               */
        doif();
    else if (lastst == kw_else)  /* #else                    */
        doelse();
    else if (lastst != id && lastst != iconst) {
        error(ERR_PREPROC, "unknown type of command");
        ac_getline(incldepth == 0);
    }
    else if (lastst == iconst) { /* Ignore line labels   */
        dbxlnum = ival - 1; /* set the line number   */
        oneline = TRUE; /* must be on one line   */
        getsym();
        oneline = FALSE;
        strcpy( __linebuf, itoa(dbxlnum));
        if (lastst == sconst) {
            ++global_flag;
            curfile = litlate(laststr);
            padstr(__filebuf, curfile);
            snp = filestmt();
            if (snp != NULL)
                addauto(snp);
            --global_flag;
        }
        else if (lastst != eof) 
            error(ERR_PREPROC, NULL);
        ac_getline(incldepth == 0);
    }
    else {
        if (strcmp(lastid, "include") == 0) 
            doinclude();
        else if (strcmp(lastid, "define") == 0) 
            dodefine();
        else if (strcmp(lastid, "ifdef") == 0) 
            doifdef(TRUE);
        else if (strcmp(lastid, "ifndef") == 0) 
            doifdef(FALSE);
        else if (strcmp(lastid, "endif") == 0) 
            doendif();
        else if (strcmp(lastid, "elif") == 0) 
            doelif();
        else if (strcmp(lastid, "undef") == 0) 
            doundef();
        else if (strcmp(lastid, "asm") == 0) 
            doasm();
        else if (strcmp(lastid, "endasm") == 0) 
            doendasm();
        else if (strcmp(lastid, "line") == 0) 
            doline();
        else if (strcmp(lastid, "error") == 0) 
            doerror();
        else if (strcmp(lastid, "warning") == 0) 
            dowarning();
        else if (strcmp(lastid, "pragma") == 0) 
            dopragma();
        else {
            error(ERR_PREPROC, "unknown command");
            ac_getline(incldepth == 0);
        }
    }
}
