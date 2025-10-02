
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
 * $Log:	C.h,v $
 * Revision 3.33  90/04/04  02:28:24  lionel
 * *** empty log message ***
 * 
 * Revision 3.32  90/02/03  16:22:59  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/* C.h
 * Compiler header file
 */

enum e_sym {
    and, asand, asdivide, aseor, aslshift, asmconst, asminus, asmodop,
    asor, asplus, asrshift, assign, astimes, autodec, autoinc, begin,
    cconst, closebr, closepa, colon, comma, compl, divide, dot, end,
    ellipsis, eq, geq, gt, hook, iconst, id, land, lconst, llconst, leq, lor,
    lshift, lt, lxor, minus, modop, neq, not, openbr, openpa, or, plus,
    pointsto, rconst, rshift, sconst, semicolon, star, uparrow,
    kw_auto, kw_break, kw_case, kw_char, kw_const, kw_continue, kw_default,
    kw_defined, kw_do, kw_double, kw_else, kw_enum, kw_extern, kw_float,
    kw_for, kw_goto, kw_if, kw_int, kw_long, kw_register, kw_return,
    kw_short, kw_signed, kw_sizeof, kw_static, kw_struct, kw_switch,
    kw_typedef, kw_union, kw_unsigned, kw_void, kw_volatile, kw_while,
    kw_bool,  /* C99 _Bool keyword */
    /* SAS/C keywords */
    kw_asm, kw_regargs, kw_stdargs, kw_saveds, kw_far, kw_near, kw_chip, 
    kw_fast, kw_interrupt, kw_aligned, eof
};

enum e_sc {
    sc_static, sc_auto, sc_global, sc_external, sc_type, sc_const,
    sc_member, sc_label, sc_ulabel, sc_parameter, sc_library, sc_proto,
    sc_define
};

enum e_bt {
    bt_char, bt_short, bt_long, bt_longlong, bt_float, bt_double, bt_pointer,
    bt_unsigned, bt_struct, bt_union, bt_enum, bt_func, bt_ifunc,
    bt_uchar, bt_ushort, bt_ulong, bt_ulonglong, bt_bool, bt_typedef, bt_void, bt_unknown
};

enum e_pm {
    pr_all, pr_if, pr_else, pr_asm
};

enum e_ps {
    ps_ignore, ps_do
};

struct slit {
    struct slit    *next;
    long        label;
    long        len;
    enum e_sym      type;
    char       *str;
};

struct sym {
    struct sym     *next;
    char       *name;
    int         key;
    enum e_sc       storage_class;
    enum e_sc       storage_type;
    union {
    long        i;
    unsigned long   u;
    double      f;
    char           *s;
    }           value;
    struct typ {
    enum e_bt   type;
    char        val_flag;   /* is it a value type */
    int     size;
    struct stab {
        struct sym     *head, *tail;
    }       lst;
    struct typ     *btp;
    char           *sname;
    char        qualifiers; /* type qualifiers: const=1, volatile=2 */
    char        mem_section; /* memory section: chip=1, far=2, near=3, fast=4 */
    }          *tp;
};

#define SYM struct sym
#define TYP struct typ
#define TABLE   struct stab

/* Type qualifier constants */
#define QUAL_CONST     1
#define QUAL_VOLATILE  2
#define QUAL_INTERRUPT 8

/* Memory section constants */
#define MEM_CHIP       1
#define MEM_FAR        2
#define MEM_NEAR       3
#define MEM_FAST       4

/* Type system function declarations */
extern TYP *integer_promote(TYP *tp, struct enode **node);
extern TYP *array_decay(TYP *tp, struct enode **node);
extern TYP *usual_arithmetic_conversions(TYP *tp1, TYP *tp2, struct enode **node1, struct enode **node2);

#define PDC_IEEESINGLE  1   /* Mask for ieeedoubbas library */
#define PDC_IEEEDOUBLE  2   /* Mask for ieeesingbas library */

#define MAX_ID      64
#define MAX_IDP1    65
#define MAX_STRLEN  1024
#define MAX_STLP1   1025
#define MAX_FUNCPARMS 99

#define ERR_SYNTAX  0
#define ERR_ILLCHAR 1
#define ERR_FPCON   2
#define ERR_ILLTYPE 3
#define ERR_UNDEFINED   4
#define ERR_DUPSYM  5
#define ERR_PUNCT   6
#define ERR_IDEXPECT    7
#define ERR_NOINIT  8
#define ERR_INCOMPLETE  9
#define ERR_ILLINIT 10
#define ERR_INITSIZE    11
#define ERR_ILLCLASS    12
#define ERR_BLOCK   13
#define ERR_NOPOINTER   14
#define ERR_NOFUNC  15
#define ERR_NOMEMBER    16
#define ERR_LVALUE  17
#define ERR_DEREF   18
#define ERR_MISMATCH    19
#define ERR_EXPREXPECT  20
#define ERR_WHILEXPECT  21
#define ERR_NOCASE  22
#define ERR_DUPCASE 23
#define ERR_LABEL   24
#define ERR_PREPROC 25
#define ERR_INCLFILE    26
#define ERR_CANTOPEN    27
#define ERR_DEFINE  28
#define ERR_PROTO   29

/* alignment sizes     */

#define AL_CHAR     1
#define AL_SHORT    2
#define AL_LONG     2
#define AL_POINTER  2
#define AL_FLOAT    2
#define AL_DOUBLE   2
#define AL_STRUCT   2

#define TRUE    1
#define FALSE   0

/* Modern error reporting functions */
extern void error_at_line(int n, char *msg, char *filename, int line, int column);
extern void warning_at_line(int n, char *msg, char *filename, int line, int column);
extern void warning(int n, char *msg);
extern int total_warnings;
extern int current_column;
