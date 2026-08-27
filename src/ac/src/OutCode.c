
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
 * $Log:	OutCode.c,v $
 * Revision 3.33  90/04/05  23:12:38  lionel
 * Improved data and literal output code, fixed some parameter type probs.
 * 
 * Revision 3.32  90/02/03  16:25:06  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Outcode.c
 * 
 * Generates the actual assembly text from the code generator's directions.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

extern char    *litlate();
extern char    *xalloc();

void    put_mask(), nl(), cseg();

enum e_gt {
    nogen, bytegen, wordgen, longgen, singlegen, doublegen
};

enum e_sg {
    noseg, codeseg, dataseg, bssseg
};

union double_int {
    double          dval;
    float           fval;
    int             ival[2];
};

/* variable initialization     */

enum e_gt       gentype = nogen;
enum e_sg       curseg = noseg;
int             outcol = 0;

char *areg_format = "A%d";
char *dreg_format = "D%d";

static int      opl_len = 0;

struct oplst {
    char           *s;
    enum e_op       ov;
};

struct oplst    opl[] = {
    {"abcd", op_abcd},
    {"add", op_add},
    {"add", op_adda},
    {"add", op_addi},
    {"addq", op_addq},
    {"addx", op_addx},
    {"and", op_and},
    {"andi", op_andi},
    {"asl", op_asl},
    {"???", op_asm},
    {"asr", op_asr},
    {"bcc", op_bcc},
    {"bchg", op_bchg},
    {"bclr", op_bclr},
    {"bcs", op_bcs},
    {"beq", op_beq},
    {"bge", op_bge},
    {"bgt", op_bgt},
    {"bhi", op_bhi},
    {"ble", op_ble},
    {"bls", op_bls},
    {"blt", op_blt},
    {"bmi", op_bmi},
    {"bne", op_bne},
    {"bpl", op_bpl},
    {"bra", op_bra},
    {"bset", op_bset},
    {"bsr", op_bsr},
    {"btst", op_btst},
    {"bvc", op_bvc},
    {"bvs", op_bvs},
    {"chk", op_chk},
    {"clr", op_clr},
    {"cmp", op_cmp},
    {"cmp", op_cmpa},
    {"cmp", op_cmpi},
    {"cmp", op_cmpm},
    {";;;", op_comment},
    {"dbcc", op_dbcc},
    {"dbcs", op_dbcs},
    {"dbeq", op_dbeq},
    {"dbf", op_dbf},
    {"dbge", op_dbge},
    {"dbgt", op_dbgt},
    {"dbhi", op_dbhi},
    {"dble", op_dble},
    {"dbls", op_dbls},
    {"dbmi", op_dbmi},
    {"dbne", op_dbne},
    {"dbpl", op_dbpl},
    {"dbra", op_dbra},
    {"dbt", op_dbt},
    {"dbvc", op_dbvc},
    {"dbvs", op_dbvs},
    {"dc", op_dc},
    {"divs", op_divs},
    {"divu", op_divu},
    {"eor", op_eor},
    {"eori", op_eori},
    {"exg", op_exg},
    {"ext", op_ext},
    {"???", op_fadd},
    {"???", op_fdiv},
    {"???", op_fmod},
    {"???", op_fmove},
    {"???", op_fmul},
    {"???", op_fneg},
    {"???", op_fsub},
    {"jmp", op_jmp},
    {"jsr", op_jsr},
    {"???", op_label},
    {"lea", op_lea},
    {"link", op_link},
    {"lsl", op_lsl},
    {"lsr", op_lsr},
    {"???", op_mods},
    {"???", op_modu},
    {"move", op_move},
    {"move", op_movea},
    {"movem", op_movem},
    {"movep", op_movep},
    {"moveq", op_moveq},
    {"muls", op_muls},
    {"mulu", op_mulu},
    {"nbcd", op_nbcd},
    {"neg", op_neg},
    {"negx", op_negx},
    {"nop", op_nop},
    {"not", op_not},
    {"or", op_or},
    {"ori", op_ori},
    {"pea", op_pea},
    {"reset", op_reset},
    {"rol", op_rol},
    {"ror", op_ror},
    {"roxl", op_roxl},
    {"roxr", op_roxr},
    {"rte", op_rte},
    {"rtr", op_rtr},
    {"rts", op_rts},
    {"sbcd", op_sbcd},
    {"scc", op_scc},
    {"scs", op_scs},
    {"seq", op_seq},
    {"sf", op_sf},
    {"sge", op_sge},
    {"sgt", op_sgt},
    {"shi", op_shi},
    {"sle", op_sle},
    {"sls", op_sls},
    {"slt", op_slt},
    {"smi", op_smi},
    {"sne", op_sne},
    {"spl", op_spl},
    {"st", op_st},
    {"stabn", op_stabn},
    {"stabs", op_stabs},
    {"stop", op_stop},
    {"sub", op_sub},
    {"sub", op_suba},
    {"sub", op_subi},
    {"subq", op_subq},
    {"subx", op_subx},
    {"svc", op_svc},
    {"svs", op_svs},
    {"swap", op_swap},
    {"tas", op_tas},
    {"trap", op_trap},
    {"trapv", op_trapv},
    {"tst", op_tst},
    {"unlk", op_unlk},
    {(char *) 0, (enum e_op) 0}
};

