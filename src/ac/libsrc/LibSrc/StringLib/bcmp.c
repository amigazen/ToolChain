/*
 - bcmp - Berklix equivalent of memcmp
 */

#include "config.h"

int				/* == 0 or != 0 for equality and inequality */
bcmp(s1, s2, length)
CONST char *s1;
CONST char *s2;	
int length;
{
	return(memcmp((CONST VOIDSTAR)s1, (CONST VOIDSTAR)s2, (SIZET)length));
}
