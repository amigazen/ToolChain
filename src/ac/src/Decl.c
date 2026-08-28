
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
 * $Log:	Decl.c,v $
 * Revision 3.33  90/04/04  23:17:41  lionel
 * Fixed parsing of multidimensional arrays.
 * 
 * Revision 3.32  90/02/03  16:23:20  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * Decl.c
 * 
 * Manages typedef's and variable declarations in the symbol table.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"

TYP            *head = NULL;
TYP            *tail = NULL;
char           *declid = NULL;
TABLE           tagtable = {NULL, NULL};
TYP             stdconst = {bt_long, 1, 0, 4, {NULL, NULL}, 0, "const", QUAL_CONST};

/* Bitfield packing state while declare() runs for struct/union members. */
static int      bf_bits_used = -1;
static int      bf_unit_bytes = 0;

/*
 * Pending SAS/C __asm on the current declarator.  Applied to the next
 * bt_func created in decl2(), and/or to head when seen as a specifier.
 */
static int      decl_asm_pending = 0;

void    decl2(), declenum(), enumbody(), declstruct();
void    decl1(void);
void    decl_callconv(void);
void    parse_alignas(void);
void    apply_decl_alignas(TYP *tp);

/*
 * Map __d0..__d7 / __a0..__a6 to ASMREG_* codes (0 if not a reg keyword).
 */
static int
m68k_reg_to_asmreg(st)
    int             st;
{
    if (st >= kw_d0 && st <= kw_d7)
        return ASMREG_D0 + (st - kw_d0);
    if (st >= kw_a0 && st <= kw_a6)
        return ASMREG_A0 + (st - kw_a0);
    return ASMREG_NONE;
}

/*
 * Consume optional SAS/C register keyword (from __REG__(d0, ...)).
 * Accept both keyword tokens (__d0 from SearchKW) and plain ids named
 * "__d0".."__a6" so a partial self-host rebuild still parses NDK headers.
 */
static void
accept_m68k_regkw()
{
    char           *s;

    if (m68k_reg_to_asmreg(lastst) != ASMREG_NONE) {
        getsym();
        return;
    }
    if (lastst != id)
        return;
    s = lastid;
    if (s[0] != '_' || s[1] != '_' || s[2] == '\0' || s[3] == '\0'
        || s[4] != '\0')
        return;
    if (s[2] == 'd' && s[3] >= '0' && s[3] <= '7')
        getsym();
    else if (s[2] == 'a' && s[3] >= '0' && s[3] <= '6')
        getsym();
}

void    apply_qualifiers();

/* Pending _Alignas/alignas value for the current declaration (0 = none). */
static int      decl_alignas = 0;

/*
 * Enum parameters must be declared as int.  Callers push 4-byte ints
 * (default promotions / pea).  A callee that uses enum e_sc / enum e_bt
 * (2-byte) shifts later args by -2: declare() then read ilc from 14(A5)
 * instead of 16(A5), so ICON16L(ilc) was always 0 and every struct member
 * was emitted as (A0) instead of N(A0).  Gen-2 overwrote SYM.next and
 * flooded the console with gigabytes of output.
 */
int     declare(TABLE *table, int al, int ilc, int ztype, int ral);
void    structbody(TYP *tp, int ztype);
void    dodecl(int defclass);

extern SYM      *search();
extern long     intexpr();
extern int      fatal;
extern int      castbegin();
extern void     needpunc();

#ifdef  GENERATE_DBX
extern int      dbx_ident();
#endif

extern struct snode *asmstmt();
extern char    *xalloc();

static int
typesize_mul(count, elemsize)
    int count, elemsize;
{
    /*
     * Incomplete types have tp->size == -1 while a tag is being defined.
     * Skip the scale when the element size is not yet known.
     *
     * Product via u16_product() - never calls ac.lib .lmuls at runtime.
     * Unpoison (e_sc<<16)|n via ICON16L.  Do NOT invent elemsize==4 when
     * the low half is 0: that turned unsigned char chstack[20] into
     * DS.b 80 under ac-self (self/GetSym.s).  Callers must pass
     * type_size(head), not a raw poisoned head->size.
     *
     * When elemsize collapses to 0 after unpoison, returning bare `count`
     * made int *a[10] / int a[10] emit DS.b 10.  Prefer 4-byte elements
     * (pointer/int width) over count-only BSS.
     */
    if (elemsize == -1)
        return count;
    if (elemsize != (int) ICON16L((long) elemsize))
        elemsize = (int) ICON16L((long) elemsize);
    if (elemsize <= 0)
        elemsize = 4;
    if (count <= 0)
        return 0;
    return (int) u16_product((long) count, (long) elemsize);
}

int
type_size(tp)
    TYP            *tp;
{
    long            sz;
    int             t;
    int             esz;
    int             n;

    /*
     * Arrays are bt_pointer nodes with val_flag set; only real pointers
     * are always four bytes on the 680x0.
     *
     * Never trust a raw tp->size through ICON16L alone: SAS/C/ac-self can
     * leave (e_sc<<16) in the high word.  ICON16L(393216)==0, which made
     * every struct member offset 0, emptied BSS, and produced ac-self2
     * RELOC32 to hunk 2099356 ("not executable").
     */
    if (tp == NULL)
        return 0;

    t = (int) tp->type;
    /* Real pointers only - sized arrays keep val_flag != 0 (see declare()). */
    if (t == bt_pointer && tp->val_flag == 0)
        return 4;

    switch (t) {
    case bt_char:
    case bt_uchar:
    case bt_bool:
        return 1;
    case bt_short:
    case bt_ushort:
        return 2;
    case bt_enum:
        return 4;   /* SAS F.3.9: enumeration type is int */
    case bt_long:
    case bt_unsigned:
    case bt_ulong:
    case bt_float:
        return 4;
    case bt_longlong:
    case bt_ulonglong:
    case bt_double:
        return 8;
    case bt_func:
    case bt_ifunc:
    case bt_void:
        return 0;
    default:
        break;
    }

    /*
     * Sized array: tp->size should be count*elemsize.  When typesize_mul
     * (or an older compiler) left a bare count (e.g. 10 for int *[10]),
     * recompute from the element type - modulo mismatch is the signal.
     */
    if (t == bt_pointer && tp->val_flag != 0 && tp->btp != NULL) {
        esz = type_size(tp->btp);
        if (esz <= 0)
            esz = 4;
        sz = (long) tp->size;
        if (sz != -1L && sz != ICON16L(sz))
            sz = ICON16L(sz);
        if (sz > 0L && esz > 0) {
            if (sz < (long) esz || (sz % (long) esz) != 0L) {
                n = (int) sz;
                if (n > 0 && n < 65536)
                    return (int) u16_product((long) n, (long) esz);
            }
            return (int) sz;
        }
        if (sz <= 0L)
            return 0;
        return (int) sz;
    }

    sz = (long) tp->size;
    /*
     * Unpoison (e_sc<<16)|n.  Do not use a 65535 literal - 16-bit hosts
     * fold it to -1 so (~65535L)==0 and the mask never strips the high word
     * (that returned 393216 as a pointer stride and broke argv[i]).
     */
    if (sz != -1L && sz != ICON16L(sz))
        sz = ICON16L(sz);
    if (sz <= 0)
        return 0;
    return (int) sz;
}

