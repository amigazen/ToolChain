/*
 * strcpy - copy string src to dst
 */

#include "config.h"

char *				/* dst */
strcpy(dst, src)
char *dst;
CONST char *src;
{
	register char *dscan;
	register CONST char *sscan;

	dscan = dst;
	sscan = src;
	while ((*dscan++ = *sscan++) != '\0')
		continue;
	return(dst);
}
