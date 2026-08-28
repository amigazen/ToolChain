
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
 * $Log:	GenFloat.c,v $
 * Revision 3.33  90/04/05  22:33:46  lionel
 * Changed library function callouts to prevent collisions with user functions
 * 
 * Revision 3.32  90/02/03  16:24:13  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * GenFloat.c
 * 
 * This module contains all of the code generation routines for evaluating
 * floating-point expressions and conditions.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

struct amode   *temp_data(), *temp_addr(), *makeareg(), *makedreg();
struct amode   *copy_addr(), *temp_float();
struct amode   *make_autocon(), *make_delta();
struct enode   *makenode();

struct amode   *request_addr(), *request_data(), *request_float();
struct amode   *request_reg();

struct amode   *gen_expr(); /* forward declaration */
void            swap_nodes();   /* ditto */

extern int      avail_data(), avail_addr();
extern struct amode *check_float();

extern struct amode push[], pop[];

/*
 * Soft-float routines leave the double result in D0:D1.  Parking that
 * pair in a fresh frame slot (and returning a memory amode) avoids
 * temp_float(): allocating an freg after FD* often gen_push-es a stale
 * D0 as sr_data - seen as move.l D0,-(A7) in getfrac, which locked the
 * Amiga when ac-self compiled a file containing 1.0.
 */
/*
 * Soft-float D0:D1 -> fresh frame slot.  Also used by make_legal(F_FREG)
 * so integer->double conversions do not share one float_auto cell.
 */
struct amode   *
float_result_mem()
{
    struct amode   *mem, *dst;

    lc_auto += 8;
    mem = make_autocon(-lc_auto);
    dst = copy_addr(mem);
    gen_code(op_move, 4, makedreg((enum e_am) 0), dst);
    dst = make_delta(dst, 4);
    gen_code(op_move, 4, makedreg((enum e_am) 1), dst);
    mem->tempflag = 1;
    return mem;
}

struct amode   *
gen_stabn(node, flags, size)

/*
 * Generate the code for a op_stabn instruction
 */
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2;

    if (node == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_stabn.\n" );
#endif
        return NULL;
    }

    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);
    ap2 = gen_expr(node->v.p[1], flags, size);
    make_legal(ap2, flags, size);

    return ap1;
}


struct amode   *
gen_stabs(node, flags, size)

/*
 * Generate the code for a op_stabs instruction
 */
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2;

    if (node == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_stabn.\n" );
#endif
        return NULL;
    }

    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);
    ap2 = gen_expr(node->v.p[1], flags, size);
    make_legal(ap2, flags, size);

    return ap1;
}

struct amode   *
gen_fsconvert(node, flags, size)
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2, *ap3;

    flags = F_DREG;
    size = 4;

    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);

    validate(ap1);
    if ((int) ap1->preg != 0) {
        ap2 = request_data(0);
        gen_code(op_move, 4, ap1, makedreg((enum e_am) 0));
        freeop(ap2);
    }
    freeop(ap1);

    ap3 = request_addr(0);
    freeop(ap3);

    switch (node->nodetype) {
    case en_cld:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".Fl2d");
        break;
    case en_cfd:
        PdcFlags |= PDC_IEEEDOUBLE | PDC_IEEESINGLE;
        call_library(".Fs2d");
        break;
    case en_clf:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".Fl2s");
        ap2 = temp_data();
        if ((int)ap2->preg != 0)
            gen_code( op_move, 4, makedreg(0), ap2 );
        return (ap2);
    case en_cfl:
        /*
         * float->long.  Prefer .Fs2d then .Fd2l: shipped math.lib
         * mis-XDEFs the .Fs2l stub as .Fd2l, so .Fs2l never links.
         */
        PdcFlags |= PDC_IEEESINGLE | PDC_IEEEDOUBLE;
        call_library(".Fs2d");
        call_library(".Fd2l");
        ap2 = temp_data();
        if ((int)ap2->preg != 0)
            gen_code( op_move, 4, makedreg(0), ap2 );
        return (ap2);

    }

    /* Fl2d / Fs2d -> D0:D1; park in memory (see float_result_mem). */
    return float_result_mem();
}