int
imax(int i, int j)
{
    return (i > j) ? i : j;
}

char           *
litlate(char *s)
{
    char           *p;

    p = (char *) xalloc(strlen(s) + 1);
    strcpy(p, s);
    return p;
}

SYM            *
copysym(SYM *sp)
{
    SYM            *esp;
    int             siz;

    siz = SZ_SYM;
    ++global_flag;
    esp = (SYM *) xalloc(siz);
    --global_flag;

    if (esp != NULL) {
        *esp = *sp;
        esp->next = NULL;
    }
    return (esp);
}


TYP            *
maketype(enum e_bt bt, int siz)
{
    TYP            *tp;

    tp = (TYP *) xalloc(SZ_TYP);
    tp->val_flag = 0;
    tp->_pad_typ = 0;
    tp->size = siz;
    tp->type = bt;
    tp->sname = NULL;
    tp->lst.head = NULL;
    tp->lst.tail = NULL;
    tp->btp = NULL;
    tp->qualifiers = 0;  /* Initialize qualifiers to none */
    tp->mem_section = 0; /* Initialize memory section to none */
    return tp;
}

TYP            *
istypedef(TABLE *table)
{
    SYM            *sp;

    if (lastst == id) {
        if (table != NULL) {
            sp = search(lastid, table->head);
            if (sp != NULL && sp->tp != NULL && sp->tp->type == bt_typedef)
                return (sp->tp->btp);
        }
        sp = search(lastid, lsyms.head);
        if (sp != NULL && sp->tp != NULL && sp->tp->type == bt_typedef)
            return (sp->tp->btp);
        sp = search(lastid, gsyms.head);
        if (sp != NULL && sp->tp != NULL && sp->tp->type == bt_typedef)
            return (sp->tp->btp);
    }
    return (NULL);
}

int
is_class_error(void)
{
    if (lastst == kw_auto) {
        error( ERR_SYNTAX, "auto keyword in wrong position" );
        getsym();
    }
    else if (lastst == kw_const) {
        error( ERR_SYNTAX, "const keyword in wrong position" );
        getsym();
    }
    else if (lastst == kw_static) {
        error( ERR_SYNTAX, "static keyword in wrong position" );
        getsym();
    }
    else if (lastst == kw_register) {
        error( ERR_SYNTAX, "register keyword in wrong position" );
        getsym();
    }
    return (0);
}

/*
 * Collect C integer type-specifier lists in any order (C89/C99/C23):
 * signed/unsigned, short, long/long long, int, char; also long double.
 * Sets head/tail.  Returns 1 on success, 0 if no specifiers were seen.
 */
static int
decl_int_specs()
{
    int             sign;       /* 0 none, 1 signed, 2 unsigned */
    int             nshort;
    int             nlong;
    int             saw_char;
    int             saw_int;
    int             cont;
    int             bad;

    sign = 0;
    nshort = 0;
    nlong = 0;
    saw_char = 0;
    saw_int = 0;
    cont = 1;
    bad = 0;

    while (cont) {
        switch (lastst) {
        case kw_signed:
            if (sign)
                bad = 1;
            else
                sign = 1;
            getsym();
            break;
        case kw_unsigned:
            if (sign)
                bad = 1;
            else
                sign = 2;
            getsym();
            break;
        case kw_short:
            if (nshort || nlong)
                bad = 1;
            else
                nshort = 1;
            getsym();
            break;
        case kw_long:
            if (nshort || nlong >= 2)
                bad = 1;
            else
                nlong++;
            getsym();
            break;
        case kw_int:
            if (saw_int || saw_char)
                bad = 1;
            else
                saw_int = 1;
            getsym();
            break;
        case kw_char:
            if (saw_char || saw_int || nshort || nlong)
                bad = 1;
            else
                saw_char = 1;
            getsym();
            break;
        case kw_double:
            /* long double - map to double (8 bytes) on Amiga */
            if (nlong == 1 && !nshort && !saw_char && !saw_int && sign == 0) {
                head = tail = maketype(bt_double, 8);
                getsym();
                return 1;
            }
            cont = 0;
            break;
        default:
            cont = 0;
            break;
        }
    }

    if (bad)
        error(ERR_SYNTAX, "invalid type specifier combination");

    if (!(sign || nshort || nlong || saw_char || saw_int))
        return 0;

    if (saw_char) {
        if (sign == 2)
            head = tail = maketype(bt_uchar, 1);
        else
            head = tail = maketype(bt_char, 1);
    } else if (nshort) {
        if (sign == 2)
            head = tail = maketype(bt_ushort, 2);
        else
            head = tail = maketype(bt_short, 2);
    } else if (nlong >= 2) {
        if (sign == 2)
            head = tail = maketype(bt_ulonglong, 8);
        else
            head = tail = maketype(bt_longlong, 8);
    } else if (nlong == 1) {
        if (sign == 2)
            head = tail = maketype(bt_unsigned, 4);
        else
            head = tail = maketype(bt_long, 4);
    } else {
        /* int / signed / unsigned [int] */
        if (sign == 2)
            head = tail = maketype(bt_unsigned, 4);
        else
            head = tail = maketype(bt_long, 4);
    }
    return 1;
}