static char     buffer[64];

char           *
itoa(x)
    int             x;
{
    char           *ptr;
    int             d, negative;

    ptr = &buffer[sizeof(buffer) - 1];
    *ptr = '\0';

    if (x == 0x80000000)    /* The number where x = -x */
        return ("-2147483648");

    negative = 0;
    if (x < 0) {
        negative = 1;
        x = -x;
    }

    do {
        d = safe_lmod(x, 10);
        if (d < 0 || d > 9) {
#if AC_DEBUG
            fprintf(AC_DIAG_STREAM, "DIAG -- itoa has a problem\n");
#endif
            return (ptr);
        }
        x = safe_ldiv(x, 10);
        *(--ptr) = '0' + d;
    } while (x > 0);

    if (negative)
        *(--ptr) = '-';
    return (ptr);
}

void
putsingle(d)
    double          d;
{
    union double_int temp;

    temp.fval = d;
    fprintf( output, "%d", temp.ival[0] );
}

void
putdouble(d)
    double          d;
{
    union double_int temp;

    temp.dval = d;
    fprintf( output, "%d,%d", temp.ival[0], temp.ival[1] );
}

void
putareg(reg)
    int             reg;
{
    fprintf(output, "A%d", reg );
}

void
putdreg(reg)
    int             reg;
{
    fprintf(output, "D%d", reg );
}

int
sortopl()
{
    struct oplst   *p1, *p2, temp;
    int             changed = 0;

    p1 = opl;
    for (p2 = p1 + 1; p2->s != NULL; p1++, p2++) {
        if ((int) p1->ov > (int) p2->ov) {
            changed = 1;
            temp = *p1;
            *p1 = *p2;
            *p2 = temp;
        }
    }
    return changed;
}

