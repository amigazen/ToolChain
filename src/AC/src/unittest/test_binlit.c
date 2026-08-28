/*
 * test_binlit.c - binary literals, digit separators, feature macros.
 */

#if !defined(__STDC_NO_ATOMICS__) || !defined(__STDC_NO_THREADS__) \
 || !defined(__STDC_NO_COMPLEX__) || !defined(__STDC_NO_VLA__)
#error missing freestanding feature-absence macros
#endif

static_assert(0b1010 == 10, "binary");
static_assert(0B1111 == 15, "BINARY");
static_assert(1'000 == 1000, "digit sep decimal");
static_assert(0xFF'FF == 65535, "digit sep hex");
static_assert(0b1010'1010 == 170, "digit sep binary");

int
main(void)
{
    int             x;

    x = 0b1000 | 0b0001;
    return x == 9 ? 0 : 1;
}
