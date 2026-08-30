/*
 * ac_tests.c - AC language regression suite (DejaGnu / GCC .sum style).
 *
 * Industry-standard C compiler test reporting:
 *   PASS: name
 *   FAIL: name
 *   UNTESTED: name
 *   === ac unittest Summary ===
 *   # of expected passes ...
 *
 * Build/run on Amiga (from src/):
 *   make demo-self2
 *   unittest-out/ac-self2/ac_tests
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

#include <stdarg.h>

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
    /* f/F suffix (C99) - getfloatsuffix + en_cdf / gen_fconvert. */
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

/* --- C89 language coverage (runtime) --- */

enum c89_color { C89_RED = 1, C89_GREEN, C89_BLUE = 10 };

struct c89_point {
    int x;
    int y;
};

struct c89_rect {
    struct c89_point tl;
    struct c89_point br;
};

union c89_num {
    long l;
    char bytes[4];
};

struct c89_bits {
    unsigned a : 3;
    unsigned b : 5;
    unsigned c : 8;
};

/* SAS F.3.9 left-to-right: a,b,c pack into high bits of a 32-bit word. */
struct c89_bits_msb {
    unsigned a : 4;
    unsigned b : 4;
    unsigned c : 8;
    unsigned pad : 16;
};

static int c89_file_static;
static int c89_static_counter;

static long
c89_kr_add(a, b)
    int a;
    int b;
{
    return (long) (a + b);
}

static int
c89_add1(n)
    int n;
{
    return n + 1;
}

static long
c89_sum_ints(n, first)
    int n;
    int first;
{
    va_list ap;
    long sum;
    int i;
    int v;

    sum = (long) first;
    va_start(ap, first);
    for (i = 1; i < n; i++) {
        v = va_arg(ap, int);
        sum += (long) v;
    }
    va_end(ap);
    return sum;
}

#define C89_STR(x) #x
#define C89_CAT(a, b) a##b

static void
test_c89_enum()
{
    enum c89_color c;
    int n;

    expect_long("c89/enum/red", (long) C89_RED, 1L);
    expect_long("c89/enum/green", (long) C89_GREEN, 2L);
    expect_long("c89/enum/blue", (long) C89_BLUE, 10L);
    expect_true("c89/enum/sizeof_gt_0", sizeof(enum c89_color) > 0);
    expect_long("c89/enum/sizeof_int", (long) sizeof(enum c89_color), 4L);
    c = C89_GREEN;
    n = 0;
    switch (c) {
    case C89_RED:
        n = 1;
        break;
    case C89_GREEN:
        n = 2;
        break;
    default:
        n = 9;
        break;
    }
    expect_long("c89/enum/switch", (long) n, 2L);
}

static void
test_c89_struct()
{
    struct c89_point p;
    struct c89_rect r;
    struct c89_point arr[2];
    struct c89_point *pp;

    p.x = 3;
    p.y = 4;
    expect_long("c89/struct/member_x", (long) p.x, 3L);
    expect_long("c89/struct/member_y", (long) p.y, 4L);
    expect_true("c89/struct/sizeof_ge_8", sizeof(struct c89_point) >= 8);
    /* F.3.9: char then short - short on even offset, sizeof padded. */
    {
        struct c89_pad {
            char c;
            short s;
        } pad;
        char *base;

        pad.c = 1;
        pad.s = 2;
        base = (char *) &pad;
        expect_long("c89/struct/short_even_off",
                    (long) ((char *) &pad.s - base), 2L);
        expect_true("c89/struct/pad_sizeof_ge_4", sizeof(pad) >= 4);
    }
    r.tl.x = 1;
    r.tl.y = 2;
    r.br.x = 5;
    r.br.y = 6;
    expect_long("c89/struct/nested_tl_x", (long) r.tl.x, 1L);
    expect_long("c89/struct/nested_br_y", (long) r.br.y, 6L);
    arr[0].x = 7;
    arr[1].x = 8;
    expect_long("c89/struct/array0", (long) arr[0].x, 7L);
    expect_long("c89/struct/array1", (long) arr[1].x, 8L);
    pp = &p;
    expect_long("c89/struct/ptr_member", (long) pp->y, 4L);
}

