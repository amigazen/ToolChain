/*
 * test_types_suffixes.c - C89 type-specifier lists and U/L/UL/LL/f/F/L suffixes.
 * Header-free for Mac-hosted AC.
 */

unsigned long int uli;
long unsigned int lui;
unsigned long long int ulli = 3ULL;
long long unsigned int llui = 4LLU;
signed short int ssi;
unsigned short int usi;
signed long int sli;
long int li;
unsigned int ui;
signed int si;
unsigned char uc;
signed char sc;
long double ld;
long long gll = 789LL;

unsigned long int
add_uli(unsigned long int a, long unsigned b)
{
    return a + b + 10UL + 10LU + 10U + 10L;
}

unsigned long long
add_ull(unsigned long long a)
{
    return a + 10ULL + 10LLU + 10LL;
}

int
main(void)
{
    unsigned long int x;
    long unsigned y;
    unsigned long z;
    long long a;
    unsigned long long b;

    x = 1UL;
    y = 2LU;
    z = 3U;
    a = 789LL;
    b = 101112ULL;
    uli = add_uli(x, y);
    lui = uli;
    ulli = add_ull(b);
    llui = ulli;
    gll = a;
    ssi = (signed short int)1;
    usi = (unsigned short int)2U;
    sli = (signed long int)3L;
    li = (long int)4L;
    ui = 5U;
    si = (signed int)6;
    uc = (unsigned char)7;
    sc = (signed char)8;
    ld = 1.0L;
    if (uli == 0UL)
        return 1;
    if (z == 0U)
        return 2;
    if (gll == 0LL)
        return 3;
    if ((int)a == 0)
        return 4;
    if ((int)(1.5f * 2.0f) != 3)
        return 5;
    if (sizeof(1.0f) != sizeof(float))
        return 6;
    if (sizeof(1.0L) != sizeof(long double))
        return 7;
    return 0;
}
