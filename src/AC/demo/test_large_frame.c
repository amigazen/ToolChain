/*
 * test_large_frame.c - automatic storage beyond the 68000 ù32K (d16,An).
 *
 * Historical limit: ICON16L truncated frame offsets and link #d, so locals
 * past 32K were wrong.  Expect link #0 + suba.l #N,A7 and correct end access.
 */

static const char *test_large_frame_stack = "$STACK: 65536";

#define FRAME_BYTES 34000

static int
touch_ends()
{
    char            buf[FRAME_BYTES];
    int             i;

    if (test_large_frame_stack[0] == 0)
        return -1;

    buf[0] = 1;
    buf[FRAME_BYTES - 1] = 2;
    /* Touch a spot past the old 32K boundary. */
    i = 33000;
    buf[i] = 3;
    return (int) buf[0] + (int) buf[FRAME_BYTES - 1] + (int) buf[i];
}

int
main(void)
{
    if (touch_ends() != 6)
        return 1;
    return 0;
}