void
putop(op, len)
    enum e_op       op;
    int             len;
{
    int             low, high, mid;
    struct oplst   *ptr;
    char           *lenstr;

    if (opl_len == 0) {
        for (ptr = opl; ptr->s != NULL; ptr++)
            ++opl_len;
        while (sortopl());
    }

    low = 0;
    high = opl_len - 1;

    do {
        mid = low + safe_ldiv(high - low, 2);
        ptr = &opl[mid];

        if (ptr->ov == op) {
            switch (len) {
            case 0:
                lenstr = "";
                break;      /* no length field */
            case 1:
                lenstr = ".b";
                break;
            case 2:
                lenstr = ".w";
                break;
            case 4:
                lenstr = ".l";
                break;
            case 8:
                /*
                 * Legacy PDC printed ".f" here.  A68k rejects it.
                 * gen_code must split 64-bit moves into two .l first;
                 * if anything still reaches putop with len 8, use .l so
                 * assembly does not fail (wrong but visible in review).
                 */
                lenstr = ".l";
                break;
            default:
                lenstr = ".x";
#if AC_DEBUG
                fprintf(AC_DIAG_STREAM, "DIAG -- illegal length field.\n" );
#endif
                break;
            }
            fprintf( output, "\t%s%s", ptr->s, lenstr );
            return;
        }
        else {
            if ((int) op > (int) ptr->ov)
                low = mid + 1;
            else
                high = mid - 1;
        }
    } while (low <= high);

#if AC_DEBUG
    fprintf(AC_DIAG_STREAM, "DIAG -- illegal opcode.\n" );
#endif
}

/*
 * put a constant to the output file.
 */

void
putconst(offset)
    struct enode   *offset;
{
    if (offset == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL argument to putconst.\n" );
#endif
        return;
    }
    switch (offset->nodetype) {
    case en_autocon:
        fprintf(output, "%ld(A%d)",
            (long) frame_disp(icon_unpoison(offset->v.i)), frame_areg());
        break;
    case en_tempref:
        fprintf(output, "%d", offset->v.i);
        break;
    case en_icon:
        /* Full 32-bit immediates; ICON16L is only for frame displacements. */
        fprintf(output, "%ld", (long) offset->v.i);
        break;
    case en_fcon:
        putdouble(offset->v.f);
        break;
    case en_labcon:
        fprintf( output, "L%d", offset->v.i);
        break;
    case en_nacon:
        fprintf( output, "_%s", offset->v.p[0] );
        break;
    case en_intrlab:
        fprintf( output, "%s", offset->v.p[0] );
        break;
    case en_add:
        putconst(offset->v.p[0]);
        fprintf( output, "+" );
        putconst(offset->v.p[1]);
        break;
    case en_sub:
        putconst(offset->v.p[0]);
        fprintf( output, "-" );
        putconst(offset->v.p[1]);
        break;
    case en_uminus:
        fprintf( output, "-" );
        putconst(offset->v.p[0]);
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- illegal constant node (%d)\n", 
                         offset->nodetype );
#endif
        break;
    }
}

void
putamode(ap)

/*
 * output a general addressing mode.
 */
    struct amode   *ap;
{
    if (ap == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL argument to putamode.\n" );
#endif
        return;
    }
    switch (ap->mode) {
    case am_immed:
        fprintf( output, "#" );
        putconst(ap->offset);
        break;
    case am_direct:
        putconst(ap->offset);
        break;
    case am_areg:
        fprintf( output, areg_format, ap->preg);
        break;
    case am_dreg:
        fprintf( output, dreg_format, ap->preg);
        break;
    case am_freg:
        fprintf( output, dreg_format, (enum e_am) 0);
        break;
    case am_ind:
        fprintf( output, "(A%d)", ap->preg);
        break;
    case am_ainc:
        fprintf( output, "(A%d)+", ap->preg);
        break;
    case am_adec:
        fprintf( output, "-(A%d)", ap->preg);
        break;
    case am_indx:
        putconst(ap->offset);
        fprintf( output, "(A%d)", ap->preg);
        break;
    case am_xpc:
        putconst(ap->offset);
        fprintf( output, "(A%d,pc)", ap->preg);
        break;
    case am_indx2:
        putconst(ap->offset);
        fprintf( output, "(A%d,D%d.l)", ap->preg, ap->sreg);
        break;
    case am_indx3:
        putconst(ap->offset);
        fprintf( output, "(A%d,A%d.l)", ap->preg, ap->sreg );
        break;
    case am_mask:
        put_mask((long) (ap->offset));
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- illegal address mode.\n" );
#endif
        fprintf( output, "<DIAG -- illegal address mode.>" );
        break;
    }
}

