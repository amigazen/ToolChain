
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

extern char    *itoa(int x);

#define LPAR    '('
#define RPAR    ')'
#define QUOT    '"'
#define SQUOT   '\''
#define COMMA   ','
#define BSLASH  '\\'

#ifndef INCLUDE   /* The standard place for include files */
#ifdef unix
#define INCLUDE "/usr/include/"
#else
#define INCLUDE "AC:include/"
#endif
#endif

FILE           *inclfile[10];
int             inclline[10];
char           *inclname[10];

int             incldepth = 0;
int             prepdepth = 0;
int             inpreproc = FALSE;
int             oneline = FALSE;
int             inclnum = 0;
unsigned char  *lptr;
char           *incldir[10];
char            prepbuffer[1024];

char            __linebuf[10];  /* buffer for __LINE__  */
char            __filebuf[64];  /* buffer for __FILE__  */
char            __datebuf[64];  /* buffer for __DATE__  */
char            __timebuf[64];  /* buffer for __TIME__  */
char            __funcbuf[64];  /* buffer for __FUNC__  */

enum e_pm       inclprep[10];
enum e_ps       inclstat[10];

enum e_pm       premode = pr_all;
enum e_ps       prestat = ps_do;

void  setdefine();

extern int      in_comment;
extern int      dbxlnum;
extern char    *litlate();
extern char    *xalloc();
extern SYM     *search();
extern enum e_sym getid();
extern struct snode *filestmt();

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
        getline(incldepth == 0);
        return;
    }

    fp = NULL;
    num = 0;
    oneline = TRUE;

    while (isspace(lastch)) /* Skip the white space */
        getch();

    if (lastch == QUOT) {   /* #include "file.h"    */
        getsym();
        oneline = FALSE;
        if (lastst == sconst) {
            strcpy( filename, laststr );
            if (search( filename, cmd_include.head) != NULL) {
                getline(incldepth == 0);
                return;
            }
            method = FALSE;
            if ((fp = fopen(laststr, "r")) == NULL) {
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
            getline(incldepth == 0);
            return;
        }
        strcpy(prepbuffer, INCLUDE);
        strcat(prepbuffer, laststr);
        method = TRUE;
        if ((fp = fopen(prepbuffer, "r")) != NULL)
            strcpy(laststr, prepbuffer);
    }

    if (lastst != sconst) {
        fatal_error(ERR_INCLFILE, "file name is not a string");
        getline(incldepth == 0);
        return;
    }

    for (num = 0; num < inclnum && fp == NULL; num++) {
        strcpy(prepbuffer, incldir[num]);
        strcat(prepbuffer, laststr);
        if ((fp = fopen(prepbuffer, "r")) != NULL)
            strcpy(laststr, prepbuffer);
    }

    if (fp == NULL) {
        fatal_error(ERR_CANTOPEN, NULL);
        getline(incldepth == 0);
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
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->value.i = method;
        sp->name = litlate(filename);
        sp->storage_class = sc_library;
        sp->storage_type  = sc_library;
        sp->tp = NULL;
        --global_flag;

        insert(sp, &cmd_local );
        
        getline(incldepth >= 1);

#ifdef NOLISTINC
        lineno = -32767;            /* don't list include files         */
                                    /* >32K lines unlikely, but a flag  */
                                    /* would be better - LDH            */
#else
        lineno = 0;                 /* list include files */
#endif

    }
}

