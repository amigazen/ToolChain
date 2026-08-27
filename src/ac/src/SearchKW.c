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
 * $Log:	SearchKW.c,v $
 * Revision 3.33  90/04/05  23:14:07  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:25:43  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/*
 * SearchKW.c
 * 
 * Pattern matches C-language keywords and generates tokens.  The lex'er.
 *
 * stype is int (not enum e_sym) so the static table matches Builtins.c's
 * aggregate-init path, which ac-self already compiles.  A runtime kw_add
 * Table is kept strcmp-sorted so searchkw needs no runtime sort.
 */

#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        "C.h"
#include        "Expr.h"
#include        "Gen.h"
#include        "Cglbdec.h"

static int      kwblk_len = 0;

struct kwblk {
    char           *word;
    int             stype;  /* enum e_sym values */
};

static struct kwblk keywords[] = {
    {"_Alignas", kw_alignas},
    {"_Alignof", kw_alignof},
    {"_Bool", kw_bool},
    {"_Noreturn", kw_noreturn},
    {"_Static_assert", kw_static_assert},
    {"__a0", kw_a0},
    {"__a1", kw_a1},
    {"__a2", kw_a2},
    {"__a3", kw_a3},
    {"__a4", kw_a4},
    {"__a5", kw_a5},
    {"__a6", kw_a6},
    {"__aligned", kw_aligned},
    {"__asm", kw_asm},
    {"__chip", kw_chip},
    {"__d0", kw_d0},
    {"__d1", kw_d1},
    {"__d2", kw_d2},
    {"__d3", kw_d3},
    {"__d4", kw_d4},
    {"__d5", kw_d5},
    {"__d6", kw_d6},
    {"__d7", kw_d7},
    {"__far", kw_far},
    {"__fast", kw_fast},
    {"__interrupt", kw_interrupt},
    {"__near", kw_near},
    {"__regargs", kw_regargs},
    {"__saveds", kw_saveds},
    {"__stdargs", kw_stdargs},
    {"alignas", kw_alignas},
    {"alignof", kw_alignof},
    {"and", land},
    {"and_eq", asand},
    {"auto", kw_auto},
    {"bitand", sym_and},
    {"bitor", sym_or},
    {"bool", kw_bool},
    {"break", kw_break},
    {"case", kw_case},
    {"char", kw_char},
    {"compl", sym_compl},
    {"const", kw_const},
    {"continue", kw_continue},
    {"default", kw_default},
    {"defined", kw_defined},
    {"do", kw_do},
    {"double", kw_double},
    {"else", kw_else},
    {"enum", kw_enum},
    {"extern", kw_extern},
    {"false", kw_false},
    {"float", kw_float},
    {"for", kw_for},
    {"goto", kw_goto},
    {"if", kw_if},
    {"inline", kw_inline},
    {"int", kw_int},
    {"int16_t", kw_int16},
    {"int32_t", kw_int32},
    {"int64_t", kw_int64},
    {"int8_t", kw_int8},
    {"intmax_t", kw_intmax},
    {"intptr_t", kw_intptr},
    {"long", kw_long},
    {"not", sym_not},
    {"not_eq", neq},
    {"nullptr", kw_nullptr},
    {"or", lor},
    {"or_eq", asor},
    {"register", kw_register},
    {"restrict", kw_restrict},
    {"return", kw_return},
    {"short", kw_short},
    {"signed", kw_signed},
    {"sizeof", kw_sizeof},
    {"static", kw_static},
    {"static_assert", kw_static_assert},
    {"struct", kw_struct},
    {"switch", kw_switch},
    {"true", kw_true},
    {"typedef", kw_typedef},
    {"uint16_t", kw_uint16},
    {"uint32_t", kw_uint32},
    {"uint64_t", kw_uint64},
    {"uint8_t", kw_uint8},
    {"uintmax_t", kw_uintmax},
    {"uintptr_t", kw_uintptr},
    {"union", kw_union},
    {"unsigned", kw_unsigned},
    {"void", kw_void},
    {"volatile", kw_volatile},
    {"while", kw_while},
    {"xor", lxor},
    {"xor_eq", aseor},
    {(char *) 0, 0}
};

enum e_sym
searchkw()
{
    struct kwblk   *kwbp;
    int             low, high, mid, compare;

    if (kwblk_len == 0) {
        /* keywords[] is kept strcmp-sorted; just count entries once. */
        kwbp = keywords;
        while (kwbp->word != NULL) {
            ++kwblk_len;
            ++kwbp;
        }
    }

    low = 0;
    high = kwblk_len - 1;

    do {
        mid = low + safe_ldiv(high - low, 2);
        kwbp = &keywords[mid];

        compare = strcmp(lastid, kwbp->word);

        if (compare == 0)
            return (lastst = (enum e_sym) kwbp->stype);
        else {
            if (compare > 0)
                low = mid + 1;
            else
                high = mid - 1;
        }
    } while (low <= high);

    return (lastst);
}
