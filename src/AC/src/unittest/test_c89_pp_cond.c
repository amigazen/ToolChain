/*
 * test_c89_pp_cond.c - nested #if / #elif / defined / #undef (compile-only).
 */
#define A 1
#define B 0

#if defined(A) && A
#if B
#error should not see B
#elif defined(B)
#define OK 1
#else
#error missing B
#endif
#else
#error A not defined
#endif

#undef A
#ifndef A
#define A_GONE 1
#endif

#if !defined(OK) || !defined(A_GONE)
#error pp cond failed
#endif

int
main(void)
{
	return OK + A_GONE;
}
