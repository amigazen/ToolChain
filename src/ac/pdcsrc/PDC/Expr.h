
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
 * $Log:	Expr.h,v $
 * Revision 3.33  90/04/05  22:48:43  lionel
 * Added real unsigned binary and assignment expressions for *, /, and %.
 * 
 * Revision 3.32  90/02/03  16:23:40  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/* Expr.h
 * Expression tree descriptions
 */

enum e_node {
    en_void,            /* used for parameter lists */
    en_info,            /* info about parameters    */
    en_comment,         /* source code comments     */
    en_asm,             /* Inline-assembler         */
    en_stabs, en_stabn, /* source level debugging   */
    en_cbw, en_cbl, en_cwl, en_clf, en_cld, en_cfd, en_cdf, en_cfl, en_cdl,
    en_icon, en_fcon, en_labcon, en_nacon, en_autocon, en_intrlab,
    en_b_ref, en_w_ref, en_l_ref, en_ub_ref, en_uw_ref,
    en_ul_ref, en_m_ref, en_f_ref, en_d_ref, en_fcall, en_tempref, 
    en_add, en_uadd,
    en_sub, en_usub,
    en_mul, en_umul,
    en_mod, en_umod,
    en_div, en_udiv,
    en_asadd, en_asuadd,
    en_assub, en_asusub,
    en_asmul, en_asumul,
    en_asdiv, en_asudiv,
    en_asmod, en_asumod,
    en_lsh, en_rsh, en_cond,
    en_assign, en_eq, en_ne, 
    en_asrsh, en_aslsh, en_asand, en_asor,
    en_aseor, en_uminus, en_not, en_compl, en_lt, en_le, en_gt,
    en_ge, en_and, en_or, en_land, en_lor, en_xor, en_ainc,
    en_adec, en_ugt,
    en_fadecd, en_faincd, 
    en_fnegd, en_faddd, en_fsubd, en_fmuld, en_fdivd, en_fmodd,
    en_fadecs, en_faincs, 
    en_fnegs, en_fadds, en_fsubs, en_fmuls, en_fdivs, en_fmods,
    en_uge, en_ule, en_ult
};

/* statement node descriptions     */

enum e_stmt {
    st_expr, st_while, st_for, st_do, st_if, st_switch,
    st_case, st_goto, st_break, st_continue, st_label,
    st_return, st_comment, st_stabn, st_stabs, st_asm
};


struct dnode {
    struct dnode   *next;   /* Next dnode in the chain      */
    char           *sp;     /* Up to 2048 bytes of info     */
    int             label;  /* Line label                   */
    int             tag;    /* Type of STAB statement       */
    int             nest;   /* Nesting level                */
    struct enode   *ref;    /* Reference to label           */
};

struct enode {
    enum e_node     nodetype;
    short           size;
    short           constflag;
    short           signedflag;
    union {
        long            i;
        double          f;
        char           *sp;
        struct dnode   *dp;
        struct enode   *p[2];
    }               v;
};

struct snode {
    enum e_stmt     stype;
    struct snode   *next;   /* next statement           */
    struct enode   *exp;    /* condition or expression  */
    struct snode   *s1, *s2;/* internal statements      */
    int             line;   /* Line number for dbx      */
    long           *label;  /* label number for goto    */
    long            value;  /* Extra value field        */
};

struct cse {
    struct cse     *next;
    struct enode   *exp;    /* optimizable expression */
    int             uses;   /* number of uses */
    int             duses;  /* number of dereferenced uses */
    int             voidf;  /* cannot optimize flag */
    int             reg;    /* allocated register */
};
