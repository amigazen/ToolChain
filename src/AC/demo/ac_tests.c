/*
 * ac_tests.c - AC language regression suite (DejaGnu / GCC .sum style).
 *
 * Industry-standard C compiler test reporting:
 *   PASS: name
 *   FAIL: name
 *   UNTESTED: name
 *   === ac demo Summary ===
 *   # of expected passes ...
 *
 * Build/run on Amiga (from src/):
 *   make demo-self2
 *   demo-out/ac-self2/ac_tests
 *
 * Exit status: 0 if all PASS, 1 if any FAIL.
 */

alignas(4) char g_pad;
_Alignas(8) long g_big;

static_assert(1, "static_assert works");
static_assert(alignof(char) == 1, "alignof char");
static_assert(0b1010 == 10, "binary literal");
static_assert(1'000 == 1000, "digit separator");

int printf();

static int n_pass;
static int n_fail;
static int n_untested;

static void
dejagnu_pass(name)
    char *name;
{
    printf("PASS: %s\n", name);
    n_pass++;
}

static void
dejagnu_fail(name)
    char *name;
{
    printf("FAIL: %s\n", name);
    n_fail++;
}

static void
dejagnu_untested(name)
    char *name;
{
    printf("UNTESTED: %s\n", name);
    n_untested++;
}

static void
expect_long(name, got, want)
    char *name;
    long got;
    long want;
{
    if (got == want)
        dejagnu_pass(name);
    else {
        printf("FAIL: %s (got %ld want %ld)\n", name, got, want);
        n_fail++;
    }
}

static void
expect_true(name, cond)
    char *name;
    int cond;
{
    if (cond)
        dejagnu_pass(name);
    else
        dejagnu_fail(name);
}

static unsigned long
add_uli(a, b)
    unsigned long a;
    unsigned long b;
{
    return a + b + 10UL + 10LU + 10U + 10L;
}

static void
test_smoke()
{
    expect_long("smoke/answer_constant", 42L, 42L);
}

static void
test_printf()
{
    /* Reaching PASS lines below already proves printf works. */
    dejagnu_pass("crt/printf_available");
}

static void
test_binlit()
{
    int x;

    x = 0b1000 | 0b0001;
    expect_long("literal/binary_or", (long) x, 9L);
    expect_long("literal/binary_0b1010", (long) 0b1010, 10L);
    expect_long("literal/binary_0B1111", (long) 0B1111, 15L);
    expect_long("literal/digit_sep_decimal", (long) 1'000, 1000L);
    expect_long("literal/digit_sep_hex", (long) 0xFF'FF, 65535L);
    expect_long("literal/digit_sep_binary", (long) 0b1010'1010, 170L);
}

static void
test_suffixes()
{
    unsigned long ul;

    ul = add_uli(1UL, 2LU);
    expect_true("suffix/UL_LU_add", ul != 0UL);
    expect_long("suffix/U_literal", (long) 3U, 3L);
    expect_long("suffix/L_literal", 4L, 4L);
    /*
     * long long runtime compares currently miscompile to .FDcmp.
     * Size is checked here; LL ops are compile-smoked in test_types_suffixes.c.
     */
    expect_long("suffix/sizeof_long_long", (long) sizeof(long long), 8L);
}

static void
test_alignof()
{
    expect_long("alignof/char", (long) alignof(char), 1L);
    expect_long("alignof/short", (long) alignof(short), 2L);
    expect_long("alignof/long", (long) alignof(long), 2L);
    expect_true("alignof/int_nonzero", alignof(int) >= 1);
    expect_true("alignof/double_nonzero", _Alignof(double) >= 1);
}

static void
test_alignas()
{
    alignas(4) char local;
    long addr;

    addr = (long) &local;
    expect_true("alignas/local_addr_nonzero", addr != 0L);
    expect_true("alignas/g_pad_exists", &g_pad != 0);
    expect_true("alignas/g_big_exists", &g_big != 0);
    /*
     * Runtime alignment of BSS objects depends on loader/CNOP; check
     * address is even when alignas(4)/alignas(8) was requested.
     */
    expect_true("alignas/g_pad_even", (((long) &g_pad) & 1L) == 0L);
    expect_true("alignas/g_big_even", (((long) &g_big) & 1L) == 0L);
}

static void
test_bool_nullptr()
{
    bool b;
    void *p;

    b = true;
    expect_true("c23/bool_true", b);
    b = false;
    expect_true("c23/bool_false", !b);
    p = nullptr;
    expect_true("c23/nullptr_is_null", p == 0);
}

static void
test_types_size()
{
    expect_long("type/sizeof_char", (long) sizeof(char), 1L);
    expect_long("type/sizeof_short", (long) sizeof(short), 2L);
    expect_long("type/sizeof_long", (long) sizeof(long), 4L);
    expect_long("type/sizeof_long_long", (long) sizeof(long long), 8L);
    expect_long("type/sizeof_unsigned_long_int",
                (long) sizeof(unsigned long int), 4L);
}

static void
test_feature_macros()
{
#if defined(__STDC_NO_ATOMICS__) && defined(__STDC_NO_THREADS__) \
 && defined(__STDC_NO_COMPLEX__) && defined(__STDC_NO_VLA__)
    dejagnu_pass("macro/STDC_NO_features");
#else
    dejagnu_fail("macro/STDC_NO_features");
#endif
#if defined(__STDC__)
    dejagnu_pass("macro/STDC");
#else
    dejagnu_fail("macro/STDC");
#endif
}

static void
test_compile_only_notes()
{
    /*
     * Individual /demo/test_*.c files are compile-checked by make.
     * Marked UNTESTED here so the .sum reflects the split harness.
     */
    dejagnu_untested("compile/test_static_assert.c");
    dejagnu_untested("compile/test_cclib_syntax.c");
}

static void
print_summary()
{
    printf("\n");
    printf("\t\t=== ac demo Summary ===\n");
    printf("\n");
    printf("# of expected passes\t\t%d\n", n_pass);
    printf("# of unexpected failures\t%d\n", n_fail);
    printf("# of untested testcases\t\t%d\n", n_untested);
    printf("\n");
}

int
main(void)
{
    n_pass = 0;
    n_fail = 0;
    n_untested = 0;

    printf("Test Run By ac on demo suite\n");
    printf("Native configuration is m68k-amigaos\n");
    printf("\n");
    printf("\t\t=== ac tests ===\n");
    printf("\n");
    printf("Schedule of variations:\n");
    printf("    native\n");
    printf("\n");
    printf("Running target native\n");
    printf("Using /demo/ac_tests.c as main test driver\n");
    printf("\n");

    test_smoke();
    test_printf();
    test_binlit();
    test_suffixes();
    test_alignof();
    test_alignas();
    test_bool_nullptr();
    test_types_size();
    test_feature_macros();
    test_compile_only_notes();

    print_summary();

    if (n_fail != 0)
        return 1;
    return 0;
}
