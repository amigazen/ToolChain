
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
 * $Log:	PeepGen.c,v $
 * Revision 3.33  90/04/05  22:43:45  lionel
 * Added code to rip out superfluous sign extensions after a moveq subst.
 * 
 * Revision 3.32  90/02/03  16:25:14  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * PeepGen.c
 * 
 * Improves code generation using peephole optimization.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

void   opt3();
extern char    *xalloc();

struct ocode   *peep_head = NULL, *peep_tail = NULL;

struct amode   *
copy_addr(ap)

/*
 * copy an address mode structure (these things dont last).
 */
    struct amode   *ap;
{
    struct amode   *newap;

    if (ap == NULL)
        return NULL;
    newap = (struct amode *) xalloc(sizeof(struct amode));
    newap->mode = ap->mode;
    newap->preg = ap->preg;
    newap->sreg = ap->sreg;
    newap->tempflag = ap->tempflag;
    newap->deep = ap->deep;
    newap->offset = ap->offset;
    return newap;
}

void
add_peep(new)

/*
 * add the ocoderuction pointed to by new to the peep list.
 */
    struct ocode   *new;
{
    if (peep_head == NULL) {
        peep_head = peep_tail = new;
        new->fwd = NULL;
        new->back = NULL;
    }
    else {
        new->fwd = NULL;
        new->back = peep_tail;
        peep_tail->fwd = new;
        peep_tail = new;
    }
}

void
gen_code(op, len, ap1, ap2)

/*
 * generate a code sequence into the peep list.
 */
    enum e_op       op;
    int             len;
    struct amode   *ap1, *ap2;
{
    struct ocode   *new;

    new = (struct ocode *) xalloc(sizeof(struct ocode));
    new->opcode = op;
    new->length = len;
    new->oper1 = copy_addr(ap1);
    new->oper2 = copy_addr(ap2);
    add_peep(new);
}

void
gen_label(labno)

/*
 * add a compiler generated label to the peep list.
 */
    int             labno;
{
    struct ocode   *new;

    new = (struct ocode *) xalloc(sizeof(struct ocode));
    new->opcode = op_label;
    new->oper1 = (struct amode *) labno;
    add_peep(new);
}

void
put_ocode(p)

/*
 * output the instruction passed.
 */
    struct ocode   *p;
{
    put_code(p->opcode, p->length, p->oper1, p->oper2);
}

void
flush_peep()

/*
 * output all code and labels in the peep list.
 */
{
    if (Options.Optimize)
        opt3();     /* do the peephole optimizations */
    while (peep_head != NULL) {
        if (peep_head->opcode == op_label)
            put_label((long) (peep_head->oper1));
        else
            put_ocode(peep_head);
        peep_head = peep_head->fwd;
    }
}

void
peep_move(ip)

/*
 * peephole optimization for move instructions. makes quick immediates when
 * possible. changes move #0,d to clr d. changes long moves to address
 * registers to short when possible. changes move immediate to stack to pea.
 */
    struct ocode   *ip;
{
    struct ocode   *prev;
    struct enode   *ep;

    if (ip->oper1->mode != am_immed) {
        if (ip->oper1->mode == am_ainc) {   /* move (A7)+,xxxx  */
            prev = ip->back;
            while (prev != NULL && is_comment(prev->opcode))
                prev = prev->back;
            if (prev != NULL) {
                if (prev->opcode == op_pea && ip->oper2->mode == am_areg) {
                    if (ip->length == 4) {
                        prev->opcode = op_lea;
                        prev->oper2 = ip->oper2;
                        prev->fwd = ip->fwd;
                        if (ip->fwd != NULL)    /* Delete 2'nd  */
                            ip->fwd->back = prev;
                    }
                }
                if (prev->opcode == op_move && prev->length == ip->length) {
                    if (prev->oper2->mode == am_adec) {
                        if (ip->oper1->preg == prev->oper2->preg) {
                            prev->oper2 = ip->oper2;
                            prev->fwd = ip->fwd;
                            if (ip->fwd != NULL)    /* Delete 2'nd  */
                                ip->fwd->back = prev;
                        }
                    }
                }
            }
            return;
        }

        if (ip->oper1->mode != am_dreg && ip->oper1->mode != am_areg)
            return;
        if (ip->oper2->mode != am_dreg && ip->oper2->mode != am_areg)
            return;

        prev = ip->back;
        while (prev != NULL && is_comment(prev->opcode))
            prev = prev->back;

        if (prev == NULL || prev->opcode != op_move)
            return;

        if ( ip->length == prev->length &&
            equal_address(ip->oper1, prev->oper1) &&    /* move.l A0,D0 */
            equal_address(ip->oper2, prev->oper2)) {    /* move.l A0,D0 */
            prev->fwd = ip->fwd;
            if (ip->fwd != NULL)    /* Delete 2'nd  */
                ip->fwd->back = prev;
            return;
        }
        if ( ip->length == prev->length &&
            equal_address(ip->oper1, prev->oper2) &&    /* move.l A0,D0 */
            equal_address(ip->oper2, prev->oper1)) {    /* move.l D0,A0 */
            if (ip->oper1->mode == ip->oper2->mode ||
                prev->oper2->mode == am_dreg) {
                prev->fwd = ip->fwd;
                if (ip->fwd != NULL)    /* Delete 2'nd  */
                    ip->fwd->back = prev;
                return;
            }
        }
        return;
    }

