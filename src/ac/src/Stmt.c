
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
 * $Log:	Stmt.c,v $
 * Revision 3.33  90/04/05  22:46:46  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:25:46  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Stmt.c
 * 
 * The statement module handles all of the possible c statements and builds a
 * parse tree of the statements.
 * 
 * Each routine returns a pointer to a statement parse node which reflects the
 * statement just parsed.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <ctype.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

extern char     in_line[];
extern char     infile[];
extern int      in_line_used;

extern int      dbxlnum;
extern long     intexpr();
extern SYM     *search();
extern TYP     *expression();
extern char    *itoa();
extern char    *litlate();
extern char    *xalloc();
extern struct enode *makenode();

int             dbxfile = FALSE;/* Is this the first # 1 "file" command */
int             dbxincl = 0;    /* Are we in an include file */

struct snode   *statement();    /* forward declaration */

void
addauto(snp)
    struct snode   *snp;
{
    if (snp == NULL)
        return;

    if (autohead == NULL)
        autohead = autotail = snp;
    else
        autotail->next = snp;

    if (autotail != NULL)
        while (autotail->next != NULL)
            autotail = autotail->next;
}

struct snode   *
makesnode(ty)
    enum e_stmt     ty;
{
    struct snode   *snp;

    snp = (struct snode *) xalloc(sizeof(struct snode));

    snp->stype = ty;

    snp->s1 = snp->s2 = snp->next = NULL;
    snp->label = NULL;
    snp->exp = NULL;

    return (snp);
}

struct snode   *
joinsnode(s1, s2)
    struct snode   *s1, *s2;
{
    struct snode   *head, *tail;

    head = s1;
    if (head == NULL)
        head = s2;
    else {
        tail = head;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = s2;
    }
    return (head);
}

struct dnode   *
makednode(tag)
    int             tag;
{
    struct dnode   *dp;

    dp = (struct dnode *) xalloc(sizeof(struct dnode));

    dp->tag = tag;
    dp->sp = NULL;
    dp->ref = NULL;
    dp->next = NULL;
    dp->nest = dp->label = 0;

    return (dp);
}

struct snode   *
whilestmt()

/*
 * whilestmt parses the c while statement.
 */
{
    struct snode   *snp;

    snp = makesnode(st_while);
    getsym();
    if (lastst != openpa)
        error(ERR_EXPREXPECT, NULL);
    else {
        getsym();
        if (expression(&(snp->exp)) == NULL)
            error(ERR_EXPREXPECT, NULL);
        needpunc(closepa);
        snp->s1 = statement();
    }
    return snp;
}

struct snode   *
declarestmt(sp)
    SYM            *sp;

/*
 * Generate a source code declaration for DBX
 */

{
    return (NULL);
}

struct snode   *
entrystmt()

/*
 * Generate a source code block entry for DBX
 */

{
    return (NULL);
}

struct snode   *
exitstmt()

/*
 * Generate a source code block exit for DBX
 */

{
    return (NULL);
}

struct snode   *
includestmt()

/*
 * Generate a source code line reference for DBX
 */

{
    return (NULL);
}

struct snode   *
filestmt()

/*
 * Generate a source code include declaration for DBX
 */

{
    return (NULL);
}

struct snode   *
linestmt()

/*
 * Generate a source code line reference for DBX
 */

{
    return (NULL);
}

struct snode   *
commentstmt()

/*
 * Generate a source code comment
 */

{
    struct snode   *snp = NULL;
    char           *s;
    int             len;

    if (!in_line_used) {
        in_line_used = TRUE;
        snp = makesnode(st_comment);

        s = litlate(in_line);
        len = strlen(s) - 1;
        if (s[len] == '\n')
            s[len] = '\0';

        snp->exp = makenode(en_comment, s, NULL);
    }
    return snp;
}

struct snode   *
asmstmt()

/*
 * Generate an assembly source statement
 */

{
    struct snode   *snp = NULL;
    char           *s;

    snp = makesnode(st_asm);
    s = litlate(laststr);
    snp->exp = makenode(en_asm, s, NULL);

    getsym();

    return snp;
}

struct snode   *
dostmt()

