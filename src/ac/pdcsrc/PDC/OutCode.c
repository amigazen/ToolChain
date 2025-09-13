
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
    {"add", op_addx},
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
    {"sub", op_subx},
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
        d = x % 10;
        if (d < 0 || d > 9) {
            fprintf(stderr, "DIAG -- itoa has a problem\n");
            return (ptr);
        }
        x = x / 10;
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
        mid = (low + high) / 2;
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
                lenstr = ".f";
                break;
            default:
                lenstr = ".x";
                fprintf( stderr, "DIAG -- illegal length field.\n" );
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

    fprintf( stderr, "DIAG -- illegal opcode.\n" );
}

/*
 * put a constant to the output file.
 */

void
putconst(offset)
    struct enode   *offset;
{
    if (offset == NULL) {
        fprintf( stderr, "DIAG -- NULL argument to putconst.\n" );
        return;
    }
    switch (offset->nodetype) {
    case en_autocon:
        fprintf(output, "%d(A%d)", offset->v.i, Options.Frame);
        break;
    case en_icon:
        fprintf(output, "%d", offset->v.i);
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
        fprintf( stderr, "DIAG -- illegal constant node (%d)\n", 
                         offset->nodetype );
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
        fprintf( stderr, "DIAG -- NULL argument to putamode.\n" );
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
        fprintf( stderr, "DIAG -- illegal address mode.\n" );
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
        if (dp->ref->nodetype == en_autocon || dp->ref->nodetype == en_tempref)
            dp->ref->nodetype = en_icon;
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
    register int bit_mod8 = bit % 8;

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
        fprintf( stderr, "DIAG -- NULL argument to genref.\n" );
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
        fprintf( stderr, "DIAG -- invalid alignment\n" );
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
        fprintf( stderr, "DIAG -- NULL argument to stringconcat.\n" );
        return (0);
    }

    for (lp = strtab; lp != NULL; lp = lp->next)
        if (lp->label == index)
            break;

    if (lp != NULL) {
        length = strlen(s);
        ++global_flag;  /* Allocate from the global space   */
        buf = (char *) xalloc(lp->len + length + 1);
        bcopy(lp->str, buf, lp->len);
        lp->str = buf;
        buf += lp->len;
        bcopy(s, buf, length + 1);
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
        fprintf( stderr, "DIAG -- NULL argument to stringlit.\n" );
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

    while (strtab != NULL) {
        cseg();
        nl();
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
        if ((count % 2) != 0) {
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
