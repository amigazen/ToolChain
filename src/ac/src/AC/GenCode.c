
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
 * $Log:	GenCode.c,v $
 * Revision 3.33  90/04/05  22:30:42  lionel
 * Changed library function callout to avoid collisions with user functions.
 * Added generation of unsigned *, /, and % expressions.
 * 
 * Revision 3.32  90/02/03  16:23:57  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * GenCode.c
 * 
 * This module contains all of the code generation routines for evaluating
 * expressions and conditions.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

extern char    *xalloc();
extern TYP      stdint;

#define MAX_SHIFT   8

SYM            *gsearch();

struct amode   *temp_data(), *temp_addr(), *makeareg(), *makedreg();
struct amode   *copy_addr(), *temp_float();
struct amode   *request_addr(), *request_data(), *request_float();
struct amode   *request_reg();
struct enode   *makenode();

struct amode   *gen_fsconvert(), *gen_fconvert();
struct amode   *gen_fsbinary(), *gen_fbinary();
struct amode   *gen_fsunary(), *gen_funary();
struct amode   *gen_fsaincdec(), *gen_faincdec();

struct amode   *gen_stabn(), *gen_stabs();

extern long floatlit();
extern struct amode push[], pop[];

struct amode   *gen_expr();                          /* forward declaration */
void            swap_nodes(), truejp(), falsejp();   /* ditto               */

int
istemp(ap)
    struct amode   *ap;
{
    if (ap->mode == am_areg && (int) ap->preg <= 1)
        return TRUE;
    else if (ap->mode == am_dreg && (int) ap->preg <= 2)
        return TRUE;
    return FALSE;
}

struct amode   *
make_label(lab)

/*
 * construct a reference node for an internal label number.
 */
    int             lab;
{
    struct enode   *lnode;
    struct amode   *ap;

    lnode = (struct enode *) xalloc(sizeof(struct enode));
    lnode->signedflag = 0;
    lnode->nodetype = en_labcon;
    lnode->v.i = lab;
    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->signedflag = 0;
    ap->mode = am_direct;
    ap->offset = lnode;
    return ap;
}

struct amode   *
make_immed(i)

/*
 * make a node to reference an immediate value i.
 */
    long            i;
{
    struct amode   *ap;
    struct enode   *ep;

    ep = (struct enode *) xalloc(sizeof(struct enode));
    ep->signedflag = 1;
    ep->nodetype = en_icon;
    ep->v.i = i;
    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->signedflag = 1;
    ap->mode = am_immed;
    ap->offset = ep;
    return ap;
}

struct amode   *
make_offset(node)

/*
 * make a direct reference to a node.
 */
    struct enode   *node;
{
    struct amode   *ap;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in make_offset.\n" );
        return NULL;
    }

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->signedflag = node->signedflag;
    ap->mode = am_direct;
    ap->offset = node;
    return ap;
}

struct amode   *
make_delta(ap1, delta)
    struct amode   *ap1;
    int             delta;
{
    if (ap1 == NULL || delta == 0)
        return (ap1);

    switch (ap1->mode) {
    case am_immed:
        ap1 = make_offset(ap1->offset); /* FALL THROUGH */
    case am_direct:
        ap1->offset = makenode(en_add, ap1->offset,
                       makenode(en_icon, delta, NULL));
        break;
    case am_ind:
        ap1->mode = am_indx;    /* 0(Ax) */
        ap1->offset = makenode(en_icon, delta, NULL);
        break;
    case am_indx:
        if (ap1->offset->nodetype == en_icon)
            ap1->offset = makenode(en_icon, (ap1->offset->v.i) + delta, NULL);
        break;
    default:
        fprintf( stderr, "DIAG -- uncoded operand in make_delta\n" );
        break;
    }
    return (ap1);
}

void
make_legal(ap, flags, size)

/*
 * make_legal will coerce the addressing mode in ap1 into a mode that is
 * satisfactory for the flag word.
 */
    struct amode   *ap;
    int             flags, size;
{
    struct amode   *ap2, *ap3;

    if (ap == NULL) {
        fprintf( stderr, "DIAG -- NULL pointer in make_legal\n" );
        return;
    }

    if (((flags & F_VOL) == 0) || ap->tempflag) {
        switch (ap->mode) {
        case am_immed:
            if (flags & F_IMMED)
                if (ap->offset != NULL && ap->offset->nodetype != en_autocon)
                    return; /* mode ok */
            break;
        case am_areg:
            if (flags & F_AREG)
                return;
            break;
        case am_dreg:
            if (flags & F_DREG)
                return;
            break;
        case am_ind:
        case am_indx:
        case am_indx2:
        case am_xpc:
        case am_direct:
        case am_indx3:
            if (flags & F_MEM)
                return;
            break;
        case am_freg:
            if (flags & F_FREG)
                return;
            break;
        }
    }
    if (flags & F_DREG) {
        freeop(ap); /* maybe we can use it... */
        ap2 = temp_data();  /* allocate to dreg */
        if (ap->mode == am_immed && ap->offset->nodetype != en_icon) {
            ap3 = temp_addr();
            gen_code(op_move, size, ap, ap3);
            gen_code(op_move, size, ap3, ap2);
            freeop(ap3);
        }
        else {
            if (!ap->signedflag)
                gen_code(op_move, 4, make_immed((long) 0), ap2);
            if (ap->mode != am_areg || size != 1)
                gen_code(op_move, size, ap, ap2);
            else {  /* Can't do "move.b A0,D0" */
                gen_code(op_move, 2, ap, ap2);
                gen_code(op_and, 4, make_immed((long) 0xff), ap2);
            }
        }
        ap->mode = ap2->mode;
        ap->preg = ap2->preg;
        ap->sreg = ap2->sreg;
        ap->deep = ap2->deep;
        ap->signedflag = ap2->signedflag;
        ap->tempflag = 1;
        return;
    }

    if (flags & F_FREG) {
        if (ap->mode == am_areg || ap->mode == am_dreg)
            fprintf( stderr, "DIAG -- Error in make_legal\n" );
        else {
            ap3 = temp_float();
            freeop(ap);
            ap2 = copy_addr(ap);
            gen_code(op_move, 4, ap2, makedreg((enum e_am) 0));
            ap2 = make_delta(ap2, 4);
            gen_code(op_move, 4, ap2, makedreg((enum e_am) 1));

            ap->mode = ap3->mode;
            ap->preg = ap3->preg;
            ap->sreg = ap3->sreg;
            ap->deep = ap3->deep;
            ap->signedflag = ap3->signedflag;
            ap->tempflag = 1;
        }
        return;
    }

    /* Must be (flags & F_AREG) */

    if (size == 1) {
        ap2 = temp_data();
        if (!ap->signedflag)
            gen_code(op_move, 4, make_immed((long) 0), ap2);
        gen_code(op_move, 1, ap, ap2);
        if (ap->signedflag)
            gen_code(op_ext, 2, ap2, NULL);

        freeop( ap );

        ap3 = temp_addr();
        gen_code(op_move, 2, ap2, ap3);
        freeop(ap2);

        ap->mode = ap3->mode;
        ap->preg = ap3->preg;
        ap->sreg = ap3->sreg;
        ap->deep = ap3->deep;
        ap->signedflag = ap3->signedflag;
        ap->tempflag = 1;
        return;
    }

    freeop(ap);
    ap2 = temp_addr();
    if (!equal_address(ap, ap2))
        gen_code(op_move, size, ap, ap2);

    ap->mode = ap2->mode;
    ap->preg = ap2->preg;
    ap->sreg = ap2->sreg;
    ap->deep = ap2->deep;
    ap->signedflag = ap2->signedflag;
    ap->tempflag = 1;
}

void
do_extend(ap, isize, osize, flags, is_signed)