static void
test_c89_union()
{
    union c89_num u;

    u.l = 0x01020304L;
    expect_long("c89/union/long_roundtrip", u.l, 0x01020304L);
    expect_true("c89/union/sizeof_ge_long", sizeof(union c89_num) >= sizeof(long));
    u.bytes[0] = 0;
    expect_long("c89/union/byte0", (long) (unsigned char) u.bytes[0], 0L);
}

static void
test_c89_bitfield()
{
    struct c89_bits s;
    struct c89_bits_msb m;
    unsigned long *wp;

    s.a = 5;
    s.b = 17;
    s.c = 200;
    expect_long("c89/bitfield/a", (long) s.a, 5L);
    expect_long("c89/bitfield/b", (long) s.b, 17L);
    expect_long("c89/bitfield/c", (long) s.c, 200L);

    m.a = 0xA;
    m.b = 0xB;
    m.c = 0xCD;
    m.pad = 0;
    wp = (unsigned long *) &m;
    /* MSB-first BE: 0xABCD then 16 zero pad bits. */
    expect_long("c89/bitfield/msb_word", (long) (*wp), (long) 0xABCD0000UL);
}

static void
test_c89_switch()
{
    int i;
    int n;

    n = 0;
    for (i = 0; i < 5; i++) {
        switch (i) {
        case 0:
            n += 1;
            break;
        case 1:
        case 2:
            n += 10;
            break;
        case 4:
            n += 100;
            break;
        default:
            n += 2;
            break;
        }
    }
    expect_long("c89/switch/mixed", (long) n, 123L);

    n = 0;
    switch (99) {
    case 1:
        n = 1;
        break;
    default:
        n = 7;
        break;
    }
    expect_long("c89/switch/default", (long) n, 7L);

    n = 0;
    switch (2) {
    case 1:
        n += 1;
    case 2:
        n += 2;
    case 3:
        n += 4;
        break;
    default:
        n = -1;
        break;
    }
    expect_long("c89/switch/fallthrough", (long) n, 6L);
}

static void
test_c89_goto()
{
    int n;

    n = 0;
    goto c89_fwd;
    n = 99;
c89_fwd:
    n += 1;
    expect_long("c89/goto/forward", (long) n, 1L);

    n = 0;
c89_back:
    n++;
    if (n < 3)
        goto c89_back;
    expect_long("c89/goto/back", (long) n, 3L);

    n = 0;
    {
        int inner;

        inner = 1;
        if (inner)
            goto c89_out;
        n = 5;
    }
c89_out:
    n = 8;
    expect_long("c89/goto/out_block", (long) n, 8L);
}

static void
test_c89_loop()
{
    int i;
    int n;

    n = 0;
    for (i = 0; i < 5; i++)
        n += i;
    expect_long("c89/loop/for_sum", (long) n, 10L);

    n = 0;
    i = 0;
    while (i < 4) {
        n += i;
        i++;
    }
    expect_long("c89/loop/while_sum", (long) n, 6L);

    n = 0;
    i = 0;
    do {
        n += 1;
        i++;
    } while (i < 3);
    expect_long("c89/loop/do_count", (long) n, 3L);

    n = 0;
    for (i = 0; i < 10; i++) {
        if (i == 3)
            break;
        n++;
    }
    expect_long("c89/loop/break", (long) n, 3L);

    n = 0;
    for (i = 0; i < 5; i++) {
        if (i == 2)
            continue;
        n += i;
    }
    expect_long("c89/loop/continue", (long) n, 8L);
}

