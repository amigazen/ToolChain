/*
 * test_c99_bool.c - _Bool / bool (compile-only).
 */
#include <stdbool.h>

static _Bool gb;
static bool gb2;

int
main(void)
{
	_Bool a;
	bool b;
	int n;

	a = 0;
	b = true;
	gb = a;
	gb2 = b;
	n = (int) sizeof(_Bool) + (int) sizeof(bool);
	if (b)
		n = n + 1;
	if (!a)
		n = n + 1;
	return n + (int) gb2;
}
