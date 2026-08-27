/*
 * test_c89_qualifiers.c - const / volatile pointer forms (compile-only).
 */
static int g;

int
main(void)
{
	const int *p1;
	int *const p2 = &g;
	volatile int v;
	const volatile int *p3;

	g = 1;
	p1 = &g;
	v = *p1;
	*p2 = v + 1;
	p3 = &v;
	return *p3 - 2;
}