struct amode   *
gen_fconvert(node, flags, size)
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2, *apt;

    /*
     * Soft-float .Fd2s / .Fd2l want the double in D0:D1.  Do not use
     * make_legal(F_FREG): that path calls temp_float() after loading
     * D0:D1 and can push/clobber the pair (seen as a hang on 1.0f when
     * Expr used en_cdf).  Load from memory like gen_fbinary's left op.
     */
    size = 8;
    flags = F_MEM | F_IMMED;

    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);
    if (ap1 != NULL && ap1->mode == am_freg)
        ap1 = check_float(ap1);

    if (ap1 == NULL)
        return NULL;

    validate(ap1);
    gen_code(op_move, 4, ap1, makedreg((enum e_am) 0));
    apt = make_delta(copy_addr(ap1), 4);
    gen_code(op_move, 4, apt, makedreg((enum e_am) 1));
    freeop(ap1);

    switch (node->nodetype) {
    case en_cdl:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".Fd2l");
        break;
    case en_cdf:
        PdcFlags |= PDC_IEEEDOUBLE | PDC_IEEESINGLE;
        call_library(".Fd2s");
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- uncoded floating convert\n" );
#endif
        break;
    }

    ap2 = temp_data();
    if ((int) ap2->preg != 0)
        gen_code(op_move, 4, makedreg((enum e_am) 0), ap2);

    return (ap2);
}

struct amode   *
gen_fsunary(node, flags, size, op)

/*
 * generate code to evaluate a unary minus or complement.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap, *ap2;

    if (node == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_fsunary.\n" );
#endif
        return NULL;
    }

    flags = F_DREG;
    ap = gen_expr(node->v.p[0], F_DREG, size);
    make_legal(ap, flags, size);

    if ((int) ap->preg != 0) {
        ap2 = request_data(0);
        gen_code(op_move, 4, ap, makedreg((enum e_am) 0));
        freeop(ap2);
    }
    freeop(ap);

    switch (op) {
    case op_fneg:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSneg");
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- uncoded unary float operation\n" );
#endif
        break;
    }

    ap = temp_data();
    if ((int) ap->preg != 0)
        gen_code(op_move, 4, makedreg((enum e_am) 0), ap);

    return ap;
}

struct amode   *
gen_funary(node, flags, size, op)

/*
 * generate code to evaluate a unary minus or complement.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap;

    if (node == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_funary.\n" );
#endif
        return NULL;
    }

    flags = F_FREG;

    ap = gen_expr(node->v.p[0], flags, size);
    make_legal(ap, flags, size);

    /* Ensure operand is in D0:D1 for .FDneg. */
    if (ap->mode != am_freg || (int) ap->preg != 0) {
        if (ap->mode == am_freg)
            ap = check_float(ap);
        validate(ap);
        gen_code(op_move, 4, ap, makedreg((enum e_am) 0));
        gen_code(op_move, 4, make_delta(copy_addr(ap), 4),
                 makedreg((enum e_am) 1));
    }
    else
        validate(ap);
    freeop(ap);

    switch (op) {
    case op_fneg:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDneg");
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- uncoded unary float operation\n" );
#endif
        break;
    }

    return float_result_mem();
}

