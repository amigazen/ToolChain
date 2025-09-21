
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
 * $Log:	GenStmt.c,v $
 * Revision 3.33  90/04/05  23:07:37  lionel
 * Changed parameter convention to call_library to avoid name collisions.
 * Added optional entry and exit routines that receive function name and stack usage as parameters.
 * 
 * Revision 3.32  90/02/03  16:24:20  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * GenStmt.c Generates the assembly code.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    <stab.h>

int             breaklab;
int             contlab;
int             retlab;
int             fnamelab;

extern TYP      stdfunc;
extern struct amode push[], pop[];
extern struct enode *makenode();
extern struct amode *make_label();
extern struct amode *make_offset();
extern struct amode *make_immed();
extern struct amode *gen_expr();
extern struct snode *makesnode();
extern SYM     *gsearch();
extern char    *itoa(), *litlate(), *in_line;
extern char    *xalloc();

void   genstmt();

struct amode   *
makedreg(r)

/*
 * make an address reference to a data register.
 */
    enum e_am       r;
{
    struct amode   *ap;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_dreg;
    ap->preg = r;
    return ap;
}

struct amode   *
makeareg(r)

/*
 * make an address reference to an address register.
 */
    enum e_am       r;
{
    struct amode   *ap;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_areg;
    ap->preg = r;
    return ap;
}

struct amode   *
make_mask(mask)

/*
 * generate the mask address structure.
 */
    int             mask;
{
    struct amode   *ap;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_mask;
    ap->offset = (struct enode *) mask;
    return ap;
}

struct amode   *
make_direct(i)

/*
 * make a direct reference to an immediate value.
 */
    char           *i;
{
    return make_offset(makenode(en_icon, i, NULL));
}

struct amode   *
make_intrlab(s)

/*
 * generate a direct reference to an intrinsic label.
 */
    char           *s;
{
    struct amode   *ap;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_direct;
    ap->offset = makenode(en_intrlab, s, NULL);
    return ap;
}

struct amode   *
make_strlab(s)

/*
 * generate a direct reference to a string label.
 */
    char           *s;
{
    struct amode   *ap;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_direct;
    ap->offset = makenode(en_nacon, s, NULL);
    return ap;
}

int
numlines(stmt)          /* Return 0, 1, 2, or 3 for number */

/*
 *  Return the number of non-comment statements with a max of 3
 */
    struct snode   *stmt;
{
    int             count = 0;

    while (stmt != NULL) {
        if (stmt->stype != st_comment) {
            count++;
            if (count > 2)
                return (count);
        }
        stmt = stmt->next;
    }
    return (count);
}

void
gencomment(stmt)

/*
 * Generate a comment line
 */
    struct snode   *stmt;
{
    struct amode   *ap;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_direct;
    ap->offset = stmt->exp;
    gen_code(op_comment, 0, ap, NULL);
}

void
genasm(stmt)

/*
 * Generate a assembly line
 */
    struct snode   *stmt;
{
    struct amode   *ap;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_direct;
    ap->offset = stmt->exp;
    gen_code(op_asm, 0, ap, NULL);
}

void
genstabn(stmt)

/*
 * Generate a stabn statement
 */
    struct snode   *stmt;
{
#ifdef GENERATE_DBX
    struct amode   *ap;
    struct dnode   *dp;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    dp = stmt->exp->v.dp;
    ap->mode = am_direct;
    ap->offset = makenode(en_stabn, (struct enode *) dp, NULL);
    gen_code(op_stabn, 0, ap, NULL);
#endif
}

void
genstabs(stmt)

/*
 * Generate a stabs statement
 */
    struct snode   *stmt;
{
#ifdef GENERATE_DBX
    struct amode   *ap;
    struct dnode   *dp;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    dp = stmt->exp->v.dp;
    ap->mode = am_direct;
    ap->offset = makenode(en_stabs, (struct enode *) dp, NULL);
    gen_code(op_stabs, 0, ap, NULL);
#endif
}

void
genwhile(stmt)

/*
 * generate code to evaluate a while statement.
 */
    struct snode   *stmt;
{
    int             lab1, lab2;
    int             determine, cond;

