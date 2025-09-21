/*
 * strrchr - find last occurrence of a character in a string
 */

#include "config.h"

#define	NULL	0

char *				/* found char, or NULL if none */
strrchr(s, charwanted)
CONST char *s;
register char charwanted;
{
	register CONST char *scan;
	register CONST char *place;

	place = NULL;
	for (scan = s; *scan != '\0'; scan++)
		if (*scan == charwanted)
			place = scan;
	if (charwanted == '\0')
		return(scan);
	return(place);
}
