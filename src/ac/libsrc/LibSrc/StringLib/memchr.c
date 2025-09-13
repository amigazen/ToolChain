/*
 * memchr - search for a byte
 *
 * CHARBITS should be defined only if the compiler lacks "unsigned char".
 * It should be a mask, e.g. 0377 for an 8-bit machine.
 */

#include "config.h"

#define	NULL	0

#ifndef CHARBITS
#	define	UNSCHAR(c)	((unsigned char)(c))
#else
#	define	UNSCHAR(c)	((c)&CHARBITS)
#endif

VOIDSTAR
memchr(s, ucharwanted, size)
CONST VOIDSTAR s;
char ucharwanted;
SIZET size;
{
	register CONST char *scan;
	register SIZET n;
	register char uc;

	scan = s;
	uc = UNSCHAR(ucharwanted);
	for (n = size; n > 0; n--)
		if (UNSCHAR(*scan) == uc)
			return(scan);
		else
			scan++;

	return(NULL);
}