    initstack();        /* initialize temp registers */
    lab1 = contlab;     /* save old continue label */
    lab2 = breaklab;    /* save old break label */
    contlab = nextlabel++;  /* new continue label */


    determine = cond = FALSE;

    if (Options.Optimize) { /* Check for constant condition */
        opt4(&stmt->exp);
        if (stmt->exp != NULL && stmt->exp->nodetype == en_icon) {
            determine = 1;
            cond = (stmt->exp->v.i != 0);
        }
    }

    if (determine && !cond) /* Condition is always false */
        return;

    gen_label(contlab);

    if (numlines(stmt->s1) > 0) {   /* has block */
        breaklab = nextlabel++;
        initstack();
        if (!determine || !cond)
            falsejp(stmt->exp, breaklab);
        genstmt(stmt->s1);
        gen_code(op_bra, 0, make_label(contlab), NULL);
        gen_label(breaklab);
        breaklab = lab2;/* restore old break label */
    }
    else {          /* no loop code */
        initstack();
        if (!determine || !cond)
            truejp(stmt->exp, contlab);
    }
    contlab = lab1;     /* restore old continue label */
}

void
gen_for(stmt)

/*
 * generate code to evaluate a for loop
 */
    struct snode   *stmt;
{
    int             old_break, old_cont, exit_label, loop_label, size;

    old_break = breaklab;
    old_cont = contlab;
    loop_label = nextlabel++;
    exit_label = nextlabel++;
    contlab = nextlabel++;
    initstack();
    if (stmt->label != NULL) {
        size = natural_size(stmt->label);
        gen_expr(stmt->label, F_ALL | F_NOVALUE | F_FREG, size );
    }
    gen_label(loop_label);
    initstack();
    if (stmt->exp != NULL)
        falsejp(stmt->exp, exit_label);
    if (numlines(stmt->s1) > 0) {
        breaklab = exit_label;
        genstmt(stmt->s1);
    }
    initstack();
    gen_label(contlab);
    if (stmt->s2 != NULL) {
        size = natural_size( stmt->s2 );
        gen_expr(stmt->s2, F_ALL | F_NOVALUE | F_FREG, size );
    }
    gen_code(op_bra, 0, make_label(loop_label), NULL);
    breaklab = old_break;
    contlab = old_cont;
    gen_label(exit_label);
}

void
genif(stmt)

/*
 * generate code to evaluate an if statement.
 */
    struct snode   *stmt;
{
    int             lab1, lab2;
    int             determine, cond;

    lab1 = nextlabel++; /* else label */
    lab2 = nextlabel++; /* exit label */

    determine = cond = FALSE;

    if (Options.Optimize) { /* Check for constant conditions */
        opt4(&stmt->exp);
        if (stmt->exp != NULL && (stmt->exp->nodetype == en_icon)) {
            determine = 1;
            cond = (stmt->exp->v.i != 0);
        }
    }

    initstack();        /* clear temps */

    if (!determine)
        falsejp(stmt->exp, lab1);

    if (!determine || cond)
        genstmt(stmt->s1);

    if (numlines(stmt->s2) > 0) {   /* else part exists */
        if (!determine) {
            gen_code(op_bra, 0, make_label(lab2), NULL);
            gen_label(lab1);
        }
        if (!determine || !cond)
            genstmt(stmt->s2);
        if (!determine)
            gen_label(lab2);
    }
    else {          /* no else code */
        if (!determine)
            gen_label(lab1);
    }
}

void
gendo(stmt)

/*
 * generate code for a do - while loop.
 */
    struct snode   *stmt;
{
    int             oldcont, oldbreak;
    int             determine, cond;

    oldcont = contlab;
    oldbreak = breaklab;
    contlab = nextlabel++;
    gen_label(contlab);

    determine = cond = FALSE;

    if (Options.Optimize) {
        opt4(&stmt->exp);
        if (stmt->exp != NULL && stmt->exp->nodetype == en_icon) {
            determine = 1;
            cond = (stmt->exp->v.i != 0);
        }
    }

