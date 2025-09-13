/*
 * Libraries and headers for PDC release 3.3 (C) 1989 Lionel Hummel.
 * PDC Software Distribution (C) 1989 Lionel Hummel and Paul Petersen.
 * PDC I/O Library (C) 1987 by J.A. Lydiatt.
 *
 * This code is freely redistributable upon the conditions that this 
 * notice remains intact and that modified versions of this file not
 * be included as part of the PDC Software Distribution without the
 * express consent of the copyright holders.  No warrantee of any
 * kind is provided with this code.  For further information, contact:
 *
 *  PDC Software Distribution    Internet:                     BIX:
 *  P.O. Box 4006             or hummel@cs.uiuc.edu            lhummel
 *  Urbana, IL  61801-8801       petersen@uicsrd.csrd.uiuc.edu
 */

/*  fprintf.c - various ways to output formatted strings
 *
 *  fprintf    prints a string to a buffered output stream
 *  printf     prints a formatted string to stdout
 *  sprintf    prints formatted a string into a given buffer
 */

#include <stdio.h>

extern int	format();

static FILE *currentStream;	/* Used by fprintf and printf */
static char *bufptr;		/* Used by sprintf. */

/*
 * This routine called by printf and fprintf to output one character.
 */

static int
output(c)
unsigned int c;
{
	return fputc(c, currentStream);
}

/*
 * This routing called by sprintf to output on character to the buffer.
 */

static int
putone(c)
unsigned int c;
{
	*bufptr++ = c;
	return c;
}

int
fprintf(fp, fmt, args)
FILE *fp;
char *fmt;
int  args;
{
	currentStream = fp;
	return format(output, fmt, &args);
}

int
printf( fmt, args )
char *fmt;
int  args;
{
	currentStream = stdout;
	return format(output, fmt, &args);
}

int
sprintf( linptr, fmt, args )
char *linptr;
char *fmt;
int args;
{
	int len;

	bufptr = linptr;
	len = format( putone, fmt, &args );
	*bufptr = '\0';
	return len;
}
