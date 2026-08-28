
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
 * PreProc.c - built-in preprocessor (token-based Phase-3 core).
 *
 * Directives and macros are handled here.  Expansion uses heap PPT tokens
 * (PpToken.c); GetSym still receives text via prepdefine() pushback.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>

#ifdef AZTEC_C
#define isidch(x)    ((ctp_[(x)+1] & 0x07) || ((x) == '_') || ((x) == '$'))
#endif

#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    "host_posix.h"
#include    "FrontEnd.h"
#include    "PpToken.h"

extern char    *itoa(int x);
extern char     in_line[];

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
 * local include/ -- not from NDK include:, which has SAS-isms AC rejects
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
        /* NDK last -- dos/, exec/, proto/, ... */
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

    while (pp_is_white(lastch)) /* Skip the white space */
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
             * SAS/C F.3.13 quoted #include "file" search order:
             *   1. current directory (literal / cwd path)
             *   2. directory of the file containing the #include
             *   3. -I / includedirectory list
             *   4. INCLUDE: (or host INCLUDE macro)
             * Angle brackets skip steps 1-2 (handled below).
             */
            fp = fopen(laststr, "r");
            if (fp == NULL
                && include_beside_caller(prepbuffer, laststr)
                && (fp = fopen(prepbuffer, "r")) != NULL)
                strcpy(laststr, prepbuffer);
            if (fp == NULL) {
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
        fprintf(AC_DIAG_STREAM, "  tried: %s\n", filename);
        if (method == FALSE && include_beside_caller(prepbuffer, filename))
            fprintf(AC_DIAG_STREAM, "  tried: %s\n", prepbuffer);
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
    while (pp_is_white(lastch))
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
            while (pp_is_white(lastch)) /* Skip the white space     */
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
                while (pp_is_white(lastch)) /* Skip the white space     */
                    getch();
            }
        }
        while (pp_is_white(lastch)) /* Skip the white space         */
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
            if (!pp_is_white((unsigned char) *ptr))
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

/*
 * Token-based #if / #elif evaluation (heap tokens via PpToken.c).
 * No ctype/_type; always parse both sides of && / ||.
 */

static PPTOKEN *pp_ev_cur;
static int      pp_ev_depth;

static long     pp_eval_lor();  /* forward */

static void
pp_ev_advance()
{
    if (pp_ev_cur != NULL)
        pp_ev_cur = pp_ev_cur->next;
}

static int
pp_ev_is_punct(code)
    int             code;
{
    return pp_ev_cur != NULL
        && pp_ev_cur->kind == PPT_PUNCT
        && pp_ev_cur->punct == code;
}