void
decl(TABLE *table)
{
    TYP            *tp;

    switch (lastst) {
    case kw_typedef:
        getsym();
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- SHOULD NEVER HAPPEN\n" );
#endif
        break;
    case kw_auto:
        getsym();
        decl(table);
        break;
    case kw_register:
        getsym();
        /* SAS/C: register __d0 LONG x  (from __REG__(d0, LONG x)) */
        accept_m68k_regkw();
        decl(table);
        break;
    case kw_asm:
        /*
         * SAS/C __asm (compiler-specific.h __ASM__): registerized
         * parameter convention.  May appear as a declaration specifier.
         */
        getsym();
        decl_asm_pending = 1;
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_ASM;
        break;
    case kw_aligned:
        /* SAS/C __aligned: force 4-byte alignment (like alignas(4)). */
        getsym();
        if (decl_alignas < 4)
            decl_alignas = 4;
        decl(table);
        apply_decl_alignas(head);
        break;
    case kw_const:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_CONST;
        break;
    case kw_volatile:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_VOLATILE;
        break;
    case kw_restrict:
    case kw_inline:
    case kw_noreturn:
        /* Accepted as no-ops; no codegen effect yet. */
        getsym();
        decl(table);
        break;
    case kw_alignas:
        /* C11/C23: alignas(N) / alignas(type) as a declaration specifier. */
        parse_alignas();
        decl(table);
        apply_decl_alignas(head);
        break;
    case kw_chip:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_CHIP;
        break;
    case kw_far:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_FAR;
        break;
    case kw_near:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_NEAR;
        break;
    case kw_fast:
        getsym();
        decl(table);
        if (head != NULL)
            head->mem_section = MEM_FAST;
        break;
    case kw_interrupt:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_INTERRUPT;
        break;
    case kw_regargs:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_REGARGS;
        break;
    case kw_stdargs:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_STDARGS;
        break;
    case kw_saveds:
        getsym();
        decl(table);
        if (head != NULL)
            head->qualifiers |= QUAL_SAVEDS;
        break;
    case kw_char:
        head = tail = maketype(bt_char, 1);
        getsym();
        is_class_error();
        break;
    case kw_short:
    case kw_long:
    case kw_int:
    case kw_signed:
    case kw_unsigned:
        /* Any-order C type-specifier lists (unsigned long int, long unsigned, ...) */
        if (!decl_int_specs())
            head = tail = maketype(bt_long, 4);
        is_class_error();
        break;
    case kw_void:
        /* Historically treated as 4-byte int for void * / void returns */
        head = tail = maketype(bt_long, 4);
        getsym();
        is_class_error();
        break;
    case kw_bool:
        head = tail = maketype(bt_bool, 1);
        getsym();
        is_class_error();
        break;
    /* Exact-width names map onto the same base types as char/short/long/ll */
    case kw_int8:
        head = tail = maketype(bt_char, 1);
        getsym();
        is_class_error();
        break;
    case kw_uint8:
        head = tail = maketype(bt_uchar, 1);
        getsym();
        is_class_error();
        break;
    case kw_int16:
        head = tail = maketype(bt_short, 2);
        getsym();
        is_class_error();
        break;
    case kw_uint16:
        head = tail = maketype(bt_ushort, 2);
        getsym();
        is_class_error();
        break;
    case kw_int32:
        head = tail = maketype(bt_long, 4);
        getsym();
        is_class_error();
        break;
    case kw_uint32:
        head = tail = maketype(bt_unsigned, 4);
        getsym();
        is_class_error();
        break;
    case kw_int64:
        head = tail = maketype(bt_longlong, 8);
        getsym();
        is_class_error();
        break;
    case kw_uint64:
        head = tail = maketype(bt_ulonglong, 8);
        getsym();
        is_class_error();
        break;
    case kw_intptr:
        head = tail = maketype(bt_long, 4);
        getsym();
        is_class_error();
        break;
    case kw_uintptr:
        head = tail = maketype(bt_unsigned, 4);
        getsym();
        is_class_error();
        break;
    case kw_intmax:
        head = tail = maketype(bt_longlong, 8);
        getsym();
        is_class_error();
        break;
    case kw_uintmax:
        head = tail = maketype(bt_ulonglong, 8);
        getsym();
        is_class_error();
        break;
    case id:        /* no type declarator, or could be a typedef  */
        if ((tp = istypedef(table)) != NULL) {
            head = tail = tp;
            getsym();
        }
        else
            head = tail = maketype(bt_long, 4);
        break;
    case kw_float:
        head = tail = maketype(bt_float, 4);
        getsym();
        is_class_error();
        break;
    case kw_double:
        head = tail = maketype(bt_double, 8);
        getsym();
        is_class_error();
        break;
    case kw_enum:
        getsym();
        declenum(table);
        is_class_error();
        break;
    case kw_struct:
        getsym();
        declstruct(bt_struct);
        is_class_error();
        break;
    case kw_union:
        getsym();
        declstruct(bt_union);
        is_class_error();
        break;
    }
}

/*
 * SAS/C allows calling-convention keywords between the base type and the
 * declarator:  void __stdargs (*f)(long);  void __saveds foo(void);
 * Also __asm from compiler-specific.h __ASM__.
 */
void
decl_callconv(void)
{
    while (lastst == kw_stdargs || lastst == kw_regargs
           || lastst == kw_saveds || lastst == kw_interrupt
           || lastst == kw_asm) {
        if (lastst == kw_asm) {
            decl_asm_pending = 1;
            if (head != NULL)
                head->qualifiers |= QUAL_ASM;
        } else if (head != NULL) {
            if (lastst == kw_stdargs)
                head->qualifiers |= QUAL_STDARGS;
            else if (lastst == kw_regargs)
                head->qualifiers |= QUAL_REGARGS;
            else if (lastst == kw_saveds)
                head->qualifiers |= QUAL_SAVEDS;
            else if (lastst == kw_interrupt)
                head->qualifiers |= QUAL_INTERRUPT;
        }
        getsym();
    }
    /* alignas may appear after the type: int alignas(8) x; */
    while (lastst == kw_alignas) {
        parse_alignas();
        apply_decl_alignas(head);
    }
}

/*
 * Parse _Alignas(const-expr) / alignas(type-name).  Updates decl_alignas
 * to the maximum requested alignment (powers of two; 0 is ignored).
 */