struct amode   *
gen_fsbinary(node, flags, size, op)

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
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_fsbinary.\n" );
#endif
        return NULL;
    }

    flags = F_DREG;

    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);
    ap2 = gen_expr(node->v.p[1], flags, size);
    make_legal(ap2, flags, size);

    validate(ap2);
    validate(ap1);

    if (op == op_fadd || op == op_fmul) {
        if ((int) ap1->preg == 0) {
            if ((int) ap2->preg != 1) {
                ap3 = request_data(1);
                gen_code(op_move, 4, ap2, ap3);
                freeop(ap3);
            }
        }
        else if ((int) ap1->preg == 1) {
            if ((int) ap2->preg != 0) {
                ap3 = request_data(0);
                gen_code(op_move, 4, ap2, ap3);
                freeop(ap3);
            }
        }
        else {
            if ((int) ap2->preg == 0) {
                ap3 = request_data(1);
                gen_code(op_move, 4, ap1, ap3);
                freeop(ap3);
            }
            else if ((int) ap2->preg == 1) {
                ap3 = request_data(0);
                gen_code(op_move, 4, ap1, ap3);
                freeop(ap3);
            }
        }
    }
    else {
        if ((int) ap1->preg == 0) {
            if ((int) ap2->preg != 1) {
                ap3 = request_data(1);
                gen_code(op_move, 4, ap2, ap3);
                freeop(ap3);
            }
        }
        else if ((int) ap2->preg == 1) {
            ap3 = request_data(0);
            gen_code(op_move, 4, ap1, ap3);
            freeop(ap3);
        }
        else {
            if ((int) ap1->preg == 1) {
                if ((int) ap2->preg == 0)
                    gen_code(op_exg, 4, ap1, ap2);
                else {
                    ap3 = request_data(0);
                    gen_code(op_move, 4, ap1, ap3);
                    gen_code(op_move, 4, ap2, ap1);
                    freeop(ap3);
                }
            }
            else if ((int) ap2->preg == 0) {
                ap3 = request_data(1);
                gen_code(op_move, 4, ap2, ap3);
                gen_code(op_move, 4, ap1, ap2);
                freeop(ap3);
            }
        }
    }

    freeop(ap2);
    freeop(ap1);

    switch (op) {
    case op_fadd:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSadd");
        break;
    case op_fsub:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSsub");
        break;
    case op_fmul:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSmul");
        break;
    case op_fdiv:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSdiv");
        break;
    case op_fmod:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSmod");
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- uncoded binary floating operation\n" );
#endif
        break;
    }

    ap1 = temp_data();

    if ((int) ap1->preg != 0)
        gen_code(op_move, size, makedreg((enum e_am) 0), ap1);

    return ap1;
}

struct amode   *
gen_fbinary(node, flags, size, op)

