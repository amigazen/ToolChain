/*
 * test_c99_longlong.c - long long declarations and literals (compile-only).
 */
static long long g_ll;
static unsigned long long g_ull;

int
main(void)
{
	long long a;
	unsigned long long b;
	int n;

	a = 1LL;
	b = 2ULL;
	g_ll = a + 3LL;
	g_ull = b + 4ULL;
	n = (int) sizeof(long long) + (int) sizeof(a) + (int) sizeof(b);
	n = n + (int) sizeof(1LL) + (int) sizeof(2ULL);
	return n + (int) g_ll + (int) g_ull;
}