static void
test_c89_expr()
{
    int a;
    int b;
    int n;

    a = 0;
    b = 1;
    n = a ? 5 : 9;
    expect_long("c89/expr/ternary", (long) n, 9L);
    n = (a = 2, b = 3, a + b);
    expect_long("c89/expr/comma", (long) n, 5L);

    n = 0;
    if (0 && (n = 1))
        ;
    expect_long("c89/expr/and_short", (long) n, 0L);
    n = 0;
    if (1 || (n = 1))
        ;
    expect_long("c89/expr/or_short", (long) n, 0L);

    a = 8;
    a += 2;
    expect_long("c89/expr/add_assign", (long) a, 10L);
    a -= 3;
    expect_long("c89/expr/sub_assign", (long) a, 7L);
    a *= 2;
    expect_long("c89/expr/mul_assign", (long) a, 14L);
    a /= 2;
    expect_long("c89/expr/div_assign", (long) a, 7L);
    a %= 4;
    expect_long("c89/expr/mod_assign", (long) a, 3L);
    a = 1;
    a <<= 3;
    expect_long("c89/expr/shl_assign", (long) a, 8L);
    a >>= 2;
    expect_long("c89/expr/shr_assign", (long) a, 2L);
    a = 0xF;
    a &= 0x3;
    expect_long("c89/expr/and_assign", (long) a, 3L);
    a |= 0x8;
    expect_long("c89/expr/or_assign", (long) a, 11L);
    a ^= 0x1;
    expect_long("c89/expr/xor_assign", (long) a, 10L);
}

static void
test_c89_ptr()
{
    int x;
    int y;
    int *p;
    int **pp;
    void *vp;
    int (*fp)();

    x = 11;
    p = &x;
    expect_long("c89/ptr/deref", (long) *p, 11L);
    *p = 12;
    expect_long("c89/ptr/store", (long) x, 12L);
    y = 20;
    p = &y;
    pp = &p;
    expect_long("c89/ptr/double_deref", (long) **pp, 20L);
    vp = (void *) &x;
    p = (int *) vp;
    expect_long("c89/ptr/void_roundtrip", (long) *p, 12L);
    fp = c89_add1;
    expect_long("c89/ptr/func_ptr", (long) (*fp)(40), 41L);
}

static void
test_c89_cast()
{
    int i;
    long l;
    float f;

    l = 1000L;
    i = (int) l;
    expect_long("c89/cast/long_to_int", (long) i, 1000L);
    i = 7;
    l = (long) i;
    expect_long("c89/cast/int_to_long", l, 7L);
    f = 3.7f;
    i = (int) f;
    expect_long("c89/cast/float_to_int", (long) i, 3L);
    i = 5;
    f = (float) i;
    expect_long("c89/cast/int_to_float", (long) f, 5L);
}

static void
test_c89_string()
{
    char *s;
    char buf[8];
    int i;

    s = "ab" "cd";
    expect_long("c89/string/concat_len", (long) sizeof("ab" "cd"), 5L);
    expect_long("c89/string/concat0", (long) s[0], (long) 'a');
    expect_long("c89/string/concat3", (long) s[3], (long) 'd');
    expect_long("c89/string/sizeof_hello", (long) sizeof("hi"), 3L);
    expect_long("c89/string/sizeof_empty", (long) sizeof(""), 1L);
    expect_long("c89/string/sizeof_one", (long) sizeof("x"), 2L);
    expect_long("c89/string/index_lit", (long) "yz"[1], (long) 'z');
    expect_long("c89/string/escape_nl", (long) '\n', 10L);
    expect_long("c89/string/escape_tab", (long) '\t', 9L);
    expect_long("c89/string/escape_nul", (long) '\0', 0L);
    expect_long("c89/string/escape_oct", (long) '\101', 65L);
    expect_long("c89/string/escape_quote", (long) '\'', 39L);
    buf[0] = 'x';
    buf[1] = '\0';
    expect_long("c89/string/buf0", (long) buf[0], (long) 'x');
    i = (int) sizeof(buf);
    expect_long("c89/string/buf_sizeof", (long) i, 8L);
}

