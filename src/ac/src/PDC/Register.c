
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
 * $Log:	Register.c,v $
 * Revision 3.33  90/04/05  22:46:23  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:25:36  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Register.c
 * 
 * This module handles the allocation and de-allocation of temporary registers.
 * when a temporary register is allocated the stack depth is saved in the
 * field deep of the address mode structure. when validate is called on an
 * address mode structure the stack is popped until the register is restored
 * to it's pre-push value.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

void    gen_pop();

extern struct amode *make_autocon();
extern struct amode *make_delta(), *make_immed();
extern struct amode *copy_addr();
extern char    *xalloc();
extern char    *itoa();

struct amode    push[] = {{am_adec, (enum e_am) 7}},
                pop[] = {{am_ainc, (enum e_am) 7}};

int             next_data = 0, next_addr = 0, stackptr = 0;

enum e_regstate {
    sr_empty, sr_addr, sr_data, sr_float,
    sr_apush, sr_dpush, sr_fpush, sr_void, sr_void2
};

struct _stack_state {
    enum e_regstate state;
    int             deep;
};

struct _stack_state stackstate[100];

enum e_regstate datatbl[100];
enum e_regstate addrtbl[100];

void
move_stack(st, loc, index, delta)
    enum e_regstate st;
    int             loc, index, delta;
{
    struct amode   *ap, *ap2;

    ap = make_immed(delta);
    ap->mode = am_indx;
    ap->preg = (enum e_am) 7;

    switch (st) {
    case sr_addr:
    case sr_data:
        gen_code(op_move, 4, ap, push);
        stackstate[index].state = sr_void;
        break;
    case sr_float:
        ap2 = make_delta(ap, 4);
        gen_code(op_move, 4, ap2, push);
        gen_code(op_move, 4, ap2, push);
        stackstate[index].state = sr_void2;
        break;
    default:
        fprintf(stderr, "DIAG -- case error in move_stack\n");
        break;
    }
}

void
push_stack(st, loc)
    enum e_regstate st;
    int             loc;
{
    stackstate[stackptr].state = st;
    stackstate[stackptr].deep = loc;
    ++stackptr;
}

void
pop_stack(st, loc)
    enum e_regstate st;
    int             loc;
{
    int             deep, i, index, delta;
    enum e_regstate state;

    index = -1;
    for (i = 0; i < stackptr; i++) {
        if (stackstate[i].state == st && stackstate[i].deep == loc) {
            index = i;
            break;
        }
    }
    if (index < 0) {
        fprintf(stderr, "PDC-BUG: pop_stack\n");
        return;
    }
    delta = 0;
    for (i = index + 1; i < stackptr; i++) {
        switch (stackstate[i].state) {
        case sr_void:
        case sr_addr:
        case sr_data:
            delta += 4;
            break;
        case sr_float:
        case sr_void2:
            delta += 8;
        }
    }

    --stackptr;
    state = stackstate[stackptr].state;
    deep = stackstate[stackptr].deep;

    while (state != st || deep != loc) {    /* We need to fix the stack */
        switch (state) {
        case sr_void:
            gen_code(op_add, 2, make_immed(4), makeareg(7));
            break;
        case sr_void2:
            gen_code(op_add, 2, make_immed(8), makeareg(7));
            break;
        case sr_addr:
            for (i = deep + 2; i < next_addr; i += 2) {
                if (addrtbl[i] != sr_empty || addrtbl[i] != sr_void) {
                    move_stack(st, loc, index, delta);
                    ++stackptr;
                    return;
                }
            }
            gen_pop((enum e_am) (deep % 2), am_areg);
            addrtbl[deep] = sr_addr;
            break;
        case sr_data:
            for (i = deep + 3; i < next_data; i += 3) {
                if (datatbl[i] != sr_empty || datatbl[i] != sr_void) {
                    move_stack(st, loc, index, delta);
                    ++stackptr;
                    return;
                }
            }
            gen_pop((enum e_am) (deep % 3), am_dreg);
            datatbl[deep] = sr_data;
            break;
        case sr_float:
            for (i = deep + 3; i < next_data; i += 3) {
                if (datatbl[i] != sr_empty || datatbl[i] != sr_void) {
                    move_stack(st, loc, index, delta);
                    ++stackptr;
                    return;
                }
            }
            gen_pop((enum e_am) (deep % 3), am_freg);
            datatbl[deep] = sr_float;
            datatbl[deep + 1] = sr_float;
            break;
        }
        if (stackptr <= 0)
            break;
        else {
            --stackptr;
            state = stackstate[stackptr].state;
            deep = stackstate[stackptr].deep;
        }
    }
}

