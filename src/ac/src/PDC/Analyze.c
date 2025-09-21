
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
 * $Log:	Analyze.c,v $
 * Revision 3.33  90/04/04  02:25:11  lionel
 * Added unsigned multiply, divide, and modulo expression handling.
 * 
 * Revision 3.32  90/02/03  16:22:48  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Analyze.c
 * 
 * This module will step through the parse tree and find all optimizable
 * expressions. at present these expressions are limited to expressions that
 * are valid throughout the scope of the function. the list of optimizable
 * expressions is:
 * 
 * Constants, global and static addresses, auto addresses, contents of auto
 * addresses.
 * 
 * Contents of auto addresses are valid only if the address is never referred to
 * without dereferencing.
 * 
 * Scan will build a list of optimizable expressions which opt1 will replace
 * during the second optimization pass.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"
#include        <stab.h>

extern struct amode push[], pop[];
extern struct amode *gen_expr(), *makedreg(), *makeareg(), *make_mask();

#ifdef  GENERATE_DBX
extern char    *dbx_addreg();

#endif
extern char    *xalloc();

static struct cse *olist;   /* list of optimizable expressions */

int
equalnode(register struct enode *node1, register struct enode *node2)

/*
 * equalnode will return 1 if the expressions pointed to by node1 and node2
 * are equivalent.
 */
{
    if (node1 == NULL || node2 == NULL)
        return FALSE;

    if (node1->nodetype != node2->nodetype)
        return FALSE;

    if (node1->v.i == node2->v.i) {
        if (node1->nodetype == en_icon || node1->nodetype == en_labcon ||
        node1->nodetype == en_nacon || node1->nodetype == en_autocon)
            return TRUE;
    }

    if (lvalue(node1) && equalnode(node1->v.p[0], node2->v.p[0]))
        return TRUE;

    return FALSE;
}

struct cse     *
searchnode(register struct enode *node)

/*
 * searchnode will search the common expression table for an entry that
 * matches the node passed and return a pointer to it.
 */
{
    register struct cse *csp;

    for (csp = olist; csp != NULL; csp = csp->next)
        if (equalnode(node, csp->exp))
            return csp;

    return NULL;
}

struct enode   *
copynode(struct enode *node)

/*
 * copy the node passed into a new enode so it wont get corrupted during
 * substitution.
 */
{
    struct enode   *temp;

    if (node == NULL)
        return NULL;
    temp = (struct enode *) xalloc(sizeof(struct enode));
    temp->nodetype = node->nodetype;
    temp->v.p[0] = node->v.p[0];
    temp->v.p[1] = node->v.p[1];
    return temp;
}

struct cse     *
enternode(struct enode *node, int duse)

/*
 * enternode will enter a reference to an expression node into the common
 * expression table. duse is a flag indicating whether or not this reference
 * will be dereferenced.
 */
{
    struct cse     *csp;

    if ((csp = searchnode(node)) == NULL) { /* add to tree */
        csp = (struct cse *) xalloc(sizeof(struct cse));
        csp->next = olist;
        csp->uses = 1;
        csp->duses = (duse != 0);
        csp->exp = copynode(node);
        csp->voidf = FALSE;
        olist = csp;
        return csp;
    }
    ++(csp->uses);
    if (duse)
        ++(csp->duses);
    return csp;
}

struct cse     *
voidauto(struct enode *node)

/*
 * voidauto will void an auto dereference node which points to the same auto
 * constant as node.
 */
{
    struct cse     *csp;

    csp = olist;
    while (csp != NULL) {
        if (lvalue(csp->exp) && equalnode(node, csp->exp->v.p[0])) {
            if (csp->voidf)
                return NULL;
            csp->voidf = TRUE;
            return csp;
        }
        csp = csp->next;
    }
    return NULL;
}

void
scanexpr(struct enode *node, int duse)