static long
pp_parse_number(text)
    char           *text;
{
    long            v;
    int             base;
    int             d;
    unsigned char  *p;

    v = 0;
    base = 10;
    p = (unsigned char *) text;
    if (p == NULL || *p == '\0')
        return 0;
    if (*p == '0') {
        p++;
        if (*p == 'x' || *p == 'X') {
            p++;
            base = 16;
        }
        else if (*p >= '0' && *p <= '7')
            base = 8;
        else {
            /* lone 0 with optional suffixes */
            while (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L')
                p++;
            return 0;
        }
    }
    for (;;) {
        if (*p >= '0' && *p <= '9')
            d = *p - '0';
        else if (*p >= 'a' && *p <= 'f')
            d = *p - 'a' + 10;
        else if (*p >= 'A' && *p <= 'F')
            d = *p - 'A' + 10;
        else
            break;
        if (d >= base)
            break;
        v = v * base + d;
        p++;
    }
    while (*p == 'u' || *p == 'U' || *p == 'l' || *p == 'L')
        p++;
    return v;
}

/*
 * Replace defined IDENT / defined ( IDENT ) with a PPT_NUMBER 0/1.
 * Operates on a token list; returns a new list (arena).
 */
static PPTOKEN *
pp_rewrite_defined(head)
    PPTOKEN        *head;
{
    PPTOKEN        *out;
    PPTOKEN        *tail;
    PPTOKEN        *t;
    PPTOKEN        *name_tok;
    int             seen;
    int             isdef;
    char           *digit;

    out = NULL;
    tail = NULL;
    t = head;
    while (t != NULL) {
        if (t->kind == PPT_IDENT && pp_is_defined_kw(t->text)) {
            t = t->next;
            seen = 0;
            name_tok = NULL;
            if (t != NULL && t->kind == PPT_PUNCT && t->punct == '(') {
                seen = 1;
                t = t->next;
            }
            if (t != NULL && t->kind == PPT_IDENT) {
                name_tok = t;
                t = t->next;
            }
            if (seen) {
                if (t != NULL && t->kind == PPT_PUNCT && t->punct == ')')
                    t = t->next;
                else
                    error(ERR_PUNCT, "expected ) after defined(");
            }
            isdef = (name_tok != NULL && name_tok->text != NULL
                     && search(name_tok->text, defsyms.head) != NULL);
            digit = isdef ? litlate("1") : litlate("0");
            pp_list_append(&out, &tail,
                           pp_tok_new(PPT_NUMBER, 0, digit));
            continue;
        }
        pp_list_append(&out, &tail, pp_tok_dup(t));
        t = t->next;
    }
    return out;
}

/*
 * Expand object-like macros in a #if token list (tp == NULL).
 * Function-like macros are left as identifiers (become 0 in primary).
 */
static PPTOKEN *
pp_expand_object_macros(head, depth)
    PPTOKEN        *head;
    int             depth;
{
    PPTOKEN        *out;
    PPTOKEN        *tail;
    PPTOKEN        *t;
    PPTOKEN        *exp;
    SYM            *sp;

    out = NULL;
    tail = NULL;
    if (depth > 32)
        return pp_list_dup(head);

    for (t = head; t != NULL; t = t->next) {
        if (t->kind == PPT_IDENT && t->text != NULL
            && !pp_is_defined_kw(t->text)) {
            sp = search(t->text, defsyms.head);
            if (sp != NULL && sp->value.s != NULL && sp->tp == NULL) {
                exp = pp_tokenize(sp->value.s);
                exp = pp_expand_object_macros(exp, depth + 1);
                while (exp != NULL) {
                    pp_list_append(&out, &tail, pp_tok_dup(exp));
                    exp = exp->next;
                }
                continue;
            }
        }
        pp_list_append(&out, &tail, pp_tok_dup(t));
    }
    return out;
}

static long
pp_ev_primary()
{
    long            v;
    SYM            *sp;
    PPTOKEN        *saved;
    PPTOKEN        *exp;

    if (pp_ev_cur == NULL) {
        error(ERR_SYNTAX, "bad token in #if");
        return 0L;
    }

    if (pp_ev_is_punct('(')) {
        pp_ev_advance();
        v = pp_eval_lor();
        if (pp_ev_is_punct(')'))
            pp_ev_advance();
        else
            error(ERR_PUNCT, "expected ) in #if");
        return v;
    }

    if (pp_ev_cur->kind == PPT_NUMBER) {
        v = pp_parse_number(pp_ev_cur->text);
        pp_ev_advance();
        return v;
    }

    if (pp_ev_cur->kind == PPT_CHAR) {
        /* Narrow: use first character after opening quote if present. */
        {
            char           *s;

            s = pp_ev_cur->text;
            v = 0;
            if (s != NULL && s[0] == '\'' && s[1] != '\0') {
                if (s[1] == '\\' && s[2] != '\0')
                    v = (unsigned char) s[2];
                else
                    v = (unsigned char) s[1];
            }
        }
        pp_ev_advance();
        return v;
    }

    if (pp_ev_cur->kind == PPT_IDENT) {
        /*
         * defined should already be rewritten; leftover treated as 0.
         * Object macros should already be expanded; leftover id => 0.
         * Still try one-shot expansion for safety.
         */
        sp = search(pp_ev_cur->text, defsyms.head);
        if (sp != NULL && sp->value.s != NULL && sp->tp == NULL) {
            if (pp_ev_depth > 32) {
                pp_ev_advance();
                return 0L;
            }
            saved = pp_ev_cur->next;
            exp = pp_tokenize(sp->value.s);
            exp = pp_expand_object_macros(exp, 0);
            pp_ev_cur = exp;
            ++pp_ev_depth;
            v = pp_eval_lor();
            --pp_ev_depth;
            pp_ev_cur = saved;
            return v;
        }
        pp_ev_advance();
        return 0L;
    }

    error(ERR_SYNTAX, "bad token in #if");
    pp_ev_advance();
    return 0L;
}

static long
pp_ev_unary()
{
    long            v;

    if (pp_ev_is_punct('!')) {
        pp_ev_advance();
        return !pp_ev_unary();
    }
    if (pp_ev_is_punct('~')) {
        pp_ev_advance();
        return ~pp_ev_unary();
    }
    if (pp_ev_is_punct('-')) {
        pp_ev_advance();
        return -pp_ev_unary();
    }
    if (pp_ev_is_punct('+')) {
        pp_ev_advance();
        return pp_ev_unary();
    }
    return pp_ev_primary();
}

static long
pp_ev_mul()
{
    long            v;
    long            r;

    v = pp_ev_unary();
    for (;;) {
        if (pp_ev_is_punct('*')) {
            pp_ev_advance();
            v = v * pp_ev_unary();
        }
        else if (pp_ev_is_punct('/')) {
            pp_ev_advance();
            r = pp_ev_unary();
            v = (r == 0) ? 0 : v / r;
        }
        else if (pp_ev_is_punct('%')) {
            pp_ev_advance();
            r = pp_ev_unary();
            v = (r == 0) ? 0 : v % r;
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_add()
{
    long            v;

    v = pp_ev_mul();
    for (;;) {
        if (pp_ev_is_punct('+')) {
            pp_ev_advance();
            v = v + pp_ev_mul();
        }
        else if (pp_ev_is_punct('-')) {
            pp_ev_advance();
            v = v - pp_ev_mul();
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_shift()
{
    long            v;

    v = pp_ev_add();
    for (;;) {
        if (pp_ev_is_punct(PP_SHL)) {
            pp_ev_advance();
            v = v << (int) pp_ev_add();
        }
        else if (pp_ev_is_punct(PP_SHR)) {
            pp_ev_advance();
            v = (long) ((unsigned long) v >> (int) pp_ev_add());
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_rel()
{
    long            v;

    v = pp_ev_shift();
    for (;;) {
        if (pp_ev_is_punct(PP_LE)) {
            pp_ev_advance();
            v = (v <= pp_ev_shift()) ? 1L : 0L;
        }
        else if (pp_ev_is_punct(PP_GE)) {
            pp_ev_advance();
            v = (v >= pp_ev_shift()) ? 1L : 0L;
        }
        else if (pp_ev_is_punct('<')) {
            pp_ev_advance();
            v = (v < pp_ev_shift()) ? 1L : 0L;
        }
        else if (pp_ev_is_punct('>')) {
            pp_ev_advance();
            v = (v > pp_ev_shift()) ? 1L : 0L;
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_eq()
{
    long            v;

    v = pp_ev_rel();
    for (;;) {
        if (pp_ev_is_punct(PP_EQ)) {
            pp_ev_advance();
            v = (v == pp_ev_rel()) ? 1L : 0L;
        }
        else if (pp_ev_is_punct(PP_NE)) {
            pp_ev_advance();
            v = (v != pp_ev_rel()) ? 1L : 0L;
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_bitand()
{
    long            v;

    v = pp_ev_eq();
    for (;;) {
        if (pp_ev_is_punct('&')) {
            pp_ev_advance();
            v = v & pp_ev_eq();
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_bitxor()
{
    long            v;

    v = pp_ev_bitand();
    for (;;) {
        if (pp_ev_is_punct('^')) {
            pp_ev_advance();
            v = v ^ pp_ev_bitand();
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_bitor()
{
    long            v;

    v = pp_ev_bitxor();
    for (;;) {
        if (pp_ev_is_punct('|')) {
            pp_ev_advance();
            v = v | pp_ev_bitxor();
        }
        else
            break;
    }
    return v;
}

static long
pp_ev_land()
{
    long            v;
    long            r;

    v = pp_ev_bitor();
    for (;;) {
        if (pp_ev_is_punct(PP_LAND)) {
            pp_ev_advance();
            /* Always parse RHS -- do not short-circuit the token walk. */
            r = pp_ev_bitor();
            v = (v && r) ? 1L : 0L;
        }
        else
            break;
    }
    return v;
}

static long
pp_eval_lor()
{
    long            v;
    long            r;

    v = pp_ev_land();
    for (;;) {
        if (pp_ev_is_punct(PP_LOR)) {
            pp_ev_advance();
            r = pp_ev_land();
            v = (v || r) ? 1L : 0L;
        }
        else
            break;
    }
    return v;
}

/*
 * Evaluate a #if/#elif expression from the rest of the physical line.
 * lastch is the first expression character; lptr points after it.
 */
static int
pp_eval_line()
{
    char           *line;
    int             cap;
    int             len;
    unsigned char  *src;
    PPTOKEN        *toks;
    long            v;
    char           *neu;
    int             i;

    /*
     * Build a heap copy of the remainder of the line (join splices).
     * Back up so lastch is included.
     */
    if (lastch != -1 && lastch != '\n' && lastch != '\0')
        --lptr;
    src = lptr;

    ++global_flag;
    cap = 128;
    line = (char *) xalloc(cap);
    len = 0;

    for (;;) {
        if (*src == '\0' || *src == '\n')
            break;
        if (*src == '\\'
            && (src[1] == '\0' || src[1] == '\n' || src[1] == '\r')) {
            if (ac_getline(incldepth == 0))
                break;
            src = (unsigned char *) in_line;
            continue;
        }
        if (len + 2 >= cap) {
            neu = (char *) xalloc(cap * 2);
            for (i = 0; i < len; i++)
                neu[i] = line[i];
            line = neu;
            cap *= 2;
        }
        line[len++] = (char) *src;
        src++;
    }
    line[len] = '\0';
    --global_flag;

    /* Consume the line so callers do not re-scan it as C tokens. */
    lptr = src;
    lastch = ' ';

    ++global_flag;
    toks = pp_tokenize(line);
    toks = pp_rewrite_defined(toks);
    toks = pp_expand_object_macros(toks, 0);
    pp_ev_cur = toks;
    pp_ev_depth = 0;
    v = pp_eval_lor();
    if (pp_ev_cur != NULL)
        error(ERR_SYNTAX, "junk after #if expression");
    --global_flag;
    pp_list_free(toks);
    return (v != 0);
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

    /*
     * Tokenize the rest of the line, rewrite defined(), expand object
     * macros, then evaluate with the token #if parser.
     */
    value = pp_eval_line();

    oneline = FALSE;

    premode = pr_if;
    prestat = ps_ignore;

    if (value != 0)     /* compile section  */
        prestat = ps_do;

    ac_getline(incldepth == 0);
}

/*
 * Macro argument collection and token-based expansion.
 * Arguments land in the arena via litlate; expansion walks PPT tokens.
 */

static char *
pp_collect_arg(stop_comma)
    int             stop_comma;
{
    char           *buf;
    int             cap;
    int             len;
    int             paren;
    int             state;
    char           *neu;
    int             i;

    while (pp_is_white(lastch))
        getch();

    ++global_flag;
    cap = 128;
    buf = (char *) xalloc(cap);
    len = 0;
    paren = 0;
    state = 0;

    for (;;) {
        if (lastch == EOF || lastch == -1)
            break;
        if (state == 0) {
            if (lastch == QUOT)
                state = 1;
            else if (lastch == SQUOT)
                state = 2;
            else if (lastch == LPAR)
                ++paren;
            else if (lastch == RPAR) {
                if (paren <= 0)
                    break;
                --paren;
            }
            else if (stop_comma && lastch == COMMA && paren <= 0) {
                getch();
                break;
            }
        }
        else if (state == 1) {
            if (lastch == QUOT)
                state = 0;
            else if (lastch == BSLASH) {
                if (len + 2 >= cap) {
                    neu = (char *) xalloc(cap * 2);
                    for (i = 0; i < len; i++)
                        neu[i] = buf[i];
                    buf = neu;
                    cap *= 2;
                }
                buf[len++] = (char) lastch;
                getch();
            }
        }
        else if (state == 2) {
            if (lastch == SQUOT)
                state = 0;
            else if (lastch == BSLASH) {
                if (len + 2 >= cap) {
                    neu = (char *) xalloc(cap * 2);
                    for (i = 0; i < len; i++)
                        neu[i] = buf[i];
                    buf = neu;
                    cap *= 2;
                }
                buf[len++] = (char) lastch;
                getch();
            }
        }

        if (len + 2 >= cap) {
            neu = (char *) xalloc(cap * 2);
            for (i = 0; i < len; i++)
                neu[i] = buf[i];
            buf = neu;
            cap *= 2;
        }
        buf[len++] = (char) lastch;
        getch();
    }

    while (len > 0 && pp_is_white((unsigned char) buf[len - 1]))
        len--;
    buf[len] = '\0';
    --global_flag;
    return litlate(buf);
}

char *
getparm(buffer)
    char           *buffer;
{
    char           *r;

    (void) buffer;
    r = pp_collect_arg(1);
    return r;
}

char *
getvarargs(buffer)
    char           *buffer;
{
    char           *r;

    (void) buffer;
    r = pp_collect_arg(0);
    return r;
}

char *
stringify_param(param)
    char           *param;
{
    PPTOKEN        *toks;
    char           *r;

    if (param == NULL)
        param = "";
    ++global_flag;
    toks = pp_tokenize(param);
    r = pp_stringize(toks);
    --global_flag;
    return r;
}

char *
paste_tokens(token1, token2)
    char           *token1, *token2;
{
    return pp_paste_text(token1, token2);
}

static void
reverse_string(str)
    char           *str;
{
    char           *start;
    char           *end;
    char            temp;

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

char *
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
     */
    if (args == NULL)
        return NULL;
    len = (int) strlen(args);
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

/*
 * Find formal by name in the function-macro parameter list.
 */
static SYM *
pp_find_formal(formals, name)
    SYM            *formals;
    char           *name;
{
    SYM            *sp1;

    if (name == NULL)
        return NULL;
    for (sp1 = formals; sp1 != NULL; sp1 = sp1->next) {
        if (sp1->name != NULL && strcmp(sp1->name, name) == 0)
            return sp1;
    }
    return NULL;
}

static PPTOKEN *
pp_arg_tokens(formal)
    SYM            *formal;
{
    if (formal == NULL || formal->value.s == NULL)
        return pp_tok_new(PPT_PLACEMARKER, 0, NULL);
    return pp_tokenize(formal->value.s);
}

/*
 * Expand function-macro replacement list with # / ## / arg substitution.
 */
static PPTOKEN *
pp_expand_replacement(repl, formals)
    PPTOKEN        *repl;
    SYM            *formals;
{
    PPTOKEN        *out;
    PPTOKEN        *tail;
    PPTOKEN        *t;
    PPTOKEN        *argtoks;
    PPTOKEN        *p;
    SYM            *formal;
    char           *left;
    char           *right;
    char           *pasted;
    PPTOKEN        *next;

    out = NULL;
    tail = NULL;
    t = repl;
    while (t != NULL) {
        /* # formal  -> stringize */
        if (t->kind == PPT_PUNCT && t->punct == '#') {
            next = t->next;
            if (next != NULL && next->kind == PPT_IDENT) {
                formal = pp_find_formal(formals, next->text);
                if (formal != NULL) {
                    argtoks = pp_arg_tokens(formal);
                    pp_list_append(&out, &tail,
                        pp_tok_new(PPT_STRING, 0, pp_stringize(argtoks)));
                    t = next->next;
                    continue;
                }
            }
            /* Not a formal: emit # as punct and continue. */
            pp_list_append(&out, &tail, pp_tok_dup(t));
            t = t->next;
            continue;
        }

        /* left ## right [## right ...] left-associative */
        if (t->next != NULL && t->next->kind == PPT_PUNCT
            && t->next->punct == PP_HASHHASH) {
            left = "";
            if (t->kind == PPT_IDENT) {
                formal = pp_find_formal(formals, t->text);
                if (formal != NULL) {
                    if (formal->value.s != NULL)
                        left = formal->value.s;
                    else
                        left = "";
                }
                else if (t->text != NULL)
                    left = t->text;
            }
            else if (t->kind == PPT_PLACEMARKER)
                left = "";
            else if (t->text != NULL)
                left = t->text;

            t = t->next;        /* now at ## */
            pasted = left;
            while (t != NULL && t->kind == PPT_PUNCT
                   && t->punct == PP_HASHHASH) {
                t = t->next;    /* operand after ## */
                right = "";
                if (t == NULL)
                    break;
                if (t->kind == PPT_IDENT) {
                    formal = pp_find_formal(formals, t->text);
                    if (formal != NULL) {
                        if (formal->value.s != NULL)
                            right = formal->value.s;
                    }
                    else if (t->text != NULL)
                        right = t->text;
                }
                else if (t->kind != PPT_PLACEMARKER && t->text != NULL)
                    right = t->text;
                pasted = pp_paste_text(pasted, right);
                t = t->next;
            }

            if (pasted == NULL || pasted[0] == '\0')
                pp_list_append(&out, &tail,
                    pp_tok_new(PPT_PLACEMARKER, 0, NULL));
            else if (pp_is_digit((int) pasted[0]))
                pp_list_append(&out, &tail,
                    pp_tok_new(PPT_NUMBER, 0, pasted));
            else
                pp_list_append(&out, &tail,
                    pp_tok_new(PPT_IDENT, 0, pasted));
            continue;
        }

        /* Formal parameter substitution (prescan: use arg tokens as-is). */
        if (t->kind == PPT_IDENT) {
            formal = pp_find_formal(formals, t->text);
            if (formal != NULL) {
                argtoks = pp_arg_tokens(formal);
                for (p = argtoks; p != NULL; p = p->next)
                    pp_list_append(&out, &tail, pp_tok_dup(p));
                t = t->next;
                continue;
            }
        }

        pp_list_append(&out, &tail, pp_tok_dup(t));
        t = t->next;
    }
    return out;
}

char *
prepdefine(sp)
    SYM            *sp;
{
    SYM            *sp1;
    PPTOKEN        *repl;
    PPTOKEN        *expanded;
    int             n;

    if (sp == NULL)
        return NULL;

    if (sp->tp == NULL)
        return sp->value.s;

    sp1 = sp->tp->lst.head;

    while (pp_is_white(lastch))
        getch();

    if (lastch != LPAR) {
        /*
         * Function-like macro name not followed by '(': leave as an
         * ordinary identifier.  Returning sp->name made getsym push the
         * name back and expand forever until LINDEPTH.
         */
        if (sp->tp != NULL)
            return NULL;
        return sp->value.s;
    }

    getch();

    while (sp1 != NULL) {
        if (sp1->value.i == 1)
            sp1->value.s = getvarargs(prepbuffer);
        else
            sp1->value.s = getparm(prepbuffer);
        sp1 = sp1->next;
    }

    while (pp_is_white(lastch))
        getch();

    if (lastch != RPAR) {
        error(ERR_DEFINE, "missing right parenthesis");
        return NULL;
    }
    getch();

    ++global_flag;
    repl = pp_tokenize(sp->value.s != NULL ? sp->value.s : "");
    expanded = pp_expand_replacement(repl, sp->tp->lst.head);

    prepbuffer[0] = '$';
    n = pp_list_to_text(expanded, prepbuffer + 1, 1023);
    if (n < 0) {
        prepbuffer[1] = '\0';
        error(ERR_DEFINE, "macro expansion too long");
    }
    --global_flag;
    pp_list_free(repl);
    pp_list_free(expanded);
    return litlate(prepbuffer + 1);
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
     * skipped).  Evaluating a candidate branch must set ps_do first --
     * otherwise the lexer ignores the expression and we hang.
     */
    if (prestat == ps_ignore) {
        switch (premode) {
        case pr_all:
            /* Nested inside a skipped region -- skip this line. */
            ac_getline(incldepth == 0);
            return;
        case pr_if:
            /* Prior branch false -- will evaluate below. */
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
            /* Prior #if/#elif true -- skip remainder of the chain. */
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
    value = pp_eval_line();
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

    while (lastch != EOF && pp_is_white(lastch))
        getch();

    while (lastch != EOF && !pp_is_white(lastch)) {
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
    char           *name;
    int             i;
    int             isdef;

    /*
     * defined(X) / defined X.  Heap name buffer; ASCII classes only.
     */
    while (pp_is_white(lastch))
        getch();

    seen = 0;
    if (lastch == LPAR) {
        seen = 1;
        getch();
        while (pp_is_white(lastch))
            getch();
    }

    while (pp_is_white(lastch))
        getch();

    if (!pp_is_idstart(lastch)) {
        error(ERR_IDEXPECT, NULL);
        return 0;
    }
    ++global_flag;
    name = (char *) xalloc(MAX_IDP1);
    --global_flag;
    i = 0;
    while (pp_is_idchar(lastch) && i < MAX_ID) {
        name[i++] = (char) lastch;
        getch();
    }
    name[i] = '\0';

    while (pp_is_white(lastch))
        getch();

    if (seen) {
        if (lastch == RPAR)
            getch();
        else
            error(ERR_PUNCT, NULL);
    }

    isdef = (search(name, defsyms.head) != NULL);
    return isdef;
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
        sp->tp = NULL;
        sp->storage_class = sc_define;
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

    /*
     * #if / #else must not depend only on searchkw: if keyword lookup
     * fails, "if" stays an id and the directive was dropped (unknown
     * command), desynchronizing NDK #if / #endif nesting.  Match by
     * spelling like the other directives below.
     */
    if (lastst == kw_if
        || (lastst == id && lastid[0] == 'i' && lastid[1] == 'f'
            && lastid[2] == '\0'))
        doif();
    else if (lastst == kw_else
             || (lastst == id && strcmp(lastid, "else") == 0))
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
