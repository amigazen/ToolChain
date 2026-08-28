/*
 * test_sasc_compiler_specific.c - SAS/C features from compiler-specific.h
 *
 * Under __SASC (AC predefines it), Hyperion/NDK clib/compiler-specific.h
 * expands to:
 *   __ASM__  -> __asm
 *   __REG__(r, p) -> register __##r p
 *   __STDARGS__ / __SAVE_DS__ / __FAR__ / __INTERRUPT__ / __CHIP__ / __FAST__
 *
 * This compile-only smoke mirrors those expansions (and graphics/monitor.h
 * style function-pointer members) without requiring a full NDK on -I.
 */
#if __STDC__
#define __CLIB_PROTOTYPE(a) a
#else
#define __CLIB_PROTOTYPE(a) ()
#endif

#ifdef __SASC
#define __ASM__ __asm
/*
 * Match Hyperion compiler-specific.h exactly: paste `__` with the register
 * name in one replacement list.  A nested `__AC_GLUE(__, r)` helper is not
 * re-expanded after __REG__ pushback (prepdefine does not rescan), so the
 * parser would see `__AC_GLUE(...)` and fail with Punctuation.
 */
#define __REG__(r, p) register __ ## r p
#define __STDARGS__ __stdargs
#define __SAVE_DS__ __saveds
#define __FAR__ __far
#define __INTERRUPT__ __interrupt
#define __CHIP__ __chip
#define __FAST__ __fast
#else
#error AC must define __SASC for this test
#endif

typedef long LONG;
typedef unsigned long ULONG;
typedef unsigned short UWORD;
typedef void *APTR;

struct MonitorSpec;

/* Specifier form: __asm LONG f(register __d0 LONG, register __a0 APTR); */
__ASM__ LONG
AsmAdd(__REG__(d0, LONG a), __REG__(d1, LONG b));

/* Declarator form used in graphics/monitor.h / gfxnodes.h */
struct Hookish {
	LONG (* __ASM__ h_Entry) __CLIB_PROTOTYPE((
		__REG__(a0, struct Hookish *),
		__REG__(a2, APTR),
		__REG__(a1, APTR)));
	LONG (* __ASM__ h_Sub) __CLIB_PROTOTYPE((
		__REG__(a0, struct MonitorSpec *),
		__REG__(d0, UWORD)));
};

/* Remaining compiler-specific.h keywords already stored as attributes */
__CHIP__ ULONG chip_flag;
__FAST__ ULONG fast_flag;
__FAR__ ULONG far_flag;

__SAVE_DS__ __STDARGS__ int
saved_std(void);

__INTERRUPT__ void
irq_stub(void);

__aligned ULONG aligned_word;

int
main(void)
{
	struct Hookish h;
	LONG (*fp) __CLIB_PROTOTYPE((__REG__(d0, LONG), __REG__(d1, LONG)));
	LONG n;

	h.h_Entry = 0;
	h.h_Sub = 0;
	fp = AsmAdd;
	n = 0;
	if (fp != 0)
		n = (*fp)(1L, 2L);
	chip_flag = 1UL;
	fast_flag = 2UL;
	far_flag = 3UL;
	aligned_word = 4UL;
	return (int) n + (int) chip_flag + (int) sizeof(struct Hookish);
}
