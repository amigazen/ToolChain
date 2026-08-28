/*
 * test_static_assert.c - Mac-host smoke for _Static_assert / static_assert.
 * Header-free except for compile-time checks.
 */

_Static_assert(1, "one is true");
_Static_assert(sizeof(int) >= 2);
static_assert(1 + 1 == 2, "arithmetic");

int
main(void)
{
    static_assert(sizeof(char) == 1, "char is 1");
    _Static_assert(1);
    return 0;
}