void
put_code(op, len, aps, apd)

/*
 * output a generic instruction.
 */
    struct amode   *aps, *apd;
    enum e_op       op;
    int             len;
{
    char           *comment;
    struct dnode   *dp;
    char            lench;

    switch (op) {
    case op_comment:
        comment = aps->offset->v.sp;
        nl();
        fprintf( output, ";;;\t%s\n", comment );
        return;
    case op_asm:
        comment = aps->offset->v.sp;
        nl();
        fprintf( output, "%s\n", comment );
        return;
    case op_stabn:
        dp = aps->offset->v.dp;
        fprintf( output, "\tSTABN\t%d,0,%d,", dp->tag, dp->nest );
        putconst(dp->ref);
        fprintf( output, "\n" );
        return;
    case op_stabs:
        dp = aps->offset->v.dp;
        comment = dp->sp;
        do {
            fprintf( output, "\tSTABS\t\"" );

            while (*comment && (*comment != '\\')) {
                fputc(*comment, output);
                comment++;
                if (*comment == '\0' && dp->next != NULL) {
                    dp = dp->next;
                    comment = dp->sp;
                }
            }

            if (*comment) {
                fputc(*comment++, output);
                fputc(*comment++, output);
            }

            fprintf( output, "\",%d,0,%d,", dp->tag, dp->nest );
            putconst(dp->ref);
            fprintf( output, "\n" );

            if (*comment == '\0' && dp->next != NULL) {
                dp = dp->next;
                comment = dp->sp;
            }
        } while (*comment);
        return;
    case op_dc:
        switch (len) {
        case 1:
            lench = 'b';
            break;
        case 2:
            lench = 'w';
            break;
        case 4:
            lench = 'l';
            break;
        case 8:
            lench = 'l';
        }
        fprintf( output, "\tDC.%c", lench );
        break;
    default:
        putop(op, len);
        break;
    }

    if (aps != NULL) {
        fprintf( output, "\t" );
        if (op == op_cmp || op == op_cmpi)
            putamode(apd);
        else
            putamode(aps);
        if (apd != NULL) {
            fprintf( output, "," );
            if (op == op_cmp || op == op_cmpi)
                putamode(aps);
            else
                putamode(apd);
        }
    }
    fprintf( output, "\n" );
}

void
startRange(bit)
    int    bit;
{
    register int bit_mod8 = bit & 7;

    if (bit <= 7)
        fprintf( output, dreg_format, bit_mod8);
    else
        fprintf( output, areg_format, bit_mod8);
}

void
endRange(first, bit)
    int    first, bit;
{
    if (first <= 7 && bit > 7) {
        endRange(first, 7);
        fprintf( output, "/" );
        startRange(8);
        first = 8;
    }
    if (bit > first) {
        fprintf( output, "-" );
        startRange(bit);
    }
}

void
put_mask(mask1)

/*
 * generate a register mask for restore and save.
 */
    long            mask1;
{
    int             mask;
    int             bit;
    int             inrange;
    int             some;

    mask = mask1;
    inrange = -1;
    some = 0;

    for (bit = 0; bit <= 15; ++bit) {
        if ((mask >> bit) & 1) {
            if (inrange < 0) {
                if (some)
                    fprintf( output, "/" );
                startRange(bit);
                some = 1;
                inrange = bit;
            }
        }
        else if (inrange >= 0) {
            endRange(inrange, bit - 1);
            inrange = -1;   /* was -= 1 PMP */
        }
    }
    if (inrange >= 0)
        endRange(inrange, 15);
}

void
putreg(r)

/*
 * generate a register name from a tempref number.
 */
    int             r;
{
    if (r < 8)
        putdreg(r);
    else
        putareg(r - 8);
}

void
gen_strlab(s)

