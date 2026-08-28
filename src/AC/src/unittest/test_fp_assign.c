/*
 * test_fp_assign.c - float/double += / -= / *= / /= and ++ / --.
 *
 * Historical bug: float postfix ++/-- (gen_fsaincdec) never stored the
 * updated value back to the lvalue.  Compound assign goes through
 * assign + fadd/fmul (etc.); prefix ++/-- uses the same path.
 */

static long
float_to_long(f)
    float f;
{
    return (long) f;
}

static long
double_to_long(d)
    double d;
{
    return (long) d;
}

static int
test_float_ops()
{
    float f;
    long oldv;

    /* f/F suffix is C99; AC must accept it (getfloatsuffix). */
    f = 10.0f;
    f += 3.0f;
    if (float_to_long(f) != 13L)
        return 1;

    f -= 5.0f;
    if (float_to_long(f) != 8L)
        return 2;

    f *= 2.0f;
    if (float_to_long(f) != 16L)
        return 3;

    f /= 4.0f;
    if (float_to_long(f) != 4L)
        return 4;

    f = 7.0f;
    oldv = float_to_long(f++);
    if (oldv != 7L || float_to_long(f) != 8L)
        return 5;

    oldv = float_to_long(f--);
    if (oldv != 8L || float_to_long(f) != 7L)
        return 6;

    ++f;
    if (float_to_long(f) != 8L)
        return 7;

    --f;
    if (float_to_long(f) != 7L)
        return 8;

    return 0;
}

static int
test_double_ops()
{
    double d;
    long oldv;

    d = 10.0;
    d += 3.0;
    if (double_to_long(d) != 13L)
        return 11;

    d -= 5.0;
    if (double_to_long(d) != 8L)
        return 12;

    d *= 2.0;
    if (double_to_long(d) != 16L)
        return 13;

    d /= 4.0;
    if (double_to_long(d) != 4L)
        return 14;

    d = 7.0;
    oldv = double_to_long(d++);
    if (oldv != 7L || double_to_long(d) != 8L)
        return 15;

    oldv = double_to_long(d--);
    if (oldv != 8L || double_to_long(d) != 7L)
        return 16;

    ++d;
    if (double_to_long(d) != 8L)
        return 17;

    --d;
    if (double_to_long(d) != 7L)
        return 18;

    return 0;
}

int
main(void)
{
    int r;

    r = test_float_ops();
    if (r != 0)
        return r;
    r = test_double_ops();
    if (r != 0)
        return r;
    return 0;
}
