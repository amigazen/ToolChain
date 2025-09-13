
/*
 - bcopy - Berklix equivalent of memcpy
 */

#include "config.h"

bcopy(src, dst, length)
CONST char *src;
char *dst;
int length;
{
	extern VOIDSTAR memcpy();

	/*(void)*/ memcpy((VOIDSTAR)dst, (CONST VOIDSTAR)src, (SIZET)length);
}