/*
 * if isize is not equal to osize then the operand ap will be loaded into a
 * register (if not already) and if osize is greater than isize it will be
 * extended to match.
 */
    struct amode   *ap;
    int             isize, osize, flags, is_signed;
{
    struct amode   *ap1;

    if (ap == NULL) {
        fprintf( stderr, "DIAG -- null node in do_extend.\n" );
        return;
    }

    if (isize == osize)
        return;

    if (ap->mode != am_areg && ap->mode != am_dreg)
        make_legal(ap, flags & (F_AREG | F_DREG), isize);

    if (ap->mode == am_areg) {
        if (is_signed)
            return; /* extend is automagic */
        ap1 = temp_addr();
        gen_code(op_move, 4, make_immed((long) 0), ap1);
        gen_code(op_move, isize, ap, ap1);
        freeop(ap);
        ap->preg = ap1->preg;
        ap->deep = ap1->deep;
    }

    if (is_signed) {
        switch (isize) {
        case 1:
            gen_code(op_ext, 2, ap, NULL);
        case 2:
            if (osize == 4)
                gen_code(op_ext, 4, ap, NULL);
        }
        ap->signedflag = 1;
    }
    else {
        if (ap->signedflag) {
            ap1 = temp_data();
            gen_code(op_move, 4, make_immed((long) 0), ap1);
            gen_code(op_move, isize, ap, ap1);
            gen_code(op_move, 4, ap1, ap);
            freeop(ap1);
        }
        ap->signedflag = 0;
    }
}

int
isshort(node)

/*
 * return true if the node passed can be generated as a short offset.
 */
    struct enode   *node;
{
    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in isshort.\n" );
        return FALSE;
    }
    return node->nodetype == en_icon &&
        (node->v.i >= -32767 && node->v.i <= 32767);
}

int
isbyte(node)

/*
 * return trues if the node passed can be evaluated as a byte offset.
 */
    struct enode   *node;
{
    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in isbyte.\n" );
        return FALSE;
    }
    return node->nodetype == en_icon &&
        (-128 <= node->v.i && node->v.i <= 127);
}

struct amode   *
gen_extend(node, flags, size)
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1;
    int             siz1, siz2, is_signed;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_extend.\n" );
        return NULL;
    }


    siz1 = size;
    siz2 = 4;
    is_signed = 1;

    switch (node->v.p[0]->nodetype) {
    case en_ub_ref:
        is_signed = 0;  /* Fall through */
    case en_b_ref:
        siz1 = 1;
        break;
    case en_uw_ref:
        is_signed = 0;  /* Fall through */
    case en_w_ref:
        siz1 = 2;
        break;
    case en_tempref:
        is_signed = node->signedflag;
        break;
    }

    switch (node->nodetype) {
    case en_cbl:
        siz1 = 1;
        siz2 = 4;
        break;
    case en_cwl:
        siz1 = 2;
        siz2 = 4;
        break;
    }

    ap1 = gen_expr(node->v.p[0], flags, siz1);
    do_extend(ap1, siz1, siz2, flags, is_signed);
    make_legal(ap1, flags, siz2);

    return (ap1);
}

struct amode   *
gen_index(node)

/*
 * generate code to evaluate an index node (^+) and return the addressing
 * mode of the result. This routine takes no flags since it always returns
 * either am_ind or am_indx.
 */
    struct enode   *node;
{
    struct amode   *ap1, *ap2, *ap3;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_index.\n" );
        return NULL;
    }
    if (node->v.p[0]->nodetype == en_tempref &&
        node->v.p[1]->nodetype == en_tempref &&
        (node->v.p[0]->v.i >= 8 || node->v.p[1]->v.i >= 8)) {
        /* both nodes are registers, one is address */
        if (node->v.p[0]->v.i < 8) {
            ap1 = gen_expr(node->v.p[1], F_AREG, 4);
            make_legal(ap1, F_AREG, 4);
            ap1->sreg = (enum e_am) (node->v.p[0]->v.i);
            ap1->mode = am_indx2;   /* 0(Ax,Dx) */
            ap1->offset = makenode(en_icon, NULL, NULL);
            return ap1;
        }
        ap1 = gen_expr(node->v.p[0], F_AREG, 4);
        make_legal(ap1, F_AREG, 4);
        ap2 = gen_expr(node->v.p[1], F_AREG | F_DREG, 4);
        make_legal(ap1, F_AREG | F_DREG, 4);
        if (ap2->mode == am_dreg) {
            ap1->mode = am_indx2;
            ap1->sreg = ap2->preg;
            ap1->deep = ap2->deep;
        }
        else {
            ap1->mode = am_indx3;
            ap1->sreg = ap2->preg;
            ap1->deep = ap2->deep;
        }
        ap1->offset = makenode(en_icon, NULL, NULL);
        return ap1;
    }

    ap1 = gen_expr(node->v.p[0], F_AREG | F_IMMED, 4);
    make_legal(ap1, F_AREG | F_IMMED, 4);

    if (ap1->mode == am_immed && isshort(ap1->offset)) {
        ap2 = gen_expr(node->v.p[1], F_AREG, 4);
        make_legal( ap2, F_AREG, 4 );
        ap2->mode = am_indx;
        ap2->offset = ap1->offset;
        return ap2;
    }

    make_legal(ap1, F_AREG, 4);
    ap2 = gen_expr(node->v.p[1], F_ALL, 4); /* get right op */

    if ((ap2->mode == am_immed && isshort(ap2->offset))
         && ap1->mode == am_areg) { /* make am_indx */
        freeop(ap2);
        freeop(ap1);
        ap1 = request_reg(ap1);
        ap1->mode = am_indx;
        ap1->offset = ap2->offset;
        return ap1;
    }

    validate(ap2);
    validate(ap1);

    if (istemp(ap1)) {
        gen_code(op_add, 4, ap2, ap1);  /* add left to address reg  */
        ap1->mode = am_ind; /* make indirect        */
        freeop(ap2);    /* release any temps in ap2 */
        return ap1; /* return indirect      */
    }
    else if (istemp(ap2)) {
        make_legal(ap2, F_AREG, 4);
        gen_code(op_add, 4, ap1, ap2);  /* add right to address reg */
        ap2->mode = am_ind; /* make indirect        */
        freeop(ap2);
        freeop(ap1);
        ap1 = request_reg(ap2);
        return ap1; /* return indirect      */
    }
    else {
        ap3 = temp_addr();
        gen_code(op_move, 4, ap1, ap3);
        gen_code(op_add, 4, ap2, ap3);  /* add left to address reg  */
        ap3->mode = am_ind;             /* make indirect        */
        freeop(ap3);
        freeop(ap2);                    /* release any temps in ap2 */
        freeop(ap1);
        ap1 = request_reg(ap3);
        return ap1;                     /* return indirect      */
    }
}


struct amode   *
make_autocon(lab)
    int             lab;
{
    struct amode   *ap1;

    ap1 = (struct amode *) xalloc(sizeof(struct amode));
    ap1->signedflag = 1;
    ap1->mode = am_indx;
    ap1->preg = (enum e_am) Options.Frame;
    ap1->deep = 0;
    ap1->offset = makenode(en_icon, (long) lab, NULL);
    return ap1;
}


struct amode   *
gen_deref(node, flags, size)

/*
 * return the addressing mode of a dereferenced node.
 */
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2;
    int             siz1;
    int             is_signed;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_deref.\n" );
        return NULL;
    }

    is_signed = 1;

    switch (node->nodetype) {   /* get load size */
    case en_ub_ref:
        is_signed = 0;  /* Fall through */
    case en_b_ref:
        siz1 = 1;
        break;
    case en_uw_ref:
        is_signed = 0;  /* Fall through */
    case en_w_ref:
        siz1 = 2;
        break;
    case en_ul_ref:
        is_signed = 0;  /* Fall through */
    case en_l_ref:
    case en_m_ref:
    case en_f_ref:
        siz1 = 4;
        break;
    case en_d_ref:
        siz1 = 8;
        break;
    }

    if (node->nodetype == en_d_ref) {
        ap1 = gen_expr(node->v.p[0], F_ALL, 4);
        if (ap1->mode == am_areg) {
            ap1->signedflag = 1;
            ap1->mode = am_indx;
            ap1->offset = makenode(en_icon, (long) 0, NULL);
            return ap1;
        }
        ap2 = temp_addr();
        gen_code(op_move, 4, ap1, ap2);
        freeop(ap1);
        ap2->signedflag = 1;
        ap2->mode = am_indx;
        ap2->offset = makenode(en_icon, (long) 0, NULL);
        return ap2;
    }

    if (node->v.p[0]->nodetype == en_add) {
        ap1 = gen_index(node->v.p[0]);
        do_extend(ap1, siz1, size, flags, is_signed);
        make_legal(ap1, flags, size);
        return ap1;
    }
    else if (node->v.p[0]->nodetype == en_autocon) {
        ap1 = (struct amode *) xalloc(sizeof(struct amode));
        ap1->signedflag = node->v.p[0]->signedflag;
        ap1->mode = am_indx;
        ap1->preg = (enum e_am) Options.Frame;
        ap1->deep = 0;
        ap1->offset = makenode(en_icon, (long) (node->v.p[0]->v.i), NULL);
        do_extend(ap1, siz1, size, flags, is_signed);
        make_legal(ap1, flags, size);
        return ap1;
    }

    ap1 = gen_expr(node->v.p[0], F_AREG | F_IMMED, 4);  /* generate address */
    make_legal(ap1, F_AREG | F_IMMED, 4);

    if (ap1->mode == am_areg) {
        ap1->mode = am_ind;
        do_extend(ap1, siz1, size, flags, is_signed);
        make_legal(ap1, flags, size);
        return ap1;
    }
    ap1->mode = am_direct;
    do_extend(ap1, siz1, size, flags, is_signed);
    make_legal(ap1, flags, size);
    return ap1;
}

