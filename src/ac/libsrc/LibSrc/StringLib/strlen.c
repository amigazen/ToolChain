/*
 * strlen - length of string (not including NUL)
 */

#include "config.h"

SIZET
strlen(s)
CONST char *s;
{
	register CONST char *scan;
	register SIZET count;

	count = 0;
	scan = s;
	while (*scan++ != '\0')
		count++;
	return(count);
}
