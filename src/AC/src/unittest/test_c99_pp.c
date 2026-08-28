/*
 * test_c99_pp.c - C99-ish preprocessor features AC implements (compile-only).
 */
#include "test_c99_pp_once.h"
#include "test_c99_pp_once.h"

#if !defined(__STDC_VERSION__)
#error __STDC_VERSION__ should be defined
#endif

#if !defined(C99_PP_ONCE_MARK)
#error #pragma once failed
#endif

#define VA_STR(x, ...) #x
#define VA_CAT(a, b, ...) a ## b

static int VA_CAT(ok, en) = 1;
static char *msg = VA_STR(c99, ignored);

int
main(void)
{
	return oken + (int) msg[0] - 'c' + C99_PP_ONCE_MARK;
}