struct amode   *
gen_unary(node, flags, size, op)

/*
 * generate code to evaluate a unary minus or complement.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap, *ap3;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_unary.\n" );
        return NULL;
    }

    ap = gen_expr(node->v.p[0], F_DREG, size);
    make_legal(ap, F_DREG, 4);

    if (istemp(ap)) {
        gen_code(op, size, ap, NULL);
        make_legal(ap, flags, size);
        return ap;
    }

    ap3 = temp_data();

    gen_code(op_move, size, ap, ap3);
    freeop(ap);
    gen_code(op, size, ap3, NULL);
    make_legal(ap3, flags, size);
    return ap3;
}

struct amode   *
gen_binary(node, flags, size, op)

/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_binary.\n" );
        return NULL;
    }

    if (size == 8)      /* If part of a double expression do in
                 * integer */
        size = 4;

    ap1 = gen_expr(node->v.p[0], F_DREG, size);
    make_legal(ap1, F_DREG, size);
    ap2 = gen_expr(node->v.p[1], F_DREG | F_IMMED | F_MEM, size);
    make_legal(ap2, F_DREG | F_IMMED | F_MEM, size);

    if (istemp(ap1)) {
        validate(ap1);  /* in case push occurred */
        gen_code(op, size, ap2, ap1);
        freeop(ap2);
        make_legal(ap1, flags, size);
        return ap1;
    }

    if (flags & F_DREG)
        ap3 = temp_data();
    else
        ap3 = temp_addr();

    validate(ap1);      /* in case push occurred */

    gen_code(op_move, size, ap1, ap3);
    gen_code(op, size, ap2, ap3);

    if (istemp(ap2)) {
        gen_code(op_move, size, ap3, ap2);
        freeop(ap3);
        freeop(ap2);
        freeop(ap1);
        ap2 = request_reg(ap2);
        make_legal(ap2, flags, size);
        return ap2;
    }

    freeop(ap3);
    freeop(ap2);
    freeop(ap1);
    ap3 = request_reg(ap3);
    make_legal(ap3, flags, size);
    return ap3;
}

struct amode   *
gen_xbin(node, flags, size, op)

/*
 * generate code to evaluate a restricted binary node and return the
 * addressing mode of the result.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_xbin.\n" );
        return NULL;
    }

    ap1 = gen_expr(node->v.p[0], F_DREG, size);
    make_legal(ap1, F_DREG, size);
    ap2 = gen_expr(node->v.p[1], F_DREG, size);
    make_legal(ap2, F_DREG, size);

    if (istemp(ap1)) {
        validate(ap1);  /* in case push occurred */
        gen_code(op, size, ap2, ap1);
        freeop(ap2);
        make_legal(ap1, flags, size);
        return ap1;
    }

    ap3 = temp_data();

    validate(ap1);      /* in case push occurred */

    gen_code(op_move, size, ap1, ap3);
    freeop(ap1);
    gen_code(op, size, ap2, ap3);
    freeop(ap2);
    make_legal(ap3, flags, size);
    return ap3;
}

struct amode   *
gen_shift(node, flags, size, op)

/*
 * generate code to evaluate a shift node and return the address mode of the
 * result.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;
    struct enode   *ep1;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_shift.\n" );
        return NULL;
    }

    ap1 = gen_expr(node->v.p[0], F_DREG, size);
    make_legal(ap1, F_DREG, size);

    ep1 = node->v.p[1];
    if (ep1->nodetype == en_icon && ep1->v.i > 8) {
        ap2 = gen_expr(ep1, F_DREG, 4);
        make_legal(ap2, F_DREG, 4);
    }
    else {
        ap2 = gen_expr(ep1, F_DREG | F_IMMED, 4);
        make_legal(ap2, F_DREG | F_IMMED, 4);
    }

    if (istemp(ap1)) {
        validate(ap1);  /* in case push occurred */
        gen_code(op, size, ap2, ap1);
        freeop(ap2);
        make_legal(ap1, flags, size);
        return ap1;
    }

    validate(ap2);
    validate(ap1);      /* in case push occurred */

    ap3 = temp_data();

    gen_code(op_move, size, ap1, ap3);
    gen_code(op, size, ap2, ap3);

    make_legal(ap3, flags, size);

    freeop(ap3);
    freeop(ap2);
    freeop(ap1);

    ap3 = request_reg(ap3);

    return ap3;
}

void
swap_nodes(node)

/*
 * exchange the two operands in a node.
 */
    struct enode   *node;
{
    struct enode   *temp;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in swap_nodes.\n" );
        return;
    }
    temp = node->v.p[0];
    node->v.p[0] = node->v.p[1];
    node->v.p[1] = temp;
}

struct amode   *
gen_modiv(node, flags, size, op )

