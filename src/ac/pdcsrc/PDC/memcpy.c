
/*
 * memcpy - copy bytes
 */

#include "config.h"

VOIDSTAR
memcpy(dst, src, size)
VOIDSTAR dst;
CONST VOIDSTAR src;
SIZET size;
{
	register char *d;
	register CONST char *s;
	register SIZET n;

	if (size <= 0)
		return(dst);

	s = src;
	d = dst;
	if (s <= d && s + (size-1) >= d) {
		/* Overlap, must copy right-to-left. */
		s += size-1;
		d += size-1;
		for (n = size; n > 0; n--)
			*d-- = *s--;
	} else
		for (n = size; n > 0; n--)
			*d++ = *s++;

	return(dst);
}