void
parse_alignas(void)
{
    long            val;
    int             a;
    TYP            *save_head;
    TYP            *save_tail;
    char           *save_declid;

    getsym();
    needpunc(openpa);
    if (castbegin(lastst)) {
        save_head = head;
        save_tail = tail;
        save_declid = declid;
        head = tail = NULL;
        declid = NULL;
        decl(NULL);
        decl1();
        if (head != NULL)
            a = alignment(head);
        else
            a = 0;
        head = save_head;
        tail = save_tail;
        declid = save_declid;
    } else {
        val = intexpr();
        a = (int) val;
    }
    needpunc(closepa);
    if (a < 0)
        a = 0;
    /* alignas(0) is a no-op; otherwise require a power of two. */
    if (a != 0 && (a & (a - 1)) != 0) {
        error(ERR_SYNTAX, "alignas requires a power-of-two alignment");
        a = 0;
    }
    if (a > 8) {
        /* Amiga CNOP supports 2/4/8; larger requests are capped. */
        warning(ERR_SYNTAX, "alignas capped at 8 for m68k");
        a = 8;
    }
    if (a > decl_alignas)
        decl_alignas = a;
}

void
apply_decl_alignas(tp)
    TYP            *tp;
{
    if (tp == NULL || decl_alignas <= 0)
        return;
    if ((unsigned char) tp->_pad_typ < (unsigned) decl_alignas)
        tp->_pad_typ = (char) decl_alignas;
}

void
decl1(void)
{
    TYP            *temp1, *temp2, *temp3, *temp4;

    decl_callconv();

    switch (lastst) {
    case id:
        declid = litlate(lastid);
        getsym();
        decl2();
        break;
    case star:
        /*
         * pointer: * type-qualifier-list_opt pointer_opt
         * Qualifiers after * bind to the pointer (int *const p).
         */
        temp1 = maketype(bt_pointer, 4);
        temp1->btp = head;
        head = temp1;
        if (tail == NULL)
            tail = head;
        getsym();
        while (lastst == kw_const || lastst == kw_volatile
               || lastst == kw_restrict || lastst == kw_asm
               || lastst == kw_stdargs || lastst == kw_regargs
               || lastst == kw_saveds || lastst == kw_interrupt) {
            if (lastst == kw_const)
                temp1->qualifiers |= QUAL_CONST;
            else if (lastst == kw_volatile)
                temp1->qualifiers |= QUAL_VOLATILE;
            else if (lastst == kw_asm) {
                /* LONG (* __asm name)(args) - __asm binds to the function. */
                decl_asm_pending = 1;
                temp1->qualifiers |= QUAL_ASM;
            } else if (lastst == kw_stdargs)
                temp1->qualifiers |= QUAL_STDARGS;
            else if (lastst == kw_regargs)
                temp1->qualifiers |= QUAL_REGARGS;
            else if (lastst == kw_saveds)
                temp1->qualifiers |= QUAL_SAVEDS;
            else if (lastst == kw_interrupt)
                temp1->qualifiers |= QUAL_INTERRUPT;
            /* restrict: accepted, no QUAL bit yet */
            getsym();
        }
        decl1();
        break;
    case openpa:
        getsym();
        temp1 = head;
        temp2 = tail;
        head = tail = NULL;
        decl1();
        needpunc(closepa);
        temp3 = head;
        temp4 = tail;
        head = temp1;
        tail = temp2;
        decl2();
        if (temp4 != NULL) {
            temp4->btp = head;
            if (temp4->type == bt_pointer && temp4->val_flag != 0
                && head != NULL)
                temp4->size = typesize_mul(temp4->size, type_size(head));
        }
        head = temp3;
        break;
    default:
        decl2();
        break;
    }
}

void
decl2(void)
{
    TYP            *temp1;

    switch (lastst) {
    case openbr:
        getsym();
        temp1 = maketype(bt_pointer, 4);
        temp1->val_flag = 1;
        if (lastst == closebr) {
            temp1->size = 0;
            temp1->val_flag = 1;
            getsym();
        }
        else {
            temp1->size = intexpr();
            needpunc(closebr);
        }
        decl2();
        if (head != NULL) {
            /* type_size() ignores poisoned tp->size for builtins. */
            temp1->size = typesize_mul(temp1->size, type_size(head));
        }
        temp1->btp = head;
        head = temp1;
        if (tail == NULL)
            tail = head;
        break;
    case openpa:
        getsym();
        temp1 = maketype(bt_func, 0);
        temp1->val_flag = 1;
        temp1->btp = head;
        if (decl_asm_pending) {
            temp1->qualifiers |= QUAL_ASM;
            decl_asm_pending = 0;
        }
        head = temp1;
        if (lastst == closepa) {
            getsym();
            if (lastst == begin)
                temp1->type = bt_ifunc;
        }
        else if (castbegin(lastst)) {
            /*
             * ANSI prototype parameter list: void (*f)(long), atexit(void (*)(void)),
             * int foo(int);  Parse now so tokens are not left for needpunc(';').
             * declproto() clobbers global head/tail/declid - restore the function
             * type in temp1 afterward so the outer declarator stays consistent.
             */
            {
                SYM            *save_h;
                SYM            *save_t;
                char           *save_declid;

                save_h = lsyms.head;
                save_t = lsyms.tail;
                save_declid = declid;
                declproto(&temp1->lst);
                needpunc(closepa);
                head = temp1;
                tail = temp1;
                declid = save_declid;
                if (lastst == begin)
                    temp1->type = bt_ifunc;
                else {
                    temp1->type = bt_func;
                    lsyms.head = save_h;
                    lsyms.tail = save_t;
                }
            }
        }
        else
            /* K&R identifier list - leave tokens for funcbody(). */
            temp1->type = bt_ifunc;
        break;
    }
}

int
alignment(TYP *tp)
{
    int             a;
    int             forced;

    if (tp == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL argument to alignment.\n" );
#endif
        return (AL_CHAR);
    }
    switch (tp->type) {
    case bt_char:
    case bt_uchar:
    case bt_bool:
        a = AL_CHAR;
        break;
    case bt_short:
    case bt_ushort:
        a = AL_SHORT;
        break;
    case bt_long:
    case bt_unsigned:
        a = AL_LONG;
        break;
    case bt_longlong:
    case bt_ulonglong:
        a = AL_LONG;  /* 8-byte objects still 2-byte-aligned on 68000 */
        break;
    case bt_enum:
        a = AL_LONG;  /* enum is int-sized; even alignment like long */
        break;
    case bt_pointer:
        if (tp->val_flag)
            a = alignment(tp->btp);
        else
            a = AL_POINTER;
        break;
    case bt_float:
        a = AL_FLOAT;
        break;
    case bt_double:
        a = AL_DOUBLE;
        break;
    case bt_struct:
    case bt_union:
        a = AL_STRUCT;
        break;
    default:
        a = AL_CHAR;
        break;
    }
    /* _Alignas / alignas stored in _pad_typ (0 = none). */
    forced = (unsigned char) tp->_pad_typ;
    if (forced > a)
        a = forced;
    return a;
}