/*
 * generate code to evaluate a mod operator or a divide operator. these
 * operations are done on only long divisors and word dividends so that the
 * 68000 div instruction can be used.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_modiv.\n" );
        return NULL;
    }

    ap1 = gen_expr(node->v.p[0], F_ALL, 4);
    ap2 = gen_expr(node->v.p[1], F_ALL, 4);
    validate(ap1);

    if (Options.MulDiv32) {
        gen_code(op_move, 4, ap2, push);
        gen_code(op_move, 4, ap1, push);
        freeop(ap2);
        freeop(ap1);

        switch (op) {
        case op_mods:
            call_library(".lmods");
            break;
        case op_modu:
            call_library( ".lmodu" );
            break;
        case op_divs:
            call_library(".ldivs");
            break;
        case op_divu:
            call_library( ".ldivu" );
            break;
        }

        ap3 = temp_data();
        gen_code(op_move, 4, pop, ap3);
        gen_code(op_add, 4, make_immed((long) 4), makeareg(am_xpc));
        return (ap3);
    }

    make_legal(ap1, F_DREG, 4);
    make_legal(ap2, F_DREG, 4);
    validate(ap1);

    if (istemp(ap1)) {

        switch (op) {
        case op_divs:
        case op_mods:
            gen_code(op_divs, 0, ap2, ap1);
            break;
        case op_divu:
        case op_modu:
            gen_code(op_divu, 0, ap2, ap1);
            break;
        }

        if (op == op_modu || op == op_mods)
            gen_code(op_swap, 0, ap1, NULL);

        gen_code(op_ext, 4, ap1, NULL);
        make_legal(ap1, flags, 4);
        freeop(ap2);
        return ap1;
    }

    ap3 = temp_data();
    gen_code(op_move, size, ap1, ap3);

    switch (op) {
    case op_divs:
    case op_mods:
        gen_code(op_divs, 0, ap2, ap3);
        break;
    case op_divu:
    case op_modu:
        gen_code(op_divu, 0, ap2, ap3);
        break;
    }

    if (op == op_modu || op == op_mods)
        gen_code(op_swap, 0, ap3, NULL);

    gen_code(op_ext, 4, ap3, NULL);

    freeop(ap3);
    freeop(ap2);
    freeop(ap1);

    ap3 = request_reg(ap3);
    make_legal(ap3, flags, 4);
    return ap3;
}

struct amode   *
gen_mul(node, flags, size, op)

/*
 * generate code to evaluate a multiply node. both operands are treated as
 * words and the result is long and is always in a register so that the 68000
 * mul instruction can be used.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;
    int             i, b2, bits, mask, last;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_mul.\n" );
        return NULL;
    }

    if (node->v.p[0]->nodetype == en_icon)
        swap_nodes(node);

    ap1 = gen_expr(node->v.p[0], F_ALL, 4);
    ap2 = gen_expr(node->v.p[1], F_ALL, 4);
    validate(ap1);

    if (ap2->mode == am_immed && ap2->offset->nodetype == en_icon) {
        /* Recode the multiplier */
        bits = 0;
        mask = ap2->offset->v.i;
        for (i = 0; i < 32; i++) {
            if ((mask >> i) & 1)
                ++bits;
        }
        if (bits >= 1 && bits < 24 && mask > 0 && Options.Optimize) {
            if (!istemp(ap1)) {
                ap3 = temp_data();
                gen_code(op_move, size, ap1, ap3);
                freeop(ap1);
                ap1 = ap3;
            }
            ap3 = temp_data();
            for (bits = 0; (mask & (1 << bits)) == 0; bits++);
            b2 = bits;
            while (b2 > MAX_SHIFT) {
                gen_code(op_asl, 4, make_immed((long) MAX_SHIFT), ap1);
                b2 -= MAX_SHIFT;
            }
            if (b2 != 0)
                gen_code(op_asl, 4, make_immed((long) b2), ap1);
            gen_code(op_move, 4, ap1, ap3);
            last = bits;
            for (++bits; bits < 32; ++bits) {
                if ((mask & (1 << bits)) != 0) {
                    b2 = (bits - last);
                    while (b2 > MAX_SHIFT) {
                        gen_code(op_asl, 4, make_immed((long) MAX_SHIFT), ap3);
                        b2 -= MAX_SHIFT;
                    }
                    if (b2 != 0)
                        gen_code(op_asl, 4, make_immed((long) b2), ap3);
                    gen_code(op_add, 4, ap3, ap1);
                    last = bits;
                }
            }
            freeop(ap3);
            freeop(ap2);
            return (ap1);
        }
    }


    if (Options.MulDiv32) {
        gen_code(op_move, 4, ap2, push);
        gen_code(op_move, 4, ap1, push);
        freeop(ap2);
        freeop(ap1);
        call_library(".lmuls");
        ap3 = temp_data();
        gen_code(op_move, 4, pop, ap3);
        gen_code(op_add, 4, make_immed((long) 4), makeareg(am_xpc));
        return (ap3);
    }

    make_legal(ap1, F_DREG, 4);
    make_legal(ap2, F_DREG, 4);
    validate(ap1);

    if (istemp(ap1)) {
        gen_code(op, 0, ap2, ap1);
        freeop(ap2);
        make_legal(ap1, flags, 4);
        return ap1;
    }

    ap3 = temp_data();
    gen_code(op_move, size, ap1, ap3);
    gen_code(op, 0, ap2, ap3);
    freeop(ap3);
    freeop(ap2);
    freeop(ap1);
    ap3 = request_reg(ap3);
    make_legal(ap3, flags, 4);
    return ap3;
}

struct amode   *
gen_hook(node, flags, size)

/*
 * generate code to evaluate a condition operator node (?:)
 */

    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2, *ap3;
    int             siz1, siz2;
    int             false_label, end_label;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_hook.\n" );
        return NULL;
    }

    false_label = nextlabel++;
    end_label = nextlabel++;

    flags = (flags & (F_AREG | F_DREG));

    falsejp(node->v.p[0], false_label);
    node = node->v.p[1];

    siz1 = natural_size(node->v.p[0]);
    siz2 = natural_size(node->v.p[1]);

    if (siz1 == 8 || siz2 == 8) {
        flags = F_FREG;
        size = 8;
    }

    ap1 = gen_expr(node->v.p[0], flags, siz1);
    make_legal(ap1, flags, size);

    gen_code(op_bra, 0, make_label(end_label), NULL);

    gen_label(false_label);

    freeop(ap1);

    ap2 = gen_expr(node->v.p[1], flags, siz2);
    make_legal(ap2, flags, size);

    if (!equal_address(ap1, ap2)) {
        if (size <= 4)
            gen_code(op_move, size, ap2, ap1);
        else {
            ap3 = copy_addr(ap2);
            gen_code(op_move, 4, ap3, makedreg((enum e_am) 0));
            ap3 = make_delta(ap3, 4);
            gen_code(op_move, 4, ap3, makedreg((enum e_am) 1));
        }
    }
    freeop(ap2);
    gen_label(end_label);
    ap1 = request_reg(ap1);
    return ap1;
}

struct amode   *
gen_asadd(node, flags, size, op)

/*
 * generate a plus equal or a minus equal node.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2;
    int             ssize;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_asadd.\n" );
        return NULL;
    }
    ssize = natural_size(node->v.p[0]);
    if (ssize > size)
        size = ssize;
    ap1 = gen_expr(node->v.p[0], F_ALL, ssize);
    ap2 = gen_expr(node->v.p[1], F_DREG | F_IMMED, size);
    make_legal(ap2, F_DREG | F_IMMED, size);
    validate(ap1);
    gen_code(op, ssize, ap2, ap1);
    freeop(ap2);
    do_extend(ap1, ssize, size, flags, 1);
    make_legal(ap1, flags, size);
    return ap1;
}

struct amode   *
gen_aslogic(node, flags, size, op)

/*
 * generate a and equal or a or equal node.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;
    int             ssize;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_aslogic.\n" );
        return NULL;
    }
    ssize = natural_size(node->v.p[0]);
    if (ssize > size)
        size = ssize;
    ap1 = gen_expr(node->v.p[0], F_ALL, ssize);
    ap2 = gen_expr(node->v.p[1], F_DREG | F_IMMED, size);
    make_legal(ap2, F_DREG | F_IMMED, size);

    validate(ap1);

    if (ap1->mode != am_areg)
        gen_code(op, ssize, ap2, ap1);
    else {
        ap3 = temp_data();
        gen_code(op_move, 4, ap1, ap3);
        gen_code(op, size, ap2, ap3);
        gen_code(op_move, size, ap3, ap1);
        freeop(ap3);
    }
    freeop(ap2);
    do_extend(ap1, ssize, size, flags, 1);
    make_legal(ap1, flags, size);
    return ap1;
}

struct amode   *
gen_asshift(node, flags, size, op)

/*
 * generate shift equals operators.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;
    struct enode   *ep1;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_asshift.\n" );
        return NULL;
    }

    ap1 = gen_expr(node->v.p[0], F_ALL, size);
    if (ap1->mode != am_dreg) {
        ap3 = temp_data();
        gen_code(op_move, size, ap1, ap3);
    }
    else
        ap3 = ap1;

    ep1 = node->v.p[1];
    if (ep1->nodetype == en_icon && ep1->v.i > 8) {
        ap2 = gen_expr(ep1, F_DREG, size);
        make_legal(ap2, F_DREG, size);
    }
    else {
        ap2 = gen_expr(ep1, F_DREG | F_IMMED, size);
        make_legal(ap2, F_DREG | F_IMMED, size);
    }

    validate(ap3);
    gen_code(op, size, ap2, ap3);
    freeop(ap2);
    if (ap3 != ap1) {
        gen_code(op_move, size, ap3, ap1);
        freeop(ap3);
    }
    make_legal(ap1, flags, size);
    return ap1;
}

struct amode   *
gen_asmul(node, flags, size)

/*
 * generate a *= node.
 */
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2, *ap3, *ap4;
    int             mask, b2, bits, i, last, numbits;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_asmul.\n" );
        return NULL;
    }

    ap1 = gen_expr(node->v.p[0], F_ALL, 4);
    ap2 = gen_expr(node->v.p[1], F_ALL, 4);
    validate(ap1);

    if (ap2->mode == am_immed && ap2->offset->nodetype == en_icon) {
        /* Recode the multiplier */
        bits = 0;   /* A3 = A3 * A1     */
        mask = ap2->offset->v.i;
        for (i = 0; i < 32; i++) {
            if ((mask >> i) & 1)
                ++bits;
        }
        if (bits >= 1 && bits < 24 && mask > 0 && Options.Optimize) {
            numbits = bits;
            ap4 = ap1;
            if (!istemp(ap1)) {
                ap3 = temp_data();
                gen_code(op_move, size, ap1, ap3);
                ap4 = ap3;
            }
            ap3 = temp_data();
            for (bits = 0; (mask & (1 << bits)) == 0; bits++);
            if (bits != 0) {
                b2 = bits;
                while (b2 > MAX_SHIFT) {
                    gen_code(op_asl, 4, make_immed((long) MAX_SHIFT), ap4);
                    b2 -= MAX_SHIFT;
                }
                if (b2 != 0)
                    gen_code(op_asl, 4, make_immed((long) b2), ap4);
            }
            if (numbits > 1)
                gen_code(op_move, 4, ap4, ap3);
            last = bits;
            for (++bits; bits < 32; ++bits) {
                if ((mask & (1 << bits)) != 0) {
                    b2 = (bits - last);
                    while (b2 > MAX_SHIFT) {
                        gen_code(op_asl, 4, make_immed((long) MAX_SHIFT), ap3);
                        b2 -= MAX_SHIFT;
                    }
                    if (b2 != 0)
                        gen_code(op_asl, 4, make_immed((long) b2), ap3);
                    gen_code(op_add, 4, ap3, ap4);
                    last = bits;
                }
            }
            gen_code(op_move, 4, ap4, ap1);
            freeop(ap3);
            freeop(ap2);
            return (ap1);
        }
    }

    if (Options.MulDiv32) {
        gen_code(op_move, 4, ap1, push);
        gen_code(op_move, 4, ap2, push);
        freeop(ap2);
        if (node->nodetype == en_asumul)
            call_library(".lmulu");
        else
            call_library(".lmuls");
        gen_code(op_move, 4, pop, ap1);
        gen_code(op_add, 4, make_immed((long) 4), makeareg(am_xpc));
        return (ap1);
    }

    make_legal(ap1, F_DREG, 4);
    make_legal(ap2, F_DREG, 4);
    validate(ap1);

    if (istemp(ap1)) {
        if (node->nodetype == en_asumul)
            gen_code(op_mulu, 0, ap2, ap1);
        else
            gen_code(op_muls, 0, ap2, ap1);
        return (ap1);
    }
    else {
        ap3 = temp_data();
        gen_code(op_move, size, ap1, ap3);
        if (node->nodetype == en_asumul)
            gen_code(op_mulu, 0, ap2, ap3);
        else
            gen_code(op_muls, 0, ap2, ap3);
        gen_code(op_move, 4, ap3, ap1);
        freeop(ap3);
        freeop(ap2);
        return (ap1);
    }
}

