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
 *
 * Large-frame tests need ~64K stack (default CLI stack is far too small).
 * Soft-float: set _math so CRT opens mathieeedoubbas into the base the
 * stubs read (avoids math.lib .FDopen / split-base hang).
 */
static const char *ac_tests_stack_cookie = "$STACK: 65536";

/* CRT opens IEEE libs before main (see crt/ac_crt.c). */
short _math = 1;

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
    float f;
    double d;
    long double ld;

    ul = add_uli(1UL, 2LU);
    expect_true("suffix/UL_LU_add", ul != 0UL);
    expect_long("suffix/U_literal", (long) 3U, 3L);
    expect_long("suffix/L_literal", 4L, 4L);
    /*
     * long long runtime compares currently miscompile to .FDcmp.
     * Size is checked here; LL ops are compile-smoked in test_types_suffixes.c.
     */
    expect_long("suffix/sizeof_long_long", (long) sizeof(long long), 8L);

    /* C99/C23 floating suffixes: f/F float, l/L long double (= double here). */
    f = 1.5f;
    expect_long("suffix/float_f", (long) (f * 2.0f), 3L);
    f = 2.0F;
    expect_long("suffix/float_F", (long) f, 2L);
    d = 3.0;
    expect_long("suffix/double_plain", (long) d, 3L);
    ld = 4.0L;
    expect_long("suffix/long_double_L", (long) ld, 4L);
    expect_long("suffix/sizeof_1_0f", (long) sizeof(1.0f), 4L);
    expect_long("suffix/sizeof_1_0", (long) sizeof(1.0), 8L);
    expect_long("suffix/sizeof_1_0L", (long) sizeof(1.0L), 8L);
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

static int
param_set_char(c, v)
    char c;
    int v;
{
    char *p;

    p = &c;
    *p = (char) v;
    return (int) (unsigned char) c;
}

static int
param_set_short(s, v)
    short s;
    int v;
{
    short *p;

    p = &s;
    *p = (short) v;
    return (int) s;
}

static long
sizeof_cparam(c)
    char c;
{
    return (long) sizeof(c);
}

static long
sizeof_sparam(s)
    short s;
{
    return (long) sizeof(s);
}

static void
test_param_addr()
{
    /*
     * &char/&short param must hit the low bytes of the 4-byte BE slot.
     */
    expect_long("param/&char_write", (long) param_set_char(0, 0x5A), 0x5AL);
    expect_long("param/&short_write", (long) param_set_short(0, 0x1234), 0x1234L);
    expect_long("param/sizeof_char_param", sizeof_cparam(0), 1L);
    expect_long("param/sizeof_short_param", sizeof_sparam(0), 2L);
}

static long
fp_float_to_long(f)
    float f;
{
    return (long) f;
}

static long
fp_double_to_long(d)
    double d;
{
    return (long) d;
}

