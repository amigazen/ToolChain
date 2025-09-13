
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
 * $Log:	Gen.h,v $
 * Revision 3.33  90/04/05  22:50:00  lionel
 * None.
 * 
 * Revision 3.32  90/02/03  16:23:53  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/* Gen.h
 * Code generation structures and constants
 */

#define F_DREG  1   /* data register direct mode allowed */
#define F_AREG  2   /* address register direct mode allowed */
#define F_MEM   4   /* memory alterable modes allowed */
#define F_IMMED 8   /* immediate mode allowed */
#define F_ALT   7   /* alterable modes */
#define F_DALT  5   /* data alterable modes */
#define F_ALL   15  /* all modes allowed */
#define F_VOL   16  /* need volitile operand */
#define F_NOVALUE 32    /* dont need result value */
#define F_FREG  64  /* Floating Point reg only */

enum e_op {
    op_abcd, op_add, op_adda, op_addi, op_addq, op_addx, op_and, op_andi,
    op_asl, op_asm, op_asr, op_bcc, op_bchg, op_bclr, op_bcs, op_beq,
    op_bge, op_bgt, op_bhi, op_ble, op_bls, op_blt, op_bmi, op_bne,
    op_bpl, op_bra, op_bset, op_bsr, op_btst, op_bvc, op_bvs, op_chk,
    op_clr, op_cmp, op_cmpa, op_cmpi, op_cmpm, op_comment, op_dbcc, op_dbcs,
    op_dbeq, op_dbf, op_dbge, op_dbgt, op_dbhi, op_dble, op_dbls, op_dbmi,
    op_dbne, op_dbpl, op_dbra, op_dbt, op_dbvc, op_dbvs, op_dc, op_divs,
    op_divu, op_eor, op_eori, op_exg, op_ext, op_fadd, op_fdiv, op_fmod,
    op_fmove, op_fmul, op_fneg, op_fsub, op_jmp, op_jsr, op_label, op_lea,
    op_link, op_lsl, op_lsr, op_mods, op_modu, op_move, op_movea, op_movem,
    op_movep, op_moveq, op_muls, op_mulu, op_nbcd, op_neg, op_negx, op_nop,
    op_not, op_or, op_ori, op_pea, op_reset, op_rol, op_ror, op_roxl,
    op_roxr, op_rte, op_rtr, op_rts, op_sbcd, op_scc, op_scs, op_seq,
    op_sf, op_sge, op_sgt, op_shi, op_sle, op_sls, op_slt, op_smi,
    op_sne, op_spl, op_st, op_stabn, op_stabs, op_stop, op_sub, op_suba,
    op_subi, op_subq, op_subx, op_svc, op_svs, op_swap, op_tas, op_trap,
    op_trapv, op_tst, op_unlk
};

enum e_am {
    am_dreg, am_areg, am_ind, am_ainc, am_adec, am_indx, am_indx2,
    am_xpc, am_direct, am_immed, am_mask, am_none, am_indx3, am_freg
};

struct libcall {
    struct libcall  *next;
    char            *basename;
    char            *funcname;
    char            *args;
    char            *offset;
};

/* addressing mode structure       */

struct amode {
    enum e_am       mode;
    enum e_am       preg;
    enum e_am       sreg;
    char        tempflag;
    char        signedflag;
    int         deep;   /* stack depth on allocation */
    struct enode   *offset;
};

/* output code structure   */

struct ocode {
    struct ocode   *fwd, *back;
    enum e_op       opcode;
    short       length;
    struct amode   *oper1, *oper2;
};
