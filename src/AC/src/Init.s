	SECTION	2,DATA
_head:
	DC.l	0
_tail:
	DC.l	0
_declid:
	DC.l	0
_tagtable:
	DC.l	0,0
_stdconst:
	DC.w	2
	DC.b	1,0
	DC.l	4,0,0
	DC.l	0,L1
	DC.b	1
	DS.b	1
	SECTION	1,CODE
_imax:
L2	EQU	0
	link	A5,#L2
	movem.l	D3-D4,-(A7)
	move.l	12(A5),D3
	move.l	8(A5),D4
	cmp.l	D3,D4
	ble	L4
	bra	L5
L4
	move.l	D3,D4
L5
	move.l	D4,D0
L6
	movem.l	(A7)+,D3-D4
	unlk	A5
	rts
_litlate:
L7	EQU	-4
	link	A5,#L7
	movem.l	D3-D4,-(A7)
	move.l	8(A5),D4
	move.l	D4,-(A7)
	jsr	_strlen
	addq.w	#4,A7
	addq.l	#1,D0
	move.l	D0,-(A7)
	jsr	_xalloc
	addq.w	#4,A7
	move.l	D0,D3
	move.l	D4,-(A7)
	move.l	D3,-(A7)
	jsr	_strcpy
	addq.w	#8,A7
	move.l	D3,D0
L9
	movem.l	(A7)+,D3-D4
	unlk	A5
	rts
_copysym:
L10	EQU	-8
	link	A5,#L10
	movem.l	D3-D4,-(A7)
	moveq	#28,D0
	move.l	D0,D4
	addq.l	#1,_global_flag
	move.l	D4,-(A7)
	jsr	_xalloc
	addq.w	#4,A7
	move.l	D0,D3
	subq.l	#1,_global_flag
	tst.l	D3
	beq	L12
	move.l	D3,A0
	move.l	8(A5),A1
	pea	(A1)
	lea	(A0),A1
	move.l	(A7)+,A0
	moveq	#7,D0
L14
	move.l	(A0)+,(A1)+
	dbra	D0,L14
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,(A0)
L12
	move.l	D3,D0
L15
	movem.l	(A7)+,D3-D4
	unlk	A5
	rts