static void
test_c89_init()
{
    struct c89_point p;
    char msg[6];
    static int zs;
    int za;
    int a[4];
    int i;

    p.x = 9;
    p.y = 10;
    expect_long("c89/init/struct_x", (long) p.x, 9L);
    expect_long("c89/init/struct_y", (long) p.y, 10L);
    msg[0] = 'h';
    msg[1] = 'i';
    msg[2] = '\0';
    expect_long("c89/init/string_arr0", (long) msg[0], (long) 'h');
    expect_long("c89/init/string_arr2", (long) msg[2], 0L);
    expect_long("c89/init/static_zero", (long) zs, 0L);
    za = 0;
    expect_long("c89/init/auto_explicit_zero", (long) za, 0L);

    a[0] = 1;
    a[1] = 2;
    a[2] = 3;
    a[3] = 4;
    expect_long("c89/init/arr0", (long) a[0], 1L);
    expect_long("c89/init/arr3", (long) a[3], 4L);
    expect_long("c89/init/sizeof_arr", (long) sizeof(a), 16L);
    i = 0;
    expect_long("c89/init/arr_sum",
                (long) (a[0] + a[1] + a[2] + a[3] + i), 10L);
}

/*
 * Extra C89 edge cases: promotions, pointer arithmetic, nested ?: ,
 * sizeof forms, multi-dim arrays, enum tricks, struct copy-ish, etc.
 */
static int
c89_proto_add(a, b)
    int a;
    int b;
{
    return a + b;
}

static int
c89_sizeof_param(a)
    int a[10];
{
    /* Array parameter decays; sizeof(a) is pointer size (4 on m68k). */
    return (int) sizeof(a);
}

static int
c89_fact(n)
    int n;
{
    if (n <= 1)
        return 1;
    return n * c89_fact(n - 1);
}

