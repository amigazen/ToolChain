/*
 *	Read in makefile
 */


#include <stdio.h>
#include	<ctype.h>
#include "h.h"


int             lineno;


/*
 *	Syntax error handler.  Print message, with line number, and exits.
 */
void
error(msg, a1, a2, a3)
    char           *msg;
{
    fprintf(stderr, "%s: ", myname);
    fprintf(stderr, msg, a1, a2, a3);
    if (lineno)
	fprintf(stderr, " near line %d", lineno);
    fputc('\n', stderr);
    exit(1);
}


/*
 *	Read a line into the supplied string of length LZ.  Remove
 *	comments, ignore blank lines. Deal with	quoted (\) #, and
 *	quoted newlines.  If EOF return TRUE.
 */
bool
getline(str, fd)
    char           *str;
    FILE           *fd;
{
    register char  *p;
    char           *q;
    char           *point;
    int             pos = 0;


    for (;;) {

	point = str + pos;

	if (fgets( point, LZ - 1 - pos, fd) == (char *) 0)
	    return TRUE;	/* EOF */

	lineno++;

	if ((p = index( point, '\n')) == (char *) 0) {

            if (feof(fd)) {		/* Fake a final newline if missing */
                strcat( point, "\n");
                p = index( point, '\n');
            }
	    else
                error("Line too long");
        }

	if (p[-1] == '\\') {	/* This won't work if there are trailing chars */
	    p[-1] = '\n';
	    pos = p - str;
	    continue;
	}
	p = str;
	while (((q = index(p, '#')) != (char *) 0) &&
	       (p != q) && (q[-1] == '\\')) {
	    char           *a;

	    a = q - 1;		/* Del \ chr; move rest back  */
	    p = q;
	    while (*a++ = *q++);
	}
	if (q != (char *) 0) {
	    q[0] = '\n';
	    q[1] = '\0';
	}
	p = str;
	while (isspace(*p))	/* Checking for blank  */
	    p++;

	if (*p != '\0')
	    return FALSE;
	pos = 0;
    }
}


/*
 *	Get a word from the current line, surounded by white space.
 *	return a pointer to it. String returned has no white spaces
 *	in it.
 */
char           *
gettok(ptr)
    char          **ptr;
{
    register char  *p;


    while (isspace(**ptr))	/* Skip spaces  */
	(*ptr)++;

    if (**ptr == '\0')		/* Nothing after spaces  */
	return NULL;

    p = *ptr;			/* word starts here  */

    while ((**ptr != '\0') && (!isspace(**ptr)))
	(*ptr)++;		/* Find end of word  */

    *(*ptr)++ = '\0';		/* Terminate it  */

    return (p);
}
