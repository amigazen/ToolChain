/*
 * PDC I/O Library Copyright (C) 1988 by J.A. Lydiatt.
 * Freely Distributable for non-commercial use.
 */
char toupper(c)
char c;
{
	if ( 'a' <= c && c <= 'z' )
		c += 'A' - 'a';
	return c;
}
