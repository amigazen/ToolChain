/*
 * ParseCc.c - POSIX/gcc-style option parsing when argv[0] basename is "cc".
 * Maps onto the same Options / define / include state as the ac front-end.
 */

#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include    "host_posix.h"
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    "FrontEnd.h"

extern char     outfile[40];
extern char    *progname;
extern void     usage();

static int
cc_is_opt_need_arg(a)
char           *a;
{
    if (strcmp(a, "-o") == 0 || strcmp(a, "-I") == 0 ||
        strcmp(a, "-D") == 0 || strcmp(a, "-U") == 0 ||
        strcmp(a, "-std") == 0)
        return 1;
    return 0;
}

static void
cc_bad_opt(opt)
char           *opt;
{
    fprintf(AC_DIAG_STREAM, "%s: error: unrecognized command line option '%s'\n",
            progname != 0 ? progname : "cc", opt);
    exit(1);
}

static void
cc_missing_arg(opt)
char           *opt;
{
    fprintf(AC_DIAG_STREAM,
            "%s: error: missing argument to '%s'\n",
            progname != 0 ? progname : "cc", opt);
    exit(1);
}

/*
 * Parse gcc-like options. Returns index of first non-option argument
 * (input file), or argc if none. Does not use getopt (ac -O is outfile).
 */
int
parse_cc_args(argc, argv)
int             argc;
char          **argv;
{
    int             i;
    char           *a;
    char           *arg;

    i = 1;
    while (i < argc) {
        a = argv[i];
        if (a == 0 || a[0] != '-' || strcmp(a, "-") == 0)
            break;

        if (strcmp(a, "-help") == 0 || strcmp(a, "--help") == 0 ||
            strcmp(a, "-?") == 0) {
            usage();
            /* not reached */
        }

        if (strcmp(a, "-c") == 0) {
            OPT_REF(OPT_OFF_CompileOnly) = 1;
            i++;
            continue;
        }
        if (strcmp(a, "-E") == 0) {
            OPT_REF(OPT_OFF_PreprocessOnly) = 1;
            i++;
            continue;
        }
        if (strcmp(a, "-g") == 0) {
            OPT_REF(OPT_OFF_Debug) = 1;
            i++;
            continue;
        }
        if (strcmp(a, "-w") == 0) {
            /* Accepted; warning suppression not wired yet. */
            i++;
            continue;
        }
        if (strcmp(a, "-ansi") == 0) {
            i++;
            continue;
        }
        if (strcmp(a, "-S") == 0) {
            /* gcc -S is assemble-only; AC -S is stack check — ignore. */
            fprintf(AC_DIAG_STREAM,
                    "%s: warning: '-S' ignored in cc mode\n",
                    progname != 0 ? progname : "cc");
            i++;
            continue;
        }
        if (strcmp(a, "-Werror") == 0) {
            OPT_REF(OPT_OFF_WarningsAsErrors) = 1;
            i++;
            continue;
        }
        if (strncmp(a, "-std=", 5) == 0) {
            /* -std=c89 / c90 accepted; other values warn. */
            if (strcmp(a + 5, "c89") != 0 && strcmp(a + 5, "c90") != 0 &&
                strcmp(a + 5, "c") != 0) {
                fprintf(AC_DIAG_STREAM,
                        "%s: warning: '-std=%s' not fully supported\n",
                        progname != 0 ? progname : "cc", a + 5);
            }
            i++;
            continue;
        }

        /* -O / -O0 / -O1 / -Os / -O2 ... */
        if (a[1] == 'O' && (a[2] == '\0' ||
                            (a[2] >= '0' && a[2] <= '9') ||
                            a[2] == 's' || a[2] == 'g')) {
            if (a[2] == '0' && a[3] == '\0')
                OPT_REF(OPT_OFF_Optimize) = 0;
            else
                OPT_REF(OPT_OFF_Optimize) = 1;
            i++;
            continue;
        }

        /* -oFILE or -o FILE */
        if (a[1] == 'o') {
            if (a[2] != '\0')
                arg = a + 2;
            else {
                i++;
                if (i >= argc)
                    cc_missing_arg("-o");
                arg = argv[i];
            }
            strcpy(outfile, arg);
            i++;
            continue;
        }

        /* -I / -D / -U with glued or separate args */
        if (a[1] == 'I' || a[1] == 'D' || a[1] == 'U') {
            char            opt2[3];

            if (a[2] != '\0')
                arg = a + 2;
            else {
                i++;
                if (i >= argc) {
                    opt2[0] = '-';
                    opt2[1] = a[1];
                    opt2[2] = '\0';
                    cc_missing_arg(opt2);
                }
                arg = argv[i];
            }
            if (a[1] == 'I')
                fe_add_idir(arg);
            else if (a[1] == 'D')
                fe_add_define(arg);
            else
                fe_add_undef(arg);
            i++;
            continue;
        }

        /* Lone -std needing following arg (rare) */
        if (cc_is_opt_need_arg(a)) {
            i++;
            if (i >= argc)
                cc_missing_arg(a);
            /* Only -std reaches here without earlier handling */
            i++;
            continue;
        }

        cc_bad_opt(a);
    }
    return i;
}
