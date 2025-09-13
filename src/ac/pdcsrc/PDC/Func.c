
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
 * $Log:	Func.c,v $
 * Revision 3.33  90/04/05  22:29:34  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:23:49  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Func.c
 * 
 * function parsing routines
 */

#include    <stdio.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

extern char    *litlate();
extern SYM     *search();
extern SYM     *copysym();

extern struct snode *compound();
extern struct snode *linestmt();
extern struct snode *joinsnode();
extern char    *xalloc();
extern char     __funcbuf[];
extern int      padstr();
extern int      fatal;

extern int      castbegin();
extern int      declproto();

void    funcbottom(), block();

int     in_function = 0;

void
funcbody(sp)

/*
 * funcbody starts with the current symbol being either the first parameter
 * id or the begin for the local block. If begin is the current symbol then
 * funcbody assumes that the function has no parameters.
 */
    SYM            *sp;
{
    char           *names[MAX_FUNCPARMS];
    int             nparms, poffset, i, size;
    SYM            *sp1, *makeint();

    ++in_function;

#ifdef  GENERATE_DBX
    dbx_line();
#endif
    if (autohead != NULL) {
        genstmt(autohead);
        autohead = autotail = NULL;
        flush_peep();
    }
#ifdef  GENERATE_DBX
    dbx_line();
#endif

    maxparmsize = 0;

    lastfunc = sp;      /* Save the last function type */
    padstr(__funcbuf, lastfunc->name);

    poffset = 8;        /* size of return block */
    nparms = 0;

    if (castbegin(lastst)) {
        if (in_function > 1)
            declproto(&(sp->tp->lst), NULL );
        else
            declproto(&(sp->tp->lst), &lsyms );
        needpunc(closepa);
        for (sp1 = sp->tp->lst.head; sp1 != NULL; sp1 = sp1->next) {
            size = sp1->tp->size;
            sp1->value.i = poffset;
            if (size >= 4)
                poffset += size;
            else {
                if (size == 1)
                    ++sp1->value.i;
                poffset += 2;
            }
            if (in_function == 1) {
                sp1->storage_class = sc_auto;
                sp1->storage_type = sc_parameter;
#ifdef  GENERATE_DBX
                dbx_ident(sp1);
#endif
            }
        }
    }
    else if (lastst == id) {/* declare parameters */
        while (lastst == id) {
            names[nparms++] = litlate(lastid);
            getsym();
            if (lastst == comma)
                getsym();
            else
                break;
        }
        needpunc(closepa);
        dodecl(sc_member);  /* declare parameters */
        for (i = 0; i < nparms; ++i) {
            if ((sp1 = search(names[i], lsyms.head)) == NULL)
                sp1 = makeint(names[i]);
            if (sp1->tp->type == bt_pointer)
                size = 4;
            else
                size = sp1->tp->size;
            sp1->value.i = poffset;
            if (size < 4) {
                if (size == 1)
                    ++sp1->value.i;
                poffset += 2;
            }
            else
                poffset += size;
            if (in_function == 1) {
                sp1->storage_class = sc_auto;
                sp1->storage_type = sc_parameter;
#ifdef  GENERATE_DBX
                dbx_ident(sp1);
#endif
            }
        }
    }
    if (lastst == semicolon) {
        getsym();
        sp->tp->type = bt_func; /* Function prototype */
        sp->storage_class = sc_external;
        sp->storage_type = sc_external;
        if (in_function == 1) {
            nl();
            lc_auto = 0;
            lsyms.head = lsyms.tail = NULL;
        }
    }
    else if (lastst != begin)
        error(ERR_BLOCK, NULL);
    else {
        int save_flag = global_flag;

/* TODO
   If we're generating a prototype file, output this function's prototype here.
*/
        global_flag = 0;
        block(sp);
        funcbottom();
        global_flag = save_flag;
    }
    --in_function;
}

SYM            *
makeint(name)
    char           *name;
{
    SYM            *sp;
    TYP            *tp;

    sp = (SYM *) xalloc(sizeof(SYM));
    tp = (TYP *) xalloc(sizeof(TYP));
    tp->type = bt_long;
    tp->size = 4;
    tp->btp = NULL;
    tp->lst.head = NULL;
    tp->sname = NULL;
    sp->name = name;
    sp->storage_class = sc_auto;
    sp->tp = tp;
    insert(sp, &lsyms);
    return sp;
}

void
check_table(head)
    SYM            *head;
{
    char           *err_fmt;

    err_fmt = (char *) "*** UNDEFINED LABEL - %s\n";
    while (head != NULL) {
        if (head->storage_class == sc_ulabel) {
            if (Options.List) {
                fprintf( list, err_fmt, head->name );
            }
            fprintf( stderr, err_fmt, head->name );
        }
        head = head->next;
    }
}

void
funcbottom()
{
    nl();
    check_table(lsyms.head);
    lc_auto = 0;
    if (Options.List)
        fprintf( list, "\n\n*** local symbol table ***\n\n" );
    list_table(&lsyms, 0);
    if (Options.List)
        fprintf( list, "\n\n\n" );
    release_local();    /* release local symbols */
}

void
block(sp)
    SYM            *sp;
{
    struct snode   *stmt;

    needpunc(begin);

    dodecl(sc_auto);
    lc2_auto = lc_base = lc_auto;

    stmt = compound(FALSE);

    stmt = joinsnode(autohead, stmt);

    autohead = autotail = NULL;

    if (lc2_auto > lc_auto)
        lc_auto = lc2_auto;

    cseg();

    gen_strlab(sp->name);

#if 0
    if (sp->storage_class == sc_static) {
        if (sp->value.i > 0)
            gen_label(sp->value.i);
        else
            error( ERR_SYNTAX, "invalid static declaration" );
    }
#endif

    if (!fatal)
        genfunc(stmt);

    flush_peep();

    getsym();
}
