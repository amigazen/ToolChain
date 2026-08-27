/*
 * test_c89_token_paste.c - ## token paste (compile-only).
 */
#define CAT(a, b) a##b

static int CAT(var_, x) = 12;

int
main(void)
{
	return CAT(1, 2) + var_x;
}