/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 *
 * Amiga soft-float ABI: left in D0:D1, right addressed by A0, result in D0:D1.
 * Always park the right-hand double in a frame temp and load the left into
 * D0:D1 - the old freg/push shuffle left unbalanced -(A7) in getfrac
 * (gen-1 partially recovered; gen-2 leaked 8-12 bytes per digit).
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    int             used;
    struct amode   *ap1, *ap2, *apt;

    if (node == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_fbinary.\n" );
#endif
        return NULL;
    }

    size = 8;

    /*
     * Right operand -> memory EA.  Use size 8 with F_MEM so gen_deref does
     * not call do_extend(8->4), which used to emit move.f into An.
     */
    if (node->v.p[1]->nodetype == en_d_ref) {
        ap2 = gen_expr(node->v.p[1], F_MEM | F_IMMED, 8);
        make_legal(ap2, F_MEM | F_IMMED, 8);
    }
    else {
        ap2 = gen_expr(node->v.p[1], F_FREG | F_MEM, 8);
        make_legal(ap2, F_FREG | F_MEM, size);
    }
    if (ap2 != NULL && ap2->mode == am_freg)
        ap2 = check_float(ap2);

    /* Left operand -> memory EA, then load into D0:D1 below. */
    if (node->v.p[0]->nodetype == en_d_ref) {
        ap1 = gen_expr(node->v.p[0], F_MEM | F_IMMED, 8);
        make_legal(ap1, F_MEM | F_IMMED, 8);
    }
    else {
        ap1 = gen_expr(node->v.p[0], F_FREG | F_MEM, 8);
        make_legal(ap1, F_FREG | F_MEM, size);
    }

    if (ap1 == NULL || ap2 == NULL)
        return (NULL);

    if (ap1->mode != am_freg || (int) ap1->preg != 0) {
        /* Load 64-bit value from memory into D0:D1. */
        if (ap1->mode == am_freg)
            ap1 = check_float(ap1);
        validate(ap1);
        gen_code(op_move, 4, ap1, makedreg((enum e_am) 0));
        apt = make_delta(copy_addr(ap1), 4);
        gen_code(op_move, 4, apt, makedreg((enum e_am) 1));
    }
    else {
        validate(ap1);
    }

    /*
     * Soft-float ABI: A0 must address the 8-byte right operand.
     * Loading the left from 8(A0) leaves A0 as the struct base - lea
     * before .FD* so A0 becomes &double (was the Optimize.c crash).
     * am_areg / am_ind / 0(A0) already hold &double.
     *
     * If left load used A0 as base and right also needs that A0, validate
     * restored it; still lea when the EA is not already a naked pointer.
     */
    used = FALSE;
    validate(ap2);
    {
        int             a0_ok;

        a0_ok = 0;
        if ((int) ap2->preg == 0) {
            if (ap2->mode == am_areg || ap2->mode == am_ind)
                a0_ok = 1;
            else if (ap2->mode == am_indx && ap2->offset != NULL
                     && ap2->offset->nodetype == en_icon
                     && ap2->offset->v.i == 0)
                a0_ok = 1;
        }
        if (!a0_ok) {
            if (used = (!avail_addr(0)))
                gen_code(op_move, 4, makeareg((enum e_am) 0), push);
            gen_code(op_lea, 0, ap2, makeareg((enum e_am) 0));
        }
    }

    freeop(ap1);
    freeop(ap2);

    switch (op) {
    case op_fadd:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDadd");
        break;
    case op_fmul:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDmul");
        break;
    case op_fsub:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDsub");
        break;
    case op_fdiv:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDdiv");
        break;
    case op_fmod:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDmod");
        break;
    case op_cmp:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDcmp");
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- uncoded binary floating operation\n" );
#endif
        break;
    }

    if (used)
        gen_code(op_move, 4, pop, makeareg((enum e_am) 0));

    return float_result_mem();
}

/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */

struct amode   *
gen_fsaincdec(node, flags, size, op)

/*
 * float postfix ++/-- (en_faincs).  Soft-float single: left in D0, right in
 * D1, .FSadd/.FSsub -> D0.  Must store the new value back through the lvalue
 * and return the old float (same contract as gen_faincdec for doubles).
 * The old gen_fsaincdec only computed FSadd into a temp and never wrote
 * memory - so f++ left f unchanged.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3, *apold;

    if (node == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_fsaincdec.\n" );
#endif
        return NULL;
    }

    size = 4;
    flags = F_MEM | F_ALL;

    /* Amount first (usually 1.0f / -1.0f) so D0 is free for the load. */
    ap2 = gen_expr(node->v.p[1], F_DREG | F_IMMED, size);
    make_legal(ap2, F_DREG | F_IMMED, size);

    /* Lvalue as memory - need an address we can store through. */
    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);

    if (ap1 == NULL || ap2 == NULL)
        return NULL;

    validate(ap1);
    validate(ap2);

    ap3 = copy_addr(ap1);

    /* Postfix result = old value; push from memory before clobbering regs. */
    gen_code(op_move, 4, ap1, push);

    /* Soft-float single ABI: D0 = left (old), D1 = right (delta). */
    if (ap2->mode == am_dreg && (int) ap2->preg == 0) {
        gen_code(op_move, 4, ap2, makedreg((enum e_am) 1));
        gen_code(op_move, 4, ap1, makedreg((enum e_am) 0));
    }
    else {
        gen_code(op_move, 4, ap1, makedreg((enum e_am) 0));
        if (ap2->mode != am_dreg || (int) ap2->preg != 1)
            gen_code(op_move, 4, ap2, makedreg((enum e_am) 1));
    }

    freeop(ap1);
    freeop(ap2);

    ap3 = request_reg(ap3);

    switch (op) {
    case op_fadd:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSadd");
        break;
    case op_fsub:
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".FSsub");
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- uncoded binary floating operation\n" );
#endif
        break;
    }

    gen_code(op_move, 4, makedreg((enum e_am) 0), ap3);
    freeop(ap3);

    /* Restore postfix old value into D0 and hand it back in a temp. */
    gen_code(op_move, 4, pop, makedreg((enum e_am) 0));

    apold = temp_data();
    if ((int) apold->preg != 0)
        gen_code(op_move, 4, makedreg((enum e_am) 0), apold);

    return apold;
}

