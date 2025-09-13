/*
 * rindex - find last occurrence of a character in a string
 */

#include "config.h"

#define	NULL	0

char *				/* found char, or NULL if none */
rindex(s, charwanted)
CONST char *s;
char charwanted;
{
	extern char *strrchr();

	return(strrchr(s, charwanted));
}