    ep = ip->oper1->offset;

    if (ep->nodetype != en_icon) {
        if (ip->oper2->mode == am_areg) {
            ip->opcode = op_lea;
            ip->length = 0;
            ip->oper1->mode = am_direct;
        }
        if (ip->oper2->mode == am_adec && (int) ip->oper2->preg == 7) {
            ip->opcode = op_pea;
            ip->length = 0;
            ip->oper1->mode = am_direct;
            ip->oper2 = NULL;
        }
        return;
    }
    if (ip->oper2->mode == am_areg) {
        if (-32767 < ep->v.i && ep->v.i <= 32767)
            ip->length = 2;
    }
    else if (ip->oper2->mode == am_dreg) {
        if (-128 <= ep->v.i && ep->v.i <= 127) {
            ip->opcode = op_moveq;
            ip->length = 0;
            while (ip->fwd && ip->fwd->opcode == op_ext) {
                ip->fwd = ip->fwd->fwd;
                if (ip->fwd != NULL) {
                    ip->fwd->back = ip;
                }
            }
        }
    }
    else {
        if (ep->v.i == 0) {
            ip->opcode = op_clr;
            ip->oper1 = ip->oper2;
            ip->oper2 = NULL;
        }
        else if (ip->oper2->mode == am_adec && (int) (ip->oper2)->preg == 7) {
            ip->opcode = op_pea;
            ip->length = 0;
            ip->oper1->mode = am_direct;
            ip->oper2 = NULL;
        }
    }
}

int
equal_address(ap1, ap2)

/*
 * compare two address nodes and return true if they are equivalent.
 */
    struct amode   *ap1, *ap2;
{
    if (ap1 == NULL || ap2 == NULL || ap1->mode != ap2->mode)
        return FALSE;

    switch (ap1->mode) {
    case am_freg:
        return TRUE;
    case am_areg:
    case am_dreg:
    case am_ainc:
    case am_adec:
        return ap1->preg == ap2->preg;
    case am_indx:
        return (ap1->preg == ap2->preg) &&
            equalnode(ap1->offset, ap2->offset);
    case am_indx2:
    case am_indx3:
        return (ap1->preg == ap2->preg) &&
            (ap1->sreg == ap2->sreg);
    }
    return FALSE;
}

void
peep_asl(ip)

/*
 * peephole optimization for add instructions. makes quick immediates out of
 * small constants.
 */
    struct ocode   *ip;
{
    struct enode   *ep;

    if (ip->oper1->mode != am_immed)
        return;

    ep = ip->oper1->offset;

    if (ep->nodetype != en_icon)
        return;

    if (ip->oper2->mode != am_areg && ip->oper2->mode != am_dreg)
        return;

    if (ep->v.i == 1) {
        ip->opcode = op_add;
        ip->oper1 = ip->oper2;
    }
}

void
peep_add(ip)

/*
 * peephole optimization for add instructions. makes quick immediates out of
 * small constants.
 */
    struct ocode   *ip;
{
    struct enode   *ep;

    if (ip->oper1->mode != am_immed)
        return;
    ep = ip->oper1->offset;
    if (ip->oper2->mode != am_areg)
        ip->opcode = op_addi;
    else {
        if (isshort(ep))
            ip->length = 2;
    }
    if (ep->nodetype != en_icon)
        return;
    if (1 <= ep->v.i && ep->v.i <= 8)
        ip->opcode = op_addq;
    else if (-8 <= ep->v.i && ep->v.i <= -1) {
        ip->opcode = op_subq;
        ep->v.i = -ep->v.i;
    }
}

void
peep_sub(ip)

/*
 * peephole optimization for subtract instructions. makes quick immediates
 * out of small constants.
 */
    struct ocode   *ip;
{
    struct enode   *ep;

    if (ip->oper1->mode != am_immed)
        return;
    ep = ip->oper1->offset;
    if (ip->oper2->mode != am_areg)
        ip->opcode = op_subi;
    else {
        if (isshort(ep))
            ip->length = 2;
    }
    if (ep->nodetype != en_icon)
        return;
    if (1 <= ep->v.i && ep->v.i <= 8)
        ip->opcode = op_subq;
    else if (-8 <= ep->v.i && ep->v.i <= -1) {
        ip->opcode = op_addq;
        ep->v.i = -ep->v.i;
    }
}

int
is_comment(op)
    enum e_op       op;
{
    return (op == op_comment || op == op_stabn || op == op_stabs);
}

void
peep_tst(ip)

/*
 * peephole optimization for test instructions. if previous instruction
 * should have set the condition codes properly delete. return value is true
 * if instruction was deleted.
 */
    struct ocode   *ip;
{
    struct ocode   *prev;

