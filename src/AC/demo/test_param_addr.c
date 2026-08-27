/*
 * test_param_addr.c - & of char/short parameters must address the value.
 *
 * Historical PDC bug: char/short params were widened to long in the SYM,
 * so &c pointed at the high byte of a big-endian slot.  Assign through
 * the pointer and read back the parameter.
 */

static int
set_char_param(c, v)
    char c;
    int v;
{
    char *p;

    p = &c;
    *p = (char) v;
    return (int) (unsigned char) c;
}

static int
set_short_param(s, v)
    short s;
    int v;
{
    short *p;

    p = &s;
    *p = (short) v;
    return (int) s;
}

static int
sizeof_char_param(c)
    char c;
{
    return (int) sizeof(c);
}

static int
sizeof_short_param(s)
    short s;
{
    return (int) sizeof(s);
}

int
main(void)
{
    if (set_char_param(0, 0x5A) != 0x5A)
        return 1;
    if (set_short_param(0, 0x1234) != 0x1234)
        return 2;
    if (sizeof_char_param(0) != 1)
        return 3;
    if (sizeof_short_param(0) != 2)
        return 4;
    return 0;
}