void
gen_push(reg, rmode)

/*
 * this routine generates code to push a register onto the stack
 */
    enum e_am       reg, rmode;
{
    struct amode   *ap1;

    if (rmode == am_freg) {
        gen_code(op_move, 4, makedreg((enum e_am) 1), push);
        gen_code(op_move, 4, makedreg((enum e_am) 0), push);
    }
    else {
        ap1 = (struct amode *) xalloc(sizeof(struct amode));
        ap1->preg = reg;
        ap1->mode = rmode;
        ap1->signedflag = 1;
        gen_code(op_move, 4, ap1, push);
    }
}

void
gen_pop(reg, rmode)

/*
 * generate code to pop the primary register in ap from the stack.
 */
    enum e_am       reg, rmode;
{
    struct amode   *ap1;

    if (rmode == am_freg) {
        gen_code(op_move, 4, pop, makedreg((enum e_am) 0));
        gen_code(op_move, 4, pop, makedreg((enum e_am) 1));
    }
    else {
        ap1 = (struct amode *) xalloc(sizeof(struct amode));
        ap1->preg = reg;
        ap1->mode = rmode;
        ap1->signedflag = 1;
        gen_code(op_move, 4, pop, ap1);
    }
}

void
initstack()

/*
 * this routine should be called before each expression is evaluated to make
 * sure the stack is balanced and all of the registers are marked free.
 */
{
    while (stackptr > 0) {
        if (stackstate[stackptr - 1].state == sr_void) {
            gen_code(op_add, 2, make_immed(4), makeareg(7));
            --stackptr;
            continue;
        }
        else if (stackstate[stackptr - 1].state == sr_void2) {
            gen_code(op_add, 2, make_immed(8), makeareg(7));
            --stackptr;
            continue;
        }
        break;
    }

    if (stackptr != 0)
        fprintf(stderr, "DIAG -- Temporaries are left on the stack\n");

    next_data = 0;
    next_addr = 0;
    stackptr = 0;

    datatbl[0] = sr_empty;
    datatbl[1] = sr_empty;
    datatbl[2] = sr_empty;

    addrtbl[0] = sr_empty;
    addrtbl[1] = sr_empty;
}

struct amode   *
check_float(ap)

/*
 * Move the floating point value whose address is in "ap" into the temporary
 * stack space reserved for floating point temporaries.  "ap" could either be
 * in the floating point register D0:D1, or on the stack.
 */
    struct amode   *ap;
{
    struct amode   *ap2, *ap3;
    int             loc;

    if (ap->mode != am_freg || (int) ap->preg != 0) {
        fprintf(stderr, "DIAG -- check_float: not a F_FREG\n");
        return (NULL);
    }

    if (float_auto == 0) {
        lc_auto += 8;
        float_auto = lc_auto;   /* double temp at -float_auto(A6) */
    }

    loc = ap->deep;

    ap3 = make_autocon(-float_auto);
    ap2 = copy_addr(ap3);

