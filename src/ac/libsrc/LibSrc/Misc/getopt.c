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

/*
 * GetOpt.c Parse command line options
 * 
 * Modification history (LDH): optsign variable added, along with the ability to
 * distinguish between '+arg' and '-arg'
 */

#include    <stdio.h>

#define ERR(spat, cpat)   \
    if (opterr) {   errbuf[0] = cpat; errbuf[1] = '\n';     \
    (void) fwrite( argv[0], 1, strlen(argv[0]), stderr);    \
    (void) fwrite( spat, 1, strlen(spat), stderr);              \
    (void) fwrite( errbuf, 1, 2, stderr); }

extern int      strcmp();
extern char    *index();
extern int      strlen();

int   opterr = 1;
int   optind = 1;
int   optopt;
char  optsign;
char  optqual;
char  *optarg;

int
getopt(argc, argv, opts)
    int             argc;
    char          **argv, *opts;
{
    static int      sp = 1;
    register int    c;
    register char  *cp;
    char            errbuf[2];

    if (sp == 1) {
        if (optind >= argc)
            return( EOF );
        optsign = *argv[optind];
        if ( (optsign != '-' && optsign != '+') || argv[optind][1] == '\0')
            return (EOF);
        else if (strcmp(argv[optind], "--") == NULL) {
            optind++;
            return (EOF);
        }
    }
    optopt = c = argv[optind][sp];
    if (c == ':' || (cp = index(opts, c)) == NULL) {
        ERR(": illegal option -- ", c);
        if (argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return ('?');
    }

    if (*++cp == ';')
        optqual = argv[optind][++sp];

    if (*cp == ':' || *cp == ';') {
        if (argv[optind][sp + 1] != '\0')
            optarg = &argv[optind++][sp + 1];
        else if (++optind >= argc) {
            ERR(": option requires an argument -- ", c);
            sp = 1;
            return ('?');
        }
        else
            optarg = argv[optind++];
        sp = 1;
    }
    else {
        if (argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return (c);
}