/*
 * scanexpr will scan the expression pointed to by node for optimizable
 * subexpressions. when an optimizable expression is found it is entered into
 * the tree. if a reference to an autocon node is scanned the corresponding
 * auto dereferenced node will be voided. duse should be set if the
 * expression will be dereferenced.
 */
{
    struct cse     *csp, *csp1;

    if (node == NULL)
        return;

    switch (node->nodetype) {
    case en_icon:
    case en_labcon:
    case en_nacon:
        enternode(node, duse);
        break;
    case en_autocon:
        if ((csp = voidauto(node)) != NULL) {
            csp1 = enternode(node, duse);
            csp1->uses = (csp1->duses += csp->uses);
        }
        else
            csp1 = enternode(node, duse);
        csp1->voidf = TRUE;
        break;
    case en_b_ref:
    case en_ub_ref:
    case en_w_ref:
    case en_uw_ref:
    case en_l_ref:
    case en_ul_ref:
        if (node->v.p[0]->nodetype == en_autocon) {
            csp = enternode(node, 1);
            csp1 = searchnode(node->v.p[0]);
            if (csp1 != NULL && csp1->voidf)
                csp1 = voidauto(node->v.p[0]);
        }
        else
            scanexpr(node->v.p[0], 1);
        break;

    case en_cbl:
    case en_cbw:
    case en_cwl:
    case en_clf:
    case en_cld:
    case en_cfd:
    case en_cfl:
    case en_cdl:
    case en_cdf:

    case en_uminus:
    case en_compl:
    case en_not:
    case en_info:
        scanexpr(node->v.p[0], duse);
        break;

    case en_ainc:
    case en_adec:
    case en_faincs:
    case en_fadecs:
    case en_faincd:
    case en_fadecd:
        scanexpr(node->v.p[0], 0);
        scanexpr(node->v.p[1], 0);
        break;

    case en_asadd:
    case en_assub:
    case en_add:
    case en_sub:
    case en_mul:
    case en_umul:
    case en_div:
    case en_udiv:
    case en_lsh:
    case en_rsh:
    case en_mod:
    case en_umod:
    case en_and:
    case en_or:
    case en_xor:
    case en_lor:
    case en_land:
    case en_eq:
    case en_ne:
    case en_gt:
    case en_ugt:
    case en_ge:
    case en_uge:
    case en_lt:
    case en_ult:
    case en_le:
    case en_ule:
    case en_asmul:
    case en_asdiv:
    case en_asmod:
    case en_asumul:
    case en_asudiv:
    case en_asumod:
    case en_aslsh:
    case en_asrsh:
    case en_asand:
    case en_asor:
    case en_aseor:
    case en_cond:
    case en_void:
    case en_assign:
        scanexpr(node->v.p[0], 0);
        scanexpr(node->v.p[1], 0);
        break;
    case en_fcall:
        if (node->v.p[0]->nodetype == en_nacon) {
            if (strncmp(node->v.p[0]->v.sp, "__BUILTIN_", 10) == 0) {
                scanexpr(node->v.p[1], 0);
                return;
            }
            if (strncmp(node->v.p[0]->v.sp, "__LIBCALL_", 10) == 0) {
                scanexpr(node->v.p[1], 0);
                return;
            }
        }
        scanexpr(node->v.p[0], 1);
        scanexpr(node->v.p[1], 0);
        break;

    case en_fnegs:
    case en_fadds:
    case en_fsubs:
    case en_fmuls:
    case en_fdivs:
    case en_fmods:

    case en_fnegd:
    case en_faddd:
    case en_fsubd:
    case en_fmuld:
    case en_fdivd:
    case en_fmodd:
    case en_d_ref:
    case en_f_ref:
    case en_m_ref:
        break;
    }
}

void
scan(struct snode *block)

/*
 * scan will gather all optimizable expressions into the expression list for
 * a block of statements.
 */
{
    while (block != NULL) {
        switch (block->stype) {
        case st_return:
        case st_expr:
            opt4(&block->exp);
            scanexpr(block->exp, 0);
            break;
        case st_while:
        case st_do:
            opt4(&block->exp);
            scanexpr(block->exp, 0);
            scan(block->s1);
            break;
        case st_for:
            opt4(&block->label);
            scanexpr((struct enode *)block->label, 0);
            opt4(&block->exp);
            scanexpr(block->exp, 0);
            scan(block->s1);
            opt4(&block->s2);
            scanexpr((struct enode *)block->s2, 0);
            break;
        case st_if:
            opt4(&block->exp);
            scanexpr(block->exp, 0);
            scan(block->s1);
            scan(block->s2);
            break;
        case st_switch:
            opt4(&block->exp);
            scanexpr(block->exp, 0);
            scan(block->s1);
            break;
        case st_case:
            scan(block->s1);
            break;
        case st_goto:
        case st_label:
        case st_break:
        case st_continue:
        case st_comment:
            break;
        }
        block = block->next;
    }
}