    if (datatbl[loc] == sr_float || datatbl[loc] == sr_fpush) {
        if (datatbl[loc] == sr_fpush) {
            pop_stack(sr_float, loc);
            gen_code(op_move, 4, pop, ap3);
            ap3 = make_delta(ap3, 4);
            gen_code(op_move, 4, pop, ap3);
        }
        else if (datatbl[loc] == sr_float) {
            gen_code(op_move, 4, makedreg((enum e_am) 0), ap3);
            ap3 = make_delta(ap3, 4);
            gen_code(op_move, 4, makedreg((enum e_am) 1), ap3);
        }
        loc = ap->deep;
        datatbl[loc] = sr_empty;    /* Reset status         */
        datatbl[loc + 1] = sr_empty;    /* of both halves  */
        loc = next_data - 1;
        while (loc >= 0) {  /* Find next register to alloc */
            if (datatbl[loc] != sr_empty && datatbl[loc] != sr_void)
                break;
            datatbl[next_data = loc] = sr_empty;
            --loc;
        }
        return (ap2);
    }

    fprintf(stderr, "DIAG -- check_float: not a float\n");
    return (NULL);
}

void
validate(ap)

/*
 * validate will make sure that if a register within an address mode has been
 * pushed onto the stack that it is popped back at this time.
 */
    struct amode   *ap;
{
    int             loc;

    if (ap == NULL) {
        fprintf(stderr, "DIAG -- validate: NULL argument\n");
        return;
    }

    switch (ap->mode) {
    case am_direct:
    case am_immed:
        return;     /* no registers used */

    case am_freg:
        if ((int) ap->preg > 0)
            return; /* not a temparary */
        loc = ap->deep;
        if (datatbl[loc] == sr_float)
            return; /* NOT on the stack */
        if (datatbl[loc] != sr_fpush) { /* ERROR in type        */
            fprintf(stderr, "DIAG -- validate: not freg\n");
            return;
        }
        datatbl[loc] = sr_float;
        datatbl[loc + 1] = sr_float;
        pop_stack(sr_float, ap->deep);
        gen_pop(ap->preg, am_freg);
        break;

    case am_dreg:
        if ((int) ap->preg > 2)
            return; /* not a temporary */
        loc = ap->deep;
        if (datatbl[loc] == sr_data)
            return; /* NOT on the stack */
        if (datatbl[loc] != sr_dpush) { /* ERROR in type        */
            fprintf(stderr, "DIAG -- validate: not dreg\n");
            return;
        }
        datatbl[loc] = sr_data;
        pop_stack(sr_data, ap->deep);
        gen_pop(ap->preg, am_dreg);
        break;

    default:
        if ((int) ap->preg > 1)
            return; /* not a temp register */
        loc = ap->deep;
        if (addrtbl[loc] == sr_addr)
            return; /* NOT on the stack */
        if (addrtbl[loc] != sr_apush) { /* ERROR in type        */
            fprintf(stderr, "DIAG -- validate: not areg\n");
            return;
        }
        addrtbl[loc] = sr_addr;
        pop_stack(sr_addr, ap->deep);
        gen_pop(ap->preg, am_areg);
        break;
    }
}

int
avail_data(num)
    int             num;
{
    int             loc;

    if (num % 3 != num) {
        fprintf(stderr, "DIAG -- avail_data: error in argument\n");
        return (FALSE);
    }

    for (loc = next_addr; loc >= 0; --loc)
        if (loc % 3 == num)
            break;

    while (loc >= 0) {
        switch (datatbl[loc]) {
        case sr_data:
        case sr_float:
            return (FALSE);
        case sr_dpush:
        case sr_fpush:
            return (TRUE);
        }
        loc -= 3;
    }
    return (TRUE);
}

int
avail_addr(num)
    int             num;
{
    int             loc;

    if (num % 2 != num) {
        fprintf(stderr, "DIAG -- avail_addr: error in argument\n");
        return (FALSE);
    }

    for (loc = next_addr; loc >= 0; --loc)
        if (loc % 2 == num)
            break;

    while (loc >= 0) {
        if (addrtbl[loc] == sr_addr)
            return (FALSE);
        if (addrtbl[loc] == sr_apush)
            return (TRUE);
        loc -= 2;
    }
    return (TRUE);
}

