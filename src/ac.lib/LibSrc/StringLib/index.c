/*
 * index - find first occurrence of a character in a string
 */

#include "config.h"
#define	NULL	0

char *				/* found char, or NULL if none */
index(s, charwanted)
CONST char *s;
char charwanted;
{
	extern char *strchr();

	return(strchr(s, charwanted));
}
