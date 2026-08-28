/*
 * test_c89_pp_expr.c - preprocessor constant expressions (compile-only).
 */
#define A 2
#define B 3
#define C (A + B)

#if C != 5
#error C should be 5
#endif

#if (1 + 2 * 3) != 7
#error mul before add
#endif

#if (2 << 2) != 8
#error shift in #if
#endif

/* NDK exec/types.h pattern - defined() rewritten before intexpr. */
#if (__STDC_VERSION__ >= 199901) || defined(AMIGA_STDC_C99)
#error C89 AC should not take the C99 types.h branch
#endif

#if defined(__STDC__) && !defined(AMIGA_STDC_C99)
/* ok - defined() alone still works via dodefined / rewrite */
#else
#error defined(__STDC__) should be true
#endif

#if defined(A) && defined(B)
#define BOTH 1
#else
#error A and B should be defined
#endif

#if !defined(MISSING)
#define NO_MISSING 1
#endif

#if BOTH && NO_MISSING
#define OK 1
#endif

#ifndef OK
#error pp expr failed
#endif

int
main(void)
{
	return OK + C;
}