struct amode   *
temp_addr()

/*
 * allocate a temporary address register and return it's addressing mode.
 */
{
    struct amode   *ap;
    int             loc;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_areg;
    ap->preg = (enum e_am) (next_addr % 2);
    ap->deep = next_addr;
    ap->signedflag = 1;

    addrtbl[next_addr] = sr_addr;

    for (loc = next_addr - 2; loc >= 0; loc -= 2) {
        if (addrtbl[loc] != sr_void) {
            if (addrtbl[loc] == sr_addr) {
                push_stack(sr_addr, loc);
                gen_push((enum e_am) (next_addr % 2), am_areg);
                addrtbl[loc] = sr_apush;
            }
            break;
        }
    }

    addrtbl[++next_addr] = sr_empty;

    return ap;
}

struct amode   *
temp_data()

/*
 * allocate a temporary data register and return it's addressing mode.
 */
{
    struct amode   *ap;
    int             loc;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_dreg;
    ap->preg = (enum e_am) (next_data % 3);
    ap->deep = next_data;
    ap->signedflag = 1;

    datatbl[next_data] = sr_data;

    for (loc = next_data - 3; loc >= 0; loc -= 3) {
        if (datatbl[loc] != sr_void) {
            if (datatbl[loc] == sr_data) {
                push_stack(sr_data, loc);
                gen_push((enum e_am) (next_data % 3), am_dreg);
                datatbl[loc] = sr_dpush;
            }
            else if (datatbl[loc] == sr_float) {
                if ((loc % 3) != 0)
                    --loc;
                push_stack(sr_float, loc);
                gen_push((enum e_am) 0, am_freg);
                datatbl[loc] = sr_fpush;
                datatbl[loc + 1] = sr_fpush;
            }
            break;
        }
    }

    datatbl[++next_data] = sr_empty;

    return ap;
}

struct amode   *
temp_float()

/*
 * allocate a temporary data register and return it's addressing mode. Make
 * sure the register allocated is the D0:D1 pair
 */
{
    struct amode   *ap;
    int             loc;

    while ((next_data % 3) != 0)    /* Find next D0:D1 pair */
        datatbl[next_data++] = sr_void;

    ap = (struct amode *) xalloc(sizeof(struct amode));
    ap->mode = am_freg;
    ap->preg = (enum e_am) (next_data % 3);
    ap->deep = next_data;
    ap->signedflag = 1;

    datatbl[next_data] = sr_float;

    for (loc = next_data - 3; loc >= 0; loc -= 3) {
        if (datatbl[loc] != sr_void) {
            if (datatbl[loc] == sr_data) {
                push_stack(sr_data, loc);
                gen_push((enum e_am) (next_data % 3), am_dreg);
                datatbl[loc] = sr_dpush;
            }
            else if (datatbl[loc] == sr_float) {
                push_stack(sr_float, loc);
                gen_push((enum e_am) (next_data % 3), am_freg);
                datatbl[loc] = sr_fpush;
                datatbl[loc + 1] = sr_fpush;
            }
            break;
        }
    }

    datatbl[++next_data] = sr_float;

    for (loc = next_data - 3; loc >= 0; loc -= 3) {
        if (datatbl[loc] != sr_void) {
            if (datatbl[loc] == sr_data) {
                push_stack(sr_data, loc);
                gen_push((enum e_am) (next_data % 3), am_dreg);
                datatbl[loc] = sr_dpush;
            }
            break;
        }
    }

    datatbl[++next_data] = sr_empty;

    return ap;
}

void
freeop(ap)

/*
 * release any temporary registers used in an addressing mode.
 */
    struct amode   *ap;
{
    int             loc;

    if (ap == NULL || ap->mode == am_immed || ap->mode == am_direct)
        return;     /* no registers used */