struct amode   *
gen_asmodiv(node, flags, size, op)

/*
 * generate /= and %= nodes.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_asmodiv.\n" );
        return NULL;
    }

    ap2 = gen_expr(node->v.p[0], F_ALL, 4);
    ap3 = gen_expr(node->v.p[1], F_ALL, 4);
    validate(ap2);

    if (Options.MulDiv32) {
        gen_code(op_move, 4, ap3, push);
        gen_code(op_move, 4, ap2, push);

        switch (op) {
        case op_mods:
            call_library(".lmods");
            break;
        case op_modu:
            call_library(".lmodu");
            break;
        case op_divs:
            call_library(".ldivs");
            break;
        case op_divu:
            call_library(".ldivu");
            break;
        }

        gen_code(op_move, 4, pop, ap2);
        gen_code(op_add, 4, make_immed((long) 4), makeareg(am_xpc));
        make_legal(ap2, flags, size);
        freeop(ap3);
        return (ap2);
    }

    ap1 = temp_data();
    make_legal(ap3, F_DREG, 4);

    if (istemp(ap2)) {
        if (op == op_divs || op == op_mods)
            gen_code(op_divs, 0, ap3, ap2);
        else
            gen_code(op_divu, 0, ap3, ap2);

        if (op == op_mods || op == op_modu)
            gen_code(op_swap, 0, ap2, NULL);

        gen_code(op_ext, 4, ap2, NULL);
        make_legal(ap2, flags, 4);
        freeop(ap1);
        freeop(ap3);
        return ap2;
    }

    validate(ap3);
    validate(ap2);

    gen_code(op_move, 4, ap2, ap1);

    if (op == op_divs || op == op_mods)
        gen_code(op_divs, 0, ap3, ap1);
    else
        gen_code(op_divu, 0, ap3, ap1);

    if (op == op_mods || op == op_modu)
        gen_code(op_swap, 0, ap1, NULL);

    gen_code(op_ext, 4, ap1, NULL);
    gen_code(op_move, 4, ap1, ap2);

    freeop(ap1);
    freeop(ap3);

    make_legal(ap1, flags, size);
    return ap2;
}

struct amode   *
gen_massign(node, flags, size)

/*
 * generate code for an memory assignment node. It requires that an en_m_ref
 * assignment is passed as the *node parameter
 */

    struct enode   *node;
    int             flags, size;
{
    struct enode   *ep1, *ep2;
    struct amode   *ap1, *ap2, *ap3, *ap4, *ap5;
    int             ssize, begin_label;

    flags = F_ALL;

    ep1 = node->v.p[0];
    ep2 = node->v.p[1];

    ap1 = gen_expr(ep1, flags, 4);
    ap2 = gen_expr(ep2, flags, 4);
    validate(ap2);
    validate(ap1);

    ssize = (ep1->v.p[1]->v.i);

    if (ssize > size)
        size = ssize;

    if (ssize <= 4) {
        gen_code(op_move, ssize, ap2, ap1);
        freeop(ap2);
        freeop(ap1);
        ap3 = request_reg(ap1);
        return (ap3);
    }

    if (ap2->mode == am_areg || ap2->mode == am_dreg)
        gen_code(op_move, 4, ap2, push);
    else
        gen_code(op_pea, 0, ap2, NULL);

    if (ap1->mode == am_areg || ap1->mode == am_dreg)
        gen_code(op_move, 4, ap1, push);
    else
        gen_code(op_pea, 0, ap1, NULL);

    freeop(ap2);
    freeop(ap1);

    ap3 = temp_data();
    ap4 = temp_addr();
    ap5 = temp_addr();

    gen_code(op_move, 4, pop, ap5);
    gen_code(op_move, 4, pop, ap4);

    ap4->mode = ap5->mode = am_ainc;
    if (ssize < 16) {
        while (ssize >= 4) {
            gen_code(op_move, 4, ap4, ap5);
            ssize -= 4;
        }
    }
    else {
        gen_code(op_move, 4, make_immed((long) ssize / 4), ap3);
        begin_label = nextlabel++;
        gen_label(begin_label);
        gen_code(op_move, 4, ap4, ap5);
        gen_code(op_dbra, 0, ap3, make_label(begin_label));
        ssize %= 4;
    }
    if (ssize >= 2) {
        gen_code(op_move, 2, ap4, ap5);
        ssize -= 2;
    }
    if (ssize >= 1) {
        gen_code(op_move, 1, ap4, ap5);
        ssize -= 1;
    }

    freeop(ap5);
    freeop(ap4);
    freeop(ap3);

    ap3 = request_reg(ap1);
    return (ap3);
}

struct amode   *
gen_assign(node, flags, size)

