*********************************************************************
*
*    Since IEEE Single Precision is apparently not supported in
*    Workbench 1.3, the approach taken here is to convert all
*    single precision quantities to double precision and then use
*    the double precision libraries.
*
*********************************************************************

               xref    _MathIeeeDoubBasBase

               xref    .Fs2d
               xref    .Fd2s
               xref    .Fd2l
               xref    .Fl2d
               xref    .FDopen

bias           SET	-30

libref         MACRO
_LVO\1         EQU	bias
bias           SET	bias-6
               ENDM

*------------------------------------------------------------------
* Note: In order to generate the correct biases, these *Must* be in the
* same order as the mathieeedoubbas_lib.fd file.
*------------------------------------------------------------------

               libref	IEEEDPFix
               libref	IEEEDPFlt
               libref	IEEEDPCmp
               libref	IEEEDPTst
               libref	IEEEDPAbs
               libref	IEEEDPNeg
               libref	IEEEDPAdd
               libref	IEEEDPSub
               libref	IEEEDPMul
               libref	IEEEDPDiv
               libref	IEEEDPFloor	; needs release 1.2+
               libref	IEEEDPCeil	; needs release 1.2+

*********************************************************************
*
	       SECTION Single,CODE

*-------------------------------------------------------------------
* Do a one parameter MathIeeeDouble function with SP arguments
*-------------------------------------------------------------------
.FSdo1:
	tst.l	_MathIeeeDoubBasBase
	bne.s	1$
	jsr	.FDopen
1$:
	movem.l	A6,-(A7)
	move.l	_MathIeeeDoubBasBase,A6	
	add.l	4(A7),A6
        jsr     .Fs2d
	jsr	(A6)
        jsr     .Fd2s
	movem.l	(A7)+,A6
	addq	#4,A7		; unstack the function offset.
	rts

*-------------------------------------------------------------------
* Do a two parameter MathIeeeDouble function with SP arguments
*-------------------------------------------------------------------
.FSdo2
	tst.l	_MathIeeeDoubBasBase
	bne.s	1$
	jsr	.FDopen
1$:
	movem.l	D2-D4/A6,-(A7)
	move.l	_MathIeeeDoubBasBase,A6	
	add.l	16(A7),A6
        move.l  d0,d4
        move.l  d1,d0
        jsr     .Fs2d
        move.l  d0,d2
        move.l  d1,d3
        move.l  d4,d0
        jsr     .Fs2d
	jsr	(A6)
        jsr     .Fd2s
	movem.l	(A7)+,D2-D4/A6
	addq	#4,A7		; Unstack the function offset.
	rts

*********************************************************************
*
*	Single multiply arguments are in D0,D1
*
	       XDEF    .FSmul
.FSmul:
               pea	_LVOIEEEDPMul
               jmp     .FSdo2

*********************************************************************
*
*	Single divide arguments are in D0,D1
*
	       XDEF    .FSdiv
.FSdiv:
               pea	_LVOIEEEDPDiv
               jmp     .FSdo2

*********************************************************************
*
*	Single add arguments are in D0,D1
*
	       XDEF    .FSadd
.FSadd:
               pea	_LVOIEEEDPAdd
               jmp     .FSdo2

*********************************************************************
*
*	Single subtract arguments are in D0,D1
*
	       XDEF    .FSsub
.FSsub:
               pea	_LVOIEEEDPSub
               jmp     .FSdo2

*********************************************************************
*
*	Single negate argument is in D0
*
	       XDEF    .FSneg
.FSneg:
               pea	_LVOIEEEDPNeg
               jmp     .FSdo1

*********************************************************************
*
*	Single compare arguments are in D0,D1
*
	       XDEF    .FScmp
.FScmp:
               tst.l	_MathIeeeDoubBasBase
               bne.s	1$
               jsr	.FDopen
1$:
               movem.l	D2-D4/A6,-(A7)
               move.l  d0,d4
               move.l  d1,d0
               jsr     .Fs2d
               move.l  d0,d2
               move.l  d1,d3
               move.l  d4,d0
               jsr     .Fs2d
               move.l	_MathIeeeDoubBasBase,A6	
               jsr     _LVOIEEEDPCmp(A6)
               movem.l	(A7)+,D2-D4/A6
	       tst.l   D0
	       bne.s   Lcmps1
	       move.l  #0,D0
	       bra.s   Lcmps3
Lcmps1
	       bmi.s   Lcmps2
	       move.l  #1,D0
	       bra.s   Lcmps3
Lcmps2
	       move.l  #-1,D0
Lcmps3
	       rts		       ; return to caller

*********************************************************************
*
*	Convert long to single : Input in D0
*
	XDEF    .Fl2s
.Fl2s:
               jsr .Fl2d
               jmp .Fd2s

*********************************************************************
*
*	Convert single to long : Input in D0
*
	XDEF    .Fd2l
.Fs2l:
               jsr .Fs2d
               jmp .Fd2l

	       END
