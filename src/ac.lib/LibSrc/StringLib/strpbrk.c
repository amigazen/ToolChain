/*
 * strpbrk - find first occurrence of any char from breakat in s
 */

#include "config.h"

#define	NULL	0

char *				/* found char, or NULL if none */
strpbrk(s, breakat)
CONST char *s;
CONST char *breakat;
{
	register CONST char *sscan;
	register CONST char *bscan;

	for (sscan = s; *sscan != '\0'; sscan++) {
		for (bscan = breakat; *bscan != '\0';)	/* ++ moved down. */
			if (*sscan == *bscan++)
				return(sscan);
	}
	return(NULL);
}
