/*
 * feof.c - EOF test for cclib.library FILE
 *
 * cclib exposes feof as a macro on _flags & _EOF when using its stdio.h.
 * AC may still emit jsr _feof against older headers; match cclib FILE.
 */

#define AC_CCLIB_EOF (1 << 3)

struct ac_cc_FILE {
	long _unit;
	char *_bp;
	char *_bend;
	char *_buff;
	char _flags;
};

int feof(fp)
void *fp;
{
	struct ac_cc_FILE *f;

	if (fp == 0)
		return 0;
	f = (struct ac_cc_FILE *)fp;
	return (f->_flags & AC_CCLIB_EOF) != 0;
}
