*------------------------------------------------------------------
* double.asm: PDC library interface to IEEE double floating point
* library supplied by Commodore.
* Loosely based on the Paul Petersen PDC v1.0 library source
* Maintenance Notes:
* 13Aug88 - Created by Jal
* 18Jan89 - .Fd2s and .Fs2d added (LDH)
*------------------------------------------------------------------

AG_OpenLib	equ $00030000
AO_MathLib	equ $00008005
bias	SET	-30

libref	MACRO
_LVO\1	EQU	bias
bias	SET	bias-6
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
               libref	IEEEDPFloor	; needs release 1.2 or beyond
               libref	IEEEDPCeil	; needs release 1.2 or beyond

               XDEF	_MathIeeeDoubBasBase
               XDEF     .FDopen

               XREF	_Alert
               XREF	_exit
               XREF	_OpenLibrary

*------------------------------------------------------------------

	SECTION	DoubleData,DATA

_MathIeeeDoubBasBase dc.l 0
MATHName	dc.b    'mathieeedoubbas.library',0

	SECTION	Double,CODE

*
*	Make sure the MathIeeeDoubBasBase is loaded correctly
*

.FDopen:
	movem.l D0-D1/A0-A1,-(A7)	; save temporary registers
	pea	0			; any version will do....
	pea     MATHName		; Load the name of library
	jsr	_OpenLibrary		; Open the library
	addq	#8,A7
	move.l  D0,_MathIeeeDoubBasBase	; Save the return code
	bne.s   1$			; If non-zero then OK

	pea	0			; Parameter for Alert.
	pea	AG_OpenLib!AO_MathLib	; Signal an error.
	jsr	_Alert
	addq	#8,A7
	pea     100			;     push return code
	jsr     _exit			;     and exit
1$:
	movem.l (A7)+,D0-D1/A0-A1	 ; restore temporaries
	rts

*-------------------------------------------------------------------
* Do a one parameter MathIeeeDouble function.
*-------------------------------------------------------------------
.FDdo1:
	tst.l	_MathIeeeDoubBasBase
	bne.s	1$
	jsr	.FDopen
1$:
	movem.l	A6,-(A7)
	move.l	_MathIeeeDoubBasBase,A6	
	add.l	4(A7),A6
	jsr	(A6)
	movem.l	(A7)+,A6
	addq	#4,A7		; unstack the function offset.
	rts

*-------------------------------------------------------------------
* Do a two parameter MathIeeeDouble function.
*-------------------------------------------------------------------
.FDdo2
	tst.l	_MathIeeeDoubBasBase
	bne.s	1$
	jsr	.FDopen
1$:
	movem.l	D2-D3/A6,-(A7)
	movem.l (A0),D2-D3
	move.l	_MathIeeeDoubBasBase,A6	
	add.l	12(A7),A6
	jsr	(A6)
	movem.l	(A7)+,D2-D3/A6
	addq	#4,A7		; Unstack the function offset.
	rts

*********************************************************************
*
*	Double multiply arguments are in D0,D1 & (A0)
*
	XDEF    .FDmul
.FDmul:
	pea	_LVOIEEEDPMul
	jmp     .FDdo2

*********************************************************************
*
*	Double divide arguments are in D0,D1 & (A0)
*
	XDEF    .FDdiv
.FDdiv:
	pea	_LVOIEEEDPDiv
	jmp     .FDdo2

*********************************************************************
*
*	Double add arguments are in D0,D1 & (A0)
*
	XDEF    .FDadd
.FDadd:
	pea	_LVOIEEEDPAdd
	jmp     .FDdo2

*********************************************************************
*
*	Double subtract arguments are in D0,D1 & (A0)
*
	XDEF    .FDsub
.FDsub:
	pea	_LVOIEEEDPSub
	jmp     .FDdo2

*********************************************************************
*
*	Double negate argument is in D0,D1
*
	XDEF    .FDneg
.FDneg:
	pea	_LVOIEEEDPNeg
	jmp     .FDdo1

