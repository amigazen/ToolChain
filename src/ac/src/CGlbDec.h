
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
 * $Log:	CGlbDec.h,v $
 * Revision 3.33  90/04/04  02:29:06  lionel
 * *** empty log message ***
 * 
 * Revision 3.32  90/02/03  16:23:03  lionel
 * None
 * 
 *------------------------------------------------------------------
 */

/* Cglbdec.h
 * Header file containing global declarations for the entire program
 */

extern FILE    *input, *list, *ac_output;
#define output ac_output


extern struct OptTab {
    int         Optimize;
    int         List;
    int         Quiet;
    int         Annote;
    int         Frame;
    int         Debug;
    int         MulDiv32;
    int         Builtin;
    int         PreComp;
    int         Stack;
    int         CompileOnly;
    int         PreprocessOnly;
    int         OutputFormat;  /* 0=GCC, 1=SASC, 2=Classic */
    int         WarningsAsErrors;
    int         ShowColumn;
}       Options;

/*
 * Byte offsets into Options.  ac-self collapses Options.member to (Options+0);
 * Cmain must use these so gen-2 still sets Quiet/List/PreComp/CompileOnly correctly.
 */
#define OPT_OFF_Optimize          0
#define OPT_OFF_List              4
#define OPT_OFF_Quiet             8
#define OPT_OFF_Annote           12
#define OPT_OFF_Frame            16
#define OPT_OFF_Debug            20
#define OPT_OFF_MulDiv32         24
#define OPT_OFF_Builtin          28
#define OPT_OFF_PreComp          32
#define OPT_OFF_Stack            36
#define OPT_OFF_CompileOnly      40
#define OPT_OFF_PreprocessOnly   44
#define OPT_OFF_OutputFormat     48
#define OPT_OFF_WarningsAsErrors 52
#define OPT_OFF_ShowColumn       56
#define OPT_REF(off) (*(int *) ((char *) &Options + (off)))

extern void default_options(void);

extern long ival;
extern double   rval;
extern int  lineno;
extern int  nextlabel;
extern int  lastch;
extern char lastid[MAX_IDP1];
extern char laststr[MAX_STLP1];
extern char    *curfile;
extern int lastst;   /* token type; int not enum — SAS/C emits 16-bit enum (DC.w) */
extern struct slit *strtab;

extern TABLE    gsyms, lsyms;
extern SYM     *lastfunc;
extern SYM     *lasthead;

extern int  lc_static;
extern int  lc_base;
extern int  lc_auto;
extern int  lc2_auto;
extern int  nest_level;
extern int  maxparmsize;

extern char data_name[64];
extern char code_name[64];
extern char bss_name[64];

extern int  float_auto;

extern struct snode *bodyptr;   /* parse tree for function */
extern struct snode *autohead;
extern struct snode *autotail;

extern struct libcall  *libpragma;
extern struct flibcall *flibpragma;
extern struct syscall  *syspragma;
extern struct tagcall  *tagpragma;
extern struct msgcall  *msgpragma;
 
extern int      global_flag;
extern TABLE    defsyms;
extern TABLE    cmd_defsyms;
extern TABLE    cmd_undefsyms;
extern TABLE    cmd_include;
extern TABLE    cmd_local;

extern int      save_mask;  /* register save mask */
extern int      PdcFlags;

/* Lexer input line reader (not POSIX getline from stdio.h). */
extern int      ac_getline(int listflag);

/* Default -I paths for Amiga self-host (compinc/ before include/). */
extern void     install_bootstrap_includes(void);