void
exchange(struct cse **c1)

/*
 * exchange will exchange the order of two expression entries following c1 in
 * the linked list.
 */
{
    struct cse     *csp1, *csp2;

    csp1 = *c1;
    csp2 = csp1->next;
    csp1->next = csp2->next;
    csp2->next = csp1;
    *c1 = csp2;
}

int
desire(struct cse *csp)

/*
 * returns the desirability of optimization for a subexpression.
 */
{
    struct enode   *ep1;

    if (csp->voidf)
        return 0;

    ep1 = csp->exp;

    if (ep1->nodetype == en_icon && ep1->v.i < 16 && ep1->v.i >= 0)
        return (0);

    if (lvalue(csp->exp))
        return 2 * csp->uses;

    return csp->uses;
}

int
bsort(struct cse **list)

/*
 * bsort implements a bubble sort on the expression list.
 */
{
    int             rc;
    struct cse     *csp1, *csp2;

    csp1 = *list;
    if (csp1 == NULL || csp1->next == NULL)
        return 0;
    rc = bsort(&(csp1->next));
    csp2 = csp1->next;
    if (desire(csp1) < desire(csp2)) {
        exchange(list);
        return 1;
    }
    return rc;
}

void
allocate(void)

/*
 * allocate will allocate registers for the expressions that have a high
 * enough desirability.
 */
{
    struct cse     *csp;
    struct enode   *exptr;
    int             datareg, addreg, mask;
    struct amode   *ap, *ap2;

    datareg = 3;
    addreg = 10;
    mask = 0;
    while (bsort(&olist));  /* sort the expression list */
    for (csp = olist; csp != NULL; csp = csp->next) {
        csp->reg = -1;
        if (desire(csp) >= 3) {
            if (csp->duses > (csp->uses / 5)) {
                if (lvalue(csp->exp) && datareg < 8)
                    csp->reg = datareg++;
                else if (addreg < 8 + Options.Frame)
                    csp->reg = addreg++;
                else if (datareg < 8)
                    csp->reg = datareg++;
            }
        }
        if (csp->reg != -1)
            mask = mask | (1 << csp->reg);
    }

    if (mask != 0)
        gen_code(op_movem, 4, make_mask(mask), push);

    save_mask = mask;

    for (csp = olist; csp != NULL; csp = csp->next) {
        if (csp->reg != -1) {   /* see if preload needed */
            exptr = csp->exp;
            if (!lvalue(exptr) || (exptr->v.p[0]->v.i > 0)) {
                initstack();
                ap = gen_expr(exptr, F_ALL, 4);
                if (csp->reg < 8) {
                    ap2 = makedreg((enum e_am) (csp->reg));
                    if (ap->mode == am_immed)
                        make_legal(ap, F_AREG, 4);
                    gen_code(op_move, 4, ap, ap2);
                }
                else {
                    ap2 = makeareg((enum e_am) (csp->reg - 8));
                    gen_code(op_move, 4, ap, ap2);
                }
                freeop(ap);
            }
        }
    }
}

void
repexpr(struct enode *node)