    if (numlines(stmt->s1) > 1) {
        breaklab = nextlabel++;
        genstmt(stmt->s1);  /* generate body */
        initstack();
        if (!determine)
            truejp(stmt->exp, contlab);
        else if (cond)
            gen_code(op_bra, 0, make_label(contlab), NULL);
        gen_label(breaklab);
    }
    else {
        genstmt(stmt->s1);
        initstack();
        if (!determine)
            truejp(stmt->exp, contlab);
        else if (cond)
            gen_code(op_bra, 0, make_label(contlab), NULL);
    }
    breaklab = oldbreak;
    contlab = oldcont;
}

void
call_library(lib_name)

/*
 * generate a call to a library routine.
 */
    char           *lib_name;
{
    SYM            *sp;

    sp = gsearch(lib_name);
    if (sp == NULL) {
        ++global_flag;
        sp = (SYM *) xalloc(sizeof(SYM));
        sp->tp = &stdfunc;
        sp->name = lib_name;
        sp->storage_class = sc_external;
        sp->storage_type = sc_library;
        sp->value.i = 1;		/* Make it referenced   */
        insert(sp, &gsyms);
        --global_flag;
    }
    gen_code(op_jsr, 0, make_intrlab(lib_name), NULL);
}

void
genswitch(stmt)

/*
 * generate a linear search switch statement.
 */
    struct snode   *stmt;
{
    int             curlab;
    struct snode   *defcase;
    struct amode   *ap;

    curlab = nextlabel++;
    defcase = NULL;
    initstack();
    ap = gen_expr(stmt->exp, F_DREG | F_VOL, 4);
    if (ap->preg != am_none)
        gen_code(op_move, 4, ap, makedreg(am_dreg));
    stmt = stmt->s1;

    call_library(".cswitch");

    while (numlines(stmt) > 0) {
        if (stmt->s2 != NULL) { /* default case ? */
            stmt->label = (long *) curlab;
            defcase = stmt;
        }
        else {
            gen_code(op_dc, 4, make_label(curlab),
                 make_direct(stmt->label));
            stmt->label = (long *) curlab;
        }
        if (numlines(stmt->s1) > 0 && numlines(stmt->next) > 0)
            curlab = nextlabel++;
        stmt = stmt->next;
    }

    if (defcase == NULL)
        gen_code(op_dc, 4, make_direct(NULL), make_label(breaklab));
    else
        gen_code(op_dc, 4, make_direct(NULL), make_label((int) (defcase->label)));
}

int
bswitch(list)

/*
 * bswitch implements a bubble sort on the statement list.
 */
    struct snode  **list;
{
    int             rc;
    struct snode   *sp1, *sp2;

    if (list == NULL)
        return 0;
    sp1 = *list;
    if (sp1 == NULL || sp1->next == NULL)
        return 0;
    rc = bswitch(&(sp1->next));
    sp2 = sp1->next;
    if (sp1->value > sp2->value || sp2->s2 != NULL) {
        sp1->next = sp2->next;
        sp2->next = sp1;
        *list = sp2;
        return 1;
    }
    return rc;
}

void
gendirswitch(stmt, minval, maxval)

