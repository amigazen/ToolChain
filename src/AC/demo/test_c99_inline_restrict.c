/*
 * test_c99_inline_restrict.c - inline / restrict accepted (compile-only).
 */
static inline int
add_i(a, b)
	int a;
	int b;
{
	return a + b;
}

static int
sum_r(p, q)
	int *restrict p;
	int *restrict q;
{
	return *p + *q;
}

int
main(void)
{
	int x;
	int y;
	int *restrict rp;

	x = 3;
	y = 4;
	rp = &x;
	return add_i(x, y) + sum_r(rp, &y) + *rp;
}
