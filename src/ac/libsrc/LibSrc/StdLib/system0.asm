*
* System0.asm - invoke system's command processor
*
* Copyright (C) by Ralph Babel, Guru Meditation Network,
* Falkenweg 3, D-6204 Taunusstein, West-Germany
*
* This piece of code may be used as part of any product as
* long as the source code for the complete program can be
* obtained free of charge (except for a small copying fee)
* and this copyright notice is left unchanged.
*
* Bug reports and improved versions appreciated.
*
* 20-Jul-1987 created original version
* 11-Jan-1988 now based on "official" information only
* 9-Apr-1990  modified heavily by Lionel Hummel for PDC release 3.3
*


        include "exec/types.i"
        include "libraries/dos.i"
        include "libraries/dosextens.i"


*
* LONG System0(name, seglist, args)
* char *name;
* BPTR seglist;
* char *args;
*

        xdef    _System0
        xref    _DOSBase

AbsExecBase equ 4

        xref    _LVOFindTask
        xref    _LVOAllocMem
        xref    _LVOSetSignal
        xref    _LVOFreeMem

        xref    _ReportSL

* parameter offsets & stack

SAVED_REGS      reg     a2-a6/d2-d7

DELTA           equ     11*4

ARG_NAME        equ     4+DELTA
ARG_SEGLIST     equ     8+DELTA
ARG_ARGS        equ     12+DELTA

* additional return codes

NO_CLI  equ     -1
NO_MEM  equ     -2

* local constants

MAXBSTR equ     255
LF      equ     10

* register usage

REG_Result      equr    d3
REG_Process     equr    a2      ;may not be A4, see below!
REG_CLI         equr    a3
REG_CIS         equr    a4      ;may not be A3, see below!
REG_PrevStack   equr    a5
REG_SysBase     equr    a6

* local stack frame

 STRUCTURE      StackFrame,0
        STRUCT  sf_CommandName,MAXBSTR+1        ;BSTR, length byte!

        APTR    sf_PrevStack

        APTR    sf_SaveReturnAddr
        BPTR    sf_SaveModule
        BPTR    sf_SaveCommandName

        APTR    sf_StackBase
        LONG    sf_StackSize
        LONG    sf_PushSize

        APTR    sf_Process
        APTR    sf_CLI
        APTR    sf_CIS

        BPTR    sf_SCB_Buf
        LONG    sf_SCB_Pos
        LONG    sf_SCB_End

        LABEL   sf_SIZEOF

* entry point

_System0:
        movem.l SAVED_REGS,-(sp)

        moveq   #NO_CLI,REG_Result      ;ERROR - not a CLI task

        movea.l AbsExecBase,REG_SysBase
        suba.l  a1,a1
        jsr     _LVOFindTask(REG_SysBase)
        movea.l d0,REG_Process
        move.l  pr_CLI(REG_Process),d0
        beq     quit0

* build local stack frame & save some values

        lsl.l   #2,d0   ;BPTR to machine pointer
        movea.l d0,REG_CLI

        movea.l sp,REG_PrevStack        ;save old stack pointer
        move.l  sp,d0
        andi.b  #$fc,d0 ;make SP longword-aligned for BPTRs
        movea.l d0,sp
        suba.w  #sf_SIZEOF,sp

        move.l  REG_PrevStack,sf_PrevStack(sp)
        move.l  REG_Process,sf_Process(sp)
        move.l  REG_CLI,sf_CLI(sp)

        move.l  pr_ReturnAddr(REG_Process),sf_SaveReturnAddr(sp)

* allocate space for stack

        moveq   #NO_MEM,REG_Result      ;ERROR - no memory for STACK

        move.l  cli_DefaultStack(REG_CLI),d0    ;in longwords for "VEC"
        lsl.l   #2,d0
        move.l  d0,sf_PushSize(sp)
        addq.l  #4,d0   ;one additional longword
        move.l  d0,sf_StackSize(sp)
        moveq   #0,d1   ;intentionally NOT "MEMF_PUBLIC"!
        jsr     _LVOAllocMem(REG_SysBase)
        tst.l   d0
        beq     quit1

        move.l  d0,sf_StackBase(sp)     ;save result