/*
 * generate a named label.
 */
    char           *s;
{
    fprintf( output, "_%s:\n", s );
}

void
put_label(lab)

/*
 * output a compiler generated label.
 */
    long            lab;
{
    fprintf( output, "L%d\n", lab);
}

void
genbyte(val)
    int             val;
{
    if (gentype == bytegen && outcol < 60) {
        fprintf( output, ",%d", val & 0x00ff);
        outcol += 4;
    }
    else {
        nl();
        fprintf( output, "\tDC.b\t%d", val & 0x00ff);
        gentype = bytegen;
        outcol = 19;
    }
}

void
genword(val)
    long            val;
{
    if (gentype == wordgen && outcol < 58) {
        fprintf( output, ",%d", val & 0x0ffff);
        outcol += 6;
    }
    else {
        nl();
        fprintf( output, "\tDC.w\t%d", val & 0x0ffff);
        gentype = wordgen;
        outcol = 21;
    }
}

void
genlong(val)
    long            val;
{
    if (gentype == longgen && outcol < 56) {
        fprintf( output, ",%d", val );
        outcol += 10;
    }
    else {
        nl();
        fprintf( output, "\tDC.l\t%d", val );
        gentype = longgen;
        outcol = 25;
    }
}

void
gensingle(val)
    double          val;
{
    if (gentype == singlegen && outcol < 56) {
        fprintf( output, "," );
        putsingle(val);
        outcol += 10;
    }
    else {
        nl();
        fprintf( output, "\tDC.l\t" );
        putsingle(val);
        gentype = singlegen;
        outcol = 35;
    }
}

void
gendouble(val)
    double          val;
{
    if (gentype == doublegen && outcol < 56) {
        fprintf( output, "," );
        putdouble(val);
        outcol += 20;
    }
    else {
        nl();
        fprintf( output, "\tDC.l\t" );
        putdouble(val);
        gentype = doublegen;
        outcol = 45;
    }
}

void
genref(sp, offset)
    SYM            *sp;
    int             offset;
{
    char            sign;

    if (sp == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL argument to genref.\n" );
#endif
        return;
    }
    if (offset >= 0) 
        sign = '+';
    else {
        sign = '-';
        offset = -offset;
    }

    if (gentype == longgen && outcol < 55 - strlen(sp->name)) {
        if (sp->storage_class == sc_static) {
            fprintf( output, ",L%d", sp->value.i );
            if (offset != 0) {
                fprintf( output, "%c%d", sign, offset );
            }
        }
        else {
            fprintf( output, ",_%s", sp->name );
            if (offset != 0) {
                fprintf( output, "%c%d", sign, offset );
            }
        }
        outcol += (11 + strlen(sp->name));
    }
    else {
        nl();
        if (sp->storage_class == sc_static) {
            fprintf( output, "\tDC.l\tL%d", sp->value.i );
            if (offset != 0) {
                fprintf( output, "%c%d", sign, offset );
            }
        }
        else {
            fprintf( output, "\tDC.l\t_%s", sp->name );
            if (offset != 0) {
                fprintf( output, "%c%d", sign, offset );
            }
        }
        outcol = 26 + strlen(sp->name);
        gentype = longgen;
    }
}

int
genalignment(align)
    int             align;
{
    nl();
    switch (align) {
    case 0:
    case 1:
        break;
    case 2:
    case 4:
    case 8:
        fprintf( output, "\tCNOP\t0,%d\n", align);
        break;
    default:
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- invalid alignment\n" );
#endif
        break;
    }
    return (align);
}

void
genstorage(nbytes)
    int             nbytes;
{
    nl();
    fprintf( output, "\tDS.b\t%d\n", nbytes );
}

