/*
 * test_bss_arrays.c - global array BSS must be count * elemsize, not count.
 *
 * Under ac-self, a bare tp->size count used to emit DS.b 10 for int a[10]
 * and char *p[10].  Compile-check: demos Makefile greps the .s for DS.b 40.
 * Runtime sizes are covered in ac_tests.c (bss/*).
 */

int g_int10[10];
char *g_ptr10[10];
long g_long5[5];
short g_short8[8];
char g_char16[16];
int *g_intptr4[4];

static int
sum_int10()
{
    int i;
    int s;

    s = 0;
    for (i = 0; i < 10; i++)
        s += g_int10[i];
    return s;
}

static int
touch_ptrs()
{
    int i;

    for (i = 0; i < 10; i++)
        g_ptr10[i] = 0;
    for (i = 0; i < 4; i++)
        g_intptr4[i] = 0;
    return 0;
}

int
main(void)
{
    /*
     * sizeof checks — if BSS was undersized, writing these at runtime
     * would smash neighbors; compile+link is the main gate on Amiga.
     */
    if (sizeof(g_int10) != 40)
        return 1;
    if (sizeof(g_ptr10) != 40)
        return 2;
    if (sizeof(g_long5) != 20)
        return 3;
    if (sizeof(g_short8) != 16)
        return 4;
    if (sizeof(g_char16) != 16)
        return 5;
    if (sizeof(g_intptr4) != 16)
        return 6;
    (void) sum_int10();
    (void) touch_ptrs();
    return 0;
}
