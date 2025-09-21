	SECTION	2,DATA
nest_level
	DC.l	-1

	SECTION	1,CODE
;
; .entry
;
; Upon entry, D0 contains the number of bytes of stack space required to make
; the function call (return addr+local frame).  A0 is a pointer to the name of
; the function.  Scrathpad registers (D0-D1/A0-A1) may be modified, all others
; must be preserved upon return.
;
.entry:
	move.l	D3,-(A7)

;	Save name (A0) on the stack so we can use it later

	move.l	A0,-(A7)

;	Output nest_level number of spaces (ascii 32)

	move.l	nest_level,D3
	bmi	inc_static

	move.l	_output,-(A7)
	pea	32
loop1
	jsr	_fputc
	dbra	D3,loop1
	add.w	#8,A7

inc_static
	addq.l	#1,nest_level

;;;	    fprintf( stderr, "%s [\n", name (which is still on the stack !-);

	pea	enter_fmt
	move.l	_output,-(A7)
	jsr	_fprintf
	add.w	#12,A7		; we're also clearing off the name

	move.l	(A7)+,D3
	rts

;
; .exit
;
; Upon entry, D0 contains the return value of the routine and A0 is a pointer 
; the name of the function.  Upon return, D0 and the non-scratchpad registers 
; are preserved.
;
.exit:
	movem.l	D0/D3,-(A7)

;	Decrement nesting count

	subq.l	#1,nest_level

;	Output nest_level number of spaces (ascii 32)

	move.l	nest_level,D3
	bmi	close_bracket

	move.l	_output,-(A7)
	pea	32
loop2
	jsr	_fputc
	dbra	D3,loop2
	addq.w	#8,A7

close_bracket

;;;	    fprintf( stderr,  "] (0x08x)\n", D0 (which saved on the stack !-);
	
	pea	exit_str
	move.l	_output,-(A7)
	jsr	_fprintf

	addq.w	#8,A7
	movem.l	(A7)+,D0/D3  	; Very important to restore return value!
	rts

exit_str
	DC.b	93,32,40,48,120,37,48,56,120,41,10,0
enter_fmt
	DC.b	37,115,32,91,10,0,0

	XDEF	.entry
	XDEF	.exit
	XREF	_output
	XREF	_fputc
	XREF	_fprintf
	END

