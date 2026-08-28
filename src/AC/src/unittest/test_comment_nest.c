/*
 * test_comment_nest.c - SAS/C COMMENTNEST (requires -Wcommentnest).
 *
 * Without -Wcommentnest, the inner slash-star would end the comment early.
 */

int
main()
{
    int x;

    x = 0;
    /* outer /* inner */ x stays 0 until here */
    x = 1;
    if (x != 1)
        return 1;
    return 0;
}
