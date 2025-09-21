
#ifdef PDC
#include	<xstdio.h>
#else
#include 	<stdio.h>
#endif

#define ERR(spat, cpat)   \
	if (opterr) {	errbuf[0] = cpat; errbuf[1] = '\n';		\
    (void) fwrite( argv[0], 1, strlen(argv[0]), stderr);	\
    (void) fwrite( spat, 1, strlen(spat), stderr);				\
    (void) fwrite( errbuf, 1, 2, stderr); }

extern int      strcmp();
extern char    *index();
extern int      strlen();

int             opterr = 1;
int             optind = 1;
int             optopt;
char           *optarg;

int
getopt(argc, argv, opts)
    int             argc;
    char          **argv, *opts;
{
    static int      sp = 1;
    register int    c;
    register char  *cp;
    char            errbuf[2];

    if (sp == 1)
        if (optind >= argc ||
            argv[optind][0] != '-' || argv[optind][1] == '\0')
            return (EOF);
        else if (strcmp(argv[optind], "--") == NULL) {
            optind++;
            return (EOF);
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
    if (*++cp == ':') {
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