int
declare(table, al, ilc, ztype, ral)
    TABLE          *table;
    int             al;     /* enum e_sc - must be int (see prototype) */
    int             ilc;
    int             ztype;  /* enum e_bt */
    int             ral;    /* enum e_sc */

/*
 * process declarations of the form:
 * 
 * <type>  <decl>, <decl>...;
 * 
 * leaves the declarations in the symbol table pointed to by table and returns
 * the number of bytes declared. al is the allocation type to assign, ilc is
 * the initial location counter. if al is sc_member then no initialization
 * will be processed. ztype should be bt_struct for normal and in structure
 * declarations and sc_union for in union declarations.
 * 
 * If al == sc_auto && ral == sc_member then we are processing parameters
 * 
 */
{
    SYM            *sp, *sp1;
    TYP            *dhead, *tp2;
    int             nbytes = 0, num;

    if (lastst == kw_typedef) {
        getsym();
        decl(table);
        dhead = head;
        apply_decl_alignas(dhead);
        for (;;) {
            declid = NULL;
            decl1();
            if (declid != NULL) {
                sp = (SYM *) xalloc(SZ_SYM);
                sp->name = declid;
                sp->storage_class = sc_type;
                sp->value.i = 0;
                tp2 = maketype(bt_typedef, 0);
                tp2->btp = head;
                tp2->size = head->size;
                head = tp2;
                sp->tp = head;
                insert(sp, table);
#ifdef  GENERATE_DBX
                dbx_ident(sp);
#endif
            }
            if (lastst != comma)
                break;
            needpunc(comma);
            head = dhead;
        }
        needpunc(semicolon);
        decl_alignas = 0;
        return (0);
    }

    decl(table);
    dhead = head;
    apply_decl_alignas(dhead);
    for (;;) {
        declid = NULL;
        decl_asm_pending = 0;
        decl1();

        if (head == NULL) {
            error( ERR_SYNTAX, "unknown type" );
            break;
        }
        
        if (ral == sc_member && al == sc_auto || ral == sc_parameter) {
            ral = sc_parameter; /* Parameter Decl */
            /*
             * Keep char/short/enum as declared so &param and sizeof work.
             * Float is still widened to double to match parmlist (en_cfd pushes
             * 8 bytes).  Func.c places char at +3 and short at +2 in a
             * 4-byte big-endian slot.
             */
            if (head->type == bt_float)
                head = tail = maketype(bt_double, 8);
            /*
             * C89: a parameter declared as "array of T" is adjusted to
             * "pointer to T".  Only apply here (parameters), never to
             * globals like char *incldir[10] (array of pointers).
             */
            if (head->type == bt_pointer && head->val_flag != 0) {
                head->val_flag = 0;
                head->size = 4;
            }
        }

        if (declid != NULL || (al == sc_member && lastst == colon)) {
            /*
             * C89 bitfields: type declarator_opt : width ;
             * Pack consecutive fields into storage units of the base type.
             */
            if (al == sc_member && lastst == colon) {
                int             width;
                int             unit_bytes;
                int             unit_bits;
                int             byte_off;

                getsym();
                width = (int) intexpr();
                if (width < 0 || width > 32) {
                    error(ERR_SYNTAX, "invalid bitfield width");
                    width = 1;
                }
                unit_bytes = type_size(head);
                if (unit_bytes <= 0)
                    unit_bytes = 4;
                if (unit_bytes > 4)
                    unit_bytes = 4;
                unit_bits = unit_bytes * 8;

                if (width == 0) {
                    /* Zero-width: align to next unit boundary. */
                    bf_bits_used = unit_bits;
                } else {
                    int             bitpos;

                    if (bf_bits_used < 0 || bf_unit_bytes != unit_bytes
                        || bf_bits_used + width > unit_bits) {
                        num = alignment(head);
                        if (num <= 0)
                            num = 1;
                        if (ztype != bt_union) {
                            while (safe_lmod(ilc + nbytes, num) != 0)
                                ++nbytes;
                            byte_off = ilc + nbytes;
                            nbytes += unit_bytes;
                        } else {
                            byte_off = ilc;
                            if (unit_bytes > nbytes)
                                nbytes = unit_bytes;
                        }
                        bf_bits_used = 0;
                        bf_unit_bytes = unit_bytes;
                    } else {
                        if (ztype == bt_union)
                            byte_off = ilc;
                        else
                            byte_off = ilc + nbytes - unit_bytes;
                    }

                    /*
                     * SAS/C F.3.9: allocate bitfields left-to-right in the
                     * storage unit (MSB first on big-endian m68k).  bitpos
                     * is the right-shift count to bring the field to bit 0.
                     */
                    bitpos = unit_bits - bf_bits_used - width;
                    if (bitpos < 0)
                        bitpos = 0;

                    if (declid != NULL) {
                        sp = (SYM *) xalloc(SZ_SYM);
                        sp->name = declid;
                        sp->storage_class = al;
                        sp->storage_type = ral;
                        sp->next = NULL;
                        sp->tp = head;
                        sp->value.i = SYM_BF_ENC(byte_off, bitpos, width);
                        insert(sp, table);
#ifdef  GENERATE_DBX
                        dbx_ident(sp);
#endif
                    }
                    bf_bits_used += width;
                }

                if (lastst == semicolon)
                    break;
                needpunc(comma);
                if (!declbegin(table, lastst))
                    break;
                head = dhead;
                continue;
            }

            /* Non-bitfield member ends the current bitfield unit. */
            if (al == sc_member) {
                bf_bits_used = -1;
                bf_unit_bytes = 0;
            }

            if (declid == NULL)
                goto decl_list_tail;

            sp = (SYM *) xalloc(SZ_SYM);
            sp->name = declid;
            sp->storage_class = al;
            sp->storage_type = ral;
            sp->next = NULL;
            sp->value.i = 0;
            num = alignment(head);
            if (num <= 0)
                num = 1;
            /*
             * Only bump nbytes here so lc_static stays accurate.
             * CNOP must be emitted in doinit() in the object's real
             * section (BSS vs DATA).  A DATA-section CNOP left BSS
             * odd; ac-self then skipped the pad when lc tracking
             * drifted, and A68k rejected move.l to odd _optarg.
             */
            if (safe_lmod(ilc + nbytes, num) != 0) {
                while (safe_lmod(ilc + nbytes, num) != 0)
                    ++nbytes;
            }
            sp->tp = head;
            if (al == sc_static) {
                sp->value.i = nextlabel++;
                if (sp->tp->type == bt_ifunc) {
                    sp1 = search(sp->name, table->head);
                    if (sp1 != NULL && sp1->tp->type == bt_func) {
                        sp->value.i = sp1->value.i;
                    }
                }
            }
            else if (ztype == bt_union)
                sp->value.i = (long) ilc;
            else if (al != sc_auto)
                sp->value.i = (long) ilc + (long) nbytes;
            else
                /* Full 32-bit frame offset; ICON16L truncated past +/-32K. */
                sp->value.i = -(long) ilc - (long) nbytes
                    - (long) type_size(head);

            if (sp->tp->type == bt_func) {
                if (sp->storage_class == sc_global || 
                    sp->storage_class == sc_auto) {
                    sp->storage_class = sc_external;
                }
            }

            if (ztype == bt_union) {
                {
                    int tsz;

                    tsz = type_size(sp->tp);
                    if (tsz > 0)
                        nbytes = imax(nbytes, tsz);
                }
            }
            else if (al != sc_external) {
                int tsz;

                tsz = type_size(head);
                if (tsz > 0)
                    nbytes += tsz;
            }

            if (sp->tp->type != bt_ifunc) 
                insert(sp, table);
            else {
                sp1 = search(sp->name, table->head);
                if (sp1 == NULL || sp1->tp->type != bt_func) 
                    insert(sp, table);
                else {
#if 0
                    if ((sp1->storage_class != sp->storage_class) ||
                        (sp1->storage_type  != sp->storage_type ))
                        error( ERR_SYNTAX, "storage class is not consistant" );
#endif
                    sp1->tp = sp->tp;
                    sp1->storage_class = sp->storage_class;
                    sp1->storage_type  = sp->storage_type;
                    sp1->value.i = sp->value.i;
                    sp = sp1;
                }
            }

            if (al == sc_auto)
                doinitauto(sp);

            if ((al == sc_global || al == sc_static) &&
                (sp->tp->type != bt_func && sp->tp->type != bt_ifunc))
                doinit(sp);

#ifdef  GENERATE_DBX
            if (ral != sc_parameter)
                dbx_ident(sp);
#endif

            if (sp->tp->type == bt_ifunc) { /* function body follows */
                funcbody(sp);
                decl_alignas = 0;
                return nbytes;
            }
        }

    decl_list_tail:
        if (lastst == semicolon)
            break;

        needpunc(comma);

        if (!declbegin(table, lastst))
            break;
        head = dhead;
    }
    getsym();
    decl_alignas = 0;
    return nbytes;
}

