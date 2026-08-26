/*
 * host_posix.c - minimal POSIX host stubs for building AC on macOS/Linux.
 *
 * open_stdio() and close_stdio() were referenced from Cmain.c on historical
 * SunOS builds but never shipped in the PDC tree.  Empty stubs are enough for
 * modern hosts where stdio needs no special setup.
 */

#include "host_posix.h"

#if defined(AC_HOST_POSIX)

void
open_stdio(void)
{
}

void
close_stdio(void)
{
}

#endif /* AC_HOST_POSIX */
