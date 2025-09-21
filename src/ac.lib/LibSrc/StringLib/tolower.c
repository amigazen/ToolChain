/*
 * PDC I/O Library Copyright (C) 1988 by J.A. Lydiatt.
 * Freely Distributable for non-commercial use.
 */
char tolower(c)
char c;
{
	if ( 'A' <= c && c <= 'Z' )
		c += 'a' - 'A';
	return c;
}
