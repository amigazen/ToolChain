/*
 * test_c89_stringize.c - # stringize operator (compile-only).
 */
#define STR(x) #x

static char *s = STR(hello);

int
main(void)
{
	/* "hello" is 6 bytes including NUL; touch s so it is not unused. */
	return (int) sizeof(STR(ab)) - 3 + (s[0] - 'h');
}