/*
 * dostmt parses the c do-while construct.
 */
{
    struct snode   *snp;

    snp = makesnode(st_do);
    getsym();
    snp->s1 = statement();
    if (lastst != kw_while)
        error(ERR_WHILEXPECT, NULL);
    else {
        getsym();
        if (lastst != openpa)
            error(ERR_EXPREXPECT, NULL);
        else {
            getsym();
            if (expression(&(snp->exp)) == NULL)
                error(ERR_EXPREXPECT, NULL);
            needpunc(closepa);
        }
        if (lastst != end)
            needpunc(semicolon);
    }
    return snp;
}

struct snode   *
forstmt()
{
    struct snode   *snp;

    snp = makesnode(st_for);
    getsym();
    needpunc(openpa);
    if (expression(&(snp->label)) == NULL)
        snp->label = NULL;
    needpunc(semicolon);
    snp->stype = st_for;
    if (expression(&(snp->exp)) == NULL)
        snp->exp = NULL;
    needpunc(semicolon);
    if (expression(&(snp->s2)) == NULL)
        snp->s2 = NULL;
    needpunc(closepa);
    snp->s1 = statement();
    return snp;
}

struct snode   *
ifstmt()

/*
 * ifstmt parses the c if statement and an else clause if one is present.
 */
{
    struct snode   *snp;

    snp = makesnode(st_if);
    getsym();
    if (lastst != openpa)
        error(ERR_EXPREXPECT, NULL);
    else {
        getsym();
        if (expression(&(snp->exp)) == NULL)
            error(ERR_EXPREXPECT, NULL);
        needpunc(closepa);
        snp->s1 = statement();
        if (lastst == kw_else) {
            getsym();
            snp->s2 = statement();
        }
        else
            snp->s2 = NULL;
    }
    return snp;
}

struct snode   *
casestmt()

/*
 * cases are returned as seperate statements. for normal cases label is the
 * case value and s2 is zero. for the default case s2 is nonzero.
 */
{
    struct snode   *snp;
    struct snode   *head, *tail;
    long            caselab;

    snp = makesnode(st_case);
    if (lastst == kw_case) {
        getsym();
        snp->s2 = (struct snode *) NULL;
        snp->stype = st_case;
        caselab = intexpr();    /* This is rather strange code */
        snp->label = (long *) caselab;  /* To recase the label, as
                         * ptr */
    }
    else if (lastst == kw_default) {
        getsym();
        snp->s2 = (struct snode *) 1;
    }
    else {
        error(ERR_NOCASE, NULL);
        return NULL;
    }
    needpunc(colon);
    head = NULL;

    while (lastst != end && lastst != kw_case && lastst != kw_default) {
        if (head == NULL)
            head = tail = statement();
        else
            tail->next = statement();
        while (tail->next != NULL)
            tail = tail->next;
    }

    snp->s1 = head;
    return snp;
}

int
checkcases(head)

/*
 * checkcases will check to see if any duplicate cases exist in the case list
 * pointed to by head.
 */
    struct snode   *head;
{
    struct snode   *top, *cur;

    cur = top = head;
    while (top != NULL) {
        cur = top->next;
        while (cur != NULL) {
            if ((!(cur->s1 || cur->s2) && cur->label == top->label)
                || (cur->s2 && top->s2)) {
                fprintf(stderr, " duplicate case label %d\n", cur->label);
                return TRUE;
            }
            cur = cur->next;
        }
        top = top->next;
    }
    return FALSE;
}

struct snode   *
switchstmt()
{
    struct snode   *snp;
    struct snode   *head, *tail;

    snp = makesnode(st_switch);
    getsym();
    needpunc(openpa);
    if (expression(&(snp->exp)) == NULL)
        error(ERR_EXPREXPECT, NULL);
    needpunc(closepa);
    needpunc(begin);
    head = NULL;

    while (lastst != end) {
        if (head == NULL)
            head = tail = casestmt();
        else
            tail->next = casestmt();
        while (tail->next != NULL)
            tail = tail->next;
    }
    snp->s1 = head;
    getsym();
    if (checkcases(head))
        error(ERR_DUPCASE, NULL);
    return snp;
}

