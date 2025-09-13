/*
 * strstr - find first occurrence of wanted in s
 */

#include "config.h"

#define	NULL	0

char *				/* found string, or NULL if none */
strstr(s, wanted)
CONST char *s;
CONST char *wanted;
{
	register CONST char *scan;
	register SIZET len;
	register char firstc;
	extern int strcmp();
	extern SIZET strlen();

	/*
	 * The odd placement of the two tests is so "" is findable.
	 * Also, we inline the first char for speed.
	 * The ++ on scan has been moved down for optimization.
	 */
	firstc = *wanted;
	len = strlen(wanted);
	for (scan = s; *scan != firstc || strncmp(scan, wanted, len) != 0; )
		if (*scan++ == '\0')
			return(NULL);
	return(scan);
}