int
declbegin(table, st)
    TABLE          *table;
    int             st; /* enum e_sym -- int width for ac-self params */
{
    TYP            *tp;

    tp = istypedef(table);

    if (tp != NULL)
        return (FALSE);

    return st == star || st == id || st == openpa || st == openbr;
}

/*
 * True if lastst begins a block-level declaration (C99 mixed decls).
 * Matches the tokens dodecl(sc_auto) will accept before falling through
 * to a statement.
 */
int
blockdeclbegin()
{
    switch (lastst) {
    case kw_static_assert:
    case kw_alignas:
    case kw_typedef:
    case kw_static:
    case kw_extern:
    case kw_auto:
    case kw_register:
    case asmconst:
        return 1;
    default:
        return castbegin(lastst);
    }
}

/*
 * C99 for-init declaration: only auto/register objects (plus type
 * specifiers / castbegin).  No typedef/static/extern/static_assert.
 */
int
fordeclbegin()
{
    if (lastst == kw_auto || lastst == kw_register || lastst == kw_alignas)
        return 1;
    if (lastst == kw_typedef || lastst == kw_static || lastst == kw_extern
        || lastst == kw_static_assert || lastst == asmconst)
        return 0;
    return castbegin(lastst);
}

void
declenum(TABLE *table)
{
    SYM            *sp;

    ++global_flag;

    if (lastst == id) {
        if ((sp = search(lastid, tagtable.head)) == NULL) {
            sp = (SYM *) xalloc(SZ_SYM);
            memset( (char *)sp, 0, SZ_SYM);
            sp->storage_class = sc_type;
            sp->name = litlate(lastid);
            sp->tp = (TYP *) xalloc(SZ_TYP);
            sp->tp->type = bt_enum;
            sp->tp->size = 4;   /* SAS F.3.9: enumeration values are int */
            sp->tp->lst.head = NULL;
            sp->tp->lst.tail = NULL;
            sp->tp->btp = NULL;
            sp->tp->sname = sp->name;
            getsym();
            if (lastst != begin)
                error(ERR_INCOMPLETE, NULL);
            else {
                insert(sp, &tagtable);
                getsym();
                enumbody(sp, table);
#ifdef  GENERATE_DBX
                dbx_ident(sp);
#endif
            }
        }
        else
            getsym();
        head = sp->tp;
    }
    else {
        sp = (SYM *) xalloc(SZ_SYM);
        memset( (char *)sp, 0, SZ_SYM );
        sp->storage_class = sc_type;
        sp->name = NULL;
        sp->tp = (TYP *) xalloc(SZ_TYP);
        sp->tp->type = bt_enum;
        sp->tp->size = 4;   /* anonymous enum also int-sized */
        sp->tp->lst.head = NULL;
        sp->tp->lst.tail = NULL;
        sp->tp->btp = NULL;
        sp->tp->sname = NULL;
        if (lastst != begin)
            error(ERR_INCOMPLETE, NULL);
        else {
            getsym();
            enumbody(sp, table);
#ifdef  GENERATE_DBX
            dbx_ident(sp);
#endif
        }
        head = sp->tp;
    }

    --global_flag;
}