struct snode   *
retstmt()
{
    TYP            *tp;
    enum e_bt       ftype = lastfunc->tp->btp->type;
    struct snode   *snp;

    snp = makesnode(st_return);
    getsym();
    tp = expression(&(snp->exp));

    if (tp != NULL) {
        switch (tp->type) {
        case bt_float:
            if (ftype == bt_double) 
                snp->exp = makenode( en_cfd, snp->exp, NULL );
            else if (ftype != bt_float) 
                snp->exp = makenode( en_cfl, snp->exp, NULL );
            break;
        
        case bt_double:
            if (ftype == bt_float)
                snp->exp = makenode( en_cdf, snp->exp, NULL );
            else if (ftype != bt_double) 
                snp->exp = makenode( en_cdl, snp->exp, NULL );
            break;

        default:
            if (ftype == bt_double)
                snp->exp = makenode( en_cld, snp->exp, NULL );
            else if (ftype == bt_float) 
                snp->exp = makenode( en_clf, snp->exp, NULL );
            break;
        }
    }

    if (lastst != end)
        needpunc(semicolon);
    return snp;
}

struct snode   *
breakstmt()
{
    struct snode   *snp;

    snp = makesnode(st_break);
    getsym();
    if (lastst != end)
        needpunc(semicolon);
    return snp;
}

struct snode   *
contstmt()
{
    struct snode   *snp;

    snp = makesnode(st_continue);
    getsym();
    if (lastst != end)
        needpunc(semicolon);
    return snp;
}

struct snode   *
exprstmt()

/*
 * exprstmt is called whenever a statement does not begin with a keyword. the
 * statement should be an expression.
 */
{
    struct snode   *snp;

    snp = makesnode(st_expr);
    if (expression(&(snp->exp)) == NULL) {
        error(ERR_EXPREXPECT, NULL);
        getsym();
    }
    if (lastst != end)
        needpunc(semicolon);
    return snp;
}

struct snode   *
compound(look_ahead)
    int look_ahead;

/*
 * compound processes a block of statements and forms a linked list of the
 * statements within the block.
 * 
 * compound expects the input pointer to already be past the begin symbol of the
 * block.
 */
{
    struct snode   *head, *tail;
    struct snode   *ahead, *atail;
    TABLE           nsyms;
    SYM            *curr, *last;
    int             local_base, local_auto;

    local_base = lc_base;
    local_auto = lc_auto;

    nsyms.head = lsyms.head;
    nsyms.tail = lsyms.tail;

    ahead = autohead;
    atail = autotail;

    autohead = autotail = NULL;
    dodecl(sc_auto);

    nest_level++;

    if (lc_auto > lc2_auto)
        lc2_auto = lc_auto;

    head = tail = NULL;


    while (lastst != end && lastst != eof) {
        if (head == NULL)
            head = tail = statement();
        else
            tail->next = statement();
        while (tail != NULL && tail->next != NULL)
            tail = tail->next;
    }

    if (lastst == eof)
        error( ERR_SYNTAX, "Unexpected end of file");

    if (autohead != NULL) {
        autotail->next = head;
        head = autohead;
    }

    if (look_ahead)
        getsym();

    autohead = ahead;
    autotail = atail;

    /* Remove all of the local symbols from the nested block */
    /* Check to see that any local labels are kept.          */

    if (nsyms.head != NULL)
        last = nsyms.tail;
    else
        last = (SYM *) & lsyms.head;

    for (curr = last->next; curr != NULL; curr = curr->next) {
        if (curr->storage_class == sc_label
            || curr->storage_class == sc_ulabel)
            last = curr;
        else
            last->next = curr->next;
    }
    lsyms.tail = last;

    lc_auto = local_auto;
    lc_base = local_base;

    nest_level--;

    return head;
}

struct snode   *
labelstmt()

