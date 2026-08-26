/*
 * GetOpt.c Parse command line options
 *
 * Modification history (LDH): optsign variable added, along with the ability to
 * distinguish between '+arg' and '-arg'
 *
 * Self-host note: ac-self collapsed locals `c` and `cp` into one register, so
 * after strchr() the option letter was lost and getopt returned a pointer.
 * Keep the letter only in global optopt and return that.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "C.h"

#define ERR(spat, cpat)   \
    if (opterr) {   errbuf[0] = (char)(cpat); errbuf[1] = '\n';     \
    (void) fwrite( argv[0], 1, strlen(argv[0]), AC_DIAG_STREAM);    \
    (void) fwrite( spat, 1, strlen(spat), AC_DIAG_STREAM);              \
    (void) fwrite( errbuf, 1, 2, AC_DIAG_STREAM); }

int             opterr = 1;
int             optind = 1;
int             optopt;
char            optsign;
char           *optarg;

int
getopt(argc, argv, opts)
    int             argc;
    char          **argv;
    char           *opts;
{
    static int      sp = 1;
    char           *cp;
    char            errbuf[2];

    if (sp == 1) {
        if (optind >= argc)
            return (EOF);
        optsign = *argv[optind];
        if ((optsign != '-' && optsign != '+') || argv[optind][1] == '\0')
            return (EOF);
        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return (EOF);
        }
    }

    /* Option letter lives in optopt (global) — not a local that strchr can clobber. */
    optopt = (unsigned char) argv[optind][sp];

    cp = strchr(opts, optopt);
    if (optopt == ':' || cp == NULL) {
        ERR(": illegal option -- ", optopt);
        sp++;
        if (argv[optind][sp] == '\0') {
            optind++;
            sp = 1;
        }
        return ('?');
    }

    cp++;
    if (*cp == ':') {
        if (argv[optind][sp + 1] != '\0')
            optarg = &argv[optind++][sp + 1];
        else if (++optind >= argc) {
            ERR(": option requires an argument -- ", optopt);
            sp = 1;
            return ('?');
        }
        else
            optarg = argv[optind++];
        sp = 1;
    }
    else {
        sp++;
        if (argv[optind][sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }
    return (optopt);
}
