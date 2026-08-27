/*
 * test_line_comments.c - C99 // line comments (GCC-compatible).
 *
 * Block comments stay C89 (first star-slash ends; no nesting).
 */

int printf();

static int
line_comment_value()
{
    int x;

    x = 0;
    // assign below must run
    x = 42;
    return x;
}

static int
block_then_code()
{
    int y;

    y = 1;
    /* plain block comment (globs like *.c in comments must stay safe) */
    y = y + 1; // trailing line comment
    return y;
}

#define LINE_CMT_MACRO 7 // stripped from macro body

int
main()
{
    if (line_comment_value() != 42)
        return 1;
    if (block_then_code() != 2)
        return 2;
    if (LINE_CMT_MACRO != 7)
        return 3;
    /* paths like .FD / .FS in comments must not end the comment early */
    return 0;
}
