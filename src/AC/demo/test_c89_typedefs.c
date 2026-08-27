/*
 * test_c89_typedefs.c - typedef to array, function pointer, struct.
 */
typedef int int10[10];
typedef int (*intfunc)();
typedef struct c89_node {
	int v;
	struct c89_node *next;
} c89_node;

static int
inc(n)
	int n;
{
	return n + 1;
}

int
main(void)
{
	int10 a;
	intfunc f;
	c89_node n;
	int i;

	i = 0;
	a[0] = 1;
	a[9] = 9;
	f = inc;
	n.v = (*f)(a[0]);
	n.next = 0;
	i = n.v + a[9];
	return i - 11;
}
