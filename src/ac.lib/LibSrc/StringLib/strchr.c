/*
 * strchr - find first occurrence of a character in a string
 */

#include "config.h"

#define	NULL	0

char *				/* found char, or NULL if none */
strchr(s, charwanted)
CONST char *s;
register char charwanted;
{
	register CONST char *scan;

	/*
	 * The odd placement of the two tests is so NUL is findable.
	 */
	for (scan = s; *scan != charwanted;)	/* ++ moved down for opt. */
		if (*scan++ == '\0')
			return(NULL);
	return(scan);
}
