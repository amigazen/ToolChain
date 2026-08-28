/*
 * test_static_assert_fail.c - must produce a static assertion error.
 */
_Static_assert(0, "expected failure");
int main(void) { return 0; }
