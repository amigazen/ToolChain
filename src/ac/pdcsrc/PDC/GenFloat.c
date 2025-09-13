
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
        fprintf( stderr, "DIAG -- null node in gen_stabn.\n" );
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
        fprintf( stderr, "DIAG -- null node in gen_stabn.\n" );
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
        PdcFlags |= PDC_IEEESINGLE;
        call_library(".Fs2l");
        ap2 = temp_data();
        if ((int)ap2->preg != 0)
            gen_code( op_move, 4, makedreg(0), ap2 );
        return (ap2);

    }

    ap2 = temp_float();
    return (ap2);
}

struct amode   *
gen_fconvert(node, flags, size)
    struct enode   *node;
    int             flags, size;
{
    struct amode   *ap1, *ap2;

    flags = F_FREG;
    size = 8;

    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);

    validate(ap1);

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
        fprintf( stderr, "DIAG -- null node in gen_fsunary.\n" );
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
        fprintf( stderr, "DIAG -- uncoded unary float operation\n" );
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
        fprintf( stderr, "DIAG -- null node in gen_funary.\n" );
        return NULL;
    }

    flags = F_FREG;

    ap = gen_expr(node->v.p[0], flags, size);
    make_legal(ap, flags, size);

    switch (op) {
    case op_fneg:
        PdcFlags |= PDC_IEEEDOUBLE;
        call_library(".FDneg");
        break;
    default:
        fprintf( stderr, "DIAG -- uncoded unary float operation\n" );
        break;
    }

    return ap;
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
        fprintf( stderr, "DIAG -- null node in gen_fsbinary.\n" );
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
        fprintf( stderr, "DIAG -- uncoded binary floating operation\n" );
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
 */
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    int             used;
    struct amode   *ap1, *ap2;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_fbinary.\n" );
        return NULL;
    }

    flags = F_MEM | F_IMMED;
    size = 8;

    if (node->v.p[1]->nodetype == en_d_ref) {
        ap2 = gen_expr(node->v.p[1], flags, 4);
        make_legal(ap2, flags, 4);
    }
    else {
        ap2 = gen_expr(node->v.p[1], F_FREG, 8);
        make_legal(ap2, F_FREG, size);
    }

    if (node->v.p[0]->nodetype == en_d_ref) {
        ap1 = gen_expr(node->v.p[0], flags, 4);
        make_legal(ap1, flags, 4);
    }
    else {
        ap1 = gen_expr(node->v.p[0], F_FREG, 8);
        make_legal(ap1, F_FREG, size);
    }


    if (ap1 == NULL || ap2 == NULL)
        return( NULL );

    /* ---------------------------------------------------- */

    if (ap1->mode == am_freg) { /* ap1 in D0:D1                 */
        if (ap2->mode == am_freg) { /* ap2 in D0:D1                 */
            validate(ap1);
            ap2 = check_float(ap2);
        }
        else {      /* ap2 as mem reference         */
            make_legal(ap1, F_FREG, 8);
            validate(ap2);
        }
    }
    else {          /* ap1 as mem reference         */
        if (ap2->mode == am_freg) { /* ap2 in D0:D1                 */
            validate(ap1);
            make_legal(ap1, F_FREG, 8);
            ap2 = check_float(ap2);
        }
        else {      /* ap2 as mem refernece         */
            make_legal(ap1, F_FREG, 8);
            validate(ap2);
        }
    }

    used = FALSE;
    if ((ap2->mode != am_areg && ap2->mode != am_indx) ||
        (int) ap2->preg != 0) {
        if (used = (!avail_addr(0)))
            gen_code(op_move, 4, makeareg((enum e_am) 0), push);
        gen_code(op_lea, 0, ap2, makeareg((enum e_am) 0));
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
        fprintf( stderr, "DIAG -- uncoded binary floating operation\n" );
        break;
    }

    if (used)
        gen_code(op_move, 4, pop, makeareg((enum e_am) 0));

    ap1 = temp_float();

    return ap1;
}

/*
 * generate code to evaluate a binary node and return the addressing mode of
 * the result.
 */

struct amode   *
gen_fsaincdec(node, flags, size, op)
    struct enode   *node;
    int             flags, size;
    enum e_op       op;
{
    struct amode   *ap1, *ap2, *ap3;

    if (node == NULL) {
        fprintf( stderr, "DIAG -- null node in gen_fsaincdec.\n" );
        return NULL;
    }

    flags = F_DREG;

    ap1 = gen_expr(node->v.p[0], flags, size);
    make_legal(ap1, flags, size);
    ap2 = gen_expr(node->v.p[1], flags, size);
    make_legal(ap2, flags, size);

    validate(ap2);
    validate(ap1);

    if (op == op_fadd) {
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
    default:
        fprintf( stderr, "DIAG -- uncoded binary floating operation\n" );
        break;
    }

    ap1 = temp_data();

    if ((int) ap1->preg != 0)
        gen_code(op_move, size, makedreg((enum e_am) 0), ap1);

    return ap1;
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
        fprintf( stderr, "DIAG -- null node in gen_faincdec.\n" );
        return NULL;
    }

    flags = F_MEM | F_IMMED;
    size = 8;

    ap2 = gen_expr(node->v.p[1], flags, 4);
    make_legal(ap2, flags, 4);

    ap1 = gen_expr(node->v.p[0], flags, 4);
    make_legal(ap1, flags, 4);

    if (ap1 == NULL || ap2 == NULL)
        return( NULL );

    validate( ap1 );
    validate( ap2 );

    ap3 = copy_addr( ap1 );

    make_legal(ap1, F_FREG, 8);
    validate(ap2);

    gen_code(op_move, 4, makedreg((enum e_am) 0), push);
    gen_code(op_move, 4, makedreg((enum e_am) 1), push);

    used = FALSE;
    if ((ap2->mode != am_areg && ap2->mode != am_indx) ||
        (int) ap2->preg != 0) {
        if (used = (!avail_addr(0)))
            gen_code(op_move, 4, makeareg((enum e_am) 0), push);
        gen_code(op_lea, 0, ap2, makeareg((enum e_am) 0));
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
        fprintf( stderr, "DIAG -- uncoded binary floating operation\n" );
        break;
    }

    gen_code(op_move, 4, makedreg((enum e_am) 0), make_delta( ap3, 0));
    gen_code(op_move, 4, makedreg((enum e_am) 1), make_delta( ap3, 4));

    freeop( ap3 );

    if (used)
        gen_code(op_move, 4, pop, makeareg((enum e_am) 0));

    ap1 = temp_float();

    gen_code(op_move, 4, pop, makedreg((enum e_am) 1));
    gen_code(op_move, 4, pop, makedreg((enum e_am) 0));

    return ap1;
}