/*
 * labelstmt processes a label that appears before a statement as a seperate
 * statement.
 */
{
    struct snode   *snp;
    SYM            *sp;

    snp = makesnode(st_label);
    if ((sp = search(lastid, lsyms.head)) == NULL) {
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->name = litlate(lastid);
        sp->storage_class = sc_label;
        sp->tp = NULL;
        sp->value.i = nextlabel++;
        insert(sp, &lsyms);
    }
    else {
        if (sp->storage_class != sc_ulabel)
            error(ERR_LABEL, NULL);
        else
            sp->storage_class = sc_label;
    }
    getsym();       /* get past id */
    needpunc(colon);
    if (sp->storage_class == sc_label) {
        snp->label = (long *) sp->value.i;
        snp->next = NULL;
        return snp;
    }
    return NULL;
}

struct snode   *
gotostmt()

/*
 * gotostmt processes the goto statement and puts undefined labels into the
 * symbol table.
 */
{
    struct snode   *snp;
    SYM            *sp;

    getsym();
    if (lastst != id) {
        error(ERR_IDEXPECT, NULL);
        return NULL;
    }
    snp = makesnode(st_goto);
    if ((sp = search(lastid, lsyms.head)) == NULL) {
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->name = litlate(lastid);
        sp->value.i = nextlabel++;
        sp->storage_class = sc_ulabel;
        sp->tp = NULL;
        insert(sp, &lsyms);
    }
    getsym();       /* get past label name */
    if (lastst != end)
        needpunc(semicolon);
    if (sp->storage_class != sc_label && sp->storage_class != sc_ulabel)
        error(ERR_LABEL, NULL);
    else {
        snp->stype = st_goto;
        snp->label = (long *) (sp->value.i);
        snp->next = NULL;
        return snp;
    }
    return NULL;
}

struct snode   *
statement2()

/*
 * statement figures out which of the statement processors should be called
 * and transfers control to the proper routine.
 */
{
    struct snode   *snp, *snc, *ptr;

    snc = NULL;

    if (Options.Annote)
        snc = commentstmt();

    switch (lastst) {
    case semicolon:
        getsym();
        snp = NULL;
        break;
    case begin:
        getsym();
        snp = compound(TRUE);
        if (snc != NULL) {
            ptr = snc;
            while (ptr != NULL && ptr->next != NULL)
                ptr = ptr->next;
            ptr->next = snp;
            return snc;
        }
        return snp;
    case asmconst:
        snp = asmstmt();
        break;
    case kw_if:
        snp = ifstmt();
        break;
    case kw_while:
        snp = whilestmt();
        break;
    case kw_for:
        snp = forstmt();
        break;
    case kw_return:
        snp = retstmt();
        break;
    case kw_break:
        snp = breakstmt();
        break;
    case kw_goto:
        snp = gotostmt();
        break;
    case kw_continue:
        snp = contstmt();
        break;
    case kw_do:
        snp = dostmt();
        break;
    case kw_switch:
        snp = switchstmt();
        break;
    case id:
        while (isspace(lastch))
            getch();
        if (lastch == ':') {
            snp = labelstmt();
            if (snc != NULL) {
                ptr = snc;
                while (ptr != NULL && ptr->next != NULL)
                    ptr = ptr->next;
                ptr->next = snp;
                return snc;
            }
            return snp;
        }
        /* else fall through to process expression */
    default:
        if (lastst != eof)
            snp = exprstmt();
        break;
    }
    if (snc != NULL) {
        ptr = snc;
        while (ptr != NULL && ptr->next != NULL)
            ptr = ptr->next;
        ptr->next = snp;
        return snc;
    }
    return snp;
}

struct snode   *
statement()

/*
 * Get the next statement and if it is only a comment or debug or label then
 * get the next statement to it.
 */
{
    struct snode   *snp, *snc, *ptr;
    int             valid, has_label;

    if (lastst == eof)
        return (NULL);

    if ((snp = statement2()) == NULL)
        return (NULL);

    valid = FALSE;
    has_label = FALSE;

    for (ptr = snp; ptr != NULL; ptr = ptr->next) {
        switch (ptr->stype) {
        case st_label:
            has_label = TRUE;
            break;
        case st_comment:
        case st_stabn:
        case st_stabs:
            break;
        default:
            valid = TRUE;
        }
    }

    if (!valid && has_label) {
        snc = statement();
        snp = joinsnode(snp, snc);
    }

    return (snp);
}