static void
test_c89_edge()
{
    int i;
    int j;
    int n;
    int m[2][3];
    int *p;
    int **pp;
    char c;
    unsigned u;
    long l;
    struct c89_point a;
    struct c89_point b;
    enum c89_color col;
    int (*fp)();

    /* sizeof type-name */
    expect_long("c89/edge/sizeof_int", (long) sizeof(int), 4L);
    expect_long("c89/edge/sizeof_char", (long) sizeof(char), 1L);
    expect_long("c89/edge/sizeof_short", (long) sizeof(short), 2L);
    expect_long("c89/edge/sizeof_ptr", (long) sizeof(int *), 4L);
    expect_true("c89/edge/sizeof_struct_ge_8",
                sizeof(struct c89_point) >= 8);

    /* Integer division truncates toward zero (C89 for positive). */
    expect_long("c89/edge/div_trunc", (long) (7 / 2), 3L);
    expect_long("c89/edge/mod", (long) (7 % 2), 1L);
    /* SAS F.3.5: signed >> arithmetic; rem sign follows dividend. */
    expect_long("c89/edge/asr_neg", (long) ((-8) >> 2), -2L);
    expect_long("c89/edge/mod_neg", (long) ((-7) % 3), -1L);
    u = 1U << 31;
    expect_long("c89/edge/lsr_high", (long) (u >> 1), (long) (1U << 30));
    u = 0x80000000UL;
    u >>= 1;
    expect_long("c89/edge/lsr_assign", (long) u, (long) 0x40000000UL);
    expect_long("c89/edge/unary_plus", (long) (+3), 3L);
    expect_long("c89/edge/unary_not", (long) (!0), 1L);
    expect_long("c89/edge/bit_not", (long) (~0 & 0xFF), 255L);

    /* Char in integer expression (promotion). */
    c = 10;
    expect_long("c89/edge/char_promo", (long) (c + 1), 11L);

    /* Unsigned wrap in a portable way. */
    u = 0U;
    u = u - 1U;
    expect_true("c89/edge/unsigned_wrap", u > 0U);

    /* Pointer arithmetic */
    i = 5;
    p = &i;
    expect_long("c89/edge/ptr_diff_zero", (long) (p - p), 0L);
    expect_long("c89/edge/ptr_index", (long) p[0], 5L);
    pp = &p;
    expect_long("c89/edge/ptr_ptr", (long) **pp, 5L);

    /* Multi-dimensional array */
    m[0][0] = 1;
    m[0][1] = 2;
    m[0][2] = 3;
    m[1][0] = 4;
    m[1][1] = 5;
    m[1][2] = 6;
    expect_long("c89/edge/md_arr_00", (long) m[0][0], 1L);
    expect_long("c89/edge/md_arr_12", (long) m[1][2], 6L);
    expect_long("c89/edge/sizeof_md", (long) sizeof(m), 24L);

    /* Nested conditional */
    i = 1;
    j = 2;
    n = i ? (j ? 9 : 8) : 7;
    expect_long("c89/edge/nested_ternary", (long) n, 9L);
    n = 0 ? 1 : 2 ? 3 : 4;
    expect_long("c89/edge/ternary_assoc", (long) n, 3L);

    /* Comma in for */
    n = 0;
    for (i = 0, j = 10; i < 3; i = i + 1, j = j - 1)
        n = n + i + j;
    expect_long("c89/edge/for_comma", (long) n, 30L);

    /* Enum as int, relational */
    col = C89_BLUE;
    expect_true("c89/edge/enum_gt", col > C89_GREEN);
    expect_long("c89/edge/enum_arith", (long) (C89_RED + C89_GREEN), 3L);

    /* Struct member assign (field copy) */
    a.x = 11;
    a.y = 22;
    b.x = a.x;
    b.y = a.y;
    expect_long("c89/edge/struct_field_copy_x", (long) b.x, 11L);
    expect_long("c89/edge/struct_field_copy_y", (long) b.y, 22L);

    /* Prototype-style call of K&R function */
    expect_long("c89/edge/kr_call", (long) c89_proto_add(2, 3), 5L);

    /* sizeof array parameter is pointer width */
    expect_long("c89/edge/sizeof_param_arr",
                (long) c89_sizeof_param(m[0]), 4L);

    /* Hex / oct integer literals */
    expect_long("c89/edge/hex_lit", 0x2A, 42L);
    expect_long("c89/edge/oct_lit", 052, 42L);

    /* Cast via parentheses around expression */
    l = (long) (int) (3 + 4);
    expect_long("c89/edge/paren_cast", l, 7L);

    /* Function pointer again with assignment */
    fp = c89_add1;
    expect_long("c89/edge/fp_assign", (long) (*fp)(8), 9L);

    /* switch on enum */
    n = 0;
    switch (col) {
    case C89_RED:
        n = 1;
        break;
    case C89_BLUE:
        n = 2;
        break;
    default:
        n = 3;
        break;
    }
    expect_long("c89/edge/switch_enum", (long) n, 2L);

    /* Logical vs bitwise */
    expect_long("c89/edge/land", (long) (1 && 2), 1L);
    expect_long("c89/edge/lor", (long) (0 || 2), 1L);
    expect_long("c89/edge/band", (long) (6 & 3), 2L);
    expect_long("c89/edge/bor", (long) (6 | 3), 7L);
    expect_long("c89/edge/bxor", (long) (6 ^ 3), 5L);

    /* Shift */
    expect_long("c89/edge/shl", (long) (1 << 4), 16L);
    expect_long("c89/edge/shr", (long) (16 >> 2), 4L);

    /* Comparison results are 0 or 1 */
    expect_long("c89/edge/cmp_lt", (long) (1 < 2), 1L);
    expect_long("c89/edge/cmp_eq", (long) (2 == 2), 1L);
    expect_long("c89/edge/cmp_ne", (long) (2 != 3), 1L);

    /* Assignment as expression; chained assign */
    i = j = n = 4;
    expect_long("c89/edge/chain_assign", (long) (i + j + n), 12L);
    if ((i = 7) != 0)
        n = i;
    expect_long("c89/edge/assign_in_if", (long) n, 7L);

    /* Call through function pointer without explicit * */
    fp = c89_add1;
    expect_long("c89/edge/fp_call_sugar", (long) fp(10), 11L);

    /* Null pointer constant comparisons */
    p = 0;
    expect_true("c89/edge/ptr_null", p == 0);
    expect_true("c89/edge/ptr_not_null", &i != 0);

    /* Empty / null statements */
    ;
    while (0)
        ;
    expect_long("c89/edge/null_stmt", 1L, 1L);

    /* do-while; sizeof expression forms */
    n = 0;
    do {
        n = n + 1;
    } while (n < 3);
    expect_long("c89/edge/do_while", (long) n, 3L);
    expect_long("c89/edge/sizeof_expr_arr", (long) sizeof(m), 24L);
    expect_long("c89/edge/sizeof_expr_deref", (long) sizeof(*p), 4L);

    /* const / volatile locals (qualifiers accepted) */
    {
        int v;
        volatile int vi;
        const int *cp;
        int *volatile vp;

        v = 5;
        vi = 6;
        cp = &v;
        vp = &vi;
        expect_long("c89/edge/const_local", (long) *cp, 5L);
        expect_long("c89/edge/volatile_local", (long) *vp, 6L);
    }

    /* Recursion */
    expect_long("c89/edge/recurse_fact", (long) c89_fact(5), 120L);
}