/*
 * generate a direct table lookup switch statement.
 */
    struct snode   *stmt;
    int             minval, maxval;
{
    int             curlab, num, tablab;
    struct snode   *defcase, *st2, *st3;
    struct amode   *ap, *ap1, *ap2, *ap3;

    defcase = NULL;
    tablab = nextlabel++;
    curlab = nextlabel++;
    initstack();

    st3 = NULL;
    for (st2 = stmt->s1; numlines(st2) > 0; st2 = st2->next) {
        if (st2->stype == st_case) {
            st3 = st2;  /* Save the last case    */
            if (st2->s2 != NULL) {  /* Default case                 */
                st2->label = (long *) curlab;
                defcase = st2;
            }
            else {
                st2->value = (int) st2->label;
                st2->label = (long *) curlab;
            }
            if (numlines(st2->s1) > 0 && numlines(st2->next) > 0)
                curlab = nextlabel++;
        }
    }

    ap = gen_expr(stmt->exp, F_DREG, 4);

    if (ap == NULL)
        return;

    make_legal(ap, F_DREG, 4);
    ap1 = makedreg(am_dreg);

    if ((int) ap->preg != 0)
        gen_code(op_move, 4, ap, ap1);

    /*
     * Test for the lower limit
     */

    gen_code(op_cmp, 4, ap1, make_immed(minval));
    if (defcase == NULL)
        gen_code(op_blt, 0, make_label(breaklab), NULL);
    else
        gen_code(op_blt, 0, make_label((int) defcase->label), NULL);

    /*
     * Test for the upper limit
     */
    if (minval != 0)
        gen_code(op_sub, 4, make_immed(minval), ap1);
    gen_code(op_cmp, 4, ap1, make_immed(maxval - minval));

    if (defcase == NULL)
        gen_code(op_bgt, 0, make_label(breaklab), NULL);
    else
        gen_code(op_bgt, 0, make_label((int) defcase->label), NULL);

    /*
     * Get the address of the routine to to execute
     */
    gen_code(op_asl, 4, make_immed(2), ap1);

    ap2 = makeareg((enum e_am) 0);
    ap3 = makeareg((enum e_am) 1);
    gen_code(op_lea, 0, make_label(tablab), ap2);

    ap2->mode = am_indx2;
    ap2->sreg = am_dreg;
    ap2->offset = makenode(en_icon, 0, NULL);
    gen_code(op_move, 4, ap2, ap3);

    ap3->mode = am_ind;
    gen_code(op_jmp, 0, ap3, NULL);

    /*
     * Make the jump table
     */

    gen_label(tablab);

    for (num = minval; num <= maxval; num++) {
        st3 = NULL;
        for (st2 = stmt->s1; st2 != NULL; st2 = st2->next) {
            if (st2->stype == st_case && st2->s2 == NULL) {
                if (st2->value == num) {
                    st3 = st2;
                    break;
                }
            }
        }
        if (st3 != NULL) {
            gen_code(op_dc, 4, make_label((int) st3->label), NULL);
            st3->s2 = (struct snode *) 2;
        }
        else {
            if (defcase == NULL)
                gen_code(op_dc, 4, make_label(breaklab), NULL);
            else
                gen_code(op_dc, 4, make_label((int) defcase->label), NULL);
        }
    }
    gen_code(op_dc, 4, make_direct(0), NULL);
}

void
gencase(stmt)

/*
 * generate all cases for a switch statement.
 */
    struct snode   *stmt;
{
    while (stmt != NULL) {
        if (stmt->stype == st_comment)
            genstmt(stmt);
        else {
            if (numlines(stmt->s1) > 0) {
                gen_label((int) (stmt->label));
                genstmt(stmt->s1);
            }
        }
        stmt = stmt->next;
    }
}

void
genxswitch(stmt)

/*
 * analyze and generate best switch statement.
 */
    struct snode   *stmt;
{
    int             oldbreak;
    int             minval, maxval, numval, val;
    struct snode   *caselab;

    oldbreak = breaklab;
    breaklab = nextlabel++;

    numval = 0;
    caselab = stmt->s1;

    while (numlines(caselab) > 0) {
        if (caselab->s2 == NULL) {
            val = (int) caselab->label;
            if (numval == 0) {
                minval = maxval = val;
                ++numval;
            }
            else {
                if (val > maxval)
                    maxval = val;
                if (val < minval)
                    minval = val;
                ++numval;
            }
        }
        caselab = caselab->next;
    }

    if ((maxval - minval) < 4 * numval && (numval > 4))
        gendirswitch(stmt, minval, maxval);
    else
        genswitch(stmt);

    gencase(stmt->s1);
    gen_label(breaklab);
    breaklab = oldbreak;
}

void
genreturn(stmt)

/*
 * generate a return statement.
 */
    struct snode   *stmt;
{
    struct amode   *ap;
    struct enode   *ep;

