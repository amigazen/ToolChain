/*
 * PDC I/O Library Copyright (C) 1987 by J.A. Lydiatt.
 * Freely Distributable for non-commercial use.
 */
/*
 *  _main() calls this routine to parse the command line passed from
 *  the CLI.  If your program does't need parameters, you can substitute
 *  a dummy _cli_parse() routine such as:
 *
 *      "_cli_parse { return; }"
 * 
 *  in place of this one - makes for a shorter program!
 *
 *  This routine fills in the global variables:-
 * 
 *  _argc		- the number of arguments.
 *  _argv		- array of pointers to argument strings.
 *  _argBuf		- buffer to store arguments (allocated here).
 *  _argBufSize	- size of the buffer allocated. 
 *
 *  Any storage allocated here will be freed by _exit().
 * 
 */

/*
 * Maintenance Notes:
 * 10Apr88 - (Jal) Note that AmigaDos seems to strip out double "" marks
 *           so although this routine should handle them (I've no way of
 *           testing it), it doesn't!
 */

#define iswhite(c)  ( c == ' ' || c == '\t' )
#include <libraries/dosextens.h>

typedef struct Process PROCESS;
typedef struct CommandLineInterface CLI;

extern int _argc, _argBufSize;
extern char **_argv, *_argBuf;

_cli_parse(tp, cmdlen, cmdptr)
PROCESS *tp;			/* pointer to task process */
long cmdlen;			/* length of command line  */
char *cmdptr;			/* pointer to AmigaDos command line */ 
{
	char *s, *argBufptr, delim;
	CLI *cli;
	int c;
	extern void *AllocMem();

	/*
	 * Allocate argBuff; allow space for the command's name.
	 */

	cli = (long)tp->pr_CLI << 2;
	s = (char *)( (long)cli->cli_CommandName << 2);
	_argBufSize = s[0] + cmdlen + 2;
	if ( (_argBuf = AllocMem( (long)_argBufSize, 0L)) == NULL )
		return;


	strncpy(_argBuf, &s[1], s[0]);
	strcpy(_argBuf + s[0], " ");
	strncat(_argBuf, cmdptr, (int)cmdlen);

	/*
	 * Parse the augmented command line.
	 */

	_argc = 0;
	argBufptr = _argBuf;
	for ( s=_argBuf; *s ; ++_argc) {
		while ( (c = *s) && iswhite(c) )
			++s;

		if ( *s < '\x1B' )
			break;

		/*
		 * If in quote mode, just copy everything up to the ending quote.
		 * A double quote is interpreted as a single quote.
		 */

		if ( *s == '"' || *s == '\'' ) {
			delim = *s++;
			while ( *s ) {
				while ( *s == delim && s[1] == delim ) {
					*argBufptr++ = delim;
					s += 2;
				}
				if ( *s == delim ) {
					++s;
					break;
				}
				while ( *s && *s != delim ) {
					*argBufptr++ = *s;
					++s;
				}
			}
		} else {
			 while ( (c = *s++) && !iswhite(c) )
				*argBufptr++ = c;

		}
		*argBufptr++ = '\0';
	}	/* end for */

	*argBufptr = '\0';
	if ( (_argv = AllocMem((long)(_argc+1)*sizeof(*_argv), 0L)) == NULL ) {
		_argc = 0;
		return;
	}

	s = _argBuf;
	for (c=0; c<_argc; ++c) {
		_argv[c] = s;
		s += strlen(s) + 1;
	}
	_argv[c] = 0;
}
