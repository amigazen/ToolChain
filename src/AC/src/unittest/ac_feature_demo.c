/*
 * ac_feature_demo.c - runnable AC 4.0 post-PDC feature demonstration.
 *
 * Build (AmigaOS, ccx on path, PDCIncDirs / PDCLibs assigned):
 *   ccx -I.. -I../src/include ac_feature_demo.c ac_feature_demo_pragmas.c
 *
 * Exit status: number of failed runtime checks (0 means all passed).
 * Compile-time features are proven by a successful link plus #warning output.
 */

#include <stdio.h>
#include "../stdbool.h"
#include "ac_feature_demo.h"
#include "ac_feature_demo.h" /* second include: #pragma once must guard */

extern void demo_pragma_anchor(void);

__chip int demo_chip_data = 42;
__far int demo_far_data = 100;
__near int demo_near_data = 200;
__fast int demo_fast_data = 300;

__regargs long demo_regargs_add(long a, long b)
{
    return a + b;
}

__stdargs long demo_stdargs_add(long a, long b)
{
    return a + b;
}

__interrupt static void demo_isr(void);

static int demo_failures;

static void demo_check(const char *name, int ok)
{
    printf("  [%s] %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok)
        demo_failures++;
}

static void demo_run_check(const char *name, int (*test_fn)(void))
{
    int ok;

    ok = test_fn();
    demo_check(name, ok);
}

static int demo_test_long_long(void)
{
    long long a;
    long long b;
    long long sum;
    long long quotient;
    long long remainder;
    unsigned long long ua;
    unsigned long long ub;
    unsigned long long usum;

    a = 123456789012345LL;
    b = 1LL;
    sum = a + b;
    if (sum != 123456789012346LL)
        return 0;

    ua = 1000000ULL;
    ub = 2000000ULL;
    usum = ua + ub;
    if (usum != 3000000ULL)
        return 0;

    quotient = a / 1000LL;
    remainder = a % 1000LL;
    if (quotient != 123456789012LL)
        return 0;
    if (remainder != 345LL)
        return 0;

    if ((a << 1) != 246913578024690LL)
        return 0;
    if ((ua >> 1) != 500000ULL)
        return 0;

    return 1;
}

static int demo_test_bool(void)
{
    _Bool flag;
    bool flag2;
    int value;

    flag = true;
    flag2 = false;
    if (!flag)
        return 0;
    if (flag2)
        return 0;
    if (!(flag && true))
        return 0;
    if (false || !flag)
        return 0;

    value = (int)flag;
    if (value != 1)
        return 0;

    return 1;
}

static int demo_test_calling_conventions(void)
{
    long r1;
    long r2;

    r1 = demo_regargs_add(10L, 32L);
    r2 = demo_stdargs_add(10L, 32L);
    if (r1 != 42L)
        return 0;
    if (r2 != 42L)
        return 0;

    return 1;
}

static int demo_test_memory_sections(void)
{
    int total;

    total = demo_chip_data + demo_far_data + demo_near_data + demo_fast_data;
    if (total != 642)
        return 0;

    return 1;
}

static int demo_test_variadic_macro(void)
{
    DEMO_VLOG("macro x=%d", 42);
    DEMO_LOG("variadic macro path OK");
    return 1;
}

static long long demo_ll_add(long long x, long long y)
{
    return x + y;
}

static int demo_test_function_pointer(void)
{
    long long (*fn)(long long, long long);
    long long result;

    fn = demo_ll_add;
    result = fn(1000000000000LL, 23456789012345LL);
    if (result != 24456789012345LL)
        return 0;

    return 1;
}

static int demo_sum_array(int *values, int count)
{
    int i;
    int total;

    total = 0;
    for (i = 0; i < count; i++)
        total += values[i];
    return total;
}

static int demo_test_array_decay(void)
{
    int values[4];
    int total;

    values[0] = 1;
    values[1] = 2;
    values[2] = 3;
    values[3] = 4;
    total = demo_sum_array(values, 4);
    if (total != 10)
        return 0;

    return 1;
}

static int demo_test_integer_promotion(void)
{
    char ca;
    short sb;
    int result;

    ca = 10;
    sb = 32;
    result = ca + sb;
    if (result != 42)
        return 0;

    return 1;
}

static int demo_test_const_volatile(void)
{
    const char *message = "const OK";
    volatile int flag;

    flag = 1;
    if (message[0] != 'c')
        return 0;
    if (flag != 1)
        return 0;

    return 1;
}

static void demo_print_banner(void)
{
    printf("Hello from AC 4.0!\n");
    printf("AC feature demonstration (post-PDC fork)\n");
    printf("  Compiler: %s\n", DEMO_COMPILER_NAME);
#if defined(__STDC__)
    printf("  __STDC__=1\n");
#else
    printf("  __STDC__=0\n");
#endif
#if defined(__STDC_VERSION__)
    printf("  __STDC_VERSION__=%s\n", DEMO_STR(__STDC_VERSION__));
#else
    printf("  __STDC_VERSION__=(undefined)\n");
#endif
#if defined(__AMIGA__)
    printf("  __AMIGA__=1\n");
#endif
#if defined(__amigaos__)
    printf("  __amigaos__=1\n");
#endif
#if defined(__SASC__)
    printf("  __SASC__=1\n");
#endif
    printf("  Built: %s %s\n", __DATE__, __TIME__);
    printf("\n");
}

static void demo_print_compile_features(void)
{
    printf("Compile-time features verified by successful link:\n");
    printf("  #pragma once, #warning, variadic macros, nested #if,\n");
    printf("  SAS/C pragmas (libcall/flibcall/syscall/tagcall/msg),\n");
    printf("  __interrupt declaration, __chip/__far/__near/__fast\n");
    if (DEMO_NEST_OK)
        printf("  nested defined() probe: OK\n");
    else
        printf("  nested defined() probe: unexpected\n");
    printf("\n");
}

__interrupt static void demo_isr(void)
{
    /* Declared and compiled; must not be called from normal task context. */
}

int main(void)
{
    const int runtime_total = 10;

    demo_failures = 0;

    demo_pragma_anchor();

    demo_print_banner();

    printf("Runtime checks:\n");

    demo_run_check("long long arithmetic", demo_test_long_long);
    demo_run_check("bool logic", demo_test_bool);
    demo_run_check("calling conventions (__regargs/__stdargs)",
                     demo_test_calling_conventions);
    demo_run_check("memory section globals", demo_test_memory_sections);
    demo_run_check("variadic DEMO_VLOG macro", demo_test_variadic_macro);
    demo_run_check("function pointer call", demo_test_function_pointer);
    demo_run_check("array decay", demo_test_array_decay);
    demo_run_check("integer promotion", demo_test_integer_promotion);
    demo_run_check("const/volatile read", demo_test_const_volatile);

    printf("\n");
    demo_print_compile_features();

    printf("%d runtime checks passed, %d failed.\n",
           runtime_total - demo_failures, demo_failures);

    return demo_failures;
}
