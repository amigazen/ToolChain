/*
 * cli_probe.c - source used by the AC command-line option smoke.
 *
 * Requires -DCLI_MUST_BE_SET (any value) and -I pointing at cli_inc/.
 * Avoids #if comparisons: -n (optimize off) currently breaks those in the
 * preprocessor; value checks belong in a separate known-bug test later.
 */

#ifndef CLI_MUST_BE_SET
#error CLI_MUST_BE_SET not defined (need -DCLI_MUST_BE_SET=...)
#endif

#ifdef CLI_MUST_BE_UNDEF
#error CLI_MUST_BE_UNDEF should be undefined (use -U after -D)
#endif

#include "cli_marker.h"

#ifndef CLI_MARKER_OK
#error cli_marker.h not found (need -I.../cli_inc)
#endif

int
main(void)
{
    return 0;
}