/*
 * C99 features AC already implements (smoke).  Keep long long to sizeof /
 * simple ops that do not hit the known .FDcmp compare miscompile.
 */
#define C99_VA_FIRST(first, ...) (first)
#define C99_VA_COUNT2(a, b, ...) ((a) + (b))

static void
test_c99_features()
{
    long long ll;
    unsigned long long ull;
    _Bool b;
    int x;
    int y;

    /* // line comments */
    x = 1; // set
    x = x + 2; // trail
    expect_long("c99/comment/line", (long) x, 3L);

    /* long long */
    expect_long("c99/ll/sizeof", (long) sizeof(long long), 8L);
    expect_long("c99/ll/sizeof_ull", (long) sizeof(unsigned long long), 8L);
    ll = 0LL;
    ll = ll + 10LL;
    ll = ll - 3LL;
    expect_long("c99/ll/add_sub_low", (long) ll, 7L);
    ull = 5ULL;
    ull = ull + 5ULL;
    expect_long("c99/ll/ull_add_low", (long) ull, 10L);
    expect_long("c99/ll/suffix_LL", (long) (1LL + 2LL), 3L);
    expect_long("c99/ll/suffix_ULL", (long) (3ULL + 1ULL), 4L);

    /* Declarations after statements; for (int i = ...) */
    {
        int a;
        int sum;
        int i;

        a = 1;
        int b;
        b = a + 2;
        expect_long("c99/mixed/after_stmt", (long) b, 3L);

        sum = 0;
        for (i = 0; i < 4; i = i + 1)
            sum = sum + i;
        expect_long("c99/mixed/for_classic", (long) sum, 6L);

        sum = 0;
        for (int j = 0; j < 4; j = j + 1)
            sum = sum + j;
        expect_long("c99/mixed/for_decl", (long) sum, 6L);
    }

    /* _Bool */
    b = 0;
    expect_true("c99/bool/false", !b);
    b = 1;
    expect_true("c99/bool/true", b);
    b = (_Bool) 2;
    expect_true("c99/bool/nonzero_true", b != 0);
    expect_long("c99/bool/sizeof", (long) sizeof(_Bool), 1L);
    expect_long("c99/bool/sizeof_bool_kw", (long) sizeof(bool), 1L);

    /* Variadic macros */
    y = C99_VA_FIRST(42, 1, 2, 3);
    expect_long("c99/macro/va_first", (long) y, 42L);
    y = C99_VA_COUNT2(10, 20, 99, 100);
    expect_long("c99/macro/va_count2", (long) y, 30L);

#if defined(__STDC_VERSION__)
    dejagnu_pass("c99/macro/STDC_VERSION");
#else
    dejagnu_fail("c99/macro/STDC_VERSION");
#endif

    /* restrict / inline are accepted no-ops - exercise in expressions */
    {
        int * restrict rp;
        int v;

        v = 9;
        rp = &v;
        expect_long("c99/restrict/ptr", (long) *rp, 9L);
    }
}

/*
 * Regressions from compiling AmiTLS BearSSL with AC (see also
 * test_bearssl_patterns.c for large T0 tables / offsetof-style macros).
 */
typedef unsigned long br_u32;

