*
* crt/ac_lvos.s - stack-callable thunks to cclib.library LVOs
*
* AC emits jsr _fopen etc.  cclib is a shared library (register LVOs).
* These wrappers adapt the C stack face to the LVO register face.
* Offsets match SDK/cclib.library Include_H/inline/cclib.h (V4).
*
* Requires _CCLibBase from crt/ac_crt.c (OpenLibrary before any LVO use).
* Assemble with A68k; link with CRT objects.
*

	SECTION	1,CODE

	XREF	_CCLibBase

*
* void ac_call0(void *fn) - call a void(void) through a pointer.
* AC cannot parse void (*f)(void); use this trampoline instead.
*
	XDEF	_ac_call0
_ac_call0:
	move.l	4(A7),A0
	jsr	(A0)
	rts

*
* task_UserData *GetSTDIO(void)  LVO -36
*
	XDEF	_GetSTDIO
_GetSTDIO:
	move.l	_CCLibBase,A6
	jsr	-36(A6)
	rts

*
* void ClearSTDIO(void)  LVO -42
*
	XDEF	_ClearSTDIO
_ClearSTDIO:
	move.l	_CCLibBase,A6
	jsr	-42(A6)
	rts

*
* long SetupSTDIO(stdin_p, stdout_p, stderr_p, errno_p, blocksize_p,
*                 type_p, mathbase, alen, aptr, wbm, exitfunc)
* LVO -66  (a0..a5, d0..d4)
*
* Save A4/A5 (and D2-D4/A2-A3/A6) around the LVO: AC may use A5 as FP.
*
	XDEF	_SetupSTDIO
_SetupSTDIO:
	movem.l	D2-D4/A2-A6,-(A7)
* After 7 longs saved, original 4(A7) is at 32(A7)
	move.l	32(A7),A0
	move.l	36(A7),A1
	move.l	40(A7),A2
	move.l	44(A7),A3
	move.l	48(A7),A4
	move.l	52(A7),A5
	move.l	56(A7),D0
	move.l	60(A7),D1
	move.l	64(A7),D2
	move.l	68(A7),D3
	move.l	72(A7),D4
	move.l	_CCLibBase,A6
	jsr	-66(A6)
	movem.l	(A7)+,D2-D4/A2-A6
	rts

* FILE *fopen(name, mode)  LVO -144  a0=name a1=mode
	XDEF	_fopen
_fopen:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	_CCLibBase,A6
	jsr	-144(A6)
	rts

* int fclose(stream)  LVO -156  a0=stream
	XDEF	_fclose
_fclose:
	move.l	4(A7),A0
	move.l	_CCLibBase,A6
	jsr	-156(A6)
	rts

* int fputc(c, stream)  LVO -204  d0=c a0=stream
	XDEF	_fputc
_fputc:
	move.l	4(A7),D0
	move.l	8(A7),A0
	move.l	_CCLibBase,A6
	jsr	-204(A6)
	rts

* size_t fwrite(ptr, size, n, stream)  LVO -252  a0=ptr d0=size d1=n a1=stream
	XDEF	_fwrite
_fwrite:
	move.l	4(A7),A0
	move.l	8(A7),D0
	move.l	12(A7),D1
	move.l	16(A7),A1
	move.l	_CCLibBase,A6
	jsr	-252(A6)
	rts

* size_t fread(ptr, size, n, stream)  LVO -288  a0=ptr d0=size d1=n a1=stream
	XDEF	_fread
_fread:
	move.l	4(A7),A0
	move.l	8(A7),D0
	move.l	12(A7),D1
	move.l	16(A7),A1
	move.l	_CCLibBase,A6
	jsr	-288(A6)
	rts

* char *fgets(s, n, stream)  LVO -300  a0=s d0=n a1=stream
	XDEF	_fgets
_fgets:
	move.l	4(A7),A0
	move.l	8(A7),D0
	move.l	12(A7),A1
	move.l	_CCLibBase,A6
	jsr	-300(A6)
	rts

* int fputs(s, stream)  LVO -306  a0=s a1=stream
	XDEF	_fputs
_fputs:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	_CCLibBase,A6
	jsr	-306(A6)
	rts

* int fprintf(stream, fmt, ...)  tagcall LVO -330  a0=stream a1=fmt a2=&args
	XDEF	_fprintf
_fprintf:
	move.l	4(A7),A0
	move.l	8(A7),A1
	lea	12(A7),A2
	move.l	_CCLibBase,A6
	jsr	-330(A6)
	rts

* int sprintf(s, fmt, ...)  tagcall LVO -342  a0=s a1=fmt a2=&args
	XDEF	_sprintf
_sprintf:
	move.l	4(A7),A0
	move.l	8(A7),A1
	lea	12(A7),A2
	move.l	_CCLibBase,A6
	jsr	-342(A6)
	rts

* int printf(fmt, ...)  tagcall LVO -318  a0=fmt a1=&args
	XDEF	_printf
_printf:
	move.l	4(A7),A0
	lea	8(A7),A1
	move.l	_CCLibBase,A6
	jsr	-318(A6)
	rts

* int fseek(stream, offset, whence)  LVO -264  a0=stream d0=offset d1=whence
	XDEF	_fseek
