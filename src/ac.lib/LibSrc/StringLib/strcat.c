/*
 * strcat - append string src to dst
 */

#include "config.h"

char *				/* dst */
strcat(dst, src)
char *dst;
CONST char *src;
{
	register char *dscan;
	register CONST char *sscan;

	for (dscan = dst; *dscan != '\0'; dscan++)
		continue;
	sscan = src;
	while ((*dscan++ = *sscan++) != '\0')
		continue;
	return(dst);
}