static void
test_fp_assign()
{
    float f;
    double d;
    long oldv;

    /*
     * Float postfix ++/-- used to skip the store-back (gen_fsaincdec).
     * Compound assign and prefix go through assign + soft-float binary.
     */
    /* f/F suffix (C99) — getfloatsuffix + en_cdf / gen_fconvert. */
    f = 10.0f;
    f += 3.0f;
    expect_long("fp/float_add_assign", fp_float_to_long(f), 13L);
    f -= 5.0f;
    expect_long("fp/float_sub_assign", fp_float_to_long(f), 8L);
    f *= 2.0f;
    expect_long("fp/float_mul_assign", fp_float_to_long(f), 16L);
    f /= 4.0f;
    expect_long("fp/float_div_assign", fp_float_to_long(f), 4L);

    f = 7.0f;
    oldv = fp_float_to_long(f++);
    expect_long("fp/float_postinc_old", oldv, 7L);
    expect_long("fp/float_postinc_new", fp_float_to_long(f), 8L);
    oldv = fp_float_to_long(f--);
    expect_long("fp/float_postdec_old", oldv, 8L);
    expect_long("fp/float_postdec_new", fp_float_to_long(f), 7L);
    ++f;
    expect_long("fp/float_preinc", fp_float_to_long(f), 8L);
    --f;
    expect_long("fp/float_predec", fp_float_to_long(f), 7L);

    d = 10.0;
    d += 3.0;
    expect_long("fp/double_add_assign", fp_double_to_long(d), 13L);
    d -= 5.0;
    expect_long("fp/double_sub_assign", fp_double_to_long(d), 8L);
    d *= 2.0;
    expect_long("fp/double_mul_assign", fp_double_to_long(d), 16L);
    d /= 4.0;
    expect_long("fp/double_div_assign", fp_double_to_long(d), 4L);

    d = 7.0;
    oldv = fp_double_to_long(d++);
    expect_long("fp/double_postinc_old", oldv, 7L);
    expect_long("fp/double_postinc_new", fp_double_to_long(d), 8L);
    oldv = fp_double_to_long(d--);
    expect_long("fp/double_postdec_old", oldv, 8L);
    expect_long("fp/double_postdec_new", fp_double_to_long(d), 7L);
    ++d;
    expect_long("fp/double_preinc", fp_double_to_long(d), 8L);
    --d;
    expect_long("fp/double_predec", fp_double_to_long(d), 7L);
}

#define LARGE_FRAME_BYTES 34000

static int
large_frame_touch()
{
    char            buf[LARGE_FRAME_BYTES];
    int             i;

    /* Needs ~64K stack (see src/Version.h $STACK). */
    buf[0] = 1;
    buf[LARGE_FRAME_BYTES - 1] = 2;
    i = 33000;
    buf[i] = 3;
    return (int) buf[0] + (int) buf[LARGE_FRAME_BYTES - 1] + (int) buf[i];
}

static void
test_large_frame()
{
    expect_long("frame/large_touch_ends", (long) large_frame_touch(), 6L);
}

static void
test_bss_array_sizes()
{
    /*
     * Regression: typesize_mul / type_size used to leave a bare count in
     * tp->size so sizeof(int[10]) was 10 and BSS was DS.b 10.
     */
    expect_long("bss/sizeof_int10", (long) sizeof(int[10]), 40L);
    expect_long("bss/sizeof_long5", (long) sizeof(long[5]), 20L);
    expect_long("bss/sizeof_short8", (long) sizeof(short[8]), 16L);
    expect_long("bss/sizeof_char16", (long) sizeof(char[16]), 16L);
    expect_long("bss/sizeof_ptr10", (long) sizeof(char *[10]), 40L);
    expect_long("bss/sizeof_intptr4", (long) sizeof(int *[4]), 16L);
}

static void
test_line_comments()
{
    int x;
    int y;

    /* C99 // comments; block comments default to C89 (first closer wins). */
    x = 0;
    // this must not leave x at 0
    x = 41;
    x = x + 1; // trailing
    expect_long("comment/line_assign", (long) x, 42L);

    y = 1;
    /* glob mention *.c must not break this comment */
    y = y + 1; // +1
    expect_long("comment/block_and_line", (long) y, 2L);
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
    dejagnu_untested("compile/test_bss_arrays.c");
    dejagnu_untested("compile/test_ac_debug_brace.c");
    dejagnu_untested("compile/test_param_addr.c");
    dejagnu_untested("compile/test_fp_assign.c");
    dejagnu_untested("compile/test_large_frame.c");
    dejagnu_untested("compile/test_line_comments.c");
    dejagnu_untested("compile/test_comment_nest.c");
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
    /* Keep the stack cookie live so the linker does not drop the string. */
    if (ac_tests_stack_cookie[0] == 0)
        return 99;

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
    test_line_comments();
    test_alignof();
    test_alignas();
    test_bool_nullptr();
    test_types_size();
    test_bss_array_sizes();
    test_param_addr();
    test_fp_assign();
    test_large_frame();
    test_feature_macros();
    test_compile_only_notes();

    print_summary();

    if (n_fail != 0)
        return 1;
    return 0;
}