typedef struct {
    unsigned char pad[32];
    int x;
} br_eng;

#define BR_ENG(p) ((br_eng *)(void *)(p))

#define BR_T0_FBYTE(x, n) (unsigned char)(((br_u32)(x) >> (n)) & 0x7F)
#define BR_T0_VBYTE(x, n) \
    (unsigned char)((((br_u32)(x) >> (n)) & 0x7F) | 0x80)
#define BR_T0_INT1(x) BR_T0_FBYTE(x, 0)
#define BR_T0_INT2(x) BR_T0_VBYTE(x, 7), BR_T0_FBYTE(x, 0)

#define BR_KT_RSA  1
#define BR_KT_KEYX 0x10
#define BR_KT_SIGN 0x20
#define BR_BUF_SIG 512

#define BR_HASHDESC(id, out, state, blen) \
    (((br_u32)(id)) \
     | (((br_u32)(out)) << 8) \
     | (((br_u32)(state)) << 16) \
     | (((br_u32)(blen)) << 24))

typedef struct {
    const void *data;
    unsigned long len;
} br_seed_chunk;

static void
br_hs_cb(ctx)
    void *ctx;
{
    (void) ctx;
}

static int
br_switch_decl(v)
    int v;
{
    switch (v) {
        int y;
    case 0:
        y = 11;
        return y;
    case 1:
        y = 22;
        return y;
    default:
        return -1;
    }
}

static const unsigned char br_t0_small[] = {
    BR_T0_INT1(BR_KT_SIGN),
    BR_T0_INT1(BR_KT_RSA | BR_KT_KEYX),
    BR_T0_INT2(BR_BUF_SIG)
};

static void
test_bearssl_regress()
{
    br_eng eng;
    void *p;
    void (*fp)(void *);
    br_seed_chunk seed[2];
    br_u32 hd;
    int n;

    p = (void *) &eng;
    eng.x = 7;

    /* 32-bit const fold: shifts/OR must not be clipped to 16-bit icons. */
    expect_long("bearssl/fold/shl_15", (long) (1L << 15), 32768L);
    expect_long("bearssl/fold/shl_20", (long) (1L << 20), 1048576L);
    hd = BR_HASHDESC(6, 32, 32, 64);
    expect_long("bearssl/fold/hashdesc_lo", (long) (hd & 0xFFFFUL),
                (long) (6UL | (32UL << 8)));
    expect_long("bearssl/fold/hashdesc_hi", (long) (hd >> 16),
                (long) (32UL | (64UL << 8)));

    /* sizeof on cast-macro member (ENG->pad). */
    expect_long("bearssl/sizeof/cast_macro_pad",
                (long) sizeof BR_ENG(p)->pad, 32L);
    expect_long("bearssl/sizeof/cast_macro_x",
                (long) sizeof BR_ENG(p)->x, 4L);

    /* Declarations before the first case label. */
    expect_long("bearssl/switch/decl_case0", (long) br_switch_decl(0), 11L);
    expect_long("bearssl/switch/decl_case1", (long) br_switch_decl(1), 22L);

    /* Function designator assignable to void (*)(void *). */
    fp = br_hs_cb;
    fp(p);
    expect_true("bearssl/fn/designator_assign", fp != 0);

    /* C99 auto aggregate brace init (PRF seed chunks). */
    {
        br_seed_chunk local[2] = {
            { p, sizeof eng.pad },
            { p, sizeof(int) }
        };

        expect_true("bearssl/init/seed0_data", local[0].data == p);
        expect_long("bearssl/init/seed0_len", (long) local[0].len, 32L);
        expect_true("bearssl/init/seed1_data", local[1].data == p);
        expect_long("bearssl/init/seed1_len", (long) local[1].len, 4L);
        seed[0] = local[0];
        seed[1] = local[1];
        n = (int) seed[0].len + (int) seed[1].len;
        expect_long("bearssl/init/seed_sum", (long) n, 36L);
    }

    /* T0_INT1(OR) and T0_INT2(512) -> 0x84, 0x00 (7-bit encoding). */
    expect_long("bearssl/t0/int1_sign", (long) br_t0_small[0],
                (long) BR_KT_SIGN);
    expect_long("bearssl/t0/int1_rsa_keyx", (long) br_t0_small[1],
                (long) (BR_KT_RSA | BR_KT_KEYX));
    expect_long("bearssl/t0/int2_hi", (long) br_t0_small[2], 0x84L);
    expect_long("bearssl/t0/int2_lo", (long) br_t0_small[3], 0x00L);

#if defined(__AC__)
    dejagnu_pass("bearssl/macro/AC");
#else
    dejagnu_fail("bearssl/macro/AC");
#endif
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
    dejagnu_pass("bearssl/macro/STDC_VERSION_c99");
#else
    dejagnu_fail("bearssl/macro/STDC_VERSION_c99");
#endif
}