/*
 * generate code for an assignment node. if the size of the assignment
 * destination is larger than the size passed then everything below this node
 * will be evaluated with the assignment size.
 */
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2, *ap3, *ap4;
    int             ssize, is_signed, size1, size2;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_assign.\n" );
        return NULL;
    }

    if (node->v.p[0]->nodetype == en_m_ref) {
        return (gen_massign(node, flags, size));
    }

	ssize = size1 = natural_size( node->v.p[0] );
	size2 = natural_size( node->v.p[1] );

    switch (node->v.p[0]->nodetype) {
    case en_ub_ref:
    case en_uw_ref:
    case en_ul_ref:
        is_signed = 0; 
		break;
    case en_b_ref:
    case en_w_ref:
    case en_l_ref:
    case en_f_ref:
    case en_tempref:
    case en_d_ref:
		is_signed = 1;
        break;
	default:
		fprintf( stderr, "DIAG -- Uncoded LHS\n" );
		break;
    }

    if (ssize > size)
        size = ssize;

    if (ssize == 8) {
        ap1 = gen_expr(node->v.p[1], F_ALL | F_FREG, size);
        ap2 = gen_expr(node->v.p[0], F_ALL, size);

        validate(ap2);
        validate(ap1);

        ap4 = copy_addr(ap2);
        if (ap1->mode == am_freg) {
            gen_code(op_move, 4, makedreg((enum e_am) 0), ap4);
            ap4 = make_delta(ap4, 4);
            gen_code(op_move, 4, makedreg((enum e_am) 1), ap4);
            freeop(ap2);
        }
        else {
            ap3 = copy_addr(ap1);
            gen_code(op_move, 4, ap3, ap4);
            ap3 = make_delta(ap3, 4);
            ap4 = make_delta(ap4, 4);
            gen_code(op_move, 4, ap3, ap4);
            freeop(ap2);
        }
        return ap1;
    }
    else {
        ap2 = gen_expr( node->v.p[1], F_ALL,  size2 );

        if (ap2->mode == am_areg && ssize == 1) 
            make_legal(ap2, F_DREG, 4); /* Don't allow move.b A0,D0 */
        else if (ap2->mode == am_immed && ap2->offset->nodetype != en_icon) 
            make_legal(ap2, F_AREG, 4); /* Don't allow move.l #_label,D0 */

        if (size2 != size1) {
            make_legal( ap2, F_AREG | F_DREG, size2 );
            do_extend( ap2, size2, size1, F_AREG | F_DREG, is_signed );
        }
  
        ap1 = gen_expr( node->v.p[0], F_ALL, size1 );

        validate(ap1);
        validate(ap2);

        if (!node->signedflag && ssize != 4)
            gen_code(op_move, 4, make_immed((long) 0), ap1);

        gen_code(op_move, size1, ap2, ap1);

        freeop(ap1);
        freeop(ap2);

        if (ap2->mode == am_immed && ap2->offset->nodetype == en_icon) {
            if (ap1->mode != am_areg && ap1->mode != am_dreg)   /* x = y = 0 */
                return ap2;
        }

        ap1 = request_reg(ap1);
        return ap1;
    }
}

struct amode   *
gen_aincdec(node, flags, size, op)

/*
 * generate an auto increment or decrement node. op should be either op_add
 * (for increment) or op_sub (for decrement).
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2;
    int             siz1, amount;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_aincdec.\n" );
        return NULL;
    }

    siz1 = natural_size(node->v.p[0]);

    amount = node->v.p[1]->v.i;

    if (flags & F_NOVALUE) {/* dont need result */
        ap1 = gen_expr(node->v.p[0], F_ALL, siz1);
        gen_code(op, siz1, make_immed( amount ), ap1 );
        freeop(ap1);
        return NULL;
    }
    if (flags & F_DREG)
        ap2 = temp_data();
    else
        ap2 = temp_addr();

    ap1 = gen_expr(node->v.p[0], F_ALL, siz1);
    validate(ap1);
    gen_code(op_move, siz1, ap1, ap2);
    gen_code(op, siz1, make_immed(amount), ap1 );
    do_extend(ap1, siz1, size, flags, 1);
    freeop(ap1);
    return ap2;
}

int
getsize(node)

/*
 * return the (hopefully) base type size of a node.
 */
    struct enode   *node;
{
    int             siz0, siz1;

    if (node == NULL)
        return 0;
    switch (node->nodetype) {
    case en_icon:
        return 4;
    case en_d_ref:
    case en_cld:
    case en_cfd:
    case en_faddd:
    case en_fsubd:
    case en_fmuld:
    case en_fdivd:
    case en_fmodd:
    case en_fnegd:
    case en_faincd:
    case en_fadecd:
        return 8;
    case en_fcall:
        return (node->size) < stdint.size ? stdint.size : (node->size);
    case en_labcon:
    case en_nacon:
    case en_autocon:
    case en_l_ref:
    case en_ul_ref:
    case en_tempref:
    case en_cbl:
    case en_cwl:
    case en_clf:
    case en_cfl:
    case en_cdl:
    case en_cdf:
    case en_f_ref:
    case en_fadds:
    case en_fsubs:
    case en_fmuls:
    case en_fdivs:
    case en_fmods:
    case en_fnegs:
    case en_faincs:
    case en_fadecs:
        return 4;
    case en_b_ref:
    case en_ub_ref:
        return 2;
    case en_cbw:
    case en_w_ref:
    case en_uw_ref:
        return 2;
    case en_eq:
    case en_ne:
    case en_lt:
    case en_le:
    case en_gt:
    case en_ge:
    case en_land:
    case en_lor:
    case en_not:
    case en_compl:
        return 4;
    case en_uminus:
    case en_assign:
    case en_ainc:
    case en_adec:
        return getsize(node->v.p[0]);
    case en_add:
    case en_sub:
    case en_mul:
    case en_umul:
    case en_div:
    case en_udiv:
    case en_mod:
    case en_umod:
    case en_and:
    case en_or:
    case en_xor:
    case en_lsh:
    case en_rsh:

    case en_asadd:
    case en_assub:
    case en_asmul:
    case en_asdiv:
    case en_asmod:
    case en_asumul:
    case en_asudiv:
    case en_asumod:
    case en_asand:
    case en_asor:
    case en_aseor:
    case en_aslsh:
    case en_asrsh:
        siz0 = getsize(node->v.p[0]);
        siz1 = getsize(node->v.p[1]);
        if (siz1 > siz0)
            return siz1;
        else
            return siz0;
    case en_void:
    case en_cond:
        return getsize(node->v.p[1]);
    default:
        fprintf( stderr, "DIAG -- getsize error.\n" );
        break;
    }
    return 0;
}

int
push_param(ep, size)

/*
 * push the operand expression onto the stack. Return size pushed
 */
    struct enode   *ep;
    int             size;
{
    struct amode   *ap1, *ap2;

    if (ep == NULL) {
        fprintf( stderr, "DIAG -- NULL pointer in push_param\n" );
        return (0);
    }

    if (size == 8)
        ap1 = gen_expr(ep, F_ALL | F_FREG, size);
    else
        ap1 = gen_expr(ep, F_ALL, size);

    if (ap1->mode == am_freg) {
        gen_code(op_move, 4, makedreg((enum e_am) 1), push);
        gen_code(op_move, 4, makedreg((enum e_am) 0), push);
        size = 8;
    }
    else {
        if (size <= 4)
            gen_code(op_move, size, ap1, push);
        else {
            ap2 = copy_addr(ap1);
            ap2 = make_delta(ap2, 4);
            gen_code(op_move, 4, ap2, push);
            ap2 = copy_addr(ap1);
            gen_code(op_move, 4, ap2, push);
            size = 8;
        }
    }
    freeop(ap1);

    return size;
}

int
gen_parms(plist)

/*
 * push a list of parameters onto the stack and return the number of
 * parameters pushed.
 */
    struct enode   *plist;
{
    int             i, size;
    struct enode   *tplist;

    i = 0;
    if (plist != NULL) {
        tplist = plist->v.p[1];
        plist = plist->v.p[0];
    }
    while (plist != NULL) {
        size = getsize(plist->v.p[0]);
        i += push_param(plist->v.p[0], size);
        plist = plist->v.p[1];
        tplist = tplist->v.p[1];
    }
    return i;
}

struct amode   *
gen_fcall(node, flags)

/*
 * generate a function call node and return the address mode of the result.
 */
    struct enode   *node;
    int             flags;
{
    struct amode   *ap, *result;
    int             i;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_fcall.\n" );
        return NULL;
    }

    flush_regs();       /* Push the used registers on the stack */

    i = gen_parms(node->v.p[1]);    /* generate parameters */

    if (!gen_builtins(node, i)) {
        if (!gen_libcall(node, i)) {
            if (node->v.p[0]->nodetype == en_nacon)
                gen_code(op_jsr, 0, make_offset(node->v.p[0]), NULL);
            else {
                ap = gen_expr(node->v.p[0], F_AREG, 4);
                make_legal(ap, F_AREG, 4);
                ap->mode = am_ind;
                gen_code(op_jsr, 0, ap, NULL);
                freeop(ap);
            }
            if (i != 0)
                gen_code(op_add, 4, make_immed((long) i), makeareg(am_xpc));
        }
    }

    if (flags & F_FREG) {
        result = temp_float();
    }
    else {
        if (flags & F_DREG)
            result = temp_data();
        else
            result = temp_addr();
        if (result->preg != am_dreg || (flags & F_DREG) == 0)
            gen_code(op_move, 4, makedreg(am_dreg), result);
    }
    return result;
}

