/*
 * test_c89_sizeof.c - sizeof on types, arrays, and string literals (compile-only).
 */
struct s {
	char c;
	long l;
};

static char buf[10];

int
main(void)
{
	int a[4];
	int n;

	n = (int) sizeof(char);
	n = n + (int) sizeof(int);
	n = n + (int) sizeof(a);
	n = n + (int) sizeof(buf);
	n = n + (int) sizeof("hi");
	n = n + (int) sizeof("ab" "cd");
	n = n + (int) sizeof("");
	n = n + (int) sizeof(struct s);
	n = n + (int) sizeof(a[0]);
	return n;
}