_fseek:
	move.l	4(A7),A0
	move.l	8(A7),D0
	move.l	12(A7),D1
	move.l	_CCLibBase,A6
	jsr	-264(A6)
	rts

* int fflush(stream)  LVO -276  a0=stream
	XDEF	_fflush
_fflush:
	move.l	4(A7),A0
	move.l	_CCLibBase,A6
	jsr	-276(A6)
	rts

* void *malloc(size)  LVO -384  d0=size
	XDEF	_malloc
_malloc:
	move.l	4(A7),D0
	move.l	_CCLibBase,A6
	jsr	-384(A6)
	rts

* void *calloc(nelem, elsize)  LVO -390  d0=nelem d1=elsize
	XDEF	_calloc
_calloc:
	move.l	4(A7),D0
	move.l	8(A7),D1
	move.l	_CCLibBase,A6
	jsr	-390(A6)
	rts

* void *realloc(ptr, size)  LVO -396  a0=ptr d0=size
	XDEF	_realloc
_realloc:
	move.l	4(A7),A0
	move.l	8(A7),D0
	move.l	_CCLibBase,A6
	jsr	-396(A6)
	rts

* void free(ptr)  LVO -402  a0=ptr
	XDEF	_free
_free:
	move.l	4(A7),A0
	move.l	_CCLibBase,A6
	jsr	-402(A6)
	rts

* int atoi(s)  LVO -486  a0=s
	XDEF	_atoi
_atoi:
	move.l	4(A7),A0
	move.l	_CCLibBase,A6
	jsr	-486(A6)
	rts

* void *memcpy(dst, src, n)  LVO -552  a0=dst a1=src d0=n
	XDEF	_memcpy
_memcpy:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	12(A7),D0
	move.l	_CCLibBase,A6
	jsr	-552(A6)
	rts

* void *memmove(dst, src, n)  LVO -918 (V4)  a0=dst a1=src d0=n
	XDEF	_memmove
_memmove:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	12(A7),D0
	move.l	_CCLibBase,A6
	jsr	-918(A6)
	rts

* void *memset(b, c, n)  LVO -558  a0=b d0=c d1=n
	XDEF	_memset
_memset:
	move.l	4(A7),A0
	move.l	8(A7),D0
	move.l	12(A7),D1
	move.l	_CCLibBase,A6
	jsr	-558(A6)
	rts

* char *strcat(dst, src)  LVO -594  a0=dst a1=src
	XDEF	_strcat
_strcat:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	_CCLibBase,A6
	jsr	-594(A6)
	rts

* char *strchr(s, c)  LVO -600  a0=s d0=c
	XDEF	_strchr
_strchr:
	move.l	4(A7),A0
	move.l	8(A7),D0
	move.l	_CCLibBase,A6
	jsr	-600(A6)
	rts

* int strcmp(s1, s2)  LVO -606  a0=s1 a1=s2
	XDEF	_strcmp
_strcmp:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	_CCLibBase,A6
	jsr	-606(A6)
	rts

* char *strcpy(dst, src)  LVO -612  a0=dst a1=src
	XDEF	_strcpy
_strcpy:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	_CCLibBase,A6
	jsr	-612(A6)
	rts

* size_t strlen(s)  LVO -624  a0=s
	XDEF	_strlen
_strlen:
	move.l	4(A7),A0
	move.l	_CCLibBase,A6
	jsr	-624(A6)
	rts

* int strncmp(s1, s2, n)  LVO -642  a0=s1 a1=s2 d0=n
	XDEF	_strncmp
_strncmp:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	12(A7),D0
	move.l	_CCLibBase,A6
	jsr	-642(A6)
	rts

* char *strncpy(dst, src, n)  LVO -648  a0=dst a1=src d0=n
	XDEF	_strncpy
_strncpy:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	12(A7),D0
	move.l	_CCLibBase,A6
	jsr	-648(A6)
	rts

* char *strstr(haystack, needle)  LVO -672  a0=haystack a1=needle
	XDEF	_strstr
_strstr:
	move.l	4(A7),A0
	move.l	8(A7),A1
	move.l	_CCLibBase,A6
	jsr	-672(A6)
	rts

*
* STRPTR GetArgStr(void)  dos.library LVO -534 (V36+)
*
* SDK amiga.lib here is 1.x-era and has no _GetArgStr stub; provide one.
* Requires _DOSBase already opened (ac_crt_entry).
*
	XREF	_DOSBase
	XDEF	_GetArgStr
_GetArgStr:
	move.l	_DOSBase,A6
	jsr	-534(A6)
	rts

*
* .cswitch - AC switch-table dispatcher (was in ac.lib SysIO/lib.asm)
*
* Table after return address:
*   long label, case  (repeated)
*   long 0, default
* Case value in D0.
*
	XDEF	.cswitch
.cswitch:
	move.l	(A7)+,A0
Lac_csw1:
	move.l	(A0)+,A1
	move.l	A1,D1
	beq.s	Lac_csw2
	cmp.l	(A0)+,D0
	bne.s	Lac_csw1
	jmp	(A1)
Lac_csw2:
	move.l	(A0),A0
	jmp	(A0)

	END
