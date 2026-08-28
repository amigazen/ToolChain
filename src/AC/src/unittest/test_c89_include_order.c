/*
 * test_c89_include_order.c - document/compile-check F.3.13 quoted order.
 * Compiles with a local header; full cwd-vs-subdir order is exercised on
 * Amiga when building the suite from src/ (cwd = src, includes use
 * unittest/... paths).
 */
#include "cli_inc/cli_marker.h"

#ifndef CLI_MARKER_OK
#error expected cli_inc/cli_marker.h via quoted include
#endif

int
main(void)
{
	return CLI_MARKER_OK;
}