struct amode   *
gen_faincdec(node, flags, size, op)

/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    int             used;
    struct amode   *ap1, *ap2, *ap3;

    if (node == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- null node in gen_faincdec.\n" );
#endif
        return NULL;
    }

    flags = F_MEM | F_IMMED;
    size = 8;

    /* size 8: avoid do_extend(8->4) -> move.f into An (see gen_fbinary). */
    ap2 = gen_expr(node->v.p[1], flags, 8);
    make_legal(ap2, flags, 8);

    ap1 = gen_expr(node->v.p[0], flags, 8);
    make_legal(ap1, flags, 8);

    if (ap1 == NULL || ap2 == NULL)
        return( NULL );

    validate( ap1 );
    validate( ap2 );

    ap3 = copy_addr( ap1 );

    make_legal(ap1, F_FREG, 8);
    validate(ap2);

    /*
     * Postfix ++/-- returns the old value: save D0:D1 on the stack
     * before .FD*, then pop after storing the new value into *ap3.
     * Do not call temp_float() between push and pop - it can push D0
     * and steal the saved pair (stack desync / hang).
     */
    gen_code(op_move, 4, makedreg((enum e_am) 0), push);
    gen_code(op_move, 4, makedreg((enum e_am) 1), push);

    used = FALSE;
    if (ap2->mode == am_freg)
        ap2 = check_float(ap2);

    /* Same A0 rule as gen_fbinary. */
    if (ap2->mode != am_dreg && ap2->mode != am_immed &&
        ap2->mode != am_freg) {
        int             a0_ok;

        a0_ok = 0;
        if ((int) ap2->preg == 0) {
            if (ap2->mode == am_areg || ap2->mode == am_ind)
                a0_ok = 1;
            else if (ap2->mode == am_indx && ap2->offset != NULL
                     && ap2->offset->nodetype == en_icon
                     && ap2->offset->v.i == 0)
                a0_ok = 1;
        }
        if (!a0_ok) {
            if (used = (!avail_addr(0)))
                gen_code(op_move, 4, makeareg((enum e_am) 0), push);
            gen_code(op_lea, 0, ap2, makeareg((enum e_am) 0));
        }
    }

    freeop(ap1);
    freeop(ap2);

    ap3 = request_reg( ap3 );

    switch (op) {
    case op_fadd:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDadd");
        break;
    case op_fsub:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDsub");
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- uncoded binary floating operation\n" );
#endif
        break;
    }

    gen_code(op_move, 4, makedreg((enum e_am) 0), make_delta( ap3, 0));
    gen_code(op_move, 4, makedreg((enum e_am) 1), make_delta( ap3, 4));

    freeop( ap3 );

    if (used)
        gen_code(op_move, 4, pop, makeareg((enum e_am) 0));

    /* Restore postfix old value into D0:D1, then park it. */
    gen_code(op_move, 4, pop, makedreg((enum e_am) 1));
    gen_code(op_move, 4, pop, makedreg((enum e_am) 0));

    return float_result_mem();
}