    if (ap->mode == am_dreg) {  /* DATA register        */
        if ((int) ap->preg < 3) {   /* D0:D2                */
            loc = ap->deep;
            if (loc != next_data - 1) {
                fprintf(stderr, "freeop LOC: %d   NEXT_DATA: %d\n",
                        loc, next_data);
            }
            datatbl[loc] = sr_empty;    /* Reset status         */
            loc = next_data - 1;
            while (loc >= 0) {  /* Find next register to
                         * alloc */
                if (datatbl[loc] != sr_empty && datatbl[loc] != sr_void)
                    break;
                datatbl[next_data = loc] = sr_empty;
                --loc;
            }
        }
    }
    else if (ap->mode == am_freg) { /* FLOAT register       */
        if ((int) ap->preg < 1) {   /* only 1, D0:D1        */
            loc = ap->deep;
            if (loc != next_data - 2) {
                fprintf(stderr, "freeop FLOAT: %d   NEXT_DATA: %d\n",
                        loc, next_data);
            }
            datatbl[loc] = sr_empty;    /* Reset status    */
            datatbl[loc + 1] = sr_empty;    /* of both halves  */
            loc = next_data - 1;
            while (loc >= 0) {  /* Find next register to
                         * alloc */
                if (datatbl[loc] != sr_empty && datatbl[loc] != sr_void)
                    break;
                datatbl[next_data = loc] = sr_empty;
                --loc;
            }
        }
    }
    else {          /* ADDR register        */
        if ((int) ap->preg < 2) {   /* A0:A1                */
            loc = ap->deep;
            if (loc != next_addr - 1) {
                fprintf(stderr, "freeop ADDR: %d   NEXT_DATA: %d\n",
                        loc, next_data);
            }
            addrtbl[loc] = sr_empty;    /* Reset status         */
            loc = next_addr - 1;
            while (loc >= 0) {  /* Find next register to
                         * alloc */
                if (addrtbl[loc] != sr_empty && addrtbl[loc] != sr_void)
                    break;
                addrtbl[next_addr = loc] = sr_empty;
                --loc;
            }
        }
    }
}

void
flush_regs()
{
    struct amode   *ap1, *ap2, *ap3;

    ap1 = temp_data();
    ap2 = temp_data();
    ap3 = temp_data();

    freeop(ap3);
    freeop(ap2);
    freeop(ap1);

    ap1 = temp_addr();
    ap2 = temp_addr();

    freeop(ap2);
    freeop(ap1);

}

struct amode   *
request_addr(num)
    int             num;
{
    if (num % 2 != num) {
        fprintf(stderr, "DIAG -- request_addr: num out of range\n");
        return (NULL);
    }

    while ((next_addr % 2) != num)
        addrtbl[next_addr++] = sr_void;

    return (temp_addr());
}

struct amode   *
request_data(num)
    int             num;
{
    if (num % 3 != num) {
        fprintf(stderr, "DIAG -- request_data: num out of range\n");
        return (NULL);
    }

    while ((next_data % 3) != num)
        datatbl[next_data++] = sr_void;

    return (temp_data());
}

struct amode   *
request_float(num)
    int             num;
{
    if (num != 0) {
        fprintf(stderr, "DIAG -- request_float: num out of range\n");
        return (NULL);
    }

    while ((next_data % 3) != num)
        datatbl[next_data++] = sr_void;

    return (temp_float());
}

struct amode   *
request_reg(ap)
    struct amode   *ap;
{
    struct amode   *ap2;

    switch (ap->mode) {
    case am_direct:
    case am_immed:
        return (ap);    /* no registers used    */
    case am_freg:
        return (request_float(0));  /* floating point reg   */
    case am_dreg:
        if ((int) ap->preg < 3) {
            ap2 = request_data((int) ap->preg);
            return (ap2);   /* Data register                */
        }
        return (ap);
    case am_areg:
    default:
        if ((int) ap->preg < 2) {
            ap2 = request_addr((int) ap->preg);
            ap2->mode = ap->mode;
            ap2->offset = ap->offset;
            return (ap2);   /* Address register             */
        }
        return (ap);
    }
}