* save old command pointer, build new BCPL command name

        move.l  cli_CommandName(REG_CLI),sf_SaveCommandName(sp)

        movea.l ARG_NAME(REG_PrevStack),a0      ;first parameter to "System0()"
        lea     sf_CommandName+1(sp),a1 ;first char location for BSTR
        move.l  a1,d0
        lsr.l   #2,d0   ;will ignore (+1), BPTR as result
        move.l  d0,cli_CommandName(REG_CLI)

        move.w  #MAXBSTR,d0
        bra.s   L2

L1      move.b  d1,(a1)+
L2      move.b  (a0)+,d1
        dbeq    d0,L1

        suba.l  ARG_NAME(REG_PrevStack),a0      ;subtract original source
        move.l  a0,d0
        subq.l  #1,d0   ;terminating null-byte
        move.b  d0,sf_CommandName(sp)   ;to first location in BSTR

* save contents of Current Input Stream

        move.l  pr_CIS(REG_Process),d0
        lsl.l   #2,d0
        movea.l d0,REG_CIS      ;contains APTR to CIS
        move.l  REG_CIS,sf_CIS(sp)

        move.l  fh_Buf(REG_CIS),sf_SCB_Buf(sp)
        move.l  fh_Pos(REG_CIS),sf_SCB_Pos(sp)
        move.l  fh_End(REG_CIS),sf_SCB_End(sp)

* setup start/end indices in Stream Control Block

        move.l  ARG_ARGS(REG_PrevStack),a0   ; LW aligned by system()
        move.l  a0,d0
        lsr.l   #2,d0   ;make buffer pointer a BPTR
        move.l  d0,fh_Buf(REG_CIS)

        move.l  #0,d0
        bra.s   L4

L3      addq.l  #1,d0
L4      move.b  (a0)+,d1
        bne     L3
        move.l  d0,fh_End(REG_CIS)

        clr.l   fh_Pos(REG_CIS)

* misc setup

        clr.l   pr_Result2(REG_Process) ;clear secondary result

        moveq   #0,d0
        move.l  #SIGBREAKF_CTRL_C,d1
        jsr     _LVOSetSignal(REG_SysBase)

* handle seglist and start address

        move.l  cli_Module(REG_CLI),sf_SaveModule(sp)

        move.l  ARG_SEGLIST(REG_PrevStack),d0   ;second argument to "System0()"
        move.l  d0,cli_Module(REG_CLI)
        lsl.l   #2,d0
        movea.l d0,a3   ;make it a machine pointer in A3

* setup processor registers & C-interface

        move.l  ARG_ARGS(REG_PrevStack),a0
        move.l  fh_End(REG_CIS),d0

* setup processor registers, BCPL-interface, stack & return address for "Exit()"

        movea.l sf_StackBase(sp),a1     ;BCPL stack, low end
        move.l  sf_PushSize(sp),d2
        lea     4(a1,d2.l),a4   ;must not destroy REG_Process!
        move.l  sp,-(a4)        ;previous stack frame
        move.l  d2,-(a4)        ;stack size in bytes
        move.l  a4,pr_ReturnAddr(REG_Process)
        movea.l a4,sp

        movea.l _DOSBase,a4     ;large data memory model!
        movem.l dl_A2(a4),a2/a5/a6

* now call the command at its entry point

        jsr     4(a3)   ;code starts one longword behind segment pointer

        move.l  d0,REG_Result   ;save return code

* get old stackframe & reload old register contents

        movea.l 4(sp),sp        ;old stack frame

        movea.l sf_Process(sp),a0
        move.l  sf_SaveReturnAddr(sp),pr_ReturnAddr(a0)

        movea.l sf_CLI(sp),a0
        move.l  sf_SaveCommandName(sp),cli_CommandName(a0)
        move.l  sf_SaveModule(sp),cli_Module(a0)

* restore original contents of Current Input Stream

        movea.l sf_CIS(sp),a0
        move.l  sf_SCB_Buf(sp),fh_Buf(a0)
        move.l  sf_SCB_Pos(sp),fh_Pos(a0)
        move.l  sf_SCB_End(sp),fh_End(a0)

* free temporary stack

        movea.l AbsExecBase,REG_SysBase
        movea.l sf_StackBase(sp),a1
        move.l  sf_StackSize(sp),d0
        jsr     _LVOFreeMem(REG_SysBase)

quit1   movea.l sf_PrevStack(sp),sp     ;UNLINK local variables

quit0   move.l  REG_Result,d0

        movem.l (sp)+,SAVED_REGS
        rts

        end
