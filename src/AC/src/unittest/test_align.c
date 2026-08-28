/*
 * test_align.c - Mac-host smoke for alignof / alignas.
 */

alignas(4) char g_pad;
_Alignas(8) long g_big;
alignas(int) char g_like_int;

static_assert(alignof(char) == 1, "char align");
static_assert(_Alignof(short) == 2, "short align");
static_assert(alignof(long) == 2, "long align");
static_assert(alignof(g_pad) >= 1, "aligned char");

int
main(void)
{
    alignas(4) char local;
    int             a;

    a = (int) alignof(int);
    a += (int) _Alignof(double);
    a += (int) sizeof(local);
    return a != 0 ? 0 : 1;
}
