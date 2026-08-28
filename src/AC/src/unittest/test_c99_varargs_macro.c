/*
 * test_c99_varargs_macro.c - __VA_ARGS__ (compile-only).
 */
#define COUNT1(...) 1
#define WRAP(a, ...) ((a) + COUNT1(__VA_ARGS__))
#define STR(x) #x

static char *s = STR(hello);

int
main(void)
{
	int n;

	n = WRAP(2, x, y, z);
	return n + (int) sizeof(STR(ab)) - 3 + (s[0] - 'h');
}
