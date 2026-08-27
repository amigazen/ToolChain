/*
 * test_c89_decl_edge.c - declarator / storage-class edge cases (compile-only).
 */
typedef unsigned uint_t;
typedef int (*cmp_fn)();

struct node {
	int val;
	struct node *next;
};

static int g;
static int *rp = &g;

static int
add2(a, b)
	int a;
	int b;
{
	return a + b;
}

int
main(void)
{
	register int r;
	int x;
	int *p;
	int **pp;
	int a[2][3];
	int (*fp)();
	uint_t u;
	struct node n;
	cmp_fn cf;
	char *msg;

	r = 1;
	x = r;
	p = &x;
	pp = &p;
	**pp = 2;
	a[1][2] = 9;
	fp = add2;
	u = (uint_t) fp(3, 4);
	n.val = (int) u;
	n.next = &n;
	cf = add2;
	msg = "hello" " " "world";
	rp = &n.val;
	return n.next->val + a[1][2] + cf(0, 0) + (int) msg[0] + *rp + r;
}