    if (stmt != NULL && stmt->exp != NULL) {
        initstack();
        ep = stmt->exp;
        if (lastfunc->tp->btp->type == bt_double) {
            ap = gen_expr(ep, F_FREG, 8);
            make_legal(ap, F_FREG, 8);
        }
        else {
            ap = gen_expr(ep, F_ALL, 4);
            if (ap->mode == am_immed && ap->offset->nodetype != en_icon)
                make_legal(ap, F_AREG, 4);
            if (ap->mode != am_dreg || ap->preg != am_none)
                gen_code(op_move, 4, ap, makedreg(am_dreg));
        }
    }
    if (retlab == -1) {
        retlab = nextlabel++;
        gen_label(retlab);

        if (save_mask != 0)
            gen_code(op_movem, 4, pop, make_mask(save_mask));
        gen_code(op_unlk, 0, makeareg((enum e_am) Options.Frame), NULL);

        if (Options.Stack) {
            if ( fnamelab == -1 ) {
                ap = make_label( stringlit( lastfunc->name) );
            }
            else {
                ap = make_label( fnamelab );
            }
            gen_code(op_lea, 4, ap, makeareg((enum e_am) 0));
            call_library( ".exit" );
        }

        gen_code(op_rts, 0, NULL, NULL);
    }
    else
        gen_code(op_bra, 0, make_label(retlab), NULL);
}

void
genstmt(stmt)

/*
 * genstmt will generate a statement and follow the next pointer until the
 * block is generated.
 */
    struct snode   *stmt;
{
    while (stmt != NULL) {
        initstack();
        switch (stmt->stype) {
        case st_comment:
            if (Options.Annote)
                gencomment(stmt);
            break;
        case st_stabn:
#ifdef GENERATE_DBX
            if (Options.Debug)
                genstabn(stmt);
#endif
            break;
        case st_stabs:
#ifdef GENERATE_DBX
            if (Options.Debug)
                genstabs(stmt);
#endif
            break;
        case st_asm:
            genasm(stmt);
            break;
        case st_label:
            gen_label((int) (stmt->label));
            break;
        case st_goto:
            gen_code(op_bra, 0, make_label((int) (stmt->label)), NULL);
            break;
        case st_expr:
            gen_expr(stmt->exp, F_ALL | F_NOVALUE | F_FREG,
                 natural_size(stmt->exp));
            break;
        case st_return:
            genreturn(stmt);
            break;
        case st_if:
            genif(stmt);
            break;
        case st_while:
            genwhile(stmt);
            break;
        case st_for:
            gen_for(stmt);
            break;
        case st_continue:
            gen_code(op_bra, 0, make_label(contlab), NULL);
            break;
        case st_break:
            gen_code(op_bra, 0, make_label(breaklab), NULL);
            break;
        case st_switch:
            genxswitch(stmt);
            break;
        case st_do:
            gendo(stmt);
            break;
        default:
            fprintf( stderr, "DIAG - unknown statement.\n" );
            break;
        }
        stmt = stmt->next;
    }
    initstack();
}

void
genfunc(stmt)

/*
 * generate a function body.
 */
    struct snode   *stmt;
{
    int             autolabel, maxlabel;
    struct amode   *ap;
    struct amode   *ap2;
    struct enode   *lnode;

    retlab = contlab = breaklab = fnamelab = -1;

    autolabel = nextlabel++;
    maxlabel = nextlabel++;

    if (lc_auto & 1)    /* if frame size odd */
        ++lc_auto;  /* make it even */

    float_auto = 0;     /* No floating point temporaries */

    if (Options.Stack) {
        ap = (struct amode *) xalloc(sizeof(struct amode));
        ap->mode = am_immed;
        ap->offset = (struct enode *) xalloc(sizeof(struct enode));
        ap->offset->nodetype = en_labcon;
        ap->offset->v.i = maxlabel;
        
        fnamelab = stringlit( lastfunc->name );
        ap2 = make_label( fnamelab );

        gen_code(op_move, 4, ap, makedreg((enum e_am) 0));
        gen_code(op_lea, 4, ap2, makeareg((enum e_am) 0));
        call_library( ".entry" );
    }

    lnode = (struct enode *) xalloc(sizeof(struct enode));
    lnode->nodetype = en_labcon;
    lnode->v.i = autolabel;
    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_immed;
    ap->offset = lnode;

    gen_code(op_link, 0, makeareg((enum e_am) Options.Frame), ap);

    opt1(stmt);
    genstmt(stmt);

    initstack();

    genreturn(NULL);

    nl();

    if (Options.Stack) {
        fprintf( output, "L%d\tEQU\t%d\n", maxlabel , maxparmsize+lc_auto );
    }

    fprintf( output, "L%d\tEQU\t%d\n", autolabel , -lc_auto );
}