_maketype:
L16	EQU	-4
	link	A5,#L16
	movem.l	D3,-(A7)
	pea	26
	jsr	_xalloc
	addq.w	#4,A7
	move.l	D0,D3
	move.l	D3,A0
	clr.b	2(A0)
	move.l	D3,A0
	move.l	10(A5),4(A0)
	move.l	8(A5),D0
	move.l	D3,A0
	move.w	D0,(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,20(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,8(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,12(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,16(A0)
	move.l	D3,A0
	clr.b	24(A0)
	move.l	D3,A0
	clr.b	25(A0)
	move.l	D3,D0
L18
	movem.l	(A7)+,D3
	unlk	A5
	rts
_istypedef:
L19	EQU	-4
	link	A5,#L19
	movem.l	D3-D4/A2,-(A7)
	move.l	8(A5),D4
	lea	_search,A2
	move.w	_lastst,D0
	cmp.w	#31,D0
	bne	L21
	tst.l	D4
	beq	L23
	move.l	D4,A0
	move.l	(A0),-(A7)
	pea	_lastid
	jsr	(A2)
	addq.w	#8,A7
	move.l	D0,D3
	beq	L25
	move.l	D3,A0
	move.l	24(A0),D0
	beq	L25
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#18,D0
	bne	L25
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	16(A0),D0
L27
	movem.l	(A7)+,D3-D4/A2
	unlk	A5
	rts
L25
L23
	move.l	_lsyms,-(A7)
	pea	_lastid
	jsr	(A2)
	addq.w	#8,A7
	move.l	D0,D3
	beq	L28
	move.l	D3,A0
	move.l	24(A0),D0
	beq	L28
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#18,D0
	bne	L28
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	16(A0),D0
	bra	L27
L28
	move.l	_gsyms,-(A7)
	pea	_lastid
	jsr	(A2)
	addq.w	#8,A7
	move.l	D0,D3
	beq	L30
	move.l	D3,A0
	move.l	24(A0),D0
	beq	L30
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#18,D0
	bne	L30
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	16(A0),D0
	bra	L27
L30
L21
	moveq	#0,D0
	bra	L27
_is_class_error:
L36	EQU	0
	link	A5,#L36
	movem.l	A2-A4,-(A7)
	lea	_getsym,A2
	lea	_error,A3
	lea	_lastst,A4
	move.w	(A4),D0
	cmp.w	#55,D0
	bne	L38
	pea	L32
	clr.l	-(A7)
	jsr	(A3)
	addq.w	#8,A7
	jsr	(A2)
	bra	L39
L38
	move.w	(A4),D0
	cmp.w	#59,D0
	bne	L40
	pea	L33
	clr.l	-(A7)
	jsr	(A3)
	addq.w	#8,A7
	jsr	(A2)
	bra	L41
L40
	move.w	(A4),D0
	cmp.w	#79,D0
	bne	L42
	pea	L34
	clr.l	-(A7)
	jsr	(A3)
	addq.w	#8,A7
	jsr	(A2)
	bra	L43
L42
	move.w	(A4),D0
	cmp.w	#74,D0
	bne	L44
	pea	L35
	clr.l	-(A7)
	jsr	(A3)
	addq.w	#8,A7
	jsr	(A2)
L44
L43
L41
L39
	moveq	#0,D0
L46
	movem.l	(A7)+,A2-A4
	unlk	A5
	rts
_decl:
L48	EQU	-4
	link	A5,#L48
	movem.l	D3-D7/A2-A4,-(A7)
	lea	_head,A2
	lea	_getsym,A3
	move.l	8(A5),D3
	lea	_tail,A4
	lea	_maketype,A0
	move.l	A0,D4
	lea	_is_class_error,A0
	move.l	A0,D5
	lea	_decl,A0
	move.l	A0,D6
	lea	_lastst,A0
	move.l	A0,D7
	move.w	(A0),D0
	ext.l	D0
	cmp.l	#31,D0
	blt	L50
	sub.l	#31,D0
	cmp.l	#66,D0
	bgt	L50
	asl.l	#2,D0
	lea	L51,A0
	move.l	0(A0,D0.l),A1
	jmp	(A1)
L51
	DC.l	L72
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L53
	DC.l	L50
	DC.l	L50
	DC.l	L65
	DC.l	L55
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L74
	DC.l	L50
	DC.l	L75
	DC.l	L50
	DC.l	L73
	DC.l	L50
	DC.l	L50
	DC.l	L50
	DC.l	L68
	DC.l	L67
	DC.l	L54
	DC.l	L50
	DC.l	L66
	DC.l	L70
	DC.l	L50
	DC.l	L50
	DC.l	L76
	DC.l	L50
	DC.l	L52
	DC.l	L77
	DC.l	L71
	DC.l	L68
	DC.l	L56
	DC.l	L50
	DC.l	L69
	DC.l	L50
	DC.l	L62
	DC.l	L63
	DC.l	L64
	DC.l	L58
	DC.l	L59
	DC.l	L57
	DC.l	L60
	DC.l	L61
	DC.l	0
L52
	jsr	(A3)
	pea	L47
	move.l	_stdout,-(A7)
	jsr	_fprintf
	addq.w	#8,A7
	bra	L50
L53
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	bra	L50
L54
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	bra	L50
L55
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L78
	move.l	(A2),A0
	or.b	#1,24(A0)
L78
	bra	L50
L56
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L80
	move.l	(A2),A0
	or.b	#2,24(A0)
L80
	bra	L50
L57
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L82
	move.l	(A2),A0
	move.b	#1,25(A0)
L82
	bra	L50
L58
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L84
	move.l	(A2),A0
	move.b	#2,25(A0)
L84
	bra	L50
L59
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L86
	move.l	(A2),A0
	move.b	#3,25(A0)
L86
	bra	L50
L60
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L88
	move.l	(A2),A0
	move.b	#4,25(A0)
L88
	bra	L50
L61
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L90
	move.l	(A2),A0
	or.b	#8,24(A0)
L90
	bra	L50
L62
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L92
	move.l	(A2),A0
	or.b	#16,24(A0)
L92
	bra	L50
L63
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L94
	move.l	(A2),A0
	or.b	#32,24(A0)
L94
	bra	L50
L64
	jsr	(A3)
	move.l	D3,-(A7)
	move.l	D6,A0
	jsr	(A0)
	addq.w	#4,A7
	move.l	(A2),D0
	beq	L96
	move.l	(A2),A0
	or.b	#64,24(A0)
L96
	bra	L50
L65
	pea	1
	clr.l	-(A7)
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L66
	pea	2
	pea	1
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D7,A0
	move.w	(A0),D0
	cmp.w	#72,D0
	bne	L98
	jsr	(A3)
L98
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L67
	pea	4
	pea	2
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D7,A0
	move.w	(A0),D0
	cmp.w	#72,D0
	bne	L100
	jsr	(A3)
	bra	L101
L100
	move.l	D7,A0
	move.w	(A0),D0
	cmp.w	#73,D0
	bne	L102
	pea	8
	pea	3
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
L102
L101
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L68
	pea	4
	pea	2
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L69
	pea	1
	pea	17
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L70
	jsr	(A3)
	move.l	D7,A0
	move.w	(A0),D0
	ext.l	D0
	move.l	D0,D0
	jsr	.cswitch
	DC.l	L105,58
	DC.l	L106,76
	DC.l	L107,72
	DC.l	L107,73
	DC.l	0,L104
L105
	pea	1
	clr.l	-(A7)
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	bra	L104
L106
	pea	2
	pea	1
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	bra	L104
L107
	pea	4
	pea	2
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
L104
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L71
	jsr	(A3)
	move.l	D7,A0
	move.w	(A0),D0
	ext.l	D0
	move.l	D0,D0
	jsr	.cswitch
	DC.l	L109,58
	DC.l	L110,76
	DC.l	L111,72
	DC.l	L111,73
	DC.l	0,L112
L109
	pea	1
	pea	13
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	bra	L108
L110
	pea	2
	pea	14
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	bra	L108
L111
	pea	4
	pea	7
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D7,A0
	move.w	(A0),D0
	cmp.w	#73,D0
	bne	L113
	pea	8
	pea	16
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
L113
	bra	L108
L112
	pea	4
	pea	7
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
L108
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L72
	move.l	D3,-(A7)
	jsr	_istypedef
	addq.w	#4,A7
	move.l	D0,-4(A5)
	move.l	-4(A5),D0
	beq	L115
	move.l	-4(A5),(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	bra	L116
L115
	pea	4
	pea	2
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
L116
	bra	L50
L73
	pea	4
	pea	4
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L74
	pea	8
	pea	5
	move.l	D4,A0
	jsr	(A0)
	addq.w	#8,A7
	move.l	D0,(A4)
	move.l	(A4),(A2)
	jsr	(A3)
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L75
	jsr	(A3)
	move.l	D3,-(A7)
	jsr	_declenum
	addq.w	#4,A7
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L76
	jsr	(A3)
	pea	8
	jsr	_declstruct
	addq.w	#4,A7
	move.l	D5,A0
	jsr	(A0)
	bra	L50
L77
	jsr	(A3)
	pea	9
	jsr	_declstruct
	addq.w	#4,A7
	move.l	D5,A0
	jsr	(A0)
L50
L117
	movem.l	(A7)+,D3-D7/A2-A4
	unlk	A5
	rts
_decl1:
L118	EQU	-16
	link	A5,#L118
	movem.l	D3-D7/A2-A4,-(A7)
	lea	_head,A2
	lea	_tail,A3
	lea	_decl2,A4
	lea	_getsym,A0
	move.l	A0,D7
	move.w	_lastst,D0
	ext.l	D0
	move.l	D0,D0
	jsr	.cswitch
	DC.l	L121,31
	DC.l	L122,53
	DC.l	L123,45
	DC.l	0,L124
L121
	pea	_lastid
	jsr	_litlate
	addq.w	#4,A7
	move.l	D0,_declid
	move.l	D7,A0
	jsr	(A0)
	jsr	(A4)
	bra	L120
L122
	pea	4
	pea	6
	jsr	_maketype
	addq.w	#8,A7
	move.l	D0,D4
	move.l	D4,A0
	move.l	(A2),16(A0)
	move.l	D4,(A2)
	move.l	(A3),D0
	bne	L125
	move.l	(A2),(A3)
L125
	move.l	D7,A0
	jsr	(A0)
	jsr	_decl1
	bra	L120
L123
	move.l	D7,A0
	jsr	(A0)
	move.l	(A2),D4
	move.l	(A3),D6
	moveq	#0,D0
	move.l	D0,(A3)
	move.l	(A3),(A2)
	jsr	_decl1
	pea	18
	jsr	_needpunc
	addq.w	#4,A7
	move.l	(A2),D5
	move.l	(A3),D3
	move.l	D4,(A2)
	move.l	D6,(A3)
	jsr	(A4)
	tst.l	D3
	beq	L127
	move.l	D3,A0
	move.l	(A2),16(A0)
	move.l	D3,A0
	move.w	(A0),D0
	cmp.w	#6,D0
	bne	L129
	move.l	D3,A0
	move.b	2(A0),D0
	beq	L129
	move.l	(A2),D0
	beq	L129
	move.l	D3,A0
	move.l	(A2),A1
	move.l	4(A0),-(A7)
	move.l	4(A1),-(A7)
	jsr	.lmuls
	move.l	(A7)+,4(A0)
	addq.w	#4,A7
L129
L127
	move.l	D5,(A2)
	bra	L120
L124
	jsr	(A4)
L120
L131
	movem.l	(A7)+,D3-D7/A2-A4
	unlk	A5
	rts
_decl2:
L132	EQU	-4
	link	A5,#L132
	movem.l	D3/A2-A4,-(A7)
	lea	_head,A2
	lea	_getsym,A3
	lea	_lastst,A4
	move.w	(A4),D0
	ext.l	D0
	move.l	D0,D0
	jsr	.cswitch
	DC.l	L135,44
	DC.l	L136,45
	DC.l	0,L134
L135
	jsr	(A3)
	pea	4
	pea	6
	jsr	_maketype
	addq.w	#8,A7
	move.l	D0,D3
	move.l	D3,A0
	move.b	#1,2(A0)
	move.w	(A4),D0
	cmp.w	#17,D0
	bne	L137
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,4(A0)
	move.l	D3,A0
	move.b	#1,2(A0)
	jsr	(A3)
	bra	L138
L137
	jsr	_intexpr
	move.l	D3,A0
	move.l	D0,4(A0)
	pea	17
	jsr	_needpunc
	addq.w	#4,A7
L138
	jsr	_decl2
	move.l	(A2),D0
	beq	L139
	move.l	D3,A0
	move.l	(A2),A1
	move.l	4(A0),-(A7)
	move.l	4(A1),-(A7)
	jsr	.lmuls
	move.l	(A7)+,4(A0)
	addq.w	#4,A7
L139
	move.l	D3,A0
	move.l	(A2),16(A0)
	move.l	D3,(A2)
	move.l	_tail,D0
	bne	L141
	move.l	(A2),_tail
L141
	bra	L134
L136
	jsr	(A3)
	clr.l	-(A7)
	pea	11
	jsr	_maketype
	addq.w	#8,A7
	move.l	D0,D3
	move.l	D3,A0
	move.b	#1,2(A0)
	move.l	D3,A0
	move.l	(A2),16(A0)
	move.l	D3,(A2)
	move.w	(A4),D0
	cmp.w	#18,D0
	bne	L143
	jsr	(A3)
	move.w	(A4),D0
	cmp.w	#15,D0
	bne	L145
	moveq	#12,D0
	move.l	D3,A0
	move.w	D0,(A0)
L145
	bra	L144
L143
	moveq	#12,D0
	move.l	D3,A0
	move.w	D0,(A0)
L144
L134
L147
	movem.l	(A7)+,D3/A2-A4
	unlk	A5
	rts
_alignment:
L149	EQU	0
	link	A5,#L149
	movem.l	D3,-(A7)
	move.l	8(A5),D3
	bne	L151
	pea	L148
	move.l	_stdout,-(A7)
	jsr	_fprintf
	addq.w	#8,A7
	moveq	#1,D0
L153
	movem.l	(A7)+,D3
	unlk	A5
	rts
L151
	move.l	D3,A0
	move.w	(A0),D0
	ext.l	D0
	blt	L165
	cmp.l	#17,D0
	bgt	L165
	asl.l	#2,D0
	lea	L155,A0
	move.l	0(A0,D0.l),A1
	jmp	(A1)
L155
	DC.l	L156
	DC.l	L157
	DC.l	L158
	DC.l	L159
	DC.l	L162
	DC.l	L163
	DC.l	L161
	DC.l	L158
	DC.l	L164
	DC.l	L164
	DC.l	L160
	DC.l	L165
	DC.l	L165
	DC.l	L156
	DC.l	L157
	DC.l	L165
	DC.l	L159
	DC.l	L156
	DC.l	0
L156
	moveq	#1,D0
	bra	L153
L157
	moveq	#2,D0
	bra	L153
L158
	moveq	#2,D0
	bra	L153
L159
	moveq	#2,D0
	bra	L153
L160
	moveq	#2,D0
	bra	L153
L161
	move.l	D3,A0
	move.b	2(A0),D0
	beq	L166
	move.l	D3,A0
	move.l	16(A0),-(A7)
	jsr	_alignment
	addq.w	#4,A7
	move.l	D0,D0
	bra	L153
L166
	moveq	#2,D0
	bra	L153
L167
L162
	moveq	#2,D0
	bra	L153
L163
	moveq	#2,D0
	bra	L153
L164
	moveq	#2,D0
	bra	L153
L165
	moveq	#1,D0
	bra	L153
L154
	bra	L153
_declare:
L169	EQU	-24
	link	A5,#L169
	movem.l	D3-D7/A2-A4,-(A7)
	move.l	12(A5),D5
	lea	_head,A2
	move.l	8(A5),D7
	move.l	14(A5),A3
	moveq	#0,D0
	move.l	D0,D6
	move.w	_lastst,D0
	cmp.w	#82,D0
	bne	L171
	jsr	_getsym
	move.l	D7,-(A7)
	jsr	_decl
	addq.w	#4,A7
	move.l	(A2),-12(A5)
L173
	moveq	#0,D0
	move.l	D0,_declid
	jsr	_decl1
	move.l	_declid,D0
	beq	L176
	pea	28
	jsr	_xalloc
	addq.w	#4,A7
	move.l	D0,D3
	move.l	D3,A0
	move.l	_declid,4(A0)
	moveq	#4,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,16(A0)
	clr.l	-(A7)
	pea	18
	jsr	_maketype
	addq.w	#8,A7
	move.l	D0,-16(A5)
	move.l	-16(A5),A0
	move.l	(A2),16(A0)
	move.l	(A2),A0
	move.l	-16(A5),A1
	move.l	4(A0),4(A1)
	move.l	-16(A5),(A2)
	move.l	D3,A0
	move.l	(A2),24(A0)
	move.l	D7,-(A7)
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
L176
	move.w	_lastst,D0
	cmp.w	#20,D0
	beq	L178
	bra	L174
L178
	pea	20
	jsr	_needpunc
	addq.w	#4,A7
	move.l	-12(A5),(A2)
L175
	bra	L173
L174
	pea	52
	jsr	_needpunc
	addq.w	#4,A7
	moveq	#0,D0
L180
	movem.l	(A7)+,D3-D7/A2-A4
	unlk	A5
	rts
L171
	move.l	D7,-(A7)
	jsr	_decl
	addq.w	#4,A7
	move.l	(A2),-12(A5)
L181
	moveq	#0,D0
	move.l	D0,_declid
	jsr	_decl1
	move.l	(A2),D0
	bne	L184
	pea	L168
	clr.l	-(A7)
	jsr	_error
	addq.w	#8,A7
	bra	L182
L184
	move.l	20(A5),D0
	cmp.l	#6,D0
	bne	L189
	cmp.l	#1,D5
	beq	L188
L189
	move.l	20(A5),D0
	cmp.l	#9,D0
	bne	L186
L188
	moveq	#9,D0
	move.l	D0,20(A5)
	move.l	(A2),A0
	move.w	(A0),D0
	ext.l	D0
	blt	L190
	cmp.l	#14,D0
	bgt	L190
	asl.l	#2,D0
	lea	L191,A0
	move.l	0(A0,D0.l),A1
	jmp	(A1)
L191
	DC.l	L193
	DC.l	L193
	DC.l	L190
	DC.l	L190
	DC.l	L194
	DC.l	L190
	DC.l	L190
	DC.l	L190
	DC.l	L190
	DC.l	L190
	DC.l	L193
	DC.l	L190
	DC.l	L190
	DC.l	L192
	DC.l	L192
	DC.l	0
L192
	pea	4
	pea	7
	jsr	_maketype
	addq.w	#8,A7
	move.l	D0,_tail
	move.l	_tail,(A2)
	bra	L190
L193
	pea	4
	pea	2
	jsr	_maketype
	addq.w	#8,A7
	move.l	D0,_tail
	move.l	_tail,(A2)
	bra	L190
L194
	pea	8
	pea	5
	jsr	_maketype
	addq.w	#8,A7
	move.l	D0,_tail
	move.l	_tail,(A2)
L190
	move.l	(A2),A0
	move.l	4(A0),D0
	beq	L197
	move.l	(A2),A0
	move.w	(A0),D0
	cmp.w	#6,D0
	bne	L195
L197
	move.l	(A2),A0
	clr.b	2(A0)
L195
L186
	move.l	_declid,D0
	beq	L198
	pea	28
	jsr	_xalloc
	addq.w	#4,A7
	move.l	D0,D3
	move.l	D3,A0
	move.l	_declid,4(A0)
	move.l	D3,A0
	move.w	D5,12(A0)
	move.l	20(A5),D0
	move.l	D3,A0
	move.w	D0,14(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,16(A0)
	move.l	(A2),-(A7)
	jsr	_alignment
	addq.w	#4,A7
	move.l	D0,A4
	move.l	A3,D0
	add.l	D6,D0
	move.l	A4,-(A7)
	move.l	D0,-(A7)
	jsr	.lmods
	move.l	(A7)+,D0
	addq.w	#4,A7
	tst.l	D0
	beq	L200
	cmp.l	#6,D5
	beq	L202
	cmp.l	#3,D5
	beq	L202
	cmp.l	#1,D5
	beq	L202
	jsr	_dseg
	move.l	A4,-(A7)
	jsr	_genalignment
	addq.w	#4,A7
L202
L204
	move.l	A3,D0
	add.l	D6,D0
	move.l	A4,-(A7)
	move.l	D0,-(A7)
	jsr	.lmods
	move.l	(A7)+,D0
	addq.w	#4,A7
	tst.l	D0
	beq	L205
	addq.l	#1,D6
	bra	L204
L205
L200
	move.l	D3,A0
	move.l	(A2),24(A0)
	tst.l	D5
	bne	L206
	move.l	_nextlabel,D0
	addq.l	#1,_nextlabel
	move.l	D3,A0
	move.l	D0,16(A0)
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#12,D0
	bne	L208
	move.l	D7,A0
	move.l	(A0),-(A7)
	move.l	D3,A0
	move.l	4(A0),-(A7)
	jsr	_search
	addq.w	#8,A7
	move.l	D0,D4
	beq	L210
	move.l	D4,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#11,D0
	bne	L210
	move.l	D4,A0
	move.l	D3,A1
	move.l	16(A0),16(A1)
L210
L208
	bra	L207
L206
	move.l	18(A5),D0
	cmp.l	#9,D0
	bne	L212
	move.l	D3,A0
	move.l	A3,16(A0)
	bra	L213
L212
	cmp.l	#1,D5
	beq	L214
	move.l	A3,D0
	add.l	D6,D0
	move.l	D3,A0
	move.l	D0,16(A0)
	bra	L215
L214
	move.l	A3,D0
	add.l	D6,D0
	move.l	(A2),A0
	add.l	4(A0),D0
	neg.l	D0
	move.l	D3,A0
	move.l	D0,16(A0)
L215
L213
L207
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#11,D0
	bne	L216
	move.l	D3,A0
	move.w	12(A0),D0
	cmp.w	#2,D0
	beq	L220
	move.l	D3,A0
	move.w	12(A0),D0
	cmp.w	#1,D0
	bne	L218
L220
	moveq	#3,D0
	move.l	D3,A0
	move.w	D0,12(A0)
L218
L216
	move.l	18(A5),D0
	cmp.l	#9,D0
	bne	L221
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	4(A0),-(A7)
	move.l	D6,-(A7)
	jsr	_imax
	addq.w	#8,A7
	move.l	D0,D6
	bra	L222
L221
	cmp.l	#3,D5
	beq	L223
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	4(A0),D0
	add.l	D0,D6
L223
L222
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#12,D0
	beq	L225
	move.l	D7,-(A7)
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
	bra	L226
L225
	move.l	D7,A0
	move.l	(A0),-(A7)
	move.l	D3,A0
	move.l	4(A0),-(A7)
	jsr	_search
	addq.w	#8,A7
	move.l	D0,D4
	beq	L229
	move.l	D4,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#11,D0
	beq	L227
L229
	move.l	D7,-(A7)
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
	bra	L228
L227
	move.l	D3,A0
	move.l	D4,A1
	move.l	24(A0),24(A1)
	move.l	D3,A0
	move.l	D4,A1
	move.w	12(A0),12(A1)
	move.l	D3,A0
	move.l	D4,A1
	move.w	14(A0),14(A1)
	move.l	D3,A0
	move.l	D4,A1
	move.l	16(A0),16(A1)
	move.l	D4,D3
L228
L226
	cmp.l	#1,D5
	bne	L230
	move.l	D3,-(A7)
	jsr	_doinitauto
	addq.w	#4,A7
L230
	cmp.l	#2,D5
	beq	L234
	tst.l	D5
	bne	L232
L234
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#11,D0
	beq	L232
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#12,D0
	beq	L232
	move.l	D3,-(A7)
	jsr	_doinit
	addq.w	#4,A7
L232
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	cmp.w	#12,D0
	bne	L235
	move.l	D3,-(A7)
	jsr	_funcbody
	addq.w	#4,A7
	move.l	D6,D0
	bra	L180
L235
L198
	move.w	_lastst,D0
	cmp.w	#52,D0
	bne	L237
	bra	L182
L237
	pea	20
	jsr	_needpunc
	addq.w	#4,A7
	move.w	_lastst,D0
	ext.l	D0
	move.l	D0,-(A7)
	move.l	D7,-(A7)
	jsr	_declbegin
	addq.w	#8,A7
	tst.l	D0
	bne	L239
	bra	L182
L239
	move.l	-12(A5),(A2)
L183
	bra	L181
L182
	jsr	_getsym
	move.l	D6,D0
	bra	L180
_declbegin:
L241	EQU	-4
	link	A5,#L241
	movem.l	D3-D4,-(A7)
	move.l	12(A5),D3
	move.l	8(A5),-(A7)
	jsr	_istypedef
	addq.w	#4,A7
	move.l	D0,D4
	beq	L243
	moveq	#0,D0
L245
	movem.l	(A7)+,D3-D4
	unlk	A5
	rts
L243
	cmp.l	#53,D3
	beq	L248
	cmp.l	#31,D3
	beq	L248
	cmp.l	#45,D3
	beq	L248
	cmp.l	#44,D3
	bne	L246
L248
	moveq	#1,D0
	bra	L247
L246
	clr.l	D0
L247
	move.l	D0,D0
	bra	L245
_declenum:
L249	EQU	-4
	link	A5,#L249
	movem.l	D3-D4/A2-A4,-(A7)
	move.l	8(A5),D4
	lea	_getsym,A2
	lea	_xalloc,A3
	lea	_lastst,A4
	addq.l	#1,_global_flag
	move.w	(A4),D0
	cmp.w	#31,D0
	bne	L251
	move.l	_tagtable,-(A7)
	pea	_lastid
	jsr	_search
	addq.w	#8,A7
	move.l	D0,D3
	bne	L253
	pea	28
	jsr	(A3)
	addq.w	#4,A7
	move.l	D0,D3
	pea	28
	clr.l	-(A7)
	move.l	D3,-(A7)
	jsr	_memset
	add.w	#12,A7
	moveq	#4,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	pea	_lastid
	jsr	_litlate
	addq.w	#4,A7
	move.l	D3,A0
	move.l	D0,4(A0)
	pea	26
	jsr	(A3)
	addq.w	#4,A7
	move.l	D3,A0
	move.l	D0,24(A0)
	moveq	#10,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	D0,(A0)
	moveq	#2,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,4(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,8(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,12(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,16(A0)
	move.l	D3,A0
	move.l	D3,A1
	move.l	24(A1),A1
	move.l	4(A0),20(A1)
	jsr	(A2)
	move.w	(A4),D0
	cmp.w	#15,D0
	beq	L255
	clr.l	-(A7)
	pea	9
	jsr	_error
	addq.w	#8,A7
	bra	L256
L255
	pea	_tagtable
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
	jsr	(A2)
	move.l	D4,-(A7)
	move.l	D3,-(A7)
	jsr	_enumbody
	addq.w	#8,A7
L256
	bra	L254
L253
	jsr	(A2)
L254
	move.l	D3,A0
	move.l	24(A0),_head
	bra	L252
L251
	pea	28
	jsr	(A3)
	addq.w	#4,A7
	move.l	D0,D3
	pea	28
	clr.l	-(A7)
	move.l	D3,-(A7)
	jsr	_memset
	add.w	#12,A7
	moveq	#4,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,4(A0)
	pea	26
	jsr	(A3)
	addq.w	#4,A7
	move.l	D3,A0
	move.l	D0,24(A0)
	moveq	#1,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	D0,(A0)
	moveq	#2,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,4(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,8(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,12(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,16(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,20(A0)
	move.w	(A4),D0
	cmp.w	#15,D0
	beq	L257
	clr.l	-(A7)
	pea	9
	jsr	_error
	addq.w	#8,A7
	bra	L258
L257
	jsr	(A2)
	move.l	D4,-(A7)
	move.l	D3,-(A7)
	jsr	_enumbody
	addq.w	#8,A7
L258
	move.l	D3,A0
	move.l	24(A0),_head
L252
	subq.l	#1,_global_flag
L259
	movem.l	(A7)+,D3-D4/A2-A4
	unlk	A5
	rts
_enumbody:
L260	EQU	-12
	link	A5,#L260
	movem.l	D3-D5/A2-A3,-(A7)
	lea	_lastst,A2
	lea	_getsym,A3
	moveq	#0,D0
	move.l	D0,D5
L262
	move.w	(A2),D0
	cmp.w	#31,D0
	bne	L263
	pea	28
	jsr	_xalloc
	addq.w	#4,A7
	move.l	D0,D3
	pea	_lastid
	jsr	_litlate
	addq.w	#4,A7
	move.l	D3,A0
	move.l	D0,4(A0)
	moveq	#5,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	move.l	D5,D0
	addq.l	#1,D5
	move.l	D3,A0
	move.l	D0,16(A0)
	lea	_stdconst,A0
	move.l	D3,A1
	move.l	A0,24(A1)
	move.l	12(A5),-(A7)
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
	move.l	D3,-(A7)
	jsr	_copysym
	addq.w	#4,A7
	move.l	D0,D4
	move.l	8(A5),A0
	move.l	24(A0),D0
	addq.l	#8,D0
	move.l	D0,-(A7)
	move.l	D4,-(A7)
	jsr	_insert
	addq.w	#8,A7
	jsr	(A3)
	move.w	(A2),D0
	cmp.w	#11,D0
	bne	L264
	jsr	(A3)
	jsr	_intexpr
	move.l	D4,A0
	move.l	D0,16(A0)
	move.l	D3,A1
	move.l	16(A0),16(A1)
	move.l	D3,A0
	move.l	16(A0),D0
	addq.l	#1,D0
	move.l	D0,D5
L264
	move.w	(A2),D0
	cmp.w	#20,D0
	bne	L266
	jsr	(A3)
	bra	L267
L266
	move.w	(A2),D0
	cmp.w	#24,D0
	beq	L268
	bra	L263
L268
L267
	bra	L262
L263
	pea	24
	jsr	_needpunc
	addq.w	#4,A7
L270
	movem.l	(A7)+,D3-D5/A2-A3
	unlk	A5
	rts
_declstruct:
L271	EQU	-4
	link	A5,#L271
	movem.l	D3-D7/A2-A4,-(A7)
	move.l	8(A5),D4
	lea	_lastst,A2
	lea	_getsym,A3
	lea	_xalloc,A4
	lea	_structbody,A0
	move.l	A0,D5
	lea	_error,A0
	move.l	A0,D6
	lea	_tagtable,A0
	move.l	A0,D7
	addq.l	#1,_global_flag
	move.w	(A2),D0
	cmp.w	#31,D0
	bne	L273
	move.l	D7,A0
	move.l	(A0),-(A7)
	pea	_lastid
	jsr	_search
	addq.w	#8,A7
	move.l	D0,D3
	bne	L275
	pea	28
	jsr	(A4)
	addq.w	#4,A7
	move.l	D0,D3
	pea	28
	clr.l	-(A7)
	move.l	D3,-(A7)
	jsr	_memset
	add.w	#12,A7
	pea	_lastid
	jsr	_litlate
	addq.w	#4,A7
	move.l	D3,A0
	move.l	D0,4(A0)
	moveq	#4,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,(A0)
	pea	26
	jsr	(A4)
	addq.w	#4,A7
	move.l	D3,A0
	move.l	D0,24(A0)
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	D4,(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,8(A0)
	move.l	D3,A0
	move.l	D3,A1
	move.l	24(A1),A1
	move.l	4(A0),20(A1)
	jsr	(A3)
	move.w	(A2),D0
	cmp.w	#53,D0
	beq	L279
	move.w	(A2),D0
	cmp.w	#52,D0
	bne	L277
L279
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,4(A0)
	move.l	D7,-(A7)
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
	bra	L278
L277
	move.w	(A2),D0
	cmp.w	#15,D0
	beq	L280
	clr.l	-(A7)
	pea	9
	move.l	D6,A0
	jsr	(A0)
	addq.w	#8,A7
	bra	L281
L280
	move.l	D7,-(A7)
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
	jsr	(A3)
	move.l	D4,-(A7)
	move.l	D3,A0
	move.l	24(A0),-(A7)
	move.l	D5,A0
	jsr	(A0)
	addq.w	#8,A7
L281
L278
	bra	L276
L275
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	4(A0),D0
	beq	L282
	jsr	(A3)
	bra	L283
L282
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	D4,(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,8(A0)
	moveq	#4,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	move.l	D3,A0
	move.l	D3,A1
	move.l	24(A1),A1
	move.l	4(A0),20(A1)
	jsr	(A3)
	move.w	(A2),D0
	cmp.w	#53,D0
	beq	L286
	move.w	(A2),D0
	cmp.w	#52,D0
	bne	L284
L286
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,4(A0)
	bra	L285
L284
	move.w	(A2),D0
	cmp.w	#15,D0
	beq	L287
	clr.l	-(A7)
	pea	9
	move.l	D6,A0
	jsr	(A0)
	addq.w	#8,A7
	bra	L288
L287
	jsr	(A3)
	move.l	D4,-(A7)
	move.l	D3,A0
	move.l	24(A0),-(A7)
	move.l	D5,A0
	jsr	(A0)
	addq.w	#8,A7
L288
L285
L283
L276
	move.l	D3,A0
	move.l	24(A0),_head
	bra	L274
L273
	pea	28
	jsr	(A4)
	addq.w	#4,A7
	move.l	D0,D3
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,4(A0)
	moveq	#4,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,(A0)
	pea	26
	jsr	(A4)
	addq.w	#4,A7
	move.l	D3,A0
	move.l	D0,24(A0)
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	D4,(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,20(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,8(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	D0,12(A0)
	move.w	(A2),D0
	cmp.w	#15,D0
	beq	L289
	clr.l	-(A7)
	pea	9
	move.l	D6,A0
	jsr	(A0)
	addq.w	#8,A7
	bra	L290
L289
	jsr	(A3)
	move.l	D4,-(A7)
	move.l	D3,A0
	move.l	24(A0),-(A7)
	move.l	D5,A0
	jsr	(A0)
	addq.w	#8,A7
L290
	move.l	D3,A0
	move.l	24(A0),_head
L274
	subq.l	#1,_global_flag
L291
	movem.l	(A7)+,D3-D7/A2-A4
	unlk	A5
	rts
_structbody:
L293	EQU	-4
	link	A5,#L293
	movem.l	D3-D5,-(A7)
	move.l	8(A5),D4
	move.l	12(A5),D5
	tst.l	D4
	bne	L295
	pea	L292
	move.l	_stdout,-(A7)
	jsr	_fprintf
	addq.w	#8,A7
L297
	movem.l	(A7)+,D3-D5
	unlk	A5
	rts
L295
	moveq	#0,D0
	move.l	D0,D3
	move.l	D4,A0
	move.b	#1,2(A0)
	moveq	#-1,D0
	move.l	D4,A0
	move.l	D0,4(A0)
L298
	move.w	_lastst,D0
	cmp.w	#24,D0
	beq	L299
	cmp.l	#8,D5
	bne	L300
	pea	6
	move.l	D5,-(A7)
	move.l	D3,-(A7)
	pea	6
	move.l	D4,D0
	addq.l	#8,D0
	move.l	D0,-(A7)
	jsr	_declare
	add.w	#20,A7
	add.l	D0,D3
	bra	L301
L300
	pea	6
	move.l	D5,-(A7)
	clr.l	-(A7)
	pea	6
	move.l	D4,D0
	addq.l	#8,D0
	move.l	D0,-(A7)
	jsr	_declare
	add.w	#20,A7
	move.l	D0,-(A7)
	move.l	D3,-(A7)
	jsr	_imax
	addq.w	#8,A7
	move.l	D0,D3
L301
	bra	L298
L299
	move.l	D3,D0
	addq.l	#1,D0
	and.l	#-2,D0
	move.l	D4,A0
	move.l	D0,4(A0)
	jsr	_getsym
	bra	L297
_dodecl:
L303	EQU	-4
	link	A5,#L303
	movem.l	D3-D6/A2-A4,-(A7)
	move.l	8(A5),D3
	lea	_declare,A2
	lea	_lc_static,A3
	lea	_error,A4
	lea	_getsym,A0
	move.l	A0,D5
	lea	_lc_auto,A0
	move.l	A0,D6
L305
	move.l	_fatal,D0
	beq	L308
L310
	movem.l	(A7)+,D3-D6/A2-A4
	unlk	A5
	rts
L308
	move.w	_lastst,D0
	ext.l	D0
	move.l	D0,D0
	jsr	.cswitch
	DC.l	L312,5
	DC.l	L313,82
	DC.l	L314,74
	DC.l	L315,55
	DC.l	L316,59
	DC.l	L317,86
	DC.l	L318,31
	DC.l	L319,58
	DC.l	L319,72
	DC.l	L319,76
	DC.l	L319,84
	DC.l	L319,73
	DC.l	L319,80
	DC.l	L319,83
	DC.l	L319,66
	DC.l	L319,85
	DC.l	L319,68
	DC.l	L319,64
	DC.l	L320,79
	DC.l	L321,67
	DC.l	0,L322
L312
	jsr	_asmstmt
	move.l	D0,-(A7)
	jsr	_addauto
	addq.w	#4,A7
	bra	L311
L313
	cmp.l	#2,D3
	bne	L323
	move.l	D3,-(A7)
	pea	8
	move.l	(A3),-(A7)
	pea	2
	pea	_gsyms
	jsr	(A2)
	add.w	#20,A7
	bra	L324
L323
	cmp.l	#1,D3
	bne	L325
	move.l	D3,-(A7)
	pea	8
	move.l	D6,A0
	move.l	(A0),-(A7)
	pea	1
	pea	_lsyms
	jsr	(A2)
	add.w	#20,A7
	bra	L326
L325
	move.l	D3,-(A7)
	pea	8
	clr.l	-(A7)
	pea	1
	pea	_lsyms
	jsr	(A2)
	add.w	#20,A7
L326
L324
	bra	L311
L314
	move.l	D5,A0
	jsr	(A0)
	cmp.l	#1,D3
	beq	L327
	cmp.l	#6,D3
	beq	L327
	clr.l	-(A7)
	pea	12
	jsr	(A4)
	addq.w	#8,A7
L327
	bra	L302
L315
	move.l	D5,A0
	jsr	(A0)
	cmp.l	#1,D3
	beq	L329
	cmp.l	#6,D3
	beq	L329
	clr.l	-(A7)
	pea	12
	jsr	(A4)
	addq.w	#8,A7
L329
	bra	L302
L316
	move.l	D5,A0
	jsr	(A0)
	cmp.l	#1,D3
	beq	L331
	cmp.l	#6,D3
	beq	L331
	clr.l	-(A7)
	pea	12
	jsr	(A4)
	addq.w	#8,A7
L331
	bra	L302
L317
	move.l	D5,A0
	jsr	(A0)
	cmp.l	#1,D3
	beq	L333
	cmp.l	#6,D3
	beq	L333
	clr.l	-(A7)
	pea	12
	jsr	(A4)
	addq.w	#8,A7
L333
	bra	L302
L318
	pea	_lsyms
	jsr	_istypedef
	addq.w	#4,A7
	move.l	D0,D4
	beq	L335
	bra	L302
L335
	pea	_gsyms
	jsr	_istypedef
	addq.w	#4,A7
	move.l	D0,D4
	beq	L337
	bra	L302
L337
	move.l	D4,D4
	cmp.l	#1,D3
	bne	L339
	bra	L310
L339
L319
L302
	cmp.l	#2,D3
	bne	L341
	move.l	D3,-(A7)
	pea	8
	move.l	(A3),-(A7)
	pea	2
	pea	_gsyms
	jsr	(A2)
	add.w	#20,A7
	add.l	D0,(A3)
	bra	L342
L341
	cmp.l	#1,D3
	bne	L343
	move.l	D6,A0
	move.l	A0,-(A7)
	move.l	D3,-(A7)
	pea	8
	move.l	D6,A1
	move.l	(A1),-(A7)
	pea	1
	pea	_lsyms
	jsr	(A2)
	add.w	#20,A7
	move.l	(A7)+,A0
	add.l	D0,(A0)
	bra	L344
L343
	move.l	D3,-(A7)
	pea	8
	clr.l	-(A7)
	pea	1
	pea	_lsyms
	jsr	(A2)
	add.w	#20,A7
L344
L342
	bra	L311
L320
	move.l	D5,A0
	jsr	(A0)
	cmp.l	#6,D3
	bne	L345
	clr.l	-(A7)
	pea	12
	jsr	(A4)
	addq.w	#8,A7
L345
	cmp.l	#1,D3
	bne	L347
	move.l	D3,-(A7)
	pea	8
	move.l	(A3),-(A7)
	clr.l	-(A7)
	pea	_lsyms
	jsr	(A2)
	add.w	#20,A7
	add.l	D0,(A3)
	bra	L348
L347
	move.l	D3,-(A7)
	pea	8
	move.l	(A3),-(A7)
	clr.l	-(A7)
	pea	_gsyms
	jsr	(A2)
	add.w	#20,A7
	add.l	D0,(A3)
L348
	bra	L311
L321
	move.l	D5,A0
	jsr	(A0)
	cmp.l	#6,D3
	bne	L349
	clr.l	-(A7)
	pea	12
	jsr	(A4)
	addq.w	#8,A7
L349
	addq.l	#1,_global_flag
	move.l	D3,-(A7)
	pea	8
	clr.l	-(A7)
	pea	3
	pea	_gsyms
	jsr	(A2)
	add.w	#20,A7
	subq.l	#1,_global_flag
	bra	L311
L322
	bra	L310
L311
L307
	bra	L305
L306
	bra	L310
_compile:
L351	EQU	0
	link	A5,#L351
	movem.l	A2,-(A7)
	lea	_autohead,A2
	move.l	(A2),D0
	beq	L353
	move.l	(A2),-(A7)
	jsr	_genstmt
	addq.w	#4,A7
	jsr	_flush_peep
	moveq	#0,D0
	move.l	D0,_autotail
	move.l	_autotail,(A2)
L353
L355
	move.w	_lastst,D0
	cmp.w	#99,D0
	beq	L356
	move.l	_fatal,D0
	beq	L357
L359
	movem.l	(A7)+,A2
	unlk	A5
	rts
L357
	pea	2
	jsr	_dodecl
	addq.w	#4,A7
	move.w	_lastst,D0
	cmp.w	#99,D0
	beq	L360
	jsr	_getsym
L360
	bra	L355
L356
	move.l	(A2),D0
	beq	L362
	move.l	(A2),-(A7)
	jsr	_genstmt
	addq.w	#4,A7
	jsr	_flush_peep
	moveq	#0,D0
	move.l	D0,_autotail
	move.l	_autotail,(A2)
L362
	jsr	_dumplits
	bra	L359
_declproto:
L364	EQU	-12
	link	A5,#L364
	movem.l	D3-D5/A2-A4,-(A7)
	lea	_head,A2
	lea	_lastst,A3
	lea	_maketype,A4
	moveq	#8,D0
	move.l	D0,D4
	addq.l	#1,_global_flag
L366
	moveq	#0,D0
	move.l	D0,_declid
	pea	_lsyms
	jsr	_decl
	addq.w	#4,A7
	jsr	_decl1
	move.l	(A2),A0
	move.l	4(A0),D0
	beq	L371
	move.l	(A2),A0
	move.w	(A0),D0
	cmp.w	#6,D0
	bne	L369
L371
	move.l	(A2),A0
	clr.b	2(A0)
L369
	pea	28
	jsr	_xalloc
	addq.w	#4,A7
	move.l	D0,D3
	move.l	D3,A0
	move.l	_declid,4(A0)
	moveq	#1,D0
	move.l	D3,A0
	move.w	D0,12(A0)
	moveq	#11,D0
	move.l	D3,A0
	move.w	D0,14(A0)
	moveq	#0,D0
	move.l	D3,A0
	move.l	D0,(A0)
	move.l	D3,A0
	move.l	D4,16(A0)
	move.l	D3,A0
	move.l	(A2),24(A0)
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	4(A0),D0
	cmp.l	#4,D0
	blt	L372
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	4(A0),D0
	add.l	D0,D4
	bra	L373
L372
	move.l	D3,A0
	move.l	24(A0),A0
	move.l	4(A0),D0
	cmp.l	#1,D0
	bne	L374
	move.l	D3,A0
	move.l	24(A0),A0
	addq.l	#1,4(A0)
L374
	addq.l	#2,D4
L373
	move.l	D3,-(A7)
	jsr	_copysym
	addq.w	#4,A7
	move.l	D0,D5
	move.l	D3,A0
	move.l	24(A0),A0
	move.w	(A0),D0
	ext.l	D0
	blt	L376
	cmp.l	#14,D0
	bgt	L376
	asl.l	#2,D0
	lea	L377,A0
	move.l	0(A0,D0.l),A1
	jmp	(A1)
L377
	DC.l	L379
	DC.l	L379
	DC.l	L376
	DC.l	L376
	DC.l	L380
	DC.l	L376
	DC.l	L376
	DC.l	L376
	DC.l	L376
	DC.l	L376
	DC.l	L379
	DC.l	L376
	DC.l	L376
	DC.l	L378
	DC.l	L378
	DC.l	0
L378
	pea	4
	pea	7
	jsr	(A4)
	addq.w	#8,A7
	move.l	D3,A0
	move.l	D0,24(A0)
	bra	L376
L379
	pea	4
	pea	2
	jsr	(A4)
	addq.w	#8,A7
	move.l	D3,A0
	move.l	D0,24(A0)
	bra	L376
L380
	pea	8
	pea	5
	jsr	(A4)
	addq.w	#8,A7
	move.l	D3,A0
	move.l	D0,24(A0)
L376
	pea	_lsyms
	move.l	D3,-(A7)
	jsr	_insert
	addq.w	#8,A7
	move.l	8(A5),-(A7)
	move.l	D5,-(A7)
	jsr	_insert
	addq.w	#8,A7
	move.w	(A3),D0
	cmp.w	#20,D0
	beq	L381
	bra	L367
L381
	jsr	_getsym
L368
	bra	L366
L367
	move.w	(A3),D0
	cmp.w	#25,D0
	bne	L383
	jsr	_getsym
	move.w	(A3),D0
	cmp.w	#18,D0
	beq	L385
	clr.l	-(A7)
	clr.l	-(A7)
	jsr	_error
	addq.w	#8,A7
L385
L383
	subq.l	#1,_global_flag
	move.l	D4,D0
L387
	movem.l	(A7)+,D3-D5/A2-A4
	unlk	A5
	rts
L292
	DC.b	68,73,65,71,32,45,45,32,78,85,76,76
	DC.b	32,97,114,103,117,109,101,110,116,32,116,111
	DC.b	32,115,116,114,117,99,116,98,111,100,121,46
	DC.b	10,0
L168
	DC.b	117,110,107,110,111,119,110,32,116,121,112,101
	DC.b	0,0
L148
	DC.b	68,73,65,71,32,45,45,32,78,85,76,76
	DC.b	32,97,114,103,117,109,101,110,116,32,116,111
	DC.b	32,97,108,105,103,110,109,101,110,116,46,10
	DC.b	0,0
L47
	DC.b	68,73,65,71,32,45,45,32,83,72,79,85
	DC.b	76,68,32,78,69,86,69,82,32,72,65,80
	DC.b	80,69,78,10,0,0
L35
	DC.b	114,101,103,105,115,116,101,114,32,107,101,121
	DC.b	119,111,114,100,32,105,110,32,119,114,111,110
	DC.b	103,32,112,111,115,105,116,105,111,110,0,0
L34
	DC.b	115,116,97,116,105,99,32,107,101,121,119,111
	DC.b	114,100,32,105,110,32,119,114,111,110,103,32
	DC.b	112,111,115,105,116,105,111,110,0,0
L33
	DC.b	99,111,110,115,116,32,107,101,121,119,111,114
	DC.b	100,32,105,110,32,119,114,111,110,103,32,112
	DC.b	111,115,105,116,105,111,110,0
L32
	DC.b	97,117,116,111,32,107,101,121,119,111,114,100
	DC.b	32,105,110,32,119,114,111,110,103,32,112,111
	DC.b	115,105,116,105,111,110,0,0
L1
	DC.b	99,111,110,115,116,0
	XREF	_stdout
	XREF	_clearerr
	XREF	_fclose
	XREF	_feof
	XREF	_ferror
	XREF	_fflush
	XREF	_fgetc
	XREF	_fgetpos
	XREF	_fgets
	XREF	_fopen
	XREF	_fprintf
	XREF	_fputc
	XREF	_fputs
	XREF	_fread
	XREF	_freopen
	XREF	_fscanf
	XREF	_fseek
	XREF	_fsetpos
	XREF	_ftell
	XREF	_fwrite
	XREF	_getc
	XREF	_getchar
	XREF	_gets
	XREF	_perror
	XREF	_printf
	XREF	_putc
	XREF	_putchar
	XREF	_puts
	XREF	_remove
	XREF	_rename
	XREF	_rewind
	XREF	_scanf
	XREF	_setbuf
	XREF	_setvbuf
	XREF	_sprintf
	XREF	_sscanf
	XREF	_tmpfile
	XREF	_tmpnam
	XREF	_ungetc
	XREF	_vfprintf
	XREF	_vprintf
	XREF	_vsprintf
	XREF	_fdopen
	XREF	_abort
	XREF	_abs
	XREF	_atof
	XREF	_atoi
	XREF	_atol
	XREF	_calloc
	XREF	_exit
	XREF	_free
	XREF	_getenv
	XREF	_malloc
	XREF	_rand
	XREF	_realloc
	XREF	_srand
	XREF	_strtod
	XREF	_strtol
	XREF	_strtoul
	XREF	_system
	XREF	_memset
	XREF	_memcpy
	XREF	_memmove
	XREF	_memcmp
	XREF	_strcpy
	XREF	_strncpy
	XREF	_strcat
	XREF	_strncat
	XREF	_strcmp
	XREF	_strncmp
	XREF	_strlen
	XREF	_strchr
	XREF	_strrchr
	XREF	_nextlabel
	XREF	_lastid
	XREF	_lastst
	XREF	_gsyms
	XREF	_lsyms
	XREF	_lc_static
	XREF	_lc_auto
	XREF	_autohead
	XREF	_autotail
	XREF	_global_flag
	XDEF	_head
	XDEF	_tail
	XDEF	_declid
	XDEF	_tagtable
	XDEF	_stdconst
	XDEF	_decl2
	XDEF	_declenum
	XDEF	_enumbody
	XDEF	_declstruct
	XDEF	_structbody
	XREF	_apply_qualifiers
	XREF	_search
	XREF	_intexpr
	XREF	_fatal
	XREF	_asmstmt
	XREF	_xalloc
	XDEF	_imax
	XDEF	_litlate
	XDEF	_copysym
	XDEF	_maketype
	XDEF	_istypedef
	XDEF	_is_class_error
	XREF	_error
	XREF	_getsym
	XDEF	_decl
	XREF	.cswitch
	XDEF	_decl1
	XREF	_needpunc
	XREF	.lmuls
	XDEF	_alignment
	XDEF	_declare
	XREF	_insert
	XREF	_dseg
	XREF	_genalignment
	XREF	_doinitauto
	XREF	_doinit
	XREF	_funcbody
	XDEF	_declbegin
	XREF	.lmods
	XDEF	_dodecl
	XREF	_addauto
	XDEF	_compile
	XREF	_genstmt
	XREF	_flush_peep
	XREF	_dumplits
	XDEF	_declproto
	END