void
enumbody(SYM *sym_sp, TABLE *table)
{
    int             evalue;
    SYM            *sp, *sp2;

    evalue = 0;
    while (lastst == id) {
        sp = (SYM *) xalloc(SZ_SYM);
        sp->name = litlate(lastid);
        sp->storage_class = sc_const;
        sp->value.i = evalue++;
        sp->tp = &stdconst;
        insert(sp, table);
        sp2 = copysym(sp);
        insert(sp2, &(sym_sp->tp->lst));
        getsym();   /* Skip the identifier */
        if (lastst == assign) {
            getsym();
            sp->value.i = sp2->value.i = intexpr();
            evalue = (sp->value.i) + 1;
        }
        if (lastst == comma)
            getsym();
        else if (lastst != end)
            break;
    }
    needpunc(end);
}

void
declstruct(enum e_bt ztype)

/*
 * declare a structure or union type. ztype should be either bt_struct or
 * bt_union.
 */
{
    SYM            *sp;

    ++global_flag;

    if (lastst == id) {
        if ((sp = search(lastid, tagtable.head)) == NULL) {
            sp = (SYM *) xalloc(SZ_SYM);
            memset((char *)sp, 0, SZ_SYM);
            sp->name = litlate(lastid);
            sp->storage_class = sc_type;
            sp->next = NULL;
            sp->tp = (TYP *) xalloc(SZ_TYP);
            sp->tp->type = ztype;
            sp->tp->lst.head = NULL;
            sp->tp->sname = sp->name;
            getsym();
            if (lastst == begin) {
                insert(sp, &tagtable);
                getsym();
                structbody(sp->tp, ztype);
#ifdef  GENERATE_DBX
                dbx_ident(sp);
#endif
            }
            else {
                /*
                 * Incomplete tag OK: struct T;  struct T *p;
                 * typedef struct T Name;  (definition may come later)
                 */
                sp->tp->size = 0;
                insert(sp, &tagtable);
            }
        }
        else {
            if (sp->tp->size != 0)
                getsym();
            else {
                sp->tp->type = ztype;
                sp->tp->lst.head = NULL;
                sp->storage_class = sc_type;
                sp->tp->sname = sp->name;
                getsym();
                if (lastst == begin) {
                    getsym();
                    structbody(sp->tp, ztype);
#ifdef  GENERATE_DBX
                    dbx_ident(sp);
#endif
                }
                else
                    sp->tp->size = 0;   /* still incomplete */
            }
        }
        head = sp->tp;
    }
    else {
        sp = (SYM *) xalloc(SZ_SYM);
        sp->name = NULL;
        sp->storage_class = sc_type;
        sp->next = NULL;
        sp->tp = (TYP *) xalloc(SZ_TYP);
        sp->tp->type = ztype;
        sp->tp->sname = NULL;
        sp->tp->lst.head = NULL;
        sp->tp->lst.tail = NULL;
        if (lastst != begin)
            error(ERR_INCOMPLETE, NULL);
        else {
            getsym();
            structbody(sp->tp, ztype);
#ifdef  GENERATE_DBX
            dbx_ident(sp);
#endif
        }
        head = sp->tp;
    }

    --global_flag;
}

void
structbody(tp, ztype)
    TYP            *tp;
    int             ztype;  /* enum e_bt - must be int (see prototype) */
{
    int             slc;

    if (tp == NULL) {
#if AC_DEBUG
        fprintf(AC_DIAG_STREAM, "DIAG -- NULL argument to structbody.\n" );
#endif
        return;
    }
    tp->lst.head = tp->lst.tail = NULL;
    slc = 0;
    tp->val_flag = 1;
    tp->size = -1;
    bf_bits_used = -1;
    bf_unit_bytes = 0;
    while (lastst != end) {
        if (ztype == bt_struct)
            slc += declare(&(tp->lst), sc_member, slc, ztype, sc_member);
        else
            slc = imax(slc, declare(&tp->lst, sc_member, 0, ztype, sc_member));
    }
    bf_bits_used = -1;
    bf_unit_bytes = 0;

    /*
     * The size of a structure includes padding, which should be a power
     * of 2 (for the implementation below
     */
    tp->size = (slc + AL_STRUCT - 1) & ~(AL_STRUCT - 1);
    getsym();
}

void
do_static_assert()
{
    long            val;
    char           *msg;
    int             assert_line;

    /*
     * C11/C23: _Static_assert(const-expr) or _Static_assert(const-expr, "msg");
     * static_assert is the same keyword spelling in AC.
     */
    assert_line = lineno;
    getsym();
    needpunc(openpa);
    val = intexpr();
    msg = NULL;
    if (lastst == comma) {
        getsym();
        if (lastst != sconst) {
            error(ERR_SYNTAX, "static_assert message must be a string literal");
            while (lastst != closepa && lastst != semicolon && lastst != (int) eof
                   && !fatal)
                getsym();
        } else {
            msg = litlate(laststr);
            getsym();
        }
    }
    needpunc(closepa);
    needpunc(semicolon);
    if (val == 0L) {
        if (msg == NULL)
            msg = "static assertion failed";
        {
            int             saved_line;

            saved_line = lineno;
            lineno = assert_line;
            error(ERR_STATICASSERT, msg);
            lineno = saved_line;
        }
    }
}

