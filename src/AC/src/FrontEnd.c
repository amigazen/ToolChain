/*
 * FrontEnd.c - basename dispatch helpers shared by ac/cc parsers.
 */

#include    <stdio.h>
#include    <string.h>
#include    "host_posix.h"
#include    "C.h"
#include    "Expr.h"
#include    "Gen.h"
#include    "Cglbdec.h"
#include    "FrontEnd.h"

int             frontend_mode = FE_AC;

extern char     prepbuffer[];
extern char    *incldir[10];
extern int      inclnum;
extern char    *litlate();
extern void     add_option(TABLE *tbl, char *cmd);
extern TABLE    cmd_defsyms;
extern TABLE    cmd_undefsyms;
extern char    *progname;

static int
fe_lower(c)
int             c;
{
    if (c >= 'A' && c <= 'Z')
        return c + ('a' - 'A');
    return c;
}

static int
fe_streq_ci(a, b)
char           *a;
char           *b;
{
    if (a == 0 || b == 0)
        return 0;
    while (*a != '\0' && *b != '\0') {
        if (fe_lower((int) *a) != fe_lower((int) *b))
            return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

/*
 * Last component of argv[0]: handles Unix '/' and Amiga ':'.
 */
static char    *
fe_basename(path)
char           *path;
{
    char           *p;
    char           *base;

    if (path == 0 || *path == '\0')
        return "";
    base = path;
    for (p = path; *p != '\0'; p++) {
        if (*p == '/' || *p == ':')
            base = p + 1;
    }
    return base;
}

void
frontend_set_from_argv0(argv0)
char           *argv0;
{
    char           *base;

    /* Default PDC-style front-end; only exact basename "cc" selects gcc mode.
     * Future: basename "sc" -> FE_SC + ReadArgs (Amiga builds only). */
    frontend_mode = FE_AC;
    base = fe_basename(argv0);
    if (fe_streq_ci(base, "cc"))
        frontend_mode = FE_CC;
}

void
fe_add_define(arg)
char           *arg;
{
    if (arg == 0)
        return;
    strcpy(prepbuffer, arg);
    add_option(&cmd_defsyms, prepbuffer);
}

void
fe_add_undef(arg)
char           *arg;
{
    if (arg == 0)
        return;
    strcpy(prepbuffer, arg);
    add_option(&cmd_undefsyms, prepbuffer);
}

void
fe_add_idir(arg)
char           *arg;
{
    int             i;
    int             c;

    if (arg == 0)
        return;
    ++global_flag;
    strcpy(prepbuffer, arg);
    c = 0;
    for (i = 0; prepbuffer[i]; i++)
        c = prepbuffer[i];
    if (c != ':' && c != '/')
        strcat(prepbuffer, "/");
    incldir[inclnum++] = litlate(prepbuffer);
    --global_flag;
}

void
usage_cc(void)
{
    fprintf(AC_DIAG_STREAM,
            "Usage: %s [options] file...\n"
            "Options:\n"
            "  -c                   Compile only; do not link\n"
            "  -E                   Preprocess only\n"
            "  -o <file>            Place output in <file>\n"
            "  -O[level]            Optimize (-O0 disables)\n"
            "  -g                   Generate debug info\n"
            "  -I <dir>             Add include directory\n"
            "  -D <macro>[=val]     Define macro\n"
            "  -U <macro>           Undefine macro\n"
            "  -Werror              Treat warnings as errors\n"
            "  -w                   Ignore (accepted)\n"
            "  -ansi                ISO C90 (accepted)\n"
            "  -std=c89|-std=c90    ISO C language (accepted)\n"
            "  -S                   Ignored (not gcc -S)\n"
            "  -help, --help        Display this information\n",
            progname != 0 ? progname : "cc");
    exit(1);
}
