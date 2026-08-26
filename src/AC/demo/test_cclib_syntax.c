/*
 * Syntaxes required by cclib.library public headers (libraries/cclib.h,
 * stdlib.h).  Compile+link smoke for the demo suite.
 */

/* Incomplete tag typedef (cclib DIR) */
typedef struct cc_DIR DIR;

/* Function-pointer typedef with SAS/C calling-convention keyword */
typedef void __stdargs (*ABORT_FUNC)(long);
typedef void (*PLAIN_FUNC)(long);

/* Named function-pointer object */
void (*exit_fcn)(void);

struct cc_DIR {
	int dummy;
};

static void
on_exit_cb()
{
}

/*
 * Abstract function-pointer parameter (atexit-style).  Keep this as a
 * local function so the demo does not emit a live XREF _atexit for Blink.
 */
static int
take_func(func)
	void (*func)();
{
	if (func == 0)
		return 0;
	return 1;
}

ABORT_FUNC abort_hook;
PLAIN_FUNC plain_hook;
DIR *dirp;

int
main()
{
	exit_fcn = on_exit_cb;
	abort_hook = 0;
	plain_hook = 0;
	dirp = 0;
	if (take_func(on_exit_cb) == 0)
		return 1;
	return 0;
}