struct amode   *
gen_expr(node, flags, size)

/*
 * general expression evaluation. returns the addressing mode of the result.
 */
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2;
    struct enode   *ep;
    int             lab0, lab1;
    int             natsize;

    if (node == NULL)
        return NULL;

    switch (node->nodetype) {
    case en_stabn:
        return gen_stabn(node, flags, size);
        break;
    case en_stabs:
        return gen_stabs(node, flags, size);
        break;
    case en_fcon:
        ep = makenode(en_labcon, floatlit(node->v.f), NULL );
        ep = makenode(en_d_ref, ep, NULL );
        return(gen_expr(ep, flags, size));
    case en_labcon:
    case en_nacon:
    case en_icon:
        ap1 = (struct amode *) xalloc(sizeof(struct amode));
        ap1->signedflag = node->signedflag;
        ap1->mode = am_immed;
        ap1->offset = node;
        if (!(flags & F_IMMED))
            make_legal(ap1, flags, size);
        return (ap1);
    case en_autocon:
        ap2 = (struct amode *) xalloc(sizeof(struct amode));
        ap2->signedflag = node->signedflag;
        ap2->mode = am_immed;
        ap2->preg = (enum e_am) Options.Frame;  /* frame pointer */
        ap2->offset = node; /* use as constant node */
        if (!(flags & F_IMMED)) {
            natsize = natural_size(node);
            if (natsize > size)
                size = natsize;
            make_legal(ap2, flags, size);
        }
        return (ap2);
    case en_b_ref:
    case en_ub_ref:
    case en_w_ref:
    case en_uw_ref:
    case en_l_ref:
    case en_ul_ref:
    case en_m_ref:
    case en_f_ref:
    case en_d_ref:
        return gen_deref(node, flags, size);
    case en_clf:
    case en_cld:
    case en_cfl:
    case en_cfd:
        return gen_fsconvert(node, flags, size);
    case en_cdf:
    case en_cdl:
        return gen_fconvert(node, flags, size);
    case en_cbl:
    case en_cwl:
        return gen_extend(node, flags, size);
    case en_tempref:
        ap1 = (struct amode *) xalloc(sizeof(struct amode));
        ap1->signedflag = node->signedflag;
        if (node->v.i < 8) {
            ap1->mode = am_dreg;
            ap1->preg = (enum e_am) (node->v.i);
            ap1->deep = 0;
        }
        else {
            ap1->mode = am_areg;
            ap1->preg = (enum e_am) (node->v.i - 8);
            ap1->deep = 0;
        }
        ap1->tempflag = 0;  /* not a temporary */
        make_legal(ap1, flags, size);
        return ap1;
    case en_fnegd:
        return gen_funary(node, flags, size, op_fneg);
    case en_fnegs:
        return gen_fsunary(node, flags, size, op_fneg);
    case en_uminus:
        return gen_unary(node, flags, size, op_neg);
    case en_compl:
        return gen_unary(node, flags, size, op_not);
    case en_faddd:
        return gen_fbinary(node, flags, size, op_fadd);
    case en_fadds:
        return gen_fsbinary(node, flags, size, op_fadd);
    case en_add:
        return gen_binary(node, flags, size, op_add);
    case en_fsubd:
        return gen_fbinary(node, flags, size, op_fsub);
    case en_fsubs:
        return gen_fsbinary(node, flags, size, op_fsub);
    case en_sub:
        return gen_binary(node, flags, size, op_sub);
    case en_and:
        return gen_binary(node, flags, size, op_and);
    case en_or:
        return gen_binary(node, flags, size, op_or);
    case en_xor:
        return gen_xbin(node, flags, size, op_eor);
    case en_fmuld:
        return gen_fbinary(node, flags, size, op_fmul);
    case en_fmuls:
        return gen_fsbinary(node, flags, size, op_fmul);
    case en_fdivd:
        return gen_fbinary(node, flags, size, op_fdiv);
    case en_fdivs:
        return gen_fsbinary(node, flags, size, op_fdiv);
    case en_fmodd:
        return gen_fbinary(node, flags, size, op_fmod);
    case en_fmods:
        return gen_fsbinary(node, flags, size, op_fmod);
    case en_faincs:
        return gen_fsaincdec(node, flags, size, op_fadd);
    case en_fadecs:
        return gen_fsaincdec(node, flags, size, op_fsub);
    case en_faincd:
        return gen_faincdec(node, flags, size, op_fadd);
    case en_fadecd:
        return gen_faincdec(node, flags, size, op_fsub);
    case en_mul:
        return gen_mul(node, flags, size, op_muls);
    case en_umul:
        return gen_mul(node, flags, size, op_mulu);
    case en_div:
        return gen_modiv(node, flags, size, op_divs);
    case en_udiv:
        return gen_modiv(node, flags, size, op_divu);
    case en_mod:
        return gen_modiv(node, flags, size, op_mods);
    case en_umod:
        return gen_modiv(node, flags, size, op_modu);
    case en_lsh:
        return gen_shift(node, flags, size, op_asl);
    case en_rsh:
        return gen_shift(node, flags, size, op_asr);
    case en_asadd:
        return gen_asadd(node, flags, size, op_add);
    case en_asuadd:
        return gen_asadd(node, flags, size, op_add);  /* TODO - broken */
    case en_assub:
        return gen_asadd(node, flags, size, op_sub);
    case en_asusub:
        return gen_asadd(node, flags, size, op_sub);  /* TODO - broken */
    case en_asand:
        return gen_aslogic(node, flags, size, op_and);
    case en_asor:
        return gen_aslogic(node, flags, size, op_or);
    case en_aseor:
        return gen_aslogic(node, flags, size, op_eor);
    case en_aslsh:
        return gen_asshift(node, flags, size, op_asl);
    case en_asrsh:
        return gen_asshift(node, flags, size, op_asr);
    case en_asmul:
        return gen_asmul(node, flags, size);
    case en_asumul:
        return gen_asmul(node, flags, size);
    case en_asdiv:
        return gen_asmodiv(node, flags, size, op_divs);
    case en_asudiv:
        return gen_asmodiv(node, flags, size, op_divu);
    case en_asmod:
        return gen_asmodiv(node, flags, size, op_mods);
    case en_asumod:
        return gen_asmodiv(node, flags, size, op_modu);
    case en_assign:
        return gen_assign(node, flags, size);
    case en_ainc:
        return gen_aincdec(node, flags, size, op_add);
    case en_adec:
        return gen_aincdec(node, flags, size, op_sub);
    case en_land:
    case en_lor:
    case en_not:
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
        lab0 = nextlabel++;
        lab1 = nextlabel++;
        falsejp(node, lab0);
        ap1 = temp_data();
        gen_code(op_moveq, 0, make_immed(1L), ap1);
        gen_code(op_bra, 0, make_label(lab1), NULL);
        gen_label(lab0);
        gen_code(op_clr, 4, ap1, NULL);
        gen_label(lab1);
        return ap1;
    case en_cond:
        return gen_hook(node, flags, size);
    case en_void:
        natsize = natural_size(node->v.p[0]);
        ap1 = gen_expr( node->v.p[0], F_ALL | F_NOVALUE, natsize);
        freeop( ap1 );
        ap2 = gen_expr( node->v.p[1], flags, size );
        return( ap2 );
    case en_fcall:
        return gen_fcall(node, flags);
    default:
        fprintf( stderr, "DIAG -- uncoded node in gen_expr.\n");
        return NULL;
    }
    return NULL;
}

int
natural_size(node)

/*
 * return the natural evaluation size of a node.
 */
    struct enode   *node;
{
    int             siz0, siz1;
    struct enode   *ep1;