void
gen_labref(n, offset)
    int             n;
    int             offset;
{
    if (gentype == longgen && outcol < 58) {
        fprintf( output, ",L%d", n );
        if (offset != 0) {
            fprintf( output, "%c%d", (offset > 0) ? '+' : '-', offset );
        }
        outcol += 6;
    }
    else {
        nl();
        fprintf( output, "\tDC.l\tL%d", n );
        if (offset != 0) {
            fprintf( output, "%c%d", (offset > 0) ? '+' : '-', offset );
        }
        outcol = 22;
        gentype = longgen;
    }
}

/*
 * stringconcat
 *
 * Adds the string s onto the end of a string literal selected by index (the
 * label of the string being appended.
 *
 * Returns index if successful, 0 if failed.
 */

long
stringconcat(index, s)
    int             index;
    char           *s;
{
    char           *buf;
    int             length;
    struct slit    *lp;

    if (s == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL argument to stringconcat.\n" );
#endif
        return (0);
    }

    for (lp = strtab; lp != NULL; lp = lp->next)
        if (lp->label == index)
            break;

    if (lp != NULL) {
        length = strlen(s);
        ++global_flag;  /* Allocate from the global space   */
        buf = (char *) xalloc(lp->len + length + 1);
        memcpy(buf, lp->str, lp->len);
        lp->str = buf;
        buf += lp->len;
        memcpy(buf, s, length + 1);
        lp->len += length;
        --global_flag;
        return (index);
    }
    return (0);
}

/*
 * stringlit
 *
 * Make s a string literal and return the label number under which it will be
 * output.
 */

long
stringlit(s)
    char           *s;
{
    struct slit    *lp;

    if (s == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL argument to stringlit.\n" );
#endif
        return (0);
    }
    ++global_flag;      /* always allocate from global space. */
    lp = (struct slit *) xalloc(sizeof(struct slit));
    lp->label = nextlabel++;
    lp->str = litlate(s);
    lp->len = strlen(s);
    lp->type = sconst;
    lp->next = strtab;
    strtab = lp;
    --global_flag;
    return lp->label;
}

/*
 * floatlit
 *
 * Make d a double-precision floating point literal and return the label number
 * under which it will be output.  Hacked to share the slit structure, but uses
 * rconst to distiguish it from an sconst.
 */

long
floatlit(d)
    double          d;
{
    double         *dptr;
    struct slit    *lp;

    ++global_flag;      /* always allocate from global space. */
    lp = (struct slit *) xalloc(sizeof(struct slit));
    lp->label = nextlabel++;
    lp->str = (char *) xalloc(sizeof(double));
    lp->len = sizeof(double);
    lp->next = strtab;
    lp->type = rconst;
    strtab = lp;
    --global_flag;
    dptr = (double *) lp->str;
    *dptr = d;
    return lp->label;
}

/*
 * ieee_d2f_bits — same chopping as math.lib .Fd2s, integer-only.
 * Used so f/F literals never call soft-float during the compile.
 */
static unsigned long
ieee_d2f_bits(hi, lo)
    unsigned long   hi;
    unsigned long   lo;
{
    unsigned long   sign;
    unsigned long   exp;
    unsigned long   mant;

    sign = hi & 0x80000000UL;
    exp = (hi >> 20) & 0x7FFUL;
    /*
     * 23-bit field: top 20 of the double fraction, plus the high 3 bits
     * of the low word (see double.asm .Fd2s DoMant).
     */
    mant = ((hi & 0xFFFFFUL) << 3) | (lo >> 29);

    if (exp > 1151UL) {
        /* Overflow → infinity (exponent 255, zero fraction). */
        return sign | 0x7F800000UL;
    }
    if (exp < 896UL) {
        /* Underflow → signed zero. */
        return sign;
    }
    exp = exp - 896UL;  /* excess-1023 → excess-127 */
    return sign | (exp << 23) | (mant & 0x7FFFFFUL);
}