*********************************************************************
*
*	Double compare arguments are in D0,D1 and (A0)
*
	XDEF    .FDcmp
.FDcmp:
	pea	_LVOIEEEDPCmp
	jmp     .FDdo2

*********************************************************************
*
*	Convert long to double : Input in D0
*
	XDEF    .Fl2d
.Fl2d:
	pea	_LVOIEEEDPFlt
	jmp     .FDdo1

*********************************************************************
*
*	Convert double to long : Input in D0,D1
*
	XDEF    .Fd2l
.Fd2l:
	pea	_LVOIEEEDPFix
	jmp     .FDdo1
*********************************************************************
*
*       Convert double to single : Input in D0:D1 : Output in D0
*
               XDEF    .Fd2s
.Fd2s
               move.l	d2,-(sp)
DoMant	                               ; Chop mantissa down to size (23 bits)
               rol.l	#3,d1          ; Chopping ain't nice, but it's an
               andi.l	#7,d1          ; expedient.
               move.l	d0,d2
               andi.l	#$FFFFF,d2
               lsl.l	#3,d2
               or.l	d2,d1
DoExp
               move.l	d0,d2          ; Shrink exp from 11 to 8 bits
               swap	d2             ; and excess-1023 to excess-127
               lsr.l	#4,d2
               andi.l	#$07ff,d2
               cmpi.w	#1151,d2       ; 1023+128 = 1151 (Max exponent)
               ble.s	ExpNotHi
               move.w	#255,d2        ; Overflow ... result effectively inf.
               bra	ExpOK
ExpNotHi
               cmpi.w	#896,d2        ; 1023-127 = 896 (Min exponent)
               bge.s	ExpNotLo
               moveq.l	#0,d1          ; Underflow ... result effectively 0
               bra	done
ExpNotLo
               sub.w	#896,d2        ; Adjust exponent's bias
ExpOK
               swap	d2             ; Put exponent in its place
               lsl.l	#7,d2
Combine
               or.l	d2,d1          ; Merge exponent and mantissa
done
               andi.l	#$80000000,d0  ; Merge with sign
               or.l	d1,d0
               move.l	(sp)+,d2       ; Shave and a haircut .....
               rts                     ; two bits!

*********************************************************************
*
*       Convert single to double : Input in D0 : Output in D0:D1
*
               XDEF    .Fs2d
.Fs2d:
               movem.l	d2-d3,-(sp)
               move.l	d0,d1
               ror.l	#3,d1
               move.l	d1,d2
               and.l	#$FFFFF,d2     ; Nab the upper mantissa in d2
               and.l	#$E0000000,d1  ; Isolate the lower in d1
ExpandExp
               move.l	d0,d3
               swap	d3
               lsr.l	#7,d3
               and.l	#$FF,d3
               add.l	#896,d3        ; Now expand to 11 bits (excess-1023)
               lsl.l	#4,d3
               swap	d3
               and.l	#$80000000,d0  ; Save the sign, toss the rest
               or.l	d3,d0          ; Merge the new exponent w/sign
               or.l	d2,d0          ; Likewise the mantissa
               movem.l	(sp)+,d2-d3
               rts                    ; return to caller

*********************************************************************
*
*	Implement the abs function.: Input in D0/D1 : Output in D0:D1
*
	XDEF    _abs
_abs:
	movem.l 4(A7),D0-D1
	pea	_LVOIEEEDPAbs
	jmp	.FDdo1

*********************************************************************
*
*	Implement the floor function.: Input in D0/D1 : Output in D0:D1
*
	XDEF    _floor
_floor:
	movem.l 4(A7),D0-D1
	pea	_LVOIEEEDPFloor
	jmp	.FDdo1

*********************************************************************
*
*	Implement the ceil function.: Input in D0/D1 : Output in D0:D1
*
	XDEF    _ceil
_ceil:
	movem.l 4(A7),D0-D1
	pea	_LVOIEEEDPCeil
	jmp	.FDdo1

	END