/*
 * repexpr will replace all allocated references within an expression with
 * tempref nodes.
 */
{
    struct cse     *csp;

    if (node == NULL)
        return;
    switch (node->nodetype) {
    case en_icon:
    case en_nacon:
    case en_labcon:
    case en_autocon:
        if ((csp = searchnode(node)) != NULL)
            if (csp->reg > 0) {
                node->nodetype = en_tempref;
                node->v.i = csp->reg;
            }
        break;
    case en_stabs:
    case en_stabn:
        if (node->v.dp != NULL && node->v.dp->ref != NULL) {
            if ((csp = searchnode(node->v.dp->ref)) != NULL) {
                if (csp->reg > 0) {
                    node->v.dp->tag = N_RSYM;
#ifdef  GENERATE_DBX
                    node->v.dp->sp = dbx_addreg(node->v.dp->sp);
#endif
                    node->v.dp->ref->nodetype = en_tempref;
                    node->v.dp->ref->v.i = csp->reg;
                }
            }
        }
        break;
    case en_b_ref:
    case en_ub_ref:
    case en_w_ref:
    case en_uw_ref:
    case en_l_ref:
    case en_ul_ref:
    case en_m_ref:
    case en_f_ref:
    case en_d_ref:
        if ((csp = searchnode(node)) != NULL) {
            if (csp->reg > 0) {
                node->nodetype = en_tempref;
                node->v.i = csp->reg;
            }
            else
                repexpr(node->v.p[0]);
        }
        else
            repexpr(node->v.p[0]);
        break;

    case en_cbl:
    case en_cbw:
    case en_cwl:
    case en_clf:
    case en_cld:
    case en_cfd:
    case en_cfl:
    case en_cdl:
    case en_cdf:

    case en_uminus:
    case en_not:
    case en_compl:
    case en_info:
        repexpr(node->v.p[0]);
        break;

    case en_ainc:
    case en_adec:
    case en_faincs:
    case en_fadecs:
    case en_faincd:
    case en_fadecd:
        repexpr(node->v.p[0]);
        repexpr(node->v.p[1]);
        break;

    case en_fnegs:
    case en_fnegd:
        repexpr(node->v.p[0]);
        break;

    case en_add:
    case en_sub:
    case en_mul:
    case en_umul:
    case en_div:
    case en_udiv:
    case en_mod:
    case en_umod:
    case en_lsh:
    case en_rsh:
    case en_and:
    case en_or:
    case en_xor:
    case en_land:
    case en_lor:
    case en_eq:
    case en_ne:
    case en_lt:
    case en_ult:
    case en_le:
    case en_ule:
    case en_gt:
    case en_ugt:
    case en_ge:
    case en_uge:
    case en_cond:
    case en_void:
    case en_asadd:
    case en_assub:
    case en_asmul:
    case en_asdiv:
    case en_asmod:
    case en_asumul:
    case en_asudiv:
    case en_asumod:
    case en_asor:
    case en_asand:
    case en_aslsh:
    case en_asrsh:
    case en_aseor:
    case en_fcall:
    case en_assign:

    case en_fadds:
    case en_fsubs:
    case en_fmuls:
    case en_fdivs:
    case en_fmods:

    case en_faddd:
    case en_fsubd:
    case en_fmuld:
    case en_fdivd:
    case en_fmodd:

        repexpr(node->v.p[0]);
        repexpr(node->v.p[1]);
        break;
    }
}

void
repcse(struct snode *block)

/*
 * repcse will scan through a block of statements replacing the optimized
 * expressions with their temporary references.
 */
{
    while (block != NULL) {
        switch (block->stype) {
        case st_stabn:
        case st_stabs:
            repexpr(block->exp);
            break;
        case st_return:
        case st_expr:
            repexpr(block->exp);
            break;
        case st_while:
        case st_do:
            repexpr(block->exp);
            repcse(block->s1);
            break;
        case st_for:
            repexpr((struct enode *)block->label);
            repexpr(block->exp);
            repcse(block->s1);
            repexpr((struct enode *)block->s2);
            break;
        case st_if:
            repexpr(block->exp);
            repcse(block->s1);
            repcse(block->s2);
            break;
        case st_switch:
            repexpr(block->exp);
            repcse(block->s1);
            break;
        case st_case:
            repcse(block->s1);
            break;
        case st_goto:
        case st_label:
        case st_break:
        case st_continue:
        case st_comment:
            break;
        }
        block = block->next;
    }
}

void
opt1(struct snode *block)

/*
 * opt1 is the externally callable optimization routine. it will collect and
 * allocate common subexpressions and substitute the tempref for all
 * occurrances of the expression within the block.
 * 
 */
{
    if (Options.Optimize) {
        olist = NULL;
        scan(block);    /* collect expressions */
        allocate(); /* allocate registers */
        repcse(block);  /* replace allocated expressions */
    }
}