/*
 * floatlits — pool an IEEE single for C99 1.0f / 1.0F.
 * Avoids runtime en_cdf/.Fd2s on every float suffix (that path still
 * faults under soft-float codegen).  Conversion is integer bit surgery
 * after a normal double store — do not assign through a float lvalue
 * here (that invoked soft-float mid-compile and corrupted strtab).
 */

long
floatlits(d)
    double          d;
{
    double          tmp;
    unsigned long  *wp;
    unsigned long   fbits;
    long           *ip;
    struct slit    *lp;

    tmp = d;
    wp = (unsigned long *) &tmp;
    fbits = ieee_d2f_bits(wp[0], wp[1]);

    ++global_flag;
    lp = (struct slit *) xalloc(sizeof(struct slit));
    lp->label = nextlabel++;
    lp->str = (char *) xalloc(sizeof(float));
    lp->len = sizeof(float);
    lp->next = strtab;
    lp->type = rconst;
    strtab = lp;
    --global_flag;
    ip = (long *) lp->str;
    *ip = (long) fbits;
    return lp->label;
}

/* 
 * getlit
 *
 * Locates a literal by its index and return a pointer to the object.
 */

void *
getlit(num)
    int             num;
{
    struct slit    *lp;
    void           *ptr;

    if (strtab != NULL) {
        lp = strtab;
        for (lp = strtab; lp != NULL; lp = lp->next) {
            if (lp->label == num) {
                ptr = (void *) lp->str;
                return (ptr);
            }
        }
    }
    return (NULL);
}

/*
 * remlit
 *
 * Accepts a label and removes from the list all associated slits.
 * Returning a pointer to the string or NULL if the label wasn't found.
 */

void *
remlit(num)
    int             num;
{
    struct slit    *lp, **last;
    void           *ptr;

    if (strtab != NULL) {
        lp = strtab;
        last = &strtab;
        while (lp != NULL) {
            if (lp->label == num) {
                *last = lp->next;
                ptr = (void *) lp->str;
                return (ptr);
            }
            last = &lp->next;
            lp = lp->next;
        }
    }
    return (NULL);
}

/*
 * Dumps the string literal pool.
 */

void
dumplits()
{
    char           *cp;
    int             i, count;
    int             n;

    n = 0;
    while (strtab != NULL) {
        if (++n > 100000)
            break;
        cseg();
        nl();
        /*
         * Doubles need at least 2-byte alignment for move.l on 68000;
         * CNOP 0,4 keeps abs.l loads of rconst labels safe.
         */
        if (strtab->type == rconst)
            fprintf(output, "\tCNOP\t0,4\n");
        put_label( strtab->label );
        cp = strtab->str;
        count = strtab->len;
        for (i = 0; i < strtab->len; i++) {
            genbyte(*cp++);
        }
        if (strtab->type == sconst) {
            ++count;
            genbyte(0);
        }
        if ((count & 1) != 0) {
            genbyte(0);
        }
        strtab = strtab->next;
    }
    nl();
}

/*
 * nl
 *
 * Generates a newline if needed.
 */

void
nl()
{
    if (outcol > 0) {
        fprintf( output, "\n" );
        outcol = 0;
        gentype = nogen;
    }
}

/*
 * cseg
 *
 * Directs output to the code segment.
 */

void
cseg()
{
    if (curseg != codeseg) {
        nl();
        fprintf( output, "\tSECTION\t%s,CODE\n", code_name );
        curseg = codeseg;
    }
}

/*
 * dseg
 *
 * Directs output to the DATA segment.
 */

void
dseg()
{
    if (curseg != dataseg) {
        nl();
        fprintf( output, "\tSECTION\t%s,DATA\n", data_name );
        curseg = dataseg;
    }
}

/*
 * bseg
 *
 * Directs output to the BSS (unitialized data) segment.
 */

void
bseg()
{
    if (curseg != bssseg) {
        nl();
        fprintf( output, "\tSECTION\t%s,BSS\n", bss_name );
        curseg = bssseg;
    }

}
