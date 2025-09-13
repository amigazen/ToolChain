/*
 * memccpy - copy bytes up to a certain char
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
memccpy(dst, src, ucharstop, size)
VOIDSTAR dst;
char  ucharstop;
CONST VOIDSTAR src;
SIZET size;
{
	register char *d;
	register CONST char *s;
	register SIZET n;
	register char uc;

	if (size <= 0)
		return(NULL);

	s = src;
	d = dst;
	uc = UNSCHAR(ucharstop);
	for (n = size; n > 0; n--)
		if (UNSCHAR(*d++ = *s++) == uc)
			return(d);

	return(NULL);
}