void
dodefine()
{
    SYM            *sp, *sp1;
    unsigned char  *ptr, *buffer;
    int             valid, in_quote;

    if (prestat == ps_ignore) {
        getline(incldepth == 0);
        return;
    }

    inpreproc = TRUE;
    oneline = TRUE;
    getsym();       /* get past #define     */
    oneline = FALSE;
    inpreproc = FALSE;

    if (lastst != id) {
        error(ERR_DEFINE, "token to define is not an identifier");
        getline(incldepth == 0);
        return;
    }

    if (search(lastid, defsyms.head) != NULL) {
        /* Check if it's an identical redefinition */
        SYM *existing = search(lastid, defsyms.head);
        if (existing->tp != NULL && lastch == '(') {
            /* Function macro - check if parameters match */
            /* For now, allow redefinition and update the value */
            existing->value.s = NULL; /* Will be set later */
        } else if (existing->tp == NULL && lastch != '(') {
            /* Object macro - allow redefinition */
            existing->value.s = NULL; /* Will be set later */
        } else {
            error(ERR_DEFINE, "already defined");
            getline(incldepth == 0);
            return;
        }
    }

    ++global_flag;      /* always do #define as globals */

    sp = (SYM *) xalloc(sizeof(SYM));
    sp->name = litlate(lastid);
    sp->storage_class = sc_define;
    sp->storage_type = sc_define;
    sp->tp = NULL;

    if (lastch == '(' /*-)-*/ ) {   /* Function macro definition    */
        sp->tp = (TYP *) xalloc(sizeof(TYP));
        sp->tp->type = bt_func;

        ptr = (unsigned char *) lastid;   /* NULL the parameter name      */
        *ptr = '\0';
        oneline = TRUE;
        getch();    /* Get past the openpa          */
        while (isspace(lastch)) /* Skip the white space         */
            getch();
        getid();    /* Get the parameter            */
        if (*lastid) {  /* If we have a parameter       */
            sp1 = (SYM *) xalloc(sizeof(SYM));
            sp1->name = litlate(lastid);
            sp1->storage_class = sc_define;
            sp1->storage_type = sc_define;
            sp->tp->lst.head = sp->tp->lst.tail = sp1;
            while (isspace(lastch)) /* Skip the white space     */
                getch();
            while (lastch == ',') {
                getch();    /* Skip the comma           */
                while (isspace(lastch)) /* Skip the white space     */
                    getch();
                getid();    /* Get the parameter        */
                if (!*lastid)
                    break;
                else if (strcmp(lastid, "...") == 0) {
                    /* Handle variadic macro parameter */
                    sp1 = (SYM *) xalloc(sizeof(SYM));
                    sp1->name = litlate("__VA_ARGS__");
                    sp1->storage_class = sc_define;
                    sp1->storage_type = sc_define;
                    sp1->value.i = 1; /* Mark as variadic */
                    sp->tp->lst.tail->next = sp1;
                    sp->tp->lst.tail = sp1;
                    break;
                } else {  /* If we have a parameter    */
                    sp1 = (SYM *) xalloc(sizeof(SYM));
                    sp1->name = litlate(lastid);
                    sp1->storage_class = sc_define;
                    sp1->storage_type = sc_define;
                    sp->tp->lst.tail->next = sp1;
                    sp->tp->lst.tail = sp1;
                }
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

    in_quote = in_comment = FALSE;

    for (ptr = lptr; *ptr; ptr++) {
        if (!isspace(*ptr))
            valid = TRUE;
        if (*ptr == '"')
            in_quote = !in_quote;
        if (*ptr == '/'  && *(ptr + 1) == '*')
            in_comment = !in_quote;
        if (*ptr == '*'  && *(ptr + 1) == '/')
            in_comment = FALSE;
        if (*ptr == '\\' && *(ptr + 1) == '\n') {
            getline(incldepth == 0);
            ptr = lptr;
        }
        *buffer++ = *ptr;
        if (in_comment && (*(ptr + 1) == '\0' || *(ptr + 1) == '\n')) {
            getline(incldepth == 0);
            ptr = lptr;
        }
    }

    *buffer = '\0';

    in_comment = FALSE;

    if (valid)
        sp->value.s = litlate(prepbuffer);
    else
        sp->value.s = litlate(" ");

    insert(sp, &defsyms);
    --global_flag;
    getline(incldepth == 0);
}

void
doundef()
{
    if (prestat == ps_ignore) {
        getline(incldepth == 0);
        return;
    }

    inpreproc = TRUE;
    getsym();       /* get past #undef */
    inpreproc = FALSE;

    if (lastst != id) {
        error(ERR_DEFINE, "missing identifier");
        getline(incldepth == 0);
        return;
    }
    remove_symbol(lastid, &defsyms);
    getline(incldepth == 0);
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
    getline(incldepth == 0);
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
    getline(incldepth == 0);
}

void
doifdef(mode)
    int             mode;
{
    SYM            *sp;

    if (prestat == ps_ignore) {
        inclstat[prepdepth] = prestat;
        inclprep[prepdepth++] = premode;
        prestat = ps_ignore;
        premode = pr_all;
        getline(incldepth == 0);
        return;
    }

    inpreproc = TRUE;
    getsym();       /* get past #ifdef */
    inpreproc = FALSE;

    if (lastst != id) {
        error(ERR_DEFINE, "identifier expected");
        getline(incldepth == 0);
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
    getline(incldepth == 0);
}

void
doif()
{
    int             value;

    if (prestat == ps_ignore) {
        inclstat[prepdepth] = prestat;
        inclprep[prepdepth++] = premode;
        prestat = ps_ignore;
        premode = pr_all;
        getline(incldepth == 0);
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

    getline(incldepth == 0);
}

char    *
getparm(buffer)
    char    *buffer;
{
    char    *ptr;
    int     paren, state;

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
    return(litlate(buffer));
}

char    *
getvarargs(buffer)
    char    *buffer;
{
    char    *ptr;
    int     paren, state;

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
        else if (*pattern == '#') {
            /* Stringification operator */
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
        else if (*pattern == '#' && *(pattern + 1) == '#') {
            /* Token pasting operator */
            pattern += 2; /* Skip the ## */
            if (isidch(*pattern)) {
                char *token1 = laststr;
                char *token2 = laststr + 64; /* Use different part of buffer */
                while (isidch(*pattern))
                    *token1++ = *pattern++;
                *token1 = '\0';
                
                /* Find the second token */
                while (isspace(*pattern))
                    pattern++;
                if (isidch(*pattern)) {
                    while (isidch(*pattern))
                        *token2++ = *pattern++;
                    *token2 = '\0';
                    
                    loc = paste_tokens(laststr, laststr + 64);
                    while (*loc)
                        *ptr++ = *loc++;
                }
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
        inclstat[prepdepth] = prestat;
        inclprep[prepdepth++] = premode;
        prestat = ps_ignore;
        premode = pr_all;
        getline(incldepth == 0);
        return;
    }

    inclstat[prepdepth] = prestat;
    inclprep[prepdepth++] = premode;

    premode = pr_asm;
    prestat = ps_do;

    getline(incldepth == 0);
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

    getline(incldepth == 0);
}

void
doline()
{
    struct snode   *snp;

    if (prestat == ps_ignore) {
        getline(incldepth == 0);
        return;
    }

    oneline = TRUE;     /* must be on one line   */
    getsym();
    oneline = FALSE;
    if (lastst != iconst) {
        error(ERR_PREPROC, "line number missing");
        getline(incldepth == 0);
        return;
    }
    else {
        dbxlnum = ival - 1; /* set the line number  */
        oneline = TRUE; /* must be on one line   */
        getsym();
        strcpy( __linebuf, itoa(dbxlnum));
        oneline = FALSE;
        if (lastst == eof) {
            getline(incldepth == 0);
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
            getline(incldepth == 0);
            return;
        }
        error(ERR_PREPROC, "invalid token");
        getline(incldepth == 0);
    }
}

void
doelif()
{
    int             value;

    if (prestat == ps_ignore) {
        switch (premode) {
        case pr_all:
            break;
        case pr_if:
            premode = pr_else;
            prestat = ps_do;
            break;
        case pr_else:
            error(ERR_DEFINE, NULL);
            break;
        }
    }
    else {          /* ps_do */
        switch (premode) {
        case pr_all:
            error(ERR_DEFINE, NULL);
            break;
        case pr_if:
            premode = pr_else;
            prestat = ps_ignore;
            break;
        case pr_else:
            error(ERR_DEFINE, NULL);
            break;
        }
    }

    if (prestat == ps_ignore) {
        inclstat[prepdepth] = prestat;
        inclprep[prepdepth++] = premode;
        prestat = ps_ignore;
        premode = pr_all;
        getline(incldepth == 0);
        return;
    }

    inclstat[prepdepth] = prestat;
    inclprep[prepdepth++] = premode;

    oneline = TRUE;     /* The expresion must be on one line */

    getsym();       /* get past #elif */

    value = intexpr();  /* get the expression */

    oneline = FALSE;

    premode = pr_if;
    prestat = ps_ignore;

    if (value != 0)     /* compile section  */
        prestat = ps_do;

    getline(incldepth == 0);
}

void
doerror()
{
    char buffer[1024];
    char *cp, *endp;

    if (prestat == ps_ignore) {
        getline(incldepth == 0);
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
    *cp++ = '\n';
    *cp = '\0';
    fprintf(stderr, "USER ERROR: %s", buffer);
    oneline = FALSE;

    getline(incldepth == 0);
}

void
dowarning()
{
    char buffer[1024];
    char *cp, *endp;

    if (prestat == ps_ignore) {
        getline(incldepth == 0);
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
    *cp++ = '\n';
    *cp = '\0';
    fprintf(stderr, "WARNING: %s", buffer);
    oneline = FALSE;

    getline(incldepth == 0);
}

void
dopragma_once()
{
    static char **included_files = NULL;
    static int num_included = 0;
    int i;
    
    if (prestat == ps_ignore) {
        getline(incldepth == 0);
        return;
    }

    /* Check if current file already included */
    for (i = 0; i < num_included; i++) {
        if (strcmp(included_files[i], curfile) == 0) {
            /* File already included, skip to end of file */
            while (getline(incldepth == 0) == 0) {
                /* Skip lines until end of file */
            }
            return;
        }
    }
    
    /* Add current file to included list */
    if (included_files == NULL) {
        included_files = (char **)xalloc(10 * sizeof(char *));
    } else if (num_included % 10 == 0) {
        included_files = (char **)xalloc((num_included + 10) * sizeof(char *));
    }
    
    included_files[num_included] = (char *)xalloc(strlen(curfile) + 1);
    strcpy(included_files[num_included], curfile);
    num_included++;
    
    getline(incldepth == 0);
}

void
gettoken()
{
    char    *ptr = laststr;

    while (isspace(lastch)) 
        getch();

    while (!isspace(lastch)) {
        *ptr++ = lastch;
        getch();
    }

    *ptr = '\0';
}

void
dopragma()
{
    struct libcall  *lib;

    if (prestat == ps_ignore) {
        getline(incldepth == 0);
        return;
    }

    oneline = TRUE;

    gettoken();
    if (strcmp( laststr, "libcall" ) == 0) {
        ++global_flag;
        lib = (struct libcall *) xalloc( sizeof(struct libcall) );
        gettoken();
        lib->basename = litlate( laststr );
        gettoken();
        lib->funcname = litlate( laststr );
        gettoken();
        lib->offset = litlate(laststr);
        gettoken();
        lib->args = litlate( laststr );

        lib->next = libpragma;
        libpragma = lib;

        strcpy( laststr, "__LIBCALL_" );
        strcat( laststr, lib->funcname );
        setdefine( lib->funcname, litlate(laststr));
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

    getline(incldepth == 0);
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
        sp = (SYM *) xalloc(sizeof(SYM));
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
        getline(incldepth == 0);
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
        getline(incldepth == 0);
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
            getline(incldepth == 0);
        }
    }
}
