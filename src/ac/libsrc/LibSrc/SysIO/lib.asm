
               SECTION 1,CODE

*****************************************************************************
* How to to long multiplication with a sixteen bit multiply:
* - Handle special cases
*   - Guaranteed < 32 bits (two shorts)
*   - Guaranteed overflow (both > 16 bits)
* - Perform as if with sixteen bit "digits":
*                      u1         l1                    H(X) = High word of X
*                    x u2         l2                     L(X) = Low word of X
*              ---------------------
*              L(l2 * u1) L(l2 * l1)
*              H(l2 * l1)                       (carry the 2^16's place)
*              L(u2 * l1)
*   H(u2 * l1)                                  (carry the 2^32's place)
*   --------------------------------
*   If the third column is greater than zero, or the total of the second column
*   is greater than 2^16, then there is an overflow condition.
*****************************************************************************
*                                                                           *
*       .lmuls  long signed multiply                                        *
*                                                                           *
*       multiplies two long operands on the stack and returns the           *
*       result on the stack, along with the second operand.                 *
*                                                                           *
*****************************************************************************
               XDEF    .lmuls
.lmuls:
               link    A6,#0           ; Link the current frame
               movem.l D0-D4,-(A7)     ; Save the registers
               tst.w   8(A6)           ; Test the high word for zero
               bne     Lmul1           ;       do a full multiply
               tst.w   12(A6)          ; Test the high word for zero
               bne     Lmul1           ;       do a full multiply
               move.w  10(A6),D0       ; Move the first word
               mulu    14(A6),D0       ; Do a half multiply
               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0-D4     ; Restore the registers
               unlk    A6
               rts                     ; Return answer in D0
Lmul1:
               move.l  #1,D4           ; Load the sign of the answer
               move.l  8(A6),D2        ; Load the first argument into D2
               bge     Lmul2
               neg.l   D2              ; Make first argument positive
               neg.l   D4              ; Adjust the sign of the result
Lmul2:
               move.l  12(A6),D3       ; Load the second argument into D3
               bge     Lmul3
               neg.l   D3              ; Make the second argument positive
               neg.l   D4              ; Adjust the sign of the result
Lmul3:
               clr.l   D0              ; Clear high word of D0
               move.w  D2,D0           ; Move low word of first argument
               mulu    D3,D0           ; Multiply by low word of second arg
               move.w  D2,D1           ; Save low word of first arg
               swap    D2              ; Exchange low and high word of arg #1
               mulu    D3,D2           ; Multiply by low word of arg #2
               swap    D3              ; Exchange low and high word of arg #2
               mulu    D3,D1           ; Multiply high word of #2 by low #1
               add.l   D2,D1           ; Add the partial results
               swap    D1              ; Swap the low and high of partial ans.
               clr.w   D1              ; Clear the unneeded low word
               add.l   D1,D0           ; Add the partial results to get ans.
               tst.l   D4              ; Test if the result should be negative
               bge     Lmul4
               neg.l   D0              ; Change result
Lmul4:
               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0-D4     ; Restore the registers
               unlk    A6
               rts

******************************************************************************
*                                                                            *
*       .lmulu   long unsigned multiply (LMU)                                *
*                                                                            *
*       multiplies two long operands on the stack and returns the            *
*       result on the stack, along with the second operand.                  *
*                                                                            *
******************************************************************************
               XDEF    .lmulu
.lmulu:
               link    A6,#0           ; Link the current frame
               movem.l D0-D3,-(A7)     ; Save the registers
               tst.w   8(A6)           ; See if we have overflow or simple case
               beq     LMUTstSecond    ;       
               tst.w   12(A6)          ; Possible overflow
               bne     LMUOverflow     ;       Uhoh; go deal with it
LMUTough:                              ; A mixed bag (tougher case)
               move.l  8(A6),D2        ; Cache u1.l1 in D2
               move.l  12(A6),D3       ; Cache u2.l2 in D3
               clr.l   D0              ; Prepare to collect result in D0
               move.w  D2,D0           ; Multiply l1 (in D2) and l2 (in D3)
               mulu    D3,D0           ; 
               move.w  D2,D1           ; l1 -> D1 and u1 -> D2
               swap    D2              ; 
               mulu    D3,D2           ; (u1 * l2) -> D2
               swap    D3              ; u2 -> D3
               mulu    D3,D1           ; (u2 * l1) -> D1
               add.l   D2,D1           ; Add the partial results
               swap    D1              ; Swap into high word where it belongs
               tst.w   D1              ; Test the low word, H(u2 * l1)
               bne     LMUOverflow
               add.l   D1,D0           ; Add the partial results to get ans.
               bcc     LMUDone
LMUOverflow:
               move.l  #$FFFFFFFF,D0   ; An alternate response should be to
               bra     LMUDone         ; throw up a SIGFPE.
LMUTstSecond:
               tst.w   12(A6)          ; Check for simple case
               bne     LMUTough
               move.w  10(A6),D0       ; Simple case!
               mulu    14(A6),D0       ; Do a half multiply and get outta town
LMUDone:
               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0-D3     ; Restore the registers
               unlk    A6
               rts                     ; Also return answer in D0

******************************************************************************
*                                                                            *
*       .ldivs   long signed divide                                          *
*                                                                            *
*       divides two long operands on the stack and returns the               *
*       result on the stack, along with the second operand.                  *
*                                                                            *
******************************************************************************

               XDEF    .ldivs
.ldivs:
               link    A6,#0           ; Link the current frame
               movem.l D0,-(A7)        ; Save the registers

               tst.w   12(A6)          ; Test the high word of arg #2
               bne.s   Ldiv1           ; If not zero do a full divide
               move.l  8(A6),D0        ; Test the high word of arg #1
               bmi.s   Ldiv1           ; If negative do full divide
               divu    14(A6),D0       ; Perform the division
               bvs.s   Ldiv1           ; If overflow then do full divide
               and.l   #65535,D0       ; Mask the results into 16 bits

               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0        ; Restore the registers
               unlk    A6
               rts                     ; Return to the caller
Ldiv1:
               movem.l D1-D5,-(A7)     ; Save the aux registers
               move.l  #1,D5           ; Set the sign of the result
               move.l  8(A6),D0        ; Load arg #1
               bge.s   Ldiv2           ; If negative, then
               neg.l   D0              ;       adjust the sign of arg #1
               neg.l   D5              ;       and the result
Ldiv2:
               move.l  D0,D3           ; Save arg #1
               move.l  12(A6),D1       ; Load arg #2
               bge.s   Ldiv3           ; If negative, then
               neg.l   D1              ;       adjust the sign of arg #2
               neg.l   D5              ;       and the result
Ldiv3:
               move.l  D1,D4           ; Save arg #2
               cmp.l   #65536,D1       ; If the divisor is bigger than a word
               bge.s   Ldiv4           ;       do the big division algorithm
               clr.w   D0              ; Clear the low word of arg #1
               swap    D0              ; Move high word into low word
               divu    D1,D0           ; Get partial results
               move.w  D0,D2           ; and Save into reg D2
               move.w  D3,D0           ; Move low word of arg #1 into D0
               divu    D1,D0           ; Get more partial results
               swap    D0              ; Exchange low and high words of result
               move.w  D2,D0           ; Move low word of partial into D0
               swap    D0              ; Exchange low and high words
               bra.s   Ldiv6
Ldiv4:
               lsr.l   #1,D0           ; Divide denumerator
               lsr.l   #1,D1           ; and numerator by 2
               cmp.l   #65536,D1       ; If still too big then
               bge.s   Ldiv4           ; repeat
               divu    D1,D0           ; Perform the division
               and.l   #65535,D0       ; Mask the results to 16 bits
               move.l  D0,D2           ; Save the preliminary result
               move.l  D0,-(A7)        ; Test the result by multiplying
               move.l  D4,-(A7)        ;       The quoient by the divisor
               bsr     .lmuls          ;       Call the multiply routine
               move.l  (A7)+,D0        ;       Retrieve the answer
               add.w   #4,A7           ;       Adjust the stack pointer
               cmp.l   D0,D3           ; If our guess is too hight
               bge.s   Ldiv5           ;       Then
               subq.l  #1,D2           ;               adjust our guess down
Ldiv5:
               move.l  D2,D0           ; Move the results into D0
Ldiv6:
               tst.l   D5              ; Test the sign of the result
               bge.s   Ldiv7           ; If negative
               neg.l   D0              ;       then adjust the result
Ldiv7:
               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D1-D5     ; Restore the aux registers
               movem.l (A7)+,D0        ; Restore the registers
               unlk    A6
               rts                     ; Return to the caller

******************************************************************************
*                                                                            *
*       .ldivu   long unsigned divide (LDU)                                  *
*                                                                            *
*       divides two long operands on the stack and returns the               *
*       result on the stack, along with the second operand.                  *
*                                                                            *
******************************************************************************

               XDEF    .ldivu
.ldivu:
               link    A6,#0           ; Link the current frame
               movem.l D0,-(A7)        ; Save the registers

               move.l  8(A6),D0        ; Get the dividend
               tst.w   12(A6)          ; Test the high word of divisor
               bne.s   LDULookCloser   ; If not zero, then maybe a full divide
               divu    14(A6),D0       ; Else it could be real easy
               bvs.s   LDU32           ; If overflow then do full 32-bits
               and.l   #65535,D0       ; Mask the results into 16 bits
               move.l  D0,8(A6)        ; Replace the answer on the stack
LDUExit:
               movem.l (A7)+,D0        ; Restore the registers
               unlk    A6
               rts                     ; Return to the caller
LDU32:
               movem.l D1/D2,-(A7)
               move.w  14(A6),D1       ; We'll need the denominator a bit more
               moveq.l #0,D2
               swap    D0
               move.w  D0,D2
               divu    D1,D2
               move.w  D2,8(A6)        ; The MSW of the answer
               swap    D2
               move.w  D2,D0
               swap    D0
               divu    D1,D0
               move.w  D0,10(A6)       ; The LSW of the answer
               movem.l (A7)+,D1/D2
               bra     LDUExit
LDULookCloser:
               movem.l D1-D5,-(A7)     ; Save the aux registers
               move.l  12(A6),D1       ; Load arg #2 (divisor) into D1
               cmp.l   D1,D0           ; Screen other simple cases
               bgt     LDULong
               beq     LDUOne
               move.l  #0,D0
               bra     LDUDone
LDUOne         move.l  #1,D0
               bra     LDUDone
LDULong:
               move.l  D0,D5           ; Save the original numerator
               move.l  D1,D3           ; Save the original denominator
LDULoop:
               lsr.l   #1,D0           ; Divide numerator
               lsr.l   #1,D1           ; and denomenator by 2
               cmp.l   #65536,D1       ; If still too big then
               bge.s   LDULoop         ; repeat
               divu    D1,D0           ; Perform the division
               and.l   #$FFFF,D0       ; Mask out the remainder
               move.l  D0,D2           ; Save the preliminary result
LDUTest:
	       move.l  D3,-(A7)        ; Test the result by multiplying
               move.l  D2,-(A7)
               bsr     .lmulu          ;       Call the multiply routine
	       
	     
               move.l  (A7)+,D0        ;       Retrieve the answer
               addq.w  #4,A7
               cmp.l   D5,D0
               beq.s   LDUGood
               blt.s   LDUChkRem
               subq.l  #1,D2
               bra.s   LDUTest         ; Retry test (is this necessary?)
LDUChkRem:
               move.l  D5,D4
               sub.l   D0,D4
               cmp.l   D3,D4           ; If low by the denominator or more
               ble.s   LDUGood         ;       Then
               addq.l  #1,D2           ;               adjust our guess up
               bra.s   LDUTest
LDUGood:
               move.l  D2,D0           ; Move the results into D0
LDUDone:
               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D1-D5     ; Restore the aux registers
               bra     LDUExit

******************************************************************************
*
*       .lmods  long signed modulus (remainder)
*
*       Compute the remainder of the two long operands on the stack and
*               returns the result on the stack, along with the second operand.
*
******************************************************************************

                        XDEF    .lmods
.lmods:
               link    A6,#0           ; Link the current frame
               movem.l D0-D4,-(A7)     ; Save the registers

               tst.w   12(A6)          ; Test the high word of arg #2
               bne.s   Lmod1           ; If non-zero do a full remainder
               move.l  8(A6),D0        ; Test the first argument
               bmi.s   Lmod1           ; If negative then do a full remainder
               divu    14(A6),D0       ; Perform the modulus
               bvs.s   Lmod1           ; If overflow then do a full remainder
               clr.w   D0              ; Clear the quotient part
               swap    D0              ; Swap the remainder into the low word

               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0-D4     ; Restore the registers
               unlk    A6
               rts                     ; Return to the caller
Lmod1:
               move.l  #1,D4           ; Set the sign of the result
               move.l  8(A6),D0        ; Get the first argument
               bge.s   Lmod2           ; If negative, then
               neg.l   D0              ;       adjust the sign of the argument
               neg.l   D4              ;       and of the result
Lmod2:
               move.l  D0,D2           ; Save the first argument
               move.l  12(A6),D1       ; Get the second argument
               bge.s   Lmod3           ; If negative, then
               neg.l   D1              ;       adjust the sign of the argument
Lmod3:
               cmp.l   #65536,D1       ; If the moduli is to large, then
               bge.s   Lmod4           ;       use the complete algorithm
               clr.w   D0              ; Clear the low word of arg #1
               swap    D0              ; Exchange low and high words
               divu    D1,D0           ; Do division to get partial result
               move.w  D2,D0           ; Replace low word of arg #1 with arg #2
               divu    D1,D0           ; Do division to get partial result
               clr.w   D0              ; Clear the low word
               swap    D0              ; Exchange low and high words
               bra.s   Lmod7           ; Exit from this routine
Lmod4:
               move.l  D1,D3           ; Save the second argument
Lmod5:
               lsr.l   #1,D0           ; Divide both the dividend and the
               lsr.l   #1,D1           ; Quotient by 2 until
               cmp.l   #65536,D1       ; They both fit in bounds
               bge.s   Lmod5
               divu    D1,D0           ; Perform the division
               and.l   #65535,D0       ; Mask the results to 16 bits

               move.l  D0,-(A7)        ; Multiply the quoient
               move.l  D3,-(A7)        ;       By the divisor
               bsr     .lmuls          ; Do the multiply
               move.l  (A7)+,D0        ; Retreive the results
               add.w   #4,A7           ; And adjust the stack

               cmp.l   D0,D2           ; If the product is greater
               bcc.s   Lmod6           ; Than what we started with, then
               sub.l   D3,D0           ;       subract off the divisor
Lmod6:
               sub.l   D2,D0           ; Get the remainder since we subtracted
               neg.l   D0              ; backwords, we will just negate the answer
Lmod7:
               tst.l   D4              ; Check the sign of the result
               bge.s   Lmod8           ; If it should be negative, then
               neg.l   D0              ;       adjust our answer
Lmod8:
               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0-D4     ; Restore the registers
               unlk    A6
               rts                     ; Return to the caller

******************************************************************************
*
*       .lmodu  long unsigned modulus (LMU)
*
*       Compute the remainder of the two long operands on the stack and
*               returns the result on the stack, along with the second operand.
*
******************************************************************************

                        XDEF    .lmodu
.lmodu:
               link    A6,#0           ; Link the current frame
               movem.l D0-D3,-(A7)     ; Save the registers

               move.l  8(A6),D0        ; Get the base
               tst.w   12(A6)          ; Test the HIGH word of the modulus
               bne.s   LMUfull         ; If non-zero do a full remainder
               divu    14(A6),D0       ; Perform the modulus
               bvs.s   LMUfull         ; If overflow then do a full remainder
               clr.w   D0              ; Clear the quotient part
               swap    D0              ; Swap the remainder into the low word

               move.l  D0,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0-D3     ; Restore the registers
               unlk    A6
               rts                     ; Return to the caller
LMUfull:
               move.l  12(A6),D3       ; Get the ENTIRE modulus
               move.l  D0,D2           ; Put the base in a safe register
               move.l  D3,-(A7)
               move.l  D0,-(A7)
               bsr     .ldivu          ; Leave result on stack and call ...
               move.l  D3,-(A7)
               bsr     .lmulu
               move.l  (A7)+,D1
               addq.w  #8,A7           ; Restore also from previous funcall
               sub.l   D1,D2           ; Return base - ((base/modulo)*modulo)
               move.l  D2,8(A6)        ; Replace the answer on the stack
               movem.l (A7)+,D0-D3     ; Restore the registers
               unlk    A6
               rts                     ; Return to the caller

*
*       .cswitch - execute c switch statement
*
*       the switch table is encoded as follows:
*
*               long    label1,case1
*               long    label2,case2
*               long    label3,case3
*               ... for all cases
*               long    0,defaultcase
*
*       the case variable is passed in D0
*
               XDEF    .cswitch

.cswitch:
               move.l  (A7)+,A0        ;get table address
L_sw1:
               move.l  (A0)+,A1        ;get a label
               move.l  A1,D1           ;test it for default
               beq.s   L_sw2           ;jump if default case
               cmp.l   (A0)+,D0        ;see if this case
               bne.s   L_sw1           ;next case if not
               jmp     (A1)            ;jump to case
L_sw2:
               move.l  (A0),A0         ;get default address
               jmp     (A0)            ;jump to default case

               END