void
dodecl(defclass)
    int             defclass;   /* enum e_sc - must be int (see prototype) */
{
    TYP            *tp;

    for (;;) {
        if (fatal)
            return;
        switch (lastst) {
        case kw_static_assert:
            do_static_assert();
            break;
        case kw_alignas:
            /* Prefix form: alignas(8) int x; - stay in loop for the type. */
            parse_alignas();
            /*
             * parse_alignas always consumes the alignas token.  If the
             * following tokens are not a valid alignas(...), lastst may
             * still be unusable; avoid spinning forever on kw_alignas.
             */
            if (lastst == kw_alignas)
                getsym();
            break;
        case asmconst:
            addauto(asmstmt());
            break;
        case kw_typedef:
            if (defclass == sc_global)
                declare(&gsyms, sc_global, lc_static, bt_struct, defclass);
            else if (defclass == sc_auto)
                declare(&lsyms, sc_auto, lc_auto, bt_struct, defclass);
            else
                declare(&lsyms, sc_auto, 0, bt_struct, defclass);
            break;
        case kw_register:
            getsym();
            if (defclass != sc_auto && defclass != sc_member)
                error(ERR_ILLCLASS, NULL);
            goto do_decl;
        case kw_auto:
            getsym();
            if (defclass != sc_auto && defclass != sc_member)
                error(ERR_ILLCLASS, NULL);
            goto do_decl;
        case kw_const:
        case kw_volatile:
            /*
             * Type qualifiers, not storage classes - leave the token for
             * decl() so QUAL_CONST / QUAL_VOLATILE are applied to the type.
             */
            goto do_decl;
        case id:
            if ((tp = istypedef(&lsyms)) != NULL)
                goto do_decl;
            if ((tp = istypedef(&gsyms)) != NULL)
                goto do_decl;
            tp = tp;
            if (defclass == sc_auto)
                return;

            /* else fall through to declare    */
        case kw_char:
        case kw_int:
        case kw_short:
        case kw_unsigned:
        case kw_signed:
        case kw_long:
        case kw_bool:
        case kw_int8:
        case kw_uint8:
        case kw_int16:
        case kw_uint16:
        case kw_int32:
        case kw_uint32:
        case kw_int64:
        case kw_uint64:
        case kw_intptr:
        case kw_uintptr:
        case kw_intmax:
        case kw_uintmax:
        case kw_restrict:
        case kw_inline:
        case kw_noreturn:
        case kw_asm:
        case kw_aligned:
        case kw_chip:
        case kw_far:
        case kw_near:
        case kw_fast:
        case kw_interrupt:
        case kw_regargs:
        case kw_stdargs:
        case kw_saveds:
        case kw_struct:
        case kw_union:
        case kw_enum:
        case kw_void:
        case kw_float:
        case kw_double:
    do_decl:    if (defclass == sc_global)
                lc_static +=
                    declare(&gsyms, sc_global, lc_static, bt_struct, defclass);
            else if (defclass == sc_auto)
                lc_auto +=
                    declare(&lsyms, sc_auto, lc_auto, bt_struct, defclass);
            else
                declare(&lsyms, sc_auto, 0, bt_struct, defclass);
            break;
        case kw_static:
            getsym();
            if (defclass == sc_member)
                error(ERR_ILLCLASS, NULL);
            if (defclass == sc_auto)
                lc_static +=
                    declare(&lsyms, sc_static, lc_static, bt_struct, defclass);
            else
                lc_static +=
                    declare(&gsyms, sc_static, lc_static, bt_struct, defclass);
            break;
        case kw_extern:
            getsym();
            if (defclass == sc_member)
                error(ERR_ILLCLASS, NULL);
            ++global_flag;
            declare(&gsyms, sc_external, 0, bt_struct, defclass);
            --global_flag;
            break;
        default:
            decl_alignas = 0;
            return;
        }
    }
}

void
compile(void)

/*
 * main compiler routine. this routine parses all of the declarations using
 * declare which will call funcbody as functions are encountered.
 */
{
    if (autohead != NULL) {
        genstmt(autohead);
        flush_peep();
        autohead = autotail = NULL;
    }

#ifdef  GENERATE_DBX
    dbx_init();
#endif

    while (lastst != eof) {
        if (fatal)
            return;
        dodecl(sc_global);
        if (lastst != eof)
            getsym();
    }

    if (autohead != NULL) {
        genstmt(autohead);
        flush_peep();
        autohead = autotail = NULL;
    }
    dumplits();
}

int
declproto(TABLE *table)  /* table is the argument table for the function */

/*
 * process declarations of the form:
 * 
 * ( <type> [<decl>] [,<type> [<decl>]] ... )
 * 
 */

{
    SYM            *sp, *sp2;
    int             nbytes;

    nbytes = 8;     /* return block */
    global_flag++;

    /*
     * ISO C: a parameter list that is exactly (void) means no parameters.
     * Do not invent a void-typed argument (broke main(void) / atexit decls).
     */
    if (lastst == kw_void) {
        getsym();
        if (lastst == closepa) {
            global_flag--;
            return nbytes;
        }
        /*
         * void was the parameter type (e.g. void *p).  Mirror decl(kw_void)
         * and continue into the normal parameter body once.
         */
        head = tail = maketype(bt_long, 4);
        declid = NULL;
        decl1();
        goto declproto_one;
    }

    for (;;) {
        declid = NULL;

        decl(&lsyms);   /* Set the base type of the variable */
        decl1();

        if (head == NULL) {
            error(ERR_SYNTAX, "unknown type");
            break;
        }

declproto_one:
        /*
         * C89: parameter "array of T" adjusts to "pointer to T"
         * (including sized arrays like int a[10]).
         */
        if (head->type == bt_pointer && head->val_flag != 0) {
            head->val_flag = 0;
            head->size = 4;
        }

        sp = (SYM *) xalloc(SZ_SYM);
        sp->name = declid;
        sp->storage_class = sc_auto;
        sp->storage_type = sc_proto;
        sp->next = NULL;

        sp->value.i = nbytes;
        sp->tp = head;

        /* Float params match 8-byte push; keep char/short declared type. */
        if (sp->tp->type == bt_float)
            sp->tp = maketype(bt_double, 8);

        /*
         * Advance with type_size(), not raw tp->size.  Poisoned size (high
         * word set / zero) made every parameter share offset 8 - ac-self2
         * then did getopt(argc,argc) and hung on argv[0].
         */
        {
            int psz;

            if (sp->tp->type == bt_pointer && sp->tp->val_flag == 0)
                psz = 4;
            else {
                psz = type_size(sp->tp);
                if (psz <= 0)
                    psz = 4;
            }
            /*
             * Stack slots match the caller (parmlist pushes 4 for
             * char/short).  Keep declared char/short type for &/sizeof.
             */
            if (psz == 1 || psz == 2)
                nbytes += 4;
            else
                nbytes += psz;
        }

        sp2 = copysym(sp);

        insert(sp, &lsyms); /* Insert into the local symbols */
        insert(sp2, table); /* Insert into the function list */

        if (lastst != comma)
            break;
        getsym();
    }
    if (lastst == ellipsis) {
        getsym();
        if (lastst != closepa)
            error(ERR_SYNTAX, NULL);
    }

    global_flag--;
    return nbytes;
}