    if (node == NULL)
        return 0;
    switch (node->nodetype) {
    case en_clf:
    case en_cdf:
    case en_f_ref:
    case en_fnegs:
    case en_fadds:
    case en_fsubs:
    case en_fmuls:
    case en_fdivs:
    case en_fmods:
    case en_faincs:
    case en_fadecs:
        return 4;
    case en_d_ref:
    case en_cld:
    case en_cfd:
    case en_fnegd:
    case en_faddd:
    case en_fsubd:
    case en_fmuld:
    case en_fdivd:
    case en_fmodd:
    case en_faincd:
    case en_fadecd:
        return 8;
    case en_m_ref:
        ep1 = node->v.p[1];
        return (ep1->v.i);
        break;
    case en_icon:
        if (-128 <= node->v.i && node->v.i <= 127)
            return 1;
        if (-32767 <= node->v.i && node->v.i <= 32767)
            return 2;
        return 4;
    case en_fcall:
    case en_labcon:
    case en_nacon:
    case en_autocon:
    case en_l_ref:
    case en_ul_ref:
    case en_tempref:
    case en_cbl:
    case en_cwl:
    case en_cdl:
    case en_cfl:
        return 4;
    case en_b_ref:
    case en_ub_ref:
        return 1;
    case en_cbw:
    case en_w_ref:
    case en_uw_ref:
        return 2;
    case en_not:
    case en_compl:
    case en_uminus:
    case en_assign:
    case en_ainc:
    case en_adec:
        return natural_size(node->v.p[0]);
    case en_add:
    case en_sub:
    case en_mul:
    case en_div:
    case en_mod:
    case en_uadd:
    case en_usub:
    case en_umul:
    case en_udiv:
    case en_umod:
    case en_and:
    case en_or:
    case en_xor:
    case en_lsh:
    case en_rsh:
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
    case en_land:
    case en_lor:
    case en_asadd:
    case en_assub:
    case en_asmul:
    case en_asdiv:
    case en_asmod:
    case en_asumul:
    case en_asudiv:
    case en_asumod:
    case en_asand:
    case en_asor:
    case en_aseor:
    case en_aslsh:
    case en_asrsh:
        siz0 = natural_size(node->v.p[0]);
        siz1 = natural_size(node->v.p[1]);
        if (siz1 > siz0)
            return siz1;
        else
            return siz0;
    case en_void:
    case en_cond:
        return natural_size(node->v.p[1]);
    default:
        fprintf( stderr, "DIAG -- natural size error.\n" );
        break;
    }
    return 0;
}

void
gen_compare(node)

/*
 * generate code to do a comparison of the two operands of node.
 */
    struct enode   *node;
{
    struct amode   *ap1, *ap2;
    int             size, flags;

    if (node->v.p[0]->nodetype == en_cbl) {
        if (node->v.p[1]->nodetype == en_cbl) {
            node->v.p[0] = node->v.p[0]->v.p[0];
            node->v.p[1] = node->v.p[1]->v.p[0];
        }
        else if (isbyte(node->v.p[1])) {
            node->v.p[0] = node->v.p[0]->v.p[0];
        }
    }

    if (node->v.p[1]->nodetype == en_cbl) {
        if (isbyte(node->v.p[0])) {
            node->v.p[1] = node->v.p[1]->v.p[0];
        }
    }

    if (node->v.p[0]->nodetype == en_cwl) {
        if (node->v.p[1]->nodetype == en_cwl) {
            node->v.p[0] = node->v.p[0]->v.p[0];
            node->v.p[1] = node->v.p[1]->v.p[0];
        }
        else if (isshort(node->v.p[1])) {
            node->v.p[0] = node->v.p[0]->v.p[0];
        }
    }

    if (node->v.p[1]->nodetype == en_cwl) {
        if (isshort(node->v.p[0])) {
            node->v.p[1] = node->v.p[1]->v.p[0];
        }
    }

    size = natural_size(node);

    if (size <= 4) {
        flags = F_AREG | F_DREG | F_IMMED | F_MEM;
        ap1 = gen_expr(node->v.p[0], F_DREG, size);
        make_legal(ap1, F_DREG, size);
        ap2 = gen_expr(node->v.p[1], flags, size);
        make_legal(ap2, flags, size);
        validate(ap1);
        gen_code(op_cmp, size, ap1, ap2);
        freeop(ap2);
        freeop(ap1);
    }
    else {
        ap1 = gen_fbinary(node, F_FREG, size, op_cmp);
        freeop(ap1);
        gen_code(op_cmp, 4, make_immed((long) 0), makedreg((enum e_am) 0));
    }
}

void
truejp(node, label)

/*
 * generate a jump to label if the node passed evaluates to a true condition.
 */
    struct enode   *node;
    int             label;
{
    struct amode   *ap1;
    int             siz1;
    int             lab0;

    if (node == NULL)
        return;
    switch (node->nodetype) {
    case en_eq:
        gen_compare(node);
        gen_code(op_beq, 0, make_label(label), NULL);
        break;
    case en_ne:
        gen_compare(node);
        gen_code(op_bne, 0, make_label(label), NULL);
        break;
    case en_lt:
        gen_compare(node);
        gen_code(op_blt, 0, make_label(label), NULL);
        break;
    case en_le:
        gen_compare(node);
        gen_code(op_ble, 0, make_label(label), NULL);
        break;
    case en_gt:
        gen_compare(node);
        gen_code(op_bgt, 0, make_label(label), NULL);
        break;
    case en_ge:
        gen_compare(node);
        gen_code(op_bge, 0, make_label(label), NULL);
        break;
    case en_ult:
        gen_compare(node);
        gen_code(op_bcs, 0, make_label(label), NULL);
        break;
    case en_ule:
        gen_compare(node);
        gen_code(op_bls, 0, make_label(label), NULL);
        break;
    case en_ugt:
        gen_compare(node);
        gen_code(op_bhi, 0, make_label(label), NULL);
        break;
    case en_uge:
        gen_compare(node);
        gen_code(op_bcc, 0, make_label(label), NULL);
        break;
    case en_land:
        lab0 = nextlabel++;
        falsejp(node->v.p[0], lab0);
        truejp(node->v.p[1], label);
        gen_label(lab0);
        break;
    case en_lor:
        truejp(node->v.p[0], label);
        truejp(node->v.p[1], label);
        break;
    case en_not:
        falsejp(node->v.p[0], label);
        break;
    default:
        siz1 = natural_size(node);
        ap1 = gen_expr(node, F_DREG, siz1);
        make_legal(ap1, F_DREG, siz1);
        gen_code(op_tst, siz1, ap1, NULL);
        freeop(ap1);
        gen_code(op_bne, 0, make_label(label), NULL);
        break;
    }
}

void
falsejp(node, label)

/*
 * generate code to execute a jump to label if the expression passed is
 * false.
 */
    struct enode   *node;
    int             label;
{
    struct amode   *ap;
    int             siz1;
    int             lab0;

    if (node == NULL)
        return;
    switch (node->nodetype) {
    case en_eq:
        gen_compare(node);
        gen_code(op_bne, 0, make_label(label), NULL);
        break;
    case en_ne:
        gen_compare(node);
        gen_code(op_beq, 0, make_label(label), NULL);
        break;
    case en_lt:
        gen_compare(node);
        gen_code(op_bge, 0, make_label(label), NULL);
        break;
    case en_le:
        gen_compare(node);
        gen_code(op_bgt, 0, make_label(label), NULL);
        break;
    case en_gt:
        gen_compare(node);
        gen_code(op_ble, 0, make_label(label), NULL);
        break;
    case en_ge:
        gen_compare(node);
        gen_code(op_blt, 0, make_label(label), NULL);
        break;
    case en_ult:
        gen_compare(node);
        gen_code(op_bcc, 0, make_label(label), NULL);
        break;
    case en_ule:
        gen_compare(node);
        gen_code(op_bhi, 0, make_label(label), NULL);
        break;
    case en_ugt:
        gen_compare(node);
        gen_code(op_bls, 0, make_label(label), NULL);
        break;
    case en_uge:
        gen_compare(node);
        gen_code(op_bcs, 0, make_label(label), NULL);
        break;
    case en_land:
        falsejp(node->v.p[0], label);
        falsejp(node->v.p[1], label);
        break;
    case en_lor:
        lab0 = nextlabel++;
        truejp(node->v.p[0], lab0);
        falsejp(node->v.p[1], label);
        gen_label(lab0);
        break;
    case en_not:
        truejp(node->v.p[0], label);
        break;
    default:
        siz1 = natural_size(node);
        ap = gen_expr(node, F_DREG, siz1);
        make_legal(ap, F_DREG, siz1);
        gen_code(op_tst, siz1, ap, NULL);
        freeop(ap);
        gen_code(op_beq, 0, make_label(label), NULL);
        break;
    }
}
