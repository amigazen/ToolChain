
	       INCLUDE "exec/types.i"
	       INCLUDE "exec/alerts.i"
	       INCLUDE "exec/nodes.i"
	       INCLUDE "exec/libraries.i"

*********************************************************************

xlib	       macro
	       xref    _LVO\1
	       endm

callsys        macro
	       CALLLIB _LVO\1
	       endm

*********************************************************************

	       xlib    IEEESPMul
	       xlib    IEEESPDiv
	       xlib    IEEESPAdd
	       xlib    IEEESPSub
	       xlib    IEEESPNeg
	       xlib    IEEESPTst
	       xlib    IEEESPCmp
	       xlib    IEEESPFlt
	       xlib    IEEESPInt
	       xlib    OpenLibrary
	       xlib    Alert

	       xref    _exit
	       xref    _MathIeeeSingBasBase

               xref    .Fs2d
               xref    .Fd2s
               xref    .Fd2l
               xref    .Fl2d

	       SECTION 2,DATA

_MathIeeeSingBasBase dc.l 0
MATHName       dc.b    'mathieeesingbas.library',0

*********************************************************************
*
	       SECTION 1,CODE

*********************************************************************
*
*	Make sure the MathIeeeSingBasBase is loaded correctly
*
	       XDEF    .FSopen
.FSopen:
	       movem.l D0-D1/A0-A1,-(A7)         ; save registers
	       move.l  _MathIeeeSingBasBase,D0	 ; Retrieve the library base
	       bne     Lopen1			 ;
						 ;
	       lea     MATHName,A1		 ;   Load the name of library
	       moveq   #0,d0			 ;   Any version will do...
	       move.l  4,A6			 ;   Get exec base
	       callsys OpenLibrary		 ;   Open the library
	       move.l  D0,_MathIeeeSingBasBase	 ;   Save the return code
	       bne     Lopen1			 ;   If non-zero then OK

	       ALERT   (AG_OpenLib!AO_MathLib)   ;     Signal an error
	       pea     100			 ;     push return code
	       jsr     _exit			 ;     and exit
	       add.w   #4,A7			 ;
Lopen1:
	       move.l  D0,A6			 ; Set A6 to be library base
	       movem.l (A7)+,D0-D1/A0-A1         ; restore temporaries
	       rts

*********************************************************************
*
*	Single multiply arguments are in D0,D1
*
	       XDEF    .FSmul
.FSmul:
	       movem.l A6,-(A7)
	       jsr     .FSopen
	       callsys IEEESPMul
	       movem.l (A7)+,A6
	       rts		       ; return to caller

*********************************************************************
*
*	Single divide arguments are in D0,D1
*
	       XDEF    .FSdiv
.FSdiv:
	       movem.l A6,-(A7)
	       jsr     .FSopen
	       callsys IEEESPDiv
	       movem.l (A7)+,A6
	       rts		       ; return to caller

*********************************************************************
*
*	Single add arguments are in D0,D1
*
	       XDEF    .FSadd
.FSadd:
	       movem.l A6,-(A7)
	       jsr     .FSopen
	       callsys IEEESPAdd
	       movem.l (A7)+,A6
	       rts		       ; return to caller

*********************************************************************
*
*	Single subtract arguments are in D0,D1
*
	       XDEF    .FSsub
.FSsub:
	       movem.l A6,-(A7)
	       jsr     .FSopen
	       callsys IEEESPSub
	       movem.l (A7)+,A6
	       rts		       ; return to caller

*********************************************************************
*
*	Single negate argument is in D0
*
	       XDEF    .FSneg
.FSneg:
	       movem.l A6,-(A7)
	       jsr     .FSopen
	       callsys IEEESPNeg
	       movem.l (A7)+,A6
	       rts		       ; return to caller

*********************************************************************
*
*	Single compare arguments are in D0,D1
*
	       XDEF    .FScmp
.FScmp:
	       movem.l A6,-(A7)
	       jsr     .FSopen
	       callsys IEEESPCmp
	       movem.l (A7)+,A6
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