    if (ip->oper1->mode == am_areg) 
        return;

    prev = ip;
    do {
        prev = prev->back;
        if (prev == NULL)
            return;
    } while (is_comment(prev->opcode));

    if ((((prev->opcode == op_move || prev->opcode == op_moveq)
          && equal_address(prev->oper1, ip->oper1))
         && prev->oper2->mode != am_areg)
        || ((prev->opcode == op_ext) && (prev->length == ip->length)
        && equal_address(prev->oper1, ip->oper1))
        || (prev->opcode != op_label
        && equal_address(prev->oper2, ip->oper1))) {
        prev->fwd = ip->fwd;
        if (prev->fwd != NULL)
            prev->fwd->back = prev;
    }
}

void
peep_cmp(ip)

/*
 * peephole optimization for compare instructions. changes compare #0 to tst
 * and if previous instruction should have set the condition codes properly
 * delete. return value is true if instruction was deleted.
 */
    struct ocode   *ip;
{
    struct enode   *ep;
    int             side;

    if (ip->oper1->mode == am_immed) {
        ep = ip->oper1->offset;
        side = 1;
    }
    else if (ip->oper2->mode == am_immed) {
        ep = ip->oper2->offset;
        side = 2;
    }
    else
        return;

    if (ip->oper2->mode == am_areg || ip->oper1->mode == am_areg) {
        if (isshort(ep))
            ip->length = 2;
        return;
    }

    ip->opcode = op_cmpi;

    if (ep->nodetype != en_icon || ep->v.i != 0)
        return;

    if (side == 1)
        ip->oper1 = ip->oper2;

    ip->oper2 = NULL;
    ip->opcode = op_tst;

    peep_tst(ip);
}

void
peep_muldiv(ip, op)

/*
 * changes multiplies and divides by convienient values to shift operations.
 * op should be either op_asl or op_asr (for divide).
 */
    struct ocode   *ip;
    enum e_op       op;
{
    int             shcnt;

    if (Options.MulDiv32 || ip->oper1->mode != am_immed)
        return;

    if (ip->oper1->offset->nodetype != en_icon)
        return;

    switch (shcnt = (int) (ip->oper1->offset->v.i)) {
    case 2:
        shcnt = 1;
        break;
    case 4:
        shcnt = 2;
        break;
    case 8:
        shcnt = 3;
        break;
    case 16:
        shcnt = 4;
        break;
    case 32:
        shcnt = 5;
        break;
    case 64:
        shcnt = 6;
        break;
    case 128:
        shcnt = 7;
        break;
    case 256:
        shcnt = 8;
        break;
    case 512:
        shcnt = 9;
        break;
    case 1024:
        shcnt = 10;
        break;
    case 2048:
        shcnt = 11;
        break;
    case 4096:
        shcnt = 12;
        break;
    case 8192:
        shcnt = 13;
        break;
    case 16384:
        shcnt = 14;
        break;
    default:
        return;
    }
    ip->oper1->offset->v.i = shcnt;
    ip->opcode = op;
    ip->length = 4;
}

void
peep_uctran(ip)

/*
 * peephole optimization for unconditional transfers. deletes instructions
 * which have no path. applies to bra, jmp, and rts instructions.
 */
    struct ocode   *ip;
{
    while (ip->fwd != NULL && ip->fwd->opcode != op_label) {
        ip->fwd = ip->fwd->fwd;
        if (ip->fwd != NULL)
            ip->fwd->back = ip;
    }

    if (ip->opcode != op_jmp && ip->opcode != op_bra)
        return;

    if (ip->fwd == NULL || ip->fwd->opcode != op_label)
        return;

    if (ip->oper1 == NULL || ip->oper1->mode != am_direct)
        return;

    if (ip->oper1->offset == NULL || ip->oper1->offset->nodetype != en_labcon)
        return;

    if ((int) ip->fwd->oper1 == ip->oper1->offset->v.i) {
        ip->back->fwd = ip->fwd;
        if (ip->fwd != NULL)
            ip->fwd->back = ip->back;
    }
}

void
opt3()

/*
 * peephole optimizer. This routine calls the instruction specific
 * optimization routines above for each instruction in the peep list.
 */
{
    struct ocode   *ip;

    for (ip = peep_head; ip != NULL; ip = ip->fwd) {
        switch (ip->opcode) {
        case op_asl:
            peep_asl(ip);
            break;
        case op_move:
            peep_move(ip);
            break;
        case op_add:
            peep_add(ip);
            break;
        case op_sub:
            peep_sub(ip);
            break;
        case op_cmp:
            peep_cmp(ip);
            break;
        case op_tst:
            peep_tst(ip);
            break;
        case op_muls:
            peep_muldiv(ip, op_asl);
            break;
        case op_divs:
            peep_muldiv(ip, op_asr);
            break;
        case op_mulu:
            peep_muldiv(ip, op_lsl);
            break;
        case op_divu:
            peep_muldiv(ip, op_lsr);
            break;
        case op_bra:
        case op_jmp:
        case op_rts:
            peep_uctran(ip);
        }
    }
}