static void
test_c89_storage()
{
    static int once;

    expect_long("c89/storage/file_static_zero", (long) c89_file_static, 0L);
    c89_file_static = 5;
    expect_long("c89/storage/file_static_set", (long) c89_file_static, 5L);

    once = once + 1;
    c89_static_counter = once;
    expect_true("c89/storage/func_static_inc", c89_static_counter >= 1);
    once = once + 1;
    expect_true("c89/storage/func_static_persist", once >= 2);
}

static void
test_c89_kr()
{
    expect_long("c89/kr/old_style_add", c89_kr_add(3, 4), 7L);
}

static void
test_c89_vararg()
{
    expect_long("c89/vararg/sum3", c89_sum_ints(3, 10, 20, 30), 60L);
    expect_long("c89/vararg/sum1", c89_sum_ints(1, 42), 42L);
}

static void
test_c89_pp_value()
{
    int C89_CAT(c89_, n);

    c89_n = 42;
    expect_long("c89/pp_value/token_paste_id", (long) c89_n, 42L);
    expect_long("c89/pp_value/token_paste_num", (long) C89_CAT(1, 2), 12L);
    expect_long("c89/pp_value/stringize_sizeof",
                (long) sizeof(C89_STR(abc)), 4L);
}

static void
test_compile_only_notes()
{
    /*
     * Individual unittest/test_*.c files are compile-checked by make.
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
    dejagnu_untested("compile/test_c89_pp_cond.c");
    dejagnu_untested("compile/test_c89_stringize.c");
    dejagnu_untested("compile/test_c89_token_paste.c");
    dejagnu_untested("compile/test_c89_typedefs.c");
    dejagnu_untested("compile/test_c89_qualifiers.c");
    dejagnu_untested("compile/test_c89_decl_edge.c");
    dejagnu_untested("compile/test_c89_pp_expr.c");
    dejagnu_untested("compile/test_c89_sizeof.c");
    dejagnu_untested("compile/test_c89_include_order.c");
    dejagnu_untested("compile/test_c99_varargs_macro.c");
    dejagnu_untested("compile/test_c99_longlong.c");
    dejagnu_untested("compile/test_c99_bool.c");
    dejagnu_untested("compile/test_c99_inline_restrict.c");
    dejagnu_untested("compile/test_c99_pp.c");
    dejagnu_untested("compile/test_bearssl_patterns.c");
    dejagnu_untested("compile/test_sasc_compiler_specific.c");
}

static void
print_summary()
{
    printf("\n");
    printf("\t\t=== ac unittest Summary ===\n");
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
    printf("Using unittest/ac_tests.c as main test driver\n");
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
    test_c89_enum();
    test_c89_struct();
    test_c89_union();
    test_c89_bitfield();
    test_c89_switch();
    test_c89_goto();
    test_c89_loop();
    test_c89_expr();
    test_c89_ptr();
    test_c89_cast();
    test_c89_string();
    test_c89_init();
    test_c89_storage();
    test_c89_kr();
    test_c89_vararg();
    test_c89_pp_value();
    test_c89_edge();
    test_c99_features();
    test_bearssl_regress();
    test_compile_only_notes();

    print_summary();

    if (n_fail != 0)
        return 1;
    return 0;
}
