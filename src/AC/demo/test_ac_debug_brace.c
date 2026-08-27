/*
 * test_ac_debug_brace.c - document the AC_DEBUG dangling-if rule.
 *
 * make_autocon historically did:
 *   if (omit_frame && lab < 0)
 *   #if AC_DEBUG
 *       fprintf(...);
 *   #endif
 *   ap1 = xalloc(...);
 * With AC_DEBUG off, xalloc became the if-body and framed functions
 * skipped allocation.  C.h now documents the brace rule; this file is a
 * compile smoke that still builds with AC_DEBUG 0 or 1.
 */

#if AC_DEBUG
static int g_debug_on = 1;
#else
static int g_debug_on = 0;
#endif

static int
maybe_diag(cond)
    int cond;
{
    /*
     * Correct pattern: #if wraps only the diagnostic, if is braced or
     * the whole if is inside #if AC_DEBUG.
     */
    if (cond) {
#if AC_DEBUG
        /* would be fprintf(AC_DIAG_STREAM, ...) in compiler sources */
        g_debug_on = g_debug_on;
#endif
    }
    return g_debug_on;
}

int
main(void)
{
    return maybe_diag(0) >= 0 ? 0 : 1;
}
