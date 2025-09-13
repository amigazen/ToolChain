*------------------------------------------------------------------
* ieedoubtrans.a: PDC library interface to IEEE double floating point
* transient library base library supplied by Commodore.
* Loosely based on the Paul Petersen PDC v1.0 library source
* Maintenance Notes:
* 13Aug88 - Created by Jal
*------------------------------------------------------------------

AG_OpenLib	equ $00030000
AO_MathLib	equ $00008005
bias	SET	-30

libref	MACRO
_LVO\1	EQU	bias
bias	SET	bias-6
	ENDM

*------------------------------------------------------------------
* Note: these *Must* be in the same order as the
* mathieeedoubtrans_lib.fd file.
*------------------------------------------------------------------

	libref	IEEEDPAtan
	libref	IEEEDPSin
	libref	IEEEDPCos
	libref	IEEEDPTan
	libref	IEEEDPSincos
	libref	IEEEDPSinh
	libref	IEEEDPCosh
	libref	IEEEDPTanh
	libref	IEEEDPExp
	libref	IEEEDPLog
	libref	IEEEDPPow
	libref	IEEEDPSqrt
	libref	IEEEDPTieee
	libref	IEEEDPFieee
	libref	IEEEDPAsin
	libref	IEEEDPAcos
	libref	IEEEDPLog10

	XREF	_Alert
	XREF	_exit
	XREF	_OpenLibrary

	XDEF	_MathIeeeDoubTransBase

*------------------------------------------------------------------

	SECTION	2,DATA

_MathIeeeDoubTransBase dc.l 0
MATHName	dc.b    'mathieeedoubtrans.library',0

	SECTION	1,CODE

*------------------------------------------------------------------
*	Open the MathIeeeDoubTrans library
*------------------------------------------------------------------

.FDTopen:
	movem.l D0-D1/A0-A1,-(A7)	; save temporary registers
	pea	0			; any version will do....
	pea     MATHName		; Load the name of library
	jsr	_OpenLibrary		; Open the library
	addq	#8,A7
	move.l  D0,_MathIeeeDoubTransBase ; Save the return code
	bne.s   1$			; If non-zero then OK

	pea	0			; Parameter for Alert.
	pea	AG_OpenLib!AO_MathLib	; Signal an error
	jsr	_Alert
	addq	#8,A7
	pea     100			;     push return code
	jsr     _exit			;     and exit
1$:
	movem.l (A7)+,D0-D1/A0-A1	 ; restore temporaries
	rts

*------------------------------------------------------------------
*	Do the MathIeeeDoubTrans function
*------------------------------------------------------------------

.FDTdo:
	tst.l	_MathIeeeDoubTransBase
	bne.s	1$
	jsr	.FDTopen
1$:
	movem.l	8(A7),D0-D1
	movem.l	A6,-(A7)
	move.l	_MathIeeeDoubTransBase,A6	
	add.l	4(A7),A6
	jsr	(A6)
	movem.l	(A7)+,A6
	addq	#4,A7		; Chuck function offset.
	rts

*------------------------------------------------------------------
* atan:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_atan
_atan:
	pea	_LVOIEEEDPAtan
	jmp	.FDTdo

*------------------------------------------------------------------
* sin:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_sin
_sin:
	pea	_LVOIEEEDPSin
	jmp	.FDTdo

*------------------------------------------------------------------
* cos:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_cos
_cos:
	pea	_LVOIEEEDPCos
	jmp	.FDTdo

*------------------------------------------------------------------
* tan:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_tan
_tan:
	pea	_LVOIEEEDPTan
	jmp	.FDTdo

*------------------------------------------------------------------
* sinh:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_sinh
_sinh:
	pea	_LVOIEEEDPSinh
	jmp	.FDTdo

*------------------------------------------------------------------
* cosh:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_cosh
_cosh:
	pea	_LVOIEEEDPCosh
	jmp	.FDTdo

*------------------------------------------------------------------
* tanh:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_tanh
_tanh:
	pea	_LVOIEEEDPTanh
	jmp	.FDTdo

*------------------------------------------------------------------
* exp:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_exp
_exp:
	pea	_LVOIEEEDPExp
	jmp	.FDTdo

*------------------------------------------------------------------
* log:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_log
_log:
	pea	_LVOIEEEDPLog
	jmp	.FDTdo

*------------------------------------------------------------------
* pow:	pow(exp, power) : Output D0/D1
*------------------------------------------------------------------
	XDEF	_pow
_pow:
	tst.l	_MathIeeeDoubTransBase
	bne.s	1$
	jsr	.FDTopen
1$:
	movem.l	D2-D3/A6,-(A7)
	movem.l	16(A7),D0-D3
	move.l	_MathIeeeDoubTransBase,A6	
	jsr	_LVOIEEEDPPow(A6)
	movem.l	(A7)+,D2-D3/A6
	rts

*------------------------------------------------------------------
* sqrt:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_sqrt
_sqrt:
	pea	_LVOIEEEDPSqrt
	jmp	.FDTdo

*------------------------------------------------------------------
* asin:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_asin
_asin:
	pea	_LVOIEEEDPAsin
	jmp	.FDTdo

*------------------------------------------------------------------
* acos:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_acos
_acos:
	pea	_LVOIEEEDPAcos
	jmp	.FDTdo

*------------------------------------------------------------------
* log10:	Input double (on stack): Output D0/D1
*------------------------------------------------------------------
	XDEF	_log10
_log10:
	pea	_LVOIEEEDPLog10
	jmp	.FDTdo

	END
